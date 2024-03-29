# include "QueryManager.h"
# include <fstream>
# include <cassert>
# include <sstream>

Node* Node::node = nullptr;

void Expression::output(std::ostringstream& out) {}

Expression* Expression::input(std::istringstream& in){
    int temp;
    in >> temp;
    CalcOp op = (CalcOp)temp;
    switch(op){
        case ADD_OP:
        case SUB_OP: 
        case MUL_OP: 
        case DIV_OP: 
        case LIKE_OP: 
        case EQ_OP: 
        case LT_OP: 
        case GT_OP: 
        case LE_OP: 
        case GE_OP: 
        case NE_OP:{
            Expression* left = input(in);
            Expression* right = input(in);
            return new Binary(left, op, right);
        }
        case OR_OP:{
            LogicalAnd* left = (LogicalAnd*)input(in);
            WhereClause* where = new WhereClause(left);
            in >> temp;
            op = (CalcOp)temp;
            while(op == OR_OP){
                where->addLogicalAnd((LogicalAnd*)input(in));
                in >> temp;
                op = (CalcOp)temp;
            }
            return where;
        }
        case AND_OP:{
            Relational* left = new Relational(in);
            LogicalAnd* where = new LogicalAnd(left);
            in >> temp;
            op = (CalcOp)temp;
            while(op == AND_OP){
                where->addRelational(new Relational(in));
                in >> temp;
                op = (CalcOp)temp;
            }
            return where;
        }  
        case IS_NULL:
        case NOT_NULL:{
            Expression* child = input(in);
            return new Unary(child, op);
        } 
        case IN_OP:{
            return new InExpr(in);
        } 
        case PRI_OP:{
            return new Primary(in);
        } 
        case COL_OP: {
            return new Col(in);
        }
        default: {
            throw "error: expression input unknown operator " + to_string(op);
        }
    }
}

Primary::Primary(std::istringstream& in): Expression(PRI_OP){
    int temp;
    in >> temp;
    val.type = (AttrType)temp;
    switch(val.type){
        case STRING:{
            std::string str;
            in >> str;
            memcpy(val.s, str.c_str(), str.length());
            val.s[str.length()] = 0; // end of string
            break;
        }
        case INTEGER:
        case DATE:{
            in >> val.val.i;
            break;
        }
        case FLOAT:{
            in >> val.val.f;
            break;
        }
        case NO_TYPE:{
            break;
        }
        default:{
            throw "error: unknown primary value";
        }
    }
}

void Primary::output(std::ostringstream& out) {
    out << op << " " << int(val.type) << " ";
    switch(val.type){
        case STRING:{
            out << val.s << " ";
            break;
        }
        case INTEGER:
        case DATE:{
            out << val.val.i << " ";
            break;
        }
        case FLOAT:{
            out << val.val.f << " ";
            break;
        }
        case NO_TYPE:{
            break;
        }
        default:{
            throw "error: unknown primary value";
        }
    }
}

Col::Col(std::istringstream& in):Expression(COL_OP){
    in >> colName;
    table = nullptr;
}

void Col::output(std::ostringstream& out) {
    out << op << " " << colName << " ";
}

Relational::Relational(std::istringstream& in){
    int temp;
    in >> temp;
    op = (CalcOp)temp;
    switch(op){
        case LIKE_OP: 
        case EQ_OP: 
        case LT_OP: 
        case GT_OP: 
        case LE_OP: 
        case GE_OP: 
        case NE_OP:{
            binary = (Binary*)input(in);
            unary = nullptr;
            break;
        }
        case IS_NULL:
        case NOT_NULL:
        case IN_OP:{
            unary = (Unary*)input(in);
            binary = nullptr;
            break;
        }
        default: {
            throw "error: relational cannot deal with operator " + to_string(op);
        }
    }
    if (binary){
        col = (Col*)binary->left;
        for (auto i = binary->tbs.begin(); i != binary->tbs.end(); i ++){
            tbs.insert(*i);
        }
    }
    else{
        col = (Col*)unary->child;
        for (auto i = unary->tbs.begin(); i != unary->tbs.end(); i ++){
            tbs.insert(*i);
        }
    }
}

void Relational::output(std::ostringstream& out) {
    out << op << " ";
    if (binary){
        binary->output(out);
    }
    else{
        unary->output(out);
    }
}

void LogicalAnd::output(std::ostringstream& out) {
    for (int i = 0; i < exprList.size(); i ++){
        out << op << " ";
        exprList[i]->output(out);
    }
    out << int(ADD_OP) << " ";
}

