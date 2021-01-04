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
    Expression* left, *right;
    CompOp op;
    Binary(Expression* l, CompOp o, Expression* r) : left(l), op(o), right(r) {
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
        bool result = compare(lVal, rVal, op);
        AttrVal ans;
        ans.type = INTEGER;
        ans.val.i = result;
        return ans;
    }
};

class Unary : public Expression{
public:
    Expression* child;
    CompOp op;
    Unary(Expression* c, CompOp o) : child(c), op(o) {
        for (auto i = child->tbs.begin(); i != child->tbs.end(); i ++){
            tbs.insert(*i);
        }
    }
    AttrVal calc(std::map<std::string, RID_t>* rids, Table* t) override {
        AttrVal cVal = child ->calc(rids, t);
        bool result = judge(cVal, op);
        AttrVal ans;
        ans.type = INTEGER;
        ans.val.i = result;
        return ans;
    }
};

class LogicalAnd: public Expression{
public:
    vector<Expression*> exprList;
    LogicalAnd(Expression* expr){
        exprList.push_back(expr);
    }
    void addRelational(Expression* expr){
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
            bool result = logic(lVal, rVal, CalcOp::AND_OP);
            lVal.val.i = result;
            if (!result){
                break;
            }
        }
        return lVal;
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
            bool result = logic(lVal, rVal, CalcOp::OR_OP);
            lVal.val.i = result;
            if (result){
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
    char __defaultVal[MAX_ATTR_LEN];

public:
    ColField(const char* colName, Type* type, bool notNull = false, Expression* defaultVal = nullptr) :
        __type(type), __notNull(notNull) {
        if (defaultVal){
            __defaultVal[0] = 1;
            restoreAttr(__defaultVal + 1, MAX_ATTR_LEN, defaultVal->calc(nullptr, nullptr));
        }
        memcpy(__colName, colName, MAX_NAME_LEN);
    }

    void addColumn(std::vector<AttrType>& types, std::vector<int>& colLens, std::vector<std::string>& names) override {
        types.push_back(__type->type);
        colLens.push_back(__type->valLen);
        names.push_back(__colName);
    }

    void setDefault(Table* table, int colId) override {
        if (__defaultVal){
            table ->setDefault(colId, __defaultVal);
        }
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