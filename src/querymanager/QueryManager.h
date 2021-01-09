# ifndef QUERY_MANAGER
# define QUERY_MANAGER

# include "SqlParser.h"
# include "../utils/compare.cpp"
# include <map>
# include <set>

class Expression {
public:
    std::set<std::string> tbs;

    virtual ~Expression() = default;

    virtual AttrVal calc(std::map<std::string, RID_t>* rids) {
        return AttrVal();
    };
    virtual bool bind(Table* table){
        return false;
    }

    AttrVal calc(std::map<std::string, RID_t>* rids, Table* table) {
        bind(table);
        return calc(rids);
    };
    bool contain(std::map<std::string, RID_t>* rids){
        for (auto i = tbs.begin(); i != tbs.end(); i ++){
            if (rids->count(*i) == 0){
                return false;
            }
        }
        return true;
    }
};

class Primary : public Expression {
    AttrVal val;
public:
    Primary(int i){
        val.val.i = i;
        val.type = INTEGER;
    }
    Primary(float f){
        val.val.f = f;
        val.type = FLOAT;
    }
    Primary(const char* s, AttrType type){
        val.type = type;
        switch(type){
            case STRING:{
                val.s[0] = 0; // for empty string to display
                memcpy(val.s, s, strlen(s) + 1);
                break;
            }
            case DATE:{
                val.val.i = dateRestore(s);
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
    AttrVal calc(std::map<std::string, RID_t>* rids) override {
        return val;
    }
};

class Col : public Expression{
public:
    Table* table;
    std::string colName;
    Col(const char* c): colName(c){
        table = nullptr;
    }
    Col(const char* t, const char* c): colName(c){
        table = Database::instance()->getTableByName(t);
        tbs.insert(t);
    }
    bool bind(Table* t) override{
        if (!table && t){
            table = t;
            tbs.insert(t->name);
            return true;
        }
        return false;
    }
    AttrVal calc(std::map<std::string, RID_t>* rids) override {
        if (!table){
            throw "error: cannot locate column " + colName;
        }
        RID_t rid = rids->at(table->name);
        char temp[table->header->pminlen];
        table->rm->getRecord(rid, temp);
        int colId = table->__colId(colName);
        return recordToAttr(temp + table->header->columnOffsets[colId], table->header->columnOffsets[colId + 1] - table->header->columnOffsets[colId], table->header->columnTypes[colId]);
    }
};

class Binary : public Expression{
public:
    CalcOp op;
    Expression* left, *right;
    Binary(Expression* l, CalcOp o, Expression* r) : left(l), right(r) {
        op = o;
        for (auto i = l->tbs.begin(); i != l->tbs.end(); i ++){
            tbs.insert(*i);
        }
        for (auto i = r->tbs.begin(); i != r->tbs.end(); i ++){
            tbs.insert(*i);
        }
    }
    bool bind(Table* t) override{
        bool flag1 = left->bind(t), flag2 = right->bind(t);
        if ((flag1 || flag2) && tbs.count(t->name) == 0){
            tbs.insert(t->name);
            return true;
        }
        return false;
    }
    AttrVal calc(std::map<std::string, RID_t>* rids) override {
        AttrVal lVal = left ->calc(rids);
        AttrVal rVal = right ->calc(rids);
        switch(op){
            case ADD_OP:
            case SUB_OP:
            case MUL_OP:
            case DIV_OP:{
                return calculate(lVal, rVal, op);
            }
            case LIKE_OP:{
                return like(lVal, rVal);
            }
            case EQ_OP: 
            case LT_OP:
            case GT_OP:
            case LE_OP: 
            case GE_OP: 
            case NE_OP:{
                return compare(lVal, rVal, op);
            }
            default:{
                throw "error: unknown binary operation " + std::to_string(op);
            }
        }
    }
    ~Binary() override{
        delete left;
        delete right;
    }
};

class Unary : public Expression{
public:
    CalcOp op;
    Expression* child;
    Unary(Expression* c, CalcOp o) : child(c) {
        op = o;
        for (auto i = child->tbs.begin(); i != child->tbs.end(); i ++){
            tbs.insert(*i);
        }
    }
    bool bind(Table* t) override{
        if (child->bind(t) && tbs.count(t->name) == 0){
            tbs.insert(t->name);
            return true;
        }
        return false;
    }
    AttrVal calc(std::map<std::string, RID_t>* rids) override {
        AttrVal cVal = child ->calc(rids);
        return judge(cVal, op);
    }
    ~Unary() override{
        delete child;
    }
};

class Relational : public Expression{
public:
    Col* col;
    CalcOp op;
    Binary* binary;
    Unary* unary;
    Relational(Binary* b, Unary* u){
        binary = b;
        unary = u;
        if (b){
            col = (Col*)b->left;
            op = b->op;
            for (auto i = b->tbs.begin(); i != b->tbs.end(); i ++){
                tbs.insert(*i);
            }
        }
        else{
            op = u->op;
            col = (Col*)u->child;
            for (auto i = u->tbs.begin(); i != u->tbs.end(); i ++){
                tbs.insert(*i);
            }
        }
    }
    ~Relational() override{
        delete binary;
        delete unary;
    }
    bool bind(Table* t) override{
        if (binary){
            return binary->bind(t);
        }
        return unary->bind(t);
    }
    AttrVal calc(std::map<std::string, RID_t>* rids) override {
        if (binary){
            return binary->calc(rids);
        }
        return unary->calc(rids);
    }
    bool isChildConstant(std::map<std::string, RID_t>* rids){
        if (binary){
            return binary->right->contain(rids);
        }
        return unary->child->contain(rids);
    }
};

struct RelTuple{
    std::string colName;
    CalcOp op;
    Expression* expr;
};

class LogicalAnd: public Expression{
    RM_FileScan* fileScan;
    IX_IndexScan* indexScan;
    std::vector<RelTuple>* maxExprList;
public:
    vector<Relational*> exprList;
    LogicalAnd(Relational* expr){
        exprList.push_back(expr);
    }
    void addRelational(Relational* expr){
        exprList.push_back(expr);
    }
    ~LogicalAnd() override{
        for (int i = 0; i < exprList.size(); i ++){
            delete exprList[i];
        }
    }

    bool bind(Table* t) override{
        bool flag = false;
        for (int i = 0; i < exprList.size(); i ++){
            if (exprList[i]->bind(t)){
                flag = true;
            }
        }
        if (flag && tbs.count(t->name) == 0){
            tbs.insert(t->name);
            return true;
        }
        return false;
    }
    AttrVal calc(std::map<std::string, RID_t>* rids) override {
        AttrVal lVal;
        lVal.type = INTEGER;
        lVal.val.i = 1;
        for (int i = 0; i < exprList.size(); i ++){
            if (!exprList[i]->contain(rids)){
                continue;
            }
            AttrVal rVal = exprList[i] ->calc(rids);
            lVal = logic(lVal, rVal, CalcOp::AND_OP);
            if (!lVal.val.i){
                break;
            }
        }
        return lVal;
    }
    void judgeScan(std::map<std::string, RID_t>* rids, Table* table){
        std::vector<RelTuple> cmpList;
        for (int i = 0; i < exprList.size(); i ++){
            if (exprList[i]->isChildConstant(rids) && exprList[i]->col->table && exprList[i]->col->table->name == table->name){
                switch(exprList[i]->op){
                    case IS_NULL:
                    case NOT_NULL:{
                        RelTuple temp;
                        temp.colName = exprList[i]->col->colName;
                        if (exprList[i]->op == IS_NULL){
                            temp.op = EQ_OP;
                        }
                        else{
                            temp.op = NE_OP;
                        }
                        temp.expr = new Primary(nullptr, NO_TYPE);
                        cmpList.push_back(temp);
                        break;
                    }
                    case EQ_OP: 
                    case LT_OP:
                    case GT_OP: 
                    case LE_OP:
                    case GE_OP:
                    case NE_OP:{
                        RelTuple temp;
                        temp.colName = exprList[i]->col->colName;
                        temp.op = exprList[i]->op;
                        temp.expr = exprList[i]->binary->right;
                        cmpList.push_back(temp);
                    }
                    default:{}
                }
            }
        }
        if (cmpList.empty()){
            fileScan = table->rm->fileScan;
            indexScan = nullptr;
            return;
        }
        maxExprList = nullptr;
        int maxId = -1;
        for (int i = 0; i < table->ims.size(); i ++){
            std::vector<RelTuple>* tempList = new std::vector<RelTuple>();
            for (int j = 0; j < table->ims[i]->keys.size(); j ++){
                std::string key = table->ims[i]->keys[j].name;
                int eqId = -1, cmpId = -1;
                for (int k = 0; k < cmpList.size(); k ++){
                    if (cmpList[k].colName == key){
                        if (cmpList[k].op == EQ_OP){
                            eqId = k;
                            break;
                        }
                        else{
                            if (cmpId < 0){
                                cmpId = k;
                            }
                        }
                    }
                }
                if (eqId >= 0){
                    tempList->push_back(cmpList[eqId]);
                }
                else{
                    if (cmpId >= 0){
                        tempList->push_back(cmpList[cmpId]);
                    }
                    break;
                }
            }
            if (!maxExprList || tempList->size() > maxExprList->size()){
                delete maxExprList;
                maxExprList = tempList;
                maxId = i;
            }
        }
        if (!maxExprList || maxExprList->empty()){
            fileScan = table->rm->fileScan;
            indexScan = nullptr;
            return;
        }
        fileScan = nullptr;
        indexScan = table->ims[maxId]->indexScan;
        printf("info: index %s has been used\n", table->ims[maxId]->ixName.c_str());
    }
    void openScan(std::map<std::string, RID_t>* rids){
        if (!indexScan){
            fileScan->openScan();
            return;
        }
        AttrList entry;
        for (int i = 0; i < maxExprList->size(); i ++){
            entry.vals.push_back(maxExprList->at(i).expr->calc(rids));
        }
        indexScan->openScan(entry, maxExprList->size(), maxExprList->at(maxExprList->size() - 1).op);
    }
    int getNextEntry(RID_t& rid){
        if (!indexScan){
            return fileScan->getNextEntry(rid);
        }
        return indexScan->getNextEntry(rid);
    }
};

class WhereClause: public Expression{
public:
    vector<LogicalAnd*> exprList;
    WhereClause(LogicalAnd* expr){
        exprList.push_back(expr);
    }
    void addLogicalAnd(LogicalAnd* expr){
        exprList.push_back(expr);
    }
    bool bind(Table* t) override{
        bool flag = false;
        for (int i = 0; i < exprList.size(); i ++){
            if (exprList[i]->bind(t)){
                flag = true;
            }
        }
        if (flag && tbs.count(t->name) == 0){
            tbs.insert(t->name);
            return true;
        }
        return false;
    }
    AttrVal calc(std::map<std::string, RID_t>* rids) override {
        AttrVal lVal;
        lVal.type = INTEGER;
        lVal.val.i = 0;
        for (int i = 0; i < exprList.size(); i ++){
            if (!exprList[i]->contain(rids)){
                continue;
            }
            AttrVal rVal = exprList[i] ->calc(rids);
            lVal = logic(lVal, rVal, CalcOp::OR_OP);
            if (lVal.val.i){
                break;
            }
        }
        return lVal;
    }
    ~WhereClause() override{
        for (int i = 0; i < exprList.size(); i ++){
            delete exprList[i];
        }
    }
};

class Row{
public:
    std::vector<RID_t> rowList;
    Row(){
        rowList.reserve(MAX_COL_NUM);
    }
    Row(const Row& r){
        rowList.reserve(MAX_COL_NUM);
        for (int k = 0; k < r.rowList.size(); k ++){
            rowList.push_back(r.rowList[k]);
        }
    }
    bool operator<(const Row& r) const{
        int i;
        for (i = 0; i < min(rowList.size(), r.rowList.size()); i ++){
            if (rowList[i] < r.rowList[i]){
                return true;
            }
            if (r.rowList[i] < rowList[i]){
                return false;
            }
        }
        return i < r.rowList.size();
    }
};

class ValueList {
public:
    std::vector<Expression*> list;
    void addValue(Expression* exp){
        list.push_back(exp);
    }
    ~ValueList(){
        for (int i = 0; i < list.size(); i ++){
            delete list[i];
        }
    }
};

class Agrg {
public:
    virtual AttrVal calc(std::set<Row>* rows, IdentList* tableList) {
        return AttrVal();
    }
    virtual void bind(Table* table){};
    virtual ~Agrg() = default;
    AttrVal calc(std::set<Row>* rows, IdentList* tableList, Table* table) {
        bind(table);
        return calc(rows, tableList);
    }
};

class AgrgCount : public Agrg {
public:
    ValueList* list;
    AgrgCount(ValueList* v){
        list = v;
    }
    AttrVal calc(std::set<Row>* rows, IdentList* tableList) override {
        int num = 0;
        if (!list){
            num = rows->size();
        }
        else{
            std::set<AttrList> attrListSet;
            AttrList attrList;
            attrList.rid = 0;
            std::map<std::string, RID_t> rids;
            for (auto it = rows->begin(); it != rows->end(); it ++){
                for (int j = 0; j < tableList->list.size(); j ++){
                    rids[tableList->list[j]] = it->rowList[j];
                }
                bool flag = true;
                for (int j = 0; j < list->list.size(); j ++){
                    attrList.vals.push_back(list->list[j]->calc(&rids));
                    if (attrList.vals[j].type == NO_TYPE){
                        flag = false;
                        break;
                    }
                }
                if (flag){
                    attrListSet.insert(attrList);
                }
            }
            num = attrListSet.size();
        }
        AttrVal ans;
        ans.type = INTEGER;
        ans.val.i = num;
        return ans;
    }
    void bind(Table* table)override{
        if (list){
            for (int i = 0; i < list->list.size(); i ++){
                list->list[i]->bind(table);
            }
        }
    }
    ~AgrgCount() override{
        delete list;
    }
};

class AgrgSum : public Agrg {
public:
    Expression* expr;
    AgrgSum(Expression* e){
        expr = e;
    }
    AttrVal calc(std::set<Row>* rows, IdentList* tableList) override {
        AttrVal ans = AttrVal();
        std::map<std::string, RID_t> rids;
        for (auto it = rows->begin(); it != rows->end(); it ++){
            for (int j = 0; j < tableList->list.size(); j ++){
                rids[tableList->list[j]] = it->rowList[j];
            }
            AttrVal val = expr->calc(&rids);
            if (ans.type == NO_TYPE){
                ans = val;
            }
            else{
                ans = calculate(ans, val, ADD_OP);
            }
        }
        return ans;
    }
    void bind(Table* table)override{
        expr->bind(table);
    }
    ~AgrgSum(){
        delete expr;
    }
};

class AgrgAvg : public Agrg {
public:
    AgrgSum* sum;
    AgrgAvg(Expression* e){
        sum = new AgrgSum(e);
    }
    AttrVal calc(std::set<Row>* rows, IdentList* tableList) override {
        AttrVal val = sum->calc(rows, tableList);
        AttrVal temp;
        temp.type = INTEGER;
        temp.val.i = rows->size();
        return calculate(val, temp, DIV_OP);
    }
    void bind(Table* table)override{
        sum->bind(table);
    }
    ~AgrgAvg(){
        delete sum;
    }
};

class AgrgMax : public Agrg {
public:
    Expression* expr;
    AgrgMax(Expression* e){
        expr = e;
    }
    AttrVal calc(std::set<Row>* rows, IdentList* tableList) override {
        AttrVal ans = AttrVal();
        std::map<std::string, RID_t> rids;
        for (auto it = rows->begin(); it != rows->end(); it ++){
            for (int j = 0; j < tableList->list.size(); j ++){
                rids[tableList->list[j]] = it->rowList[j];
            }
            AttrVal val = expr->calc(&rids);
            if (ans.type == NO_TYPE){
                ans = val;
            }
            else{
                AttrVal cmp = compare(val, ans, GT_OP);
                if (cmp.type == INTEGER && cmp.val.i){
                    ans = val;
                }
            }
        }
        return ans;
    }
    void bind(Table* table)override{
        expr->bind(table);
    }
    ~AgrgMax(){
        delete expr;
    }
};

class AgrgMin : public Agrg {
public:
    Expression* expr;
    AgrgMin(Expression* e){
        expr = e;
    }
    AttrVal calc(std::set<Row>* rows, IdentList* tableList) override {
        AttrVal ans = AttrVal();
        std::map<std::string, RID_t> rids;
        for (auto it = rows->begin(); it != rows->end(); it ++){
            for (int j = 0; j < tableList->list.size(); j ++){
                rids[tableList->list[j]] = it->rowList[j];
            }
            AttrVal val = expr->calc(&rids);
            if (ans.type == NO_TYPE){
                ans = val;
            }
            else{
                AttrVal cmp = compare(val, ans, LT_OP);
                if (cmp.type == INTEGER && cmp.val.i){
                    ans = val;
                }
            }
        }
        return ans;
    }
    void bind(Table* table)override{
        expr->bind(table);
    }
    ~AgrgMin(){
        delete expr;
    }
};

class ValueLists {
public:
    std::vector<ValueList*> list;
    void addValueList(ValueList* valueList){
        list.push_back(valueList);
    }
    ~ValueLists(){
        for (int i = 0; i < list.size(); i ++){
            delete list[i];
        }
    }
};

class Selector {
public:
    ValueList valueList;
    std::vector<Agrg*> agrgList;
    void addValue(Expression* exp){
        if (agrgList.size() > 0){
            throw "error: aggregate function cannot select together with column";
        }
        valueList.addValue(exp);
    }
    void addAgrg(Agrg* agrg){
        if (valueList.list.size() > 0){
            throw "error: aggregate function cannot select together with column";
        }
        agrgList.push_back(agrg);
    }
    ~Selector(){
        for (int i = 0; i < agrgList.size(); i ++){
            delete agrgList[i];
        }
    }
};

class SetList {
public:
    IdentList identList;
    ValueList valueList;
    void addSetClause(const char* ident, Expression* value){
        identList.addIdent(ident);
        valueList.addValue(value);
    }
};

class ColField : public Field {
    char __colName[MAX_NAME_LEN];
    Type* __type;
    bool __notNull;
    char* __defaultVal = nullptr;

public:
    ColField(const char* colName, Type* type, bool notNull = false, Expression* defaultVal = nullptr) :
        __type(type), __notNull(notNull) {
        if (defaultVal){
            AttrVal val = defaultVal->calc(nullptr, nullptr);
            if (val.type != NO_TYPE && !attrConvert(val, type->type)){
                printf("warning: column field cannot convert %s to %d\n", attrToString(val).c_str(), type->type);
                return;
            }
            __defaultVal = new char[MAX_ATTR_LEN];
            restoreAttr(__defaultVal, MAX_ATTR_LEN, val);
        }
        memcpy(__colName, colName, MAX_NAME_LEN);
    }

    void addColumn(std::vector<AttrType>& types, std::vector<int>& colLens, std::vector<std::string>& names, std::vector<bool>& notNulls, std::vector<char*>& defaults) override {
        types.push_back(__type->type);
        colLens.push_back(__type->valLen);
        names.push_back(__colName);
        notNulls.push_back(__notNull);
        defaults.push_back(__defaultVal);
    }

    void addColumn(Table* table) override {
        table -> addColumn(__type -> type, __type -> valLen, __colName, __notNull, __defaultVal);
    };

    void changeColumn(Table* table, char oldColName[MAX_NAME_LEN]) override {
        table -> changeColumn(oldColName, __type -> type, __type -> valLen, __colName, __notNull, __defaultVal);
    };
    ~ColField() override{
        delete __type;
        delete[] __defaultVal;
    }
};

class Insert : public Node {
    std::string tbName;
    ValueLists* lists;
public:
    Insert(const char* name, ValueLists* l){
        tbName = name;
        lists = l;
    }
    void run() override;
    ~Insert() override{
        delete lists;
    }
};

class Copy : public Node {
    std::string tbName, path, format, delimiter;
public:
    static std::vector<char*> inserts;
    Copy(const char* n, const char* p, const char* f, const char* d){
        tbName = n;
        path = p;
        format = f;
        delimiter = d;
    }
    void run() override;
};

class Delete : public Node {
    std::string tbName;
    WhereClause* clause;
public:
    Delete(const char* name, WhereClause* c){
        tbName = name;
        clause = c;
    }
    void run() override;
    ~Delete() override{
        delete clause;
    }
};

class Update : public Node {
    std::string tbName;
    SetList* list;
    WhereClause* clause;
public:
    Update(const char* name, SetList* l, WhereClause* c){
        tbName = name;
        list = l;
        clause = c;
    }
    void run() override;
    ~Update() override{
        delete list;
        delete clause;
    }
};

class Select : public Node {
    Selector* selector;
    IdentList* tableList;
    WhereClause* clause;
public:
    Select(Selector* s, IdentList* t, WhereClause* c = nullptr){
        selector = s;
        tableList = t;
        clause = c;
    }
    void run() override;
    void run(std::vector<AttrVal>* selectResult);
    ~Select() override{
        delete selector;
        delete tableList;
        delete clause;
    }
};

class InExpr : public Unary{
public:
    std::vector<AttrVal>* valList;
    InExpr(Expression* c, ValueList* list) : Unary(c, IN_OP){
        valList = new std::vector<AttrVal>();
        for (int i = 0; i < list->list.size(); i ++){
            valList->push_back(list->list[i]->calc(nullptr));
        }
    }
    InExpr(Expression* c, Select* select) : Unary(c, IN_OP){
        valList = new std::vector<AttrVal>();
        select->run(valList);
        delete select;
    }
    AttrVal calc(std::map<std::string, RID_t>* rids) override {
        AttrVal result;
        result.type = INTEGER;
        AttrVal cVal = child ->calc(rids);
        for (int i = 0; i < valList->size(); i ++){
            AttrVal temp = compare(cVal, valList->at(i), EQ_OP);
            if (temp.type == INTEGER && temp.val.i){
                result.val.i = 1;
                return result;
            }
        }
        result.val.i = 0;
        return result;
    }
    ~InExpr() override{
        delete valList;
    }
};
# endif