void WhereClause::output(std::ostringstream& out) {
    for (int i = 0; i < exprList.size(); i ++){
        out << op << " ";
        exprList[i]->output(out);
    }
    out << int(ADD_OP) << " ";
}

void InExpr::output(std::ostringstream& out) {
    Unary::output(out);
    int len = valList->size();
    out << len << " ";
    for (int i = 0; i < len; i ++){
        Primary(valList->at(i)).output(out);
    }
}

InExpr::InExpr(std::istringstream& in): Unary(input(in), IN_OP) {
    valList = new std::vector<AttrVal>();
    int len;
    in >> len;
    for (int i = 0; i < len; i ++){
        valList->push_back(input(in)->calc(nullptr));
    }
}

static bool checkNotNulDefault(AttrVal& val, Table* table, int colId){
    if (val.type == NO_TYPE){
        if (table->header->constraints[colId] & 1){
            val = recordToAttr(table->rm->defaultRow + table->header->columnOffsets[colId], table->header->columnOffsets[colId + 1] - table->header->columnOffsets[colId], table->header->columnTypes[colId]);
        }
        else{
            if (table->header->constraints[colId] & 4){
                printf("warning: null value cannot insert into column %s\n", table->header->columnNames[colId]);
                return false;
            }
        }
    }
    return true;
}

static bool checkIndex(const std::vector<AttrVal>& valList, Table* table, RID_t updateRid = 0){
    for (int i = 0; i < table->ims.size(); i ++){
        if (table->ims[i]->ixClass == "unique" || table->ims[i]->ixClass == "pri"){
            AttrList entry;
            for (int k = 0; k < table->ims[i]->keys.size(); k ++){
                std::string colName = table->ims[i]->keys[k].name;
                int colId = table->__colId(colName);
                entry.vals.push_back(valList[colId - 1]);
            }
            table->ims[i]->indexScan->openScan(entry, table->ims[i]->keys.size(), EQ_OP);
            RID_t rid;
            if(!table->ims[i]->indexScan->getNextEntry(rid) && rid != updateRid){
                printf("warning: unique index %s cannot insert", table->ims[i]->ixName.c_str());
                for (int k = 0; k < table->ims[i]->keys.size(); k ++){
                    printf(" %s", attrToString(entry.vals[k]).c_str());
                }
                printf("\n");
                return false;
            }
        }
        else if (table->ims[i]->ixClass == "foreign"){
            Table* refTb = Database::instance()->getTableByName(table->ims[i]->refTbName);
            AttrList entry;
            for (int k = 0; k < table->ims[i]->keys.size(); k ++){
                std::string colName = table->ims[i]->keys[k].name;
                int colId = table->__colId(colName);
                entry.vals.push_back(valList[colId - 1]);
            }
            refTb->priIx->indexScan->openScan(entry, table->ims[i]->keys.size(), EQ_OP);
            RID_t rid;
            if(refTb->priIx->indexScan->getNextEntry(rid)){
                printf("warning: foreign key %s reference to %s cannot insert\n", table->ims[i]->ixName.c_str(), table->ims[i]->refTbName.c_str());
                for (int k = 0; k < table->ims[i]->keys.size(); k ++){
                    printf(" %s", attrToString(entry.vals[k]).c_str());
                }
                printf("\n");
                return false;
            }
        }
    }
    return true;
}

static bool checkIndex(const char* data, Table* table, RID_t updateRid = 0){
    std::vector<AttrVal> attrValList;
    for (int i = 1; i < table->header->columnCnt; i ++){
        attrValList.push_back(recordToAttr(data + table->header->columnOffsets[i], table->header->columnOffsets[i + 1] - table->header->columnOffsets[i], table->header->columnTypes[i]));
    }
    return checkIndex(attrValList, table, updateRid);
}

