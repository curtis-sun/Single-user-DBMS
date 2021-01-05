# ifndef QUERY_MANAGER
# define QUERY_MANAGER

# include "SqlParser.h"
# include "../utils/compare.cpp"
# include <map>
# include <set>

class Expression {
public:
    std::set<std::string> tbs;
    virtual AttrVal calc(std::map<std::string, RID_t>* rids, Table* table) {
        return AttrVal();
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
                memcpy(val.s, s, strlen(s));
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
    AttrVal calc(std::map<std::string, RID_t>* rids, Table* table) override {
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
    AttrVal calc(std::map<std::string, RID_t>* rids, Table* t) override {
        if (!table){
            if (!t){
                throw "error: cannot locate column " + colName;
            }
            table = t;
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
    AttrVal calc(std::map<std::string, RID_t>* rids, Table* t) override {
        AttrVal lVal = left ->calc(rids, t);
        AttrVal rVal = right ->calc(rids, t);
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
    AttrVal calc(std::map<std::string, RID_t>* rids, Table* t) override {
        AttrVal cVal = child ->calc(rids, t);
        return judge(cVal, op);
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
    AttrVal calc(std::map<std::string, RID_t>* rids, Table* t) override {
        if (binary){
            return binary->calc(rids, t);
        }
        return unary->calc(rids, t);
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
    AttrVal calc(std::map<std::string, RID_t>* rids, Table* table) override {
        AttrVal lVal;
        lVal.type = INTEGER;
        lVal.val.i = 1;
        for (int i = 0; i < exprList.size(); i ++){
            if (!exprList[i]->contain(rids)){
                continue;
            }
            AttrVal rVal = exprList[i] ->calc(rids, table);
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
            if (exprList[i]->isChildConstant(rids) && (!exprList[i]->col->table || exprList[i]->col->table->name == table->name)){
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
                    default:{
                        RelTuple temp;
                        temp.colName = exprList[i]->col->colName;
                        temp.op = exprList[i]->op;
                        temp.expr = exprList[i]->binary->right;
                        cmpList.push_back(temp);
                    }
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
                std::string key = table->ims[i]->keys[j];
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
    void openScan(std::map<std::string, RID_t>* rids, Table* table){
        if (!indexScan){
            fileScan->openScan();
            return;
        }
        Entry entry;
        for (int i = 0; i < maxExprList->size(); i ++){
            entry.vals[i] = maxExprList->at(i).expr->calc(rids, table);
        }
        indexScan->openScan(entry, maxExprList->size(), maxExprList->at(maxExprList->size() - 1).op);
    }
    int getNextEntry(RID_t& rid, Table* table){
        if (!indexScan){
            char temp[table->header->pminlen];
            return fileScan->getNextEntry(temp, rid);
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
    AttrVal calc(std::map<std::string, RID_t>* rids, Table* table) override {
        AttrVal lVal;
        lVal.type = INTEGER;
        lVal.val.i = 0;
        for (int i = 0; i < exprList.size(); i ++){
            if (!exprList[i]->contain(rids)){
                continue;
            }
            AttrVal rVal = exprList[i] ->calc(rids, table);
            lVal = logic(lVal, rVal, CalcOp::OR_OP);
            if (lVal.val.i){
                break;
            }
        }
        return lVal;
    }
};

class ValueList {
public:
    std::vector<Expression*> list;
    void addValue(Expression* exp){
        list.push_back(exp);
    }
};

class ValueLists {
public:
    std::vector<ValueList*> list;
    void addValueList(ValueList* valueList){
        list.push_back(valueList);
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
    char* __defaultVal;

public:
    ColField(const char* colName, Type* type, bool notNull = false, Expression* defaultVal = nullptr) :
        __type(type), __notNull(notNull) {
        if (defaultVal){
            __defaultVal = new char[MAX_ATTR_LEN];
            restoreAttr(__defaultVal, MAX_ATTR_LEN, defaultVal->calc(nullptr, nullptr));
        }
        else{
            __defaultVal = nullptr;
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
};

class Select : public Node {
    ValueList* selector;
    IdentList* tableList;
    WhereClause* clause;
public:
    Select(ValueList* s, IdentList* t, WhereClause* c){
        selector = s;
        tableList = t;
        clause = c;
    }
    void run() override;
};
# endif