void Insert::run(){
    std::map<std::string, RID_t> rids;
    Table* table = Database::instance()->getTableByName(tbName);
    std::vector<Expression*> exprs;
    for (int i = 0; i < table->header->checkCnt; i ++){
        istringstream in(table->header->checks[i]);
        exprs.push_back(Expression::input(in));
    }
    for (int i = 0; i < lists->list.size(); i ++){
        bool flag = true;
        std::vector<AttrVal> attrValList;
        ValueList* valueList = lists->list[i];
        char temp[table->header->pminlen];
        for (int j = 0; j < valueList->list.size(); j ++){
            AttrVal val = valueList->list[j]->calc(nullptr, table);
            if (!checkNotNulDefault(val, table, j + 1)){
                flag = false;
                break;
            }
            if (val.type != NO_TYPE && !attrConvert(val, table->header->columnTypes[j + 1])){
                printf("warning: insert cannot convert %s to type %d\n", attrToString(val).c_str(), table->header->columnTypes[j + 1]);
                flag = false;
                break;
            }
            attrValList.push_back(val);
            restoreAttr(temp + table->header->columnOffsets[j + 1], table->header->columnOffsets[j + 2] - table->header->columnOffsets[j + 1], attrValList[j]);
        }
        if (!flag || !checkIndex(attrValList, table)){
            continue;
        }
        RID_t rid;
        table->rm->insertRecord(temp, rid);
        rids[tbName] = rid;
        for (int j = 0; j < exprs.size(); j ++){
            AttrVal val = exprs[j]->calc(&rids, table);
            if (val.type != INTEGER || !val.val.i){
                printf("warning: row %d check fail\n", i);
                flag = false;
                break;
            }
        }
        if (!flag){
            table->rm->deleteRecord(rid);
            continue;
        }
        for (int j = 0; j < table->ims.size(); j ++){
            AttrList entry;
            entry.rid = rid;
            for (int k = 0; k < table->ims[j]->keys.size(); k ++){
                std::string colName = table->ims[j]->keys[k].name;
                int colId = table->__colId(colName);
                entry.vals.push_back(attrValList[colId - 1]);
            }
            table->ims[j]->insertEntry(entry);
        }
    }
    for (int i = 0; i < exprs.size(); i ++){
        delete exprs[i];
    }
}

static std::set<Row>* scan(const std::vector<std::string>& list, std::set<Row>* oldRows, int i, LogicalAnd* logicalAnd){
    std::set<Row>* newRows = new set<Row>;
    Table* table = Database::instance()->getTableByName(list[i]);
    // no select
    if (!logicalAnd){
        table->rm->fileScan->openScan();
        RID_t rid;
        while(!table->rm->fileScan->getNextEntry(rid)){
            if (!oldRows){
                Row row;
                row.rowList.push_back(rid);
                newRows->insert(row);
            }
            else{
                for (auto it = oldRows->begin(); it != oldRows->end(); it ++){
                    Row row(*it);
                    row.rowList.push_back(rid);
                    newRows->insert(row);
                }
            }
        }
        return newRows;
    }
    // use logicalAnd to select
    Table* gap = nullptr;
    if (list.size() == 1){
        gap = table;
    }
    logicalAnd->bind(gap);
    if (!oldRows){
        std::map<std::string, RID_t> rids;
        logicalAnd->judgeScan(&rids, table);
        logicalAnd->openScan(&rids);
        RID_t rid;
        while(!logicalAnd->getNextEntry(rid)){
            rids[list[i]] = rid;
            AttrVal val = logicalAnd->calc(&rids);
            if (val.type == INTEGER && val.val.i){
                Row row;
                row.rowList.push_back(rid);
                newRows->insert(row);
            }
        }
    }
    else{
        for (auto it = oldRows->begin(); it != oldRows->end(); it ++){
            std::map<std::string, RID_t> rids;
            for (int j = 0; j < i; j ++){
                rids[list[j]] = it->rowList[j];
            }
            if (it == oldRows->begin()){
                logicalAnd->judgeScan(&rids, table);
            }
            logicalAnd->openScan(&rids);
            RID_t rid;
            while(!logicalAnd->getNextEntry(rid)){
                rids[list[i]] = rid;
                AttrVal val = logicalAnd->calc(&rids);
                if (val.type == INTEGER && val.val.i){
                    Row row(*it);
                    row.rowList.push_back(rid);
                    newRows->insert(row);
                }
            }
        }
    }
    return newRows;
}

void Delete::run(){
    std::set<Row> rows;
    for (int g = 0; g < clause->exprList.size(); g ++){
        std::vector<std::string> list;
        list.push_back(tbName);
        std::set<Row>* newRows = scan(list, nullptr, 0, clause->exprList[g]);
        for (auto it = newRows->begin(); it != newRows->end(); it ++){
            rows.insert(*it);
        }
    }
    Table* table = Database::instance()->getTableByName(tbName);
    std::map<std::string, RID_t> rids;
    for (auto it = rows.begin(); it != rows.end(); it ++){
        // initialize
        int rid = it->rowList[0];
        char data[table->header->pminlen];
        table->rm->getRecord(rid, data);
        // delete
        table->rm->deleteRecord(rid);
        for (int j = 0; j < table->ims.size(); j ++){
            AttrList entry;
            entry.rid = rid;
            for (int k = 0; k < table->ims[j]->keys.size(); k ++){
                std::string colName = table->ims[j]->keys[k].name;
                int colId = table->__colId(colName);
                entry.vals.push_back(recordToAttr(data + table->header->columnOffsets[colId], table->header->columnOffsets[colId + 1] - table->header->columnOffsets[colId], table->header->columnTypes[colId]));
            }
            table->ims[j]->deleteEntry(entry);
        }
    }
}

void Update::run(){
    std::set<Row> rows;
    for (int g = 0; g < clause->exprList.size(); g ++){
        std::vector<std::string> list;
        list.push_back(tbName);
        std::set<Row>* newRows = scan(list, nullptr, 0, clause->exprList[g]);
        for (auto it = newRows->begin(); it != newRows->end(); it ++){
            rows.insert(*it);
        }
    }
    Table* table = Database::instance()->getTableByName(tbName);
    std::map<std::string, RID_t> rids;
    for (auto it = rows.begin(); it != rows.end(); it ++){
        // initialize
        int rid = it->rowList[0];
        char data[table->header->pminlen];
        char newData[table->header->pminlen];
        table->rm->getRecord(rid, data);
        memcpy(newData, data, table->header->pminlen);
        // update
        rids[tbName] = rid;
        for (int i = 0; i < list->identList.list.size(); i ++){
            AttrVal val = list->valueList.list[i]->calc(&rids, table);
            int colId = table->__colId(list->identList.list[i]);
            if (!checkNotNulDefault(val, table, colId)){
                return;
            }
            if (val.type != NO_TYPE && !attrConvert(val, table->header->columnTypes[colId])){
                printf("warning: update cannot convert %s to type %d\n", attrToString(val).c_str(), table->header->columnTypes[colId]);
                return;
            }
            restoreAttr(newData + table->header->columnOffsets[colId], table->header->columnOffsets[colId + 1] - table->header->columnOffsets[colId], val);
        }
        if (!checkIndex(newData, table, rid)){
            return;
        }
        table->rm->updateRecord(rid, newData);
        for (int j = 0; j < table->ims.size(); j ++){
            AttrList oldEntry, newEntry;
            oldEntry.rid = newEntry.rid = rid;
            for (int k = 0; k < table->ims[j]->keys.size(); k ++){
                std::string colName = table->ims[j]->keys[k].name;
                int colId = table->__colId(colName);
                oldEntry.vals.push_back(recordToAttr(data + table->header->columnOffsets[colId], table->header->columnOffsets[colId + 1] - table->header->columnOffsets[colId], table->header->columnTypes[colId]));
                newEntry.vals.push_back(recordToAttr(newData + table->header->columnOffsets[colId], table->header->columnOffsets[colId + 1] - table->header->columnOffsets[colId], table->header->columnTypes[colId]));
            }
            table->ims[j]->deleteEntry(oldEntry);
            table->ims[j]->insertEntry(newEntry);
        }
    }
}

void Select::run(){
    run(nullptr);
}

void Select::run(std::vector<AttrVal>* selectResult){
    std::set<Row> rows;
    if (!clause){
        std::set<Row>* oldRows = nullptr, *newRows;
        for (int i = 0; i < tableList->list.size(); i ++){
            newRows = scan(tableList->list, oldRows, i, nullptr);
            delete oldRows;
            oldRows = newRows;
            if (oldRows->empty()){
                break;
            }
        }
        for (auto it = oldRows->begin(); it != oldRows->end(); it ++){
            rows.insert(*it);
        }
        delete oldRows;
    }
    else{
        for (int g = 0; g < clause->exprList.size(); g ++){
            std::set<Row>* oldRows = nullptr, *newRows;
            for (int i = 0; i < tableList->list.size(); i ++){
                newRows = scan(tableList->list, oldRows, i, clause->exprList[g]);
                delete oldRows;
                oldRows = newRows;
                if (oldRows->empty()){
                    break;
                }
            }
            for (auto it = oldRows->begin(); it != oldRows->end(); it ++){
                rows.insert(*it);
            }
            delete oldRows;
        }
    }
    // gap for calc function
    Table* gap = nullptr;
    if (tableList->list.size() == 1){
        gap = Database::instance()->getTableByName(tableList->list[0]);
    }
    // output select result
    if (!selector){
        for (auto it = rows.begin(); it != rows.end(); it ++){
            for (int j = 0; j < tableList->list.size(); j ++){
                Table* table = Database::instance()->getTableByName(tableList->list[j]);
                char data[table->header->pminlen];
                table->rm->getRecord(it->rowList[j], data);
                for (int i = 1; i < table->header->columnCnt; i ++){
                    int colLen = table->header->columnOffsets[i + 1] - table->header->columnOffsets[i];
                    AttrVal val = recordToAttr(data + table->header->columnOffsets[i], colLen, table->header->columnTypes[i]);
                    if (selectResult){
                        if (j == 0 && i == 1){
                            selectResult->push_back(val);
                        }
                    }
                    else{
                        std::string str = attrToString(val);
                        printf(" %-*s ", str.length(), str.c_str());
                        if (i < table->header->columnCnt - 1 || j < tableList->list.size() - 1){
                            printf("|");
                        }
                        else{
                            printf("\n");
                        }
                    }
                }
            }
        }
    }
    else{
        if (selector->valueList.list.size() > 0){
            for (auto it = rows.begin(); it != rows.end(); it ++){
                std::map<std::string, RID_t> rids;
                for (int j = 0; j < tableList->list.size(); j ++){
                    rids[tableList->list[j]] = it->rowList[j];
                }
                for (int i = 0; i < selector->valueList.list.size(); i ++){
                    AttrVal val = selector->valueList.list[i]->calc(&rids, gap);
                    if (selectResult){
                        if (i == 0){
                            selectResult->push_back(val);
                        }
                    }
                    else{
                        std::string str = attrToString(val);
                        printf(" %-*s ", str.length(), str.c_str());
                        if (i < selector->valueList.list.size() - 1){
                            printf("|");
                        }
                        else{
                            printf("\n");
                        }
                    }
                }
            }
        }
        else{
            for (int i = 0; i < selector->agrgList.size(); i ++){
                AttrVal val = selector->agrgList[i]->calc(&rows, tableList, gap);
                if (selectResult){
                    if (i == 0){
                        selectResult->push_back(val);
                    }
                }
                else{
                    std::string str = attrToString(val);
                    printf(" %-*s ", str.length(), str.c_str());
                    if (i < selector->agrgList.size() - 1){
                        printf("|");
                    }
                    else{
                        printf("\n");
                    }
                }
            }
        }
    }
}

std::vector<char*> Copy::inserts;
void Copy::run(){
    Table* table = Database::instance()->getTableByName(tbName);
    const std::string insertConstant = "INSERT INTO " + tbName + " VALUES ";
    std::string ans = insertConstant;
    ifstream fin(path);
    std::string line;
    int lineNo = 0;
    while(getline(fin, line)){
        ans.append("(");
        int position = 0, oldPosition = 0, colId = 1;
        while((position = line.find(delimiter, oldPosition)) != std::string::npos){
            if (table->header->columnTypes[colId] == STRING || table->header->columnTypes[colId] == DATE){
                if (line[oldPosition] != 39 || line[position - 1] != 39){
                    line.insert(oldPosition, 1, 39);
                    line.insert(position + 1, 1, 39);
                    position += 2;
                }
            }
            line.replace(position, delimiter.length(), ","); 
            colId ++;
            oldPosition = position + 1; 
            if (colId == table->header->columnCnt){
                line = line.substr(0, position + 1);
                break;
            }
        }

        if (oldPosition < line.length()){
            if (table->header->columnTypes[colId] == STRING){
                if (line[oldPosition] != 39 || line[line.length() - 1] != 39){
                    line.insert(oldPosition, 1, 39);
                    line.append("'");
                }
            }
            colId ++;
        }
        if (line[line.length() - 1] == ','){
            line[line.length() - 1] = ')';
        }
        else{
            line.append(")");
        }
        ans.append(line);
        ans.append(",");
        lineNo ++;
        if (lineNo == 1000){
            if (ans[ans.length() - 1] == ','){
                ans[ans.length() - 1] = ';';
                char* result = new char[ans.length() + 1];
                memcpy(result, ans.c_str(), ans.length() + 1);
                inserts.push_back(result);
                ans = insertConstant;
            }
            lineNo = 0;
        }
    }
    if (ans[ans.length() - 1] == ','){
        ans[ans.length() - 1] = ';';
        char* result = new char[ans.length() + 1];
        memcpy(result, ans.c_str(), ans.length() + 1);
        inserts.push_back(result);
    }
    fin.close();
}