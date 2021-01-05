# ifndef SQL_PARSER
# define SQL_PARSER

# include "../systemmanager/Database.h"
# include <iostream>

class IdentList {
public:
    std::vector<std::string> list;
    void addIdent(const char* ident){
        list.push_back(ident);
    }
};

class Type {
public:
    AttrType type;
    int valLen;
    Type(AttrType m_type): type(m_type){
        switch(m_type){
            case INTEGER:{
                valLen = 5;
                break;
            }
            case FLOAT:{
                valLen = 5;
                break;
            }
            case DATE:{
                valLen = 5;
                break;
            }
            default:{
                throw "error: unknown value type";
            }
        }
    }
    Type(AttrType m_type, int len): type(m_type){
        switch(m_type){
            case INTEGER:
            case STRING:{
                valLen = len + 1;
                break;
            }
            default:{
                throw "error: unknown value type";
            }
        }
    }
};

class Field {
public:
    virtual void setPri(Table* table, const std::string& priName) {};
    virtual void addForeign(Table* table, const std::string& ixName) {};
    virtual void addColumn(std::vector<AttrType>& types, std::vector<int>& colLens, std::vector<std::string>& names, std::vector<bool>& notNulls, std::vector<char*>& defaults) {};
    virtual void addColumn(Table* table) {};
    virtual void changeColumn(Table* table, char oldColName[MAX_NAME_LEN]) {};
    virtual ~Field() = default;
};

class PrimaryField : public Field {
    IdentList* list;

public:
    PrimaryField(IdentList* colList) {
        list = colList;
    }

    void setPri(Table* table, const std::string& priName) override {
        table->setPrimary(list->list, priName);
    };

    ~PrimaryField() override {
        delete list;
    }
};

class ForeignField : public Field {
    std::string refTbName;
    std::vector<std::string> key, refKey;

public:
    ForeignField(const char* k, const char* rn, const char* rk) {
        key.push_back(k);
        refTbName = rn;
        refKey.push_back(rk);
    }

    void addForeign(Table* table, const std::string& ixName) override {
        Table* refTb = Database::instance()->getTableByName(refTbName);
        table->addForeignKey(key, ixName, refTbName, refKey, refTb);
    };
};

class FieldList {
    std::vector<Field*> list;
    std::vector<AttrType> types;
    std::vector<int> colLens;
    std::vector<std::string> names;
    std::vector<bool> notNulls;
    std::vector<char*> defaults;

public:
    void addField(Field* field){
        list.push_back(field);
        field->addColumn(types, colLens, names, notNulls, defaults);
    }

    void createTable(Table* table){
        table->createTable(types, colLens, names, notNulls, defaults);
        for (int i = 0; i < list.size(); i ++){
            list[i]->setPri(table, "pri");
            list[i]->addForeign(table, "foreign_" + to_string(i));
        }
    }
};

class Node {
public:
    virtual ~Node() = default;

    virtual void run() {}

    static void setInstance(Node *n) {
        delete node;
        node = n;
    }

    static void execute() {
        if (node){
            node->run();
        }
    }

    static Node *node;
};

class Quit : public Node {
public:
    void run() override {
        if (Database::database){
            Database::database  ->closeDatabase();
            delete Database::database;
            Database::database = nullptr;
        }
    }
};

class ShowDatabases : public Node {
    std::vector<std::string> databases;
public:
    ShowDatabases(){
        DIR *p_dir = NULL;
        struct dirent *p_entry = NULL;
        struct stat statbuf;
        std::string path = "data";

        if((p_dir = opendir(path.c_str())) == NULL){
            throw "error: opendir " +  path + " fail";
        }

        int err = chdir(path.c_str());
        if (err){
            throw "error: chdir " +  path + " fail";
        }

        while(NULL != (p_entry = readdir(p_dir))) { // 获取下一级目录信息
            stat(p_entry->d_name, &statbuf);   // 获取下一级成员属性
            if(S_IFDIR & statbuf.st_mode) {      // 判断下一级成员是否是目录
                std::string dbName = p_entry->d_name; 
                if (dbName == "." || dbName == "..")
                    continue;
                databases.push_back(dbName);
            } 
        }
        err = chdir(".."); // 回到上级目录 
        if (err){
            throw "error: chdir .. fail";
        } 
        err = closedir(p_dir);
        if (err){
            throw "error: closedir " +  path + " fail";
        } 
    }
    void run() override {
        int maxNameLen = 4;
        for (int i = 0; i < databases.size(); i ++){
            int nameLen = databases[i].length();
            if (nameLen > maxNameLen){
                maxNameLen = nameLen;
            }
        }
        printf(" %-*s\n-", maxNameLen, "Name");
        for(int i = 0; i < maxNameLen; i ++){
            printf("-");
        }
        printf("\n");
        for (int i = 0; i < databases.size(); i ++){
            printf(" %-*s\n", maxNameLen, databases[i].c_str());
        }
    }
};

class CreateDatabase : public Node {
    std::string __dbName;
public:
    explicit CreateDatabase(const char *dbName) {
        __dbName = dbName;
    }

    void run() override {
        Database::database = new Database(__dbName);
        Database::database ->createDatabase();
    }
};

class DropDatabase : public Node {
    std::string __dbName;
public:
    explicit DropDatabase(const char *dbName) {
        __dbName = dbName;
    }

    void run() override {
        if (Database::database && __dbName == Database::database->dbname){
            throw "error: cannot drop current used database";
        }
        Database* database = new Database(__dbName);
        database ->openDatabase();
        database ->destroyDatabase();
        delete database;
    }
};

class UseDatabase : public Node {
    std::string __dbName;
public:
    explicit UseDatabase(const char *dbName) {
        __dbName = dbName;
    }

    void run() override {
        if (Database::database){
            if (__dbName == Database::database->dbname){
                return;
            }
            Database::database->closeDatabase();
            delete Database::database;
        }
        Database::database = new Database(__dbName);
        Database::database ->openDatabase();
    }
};

class ShowTables : public Node {
public:
    void run() override {
        Database::instance() ->showTables();
    }
};

class CreateTable : public Node {
    std::string tbName;
    FieldList* fieldList;
public:
    explicit CreateTable(const char* name, FieldList* list) {
        tbName = name;
        fieldList = list;
    }

    void run() override {
        Table* table = Database::instance() ->createTable(tbName);
        fieldList->createTable(table);
    }

    ~CreateTable() override {
        delete fieldList;
    }
};

class DropTable : public Node {
    std::string tbName;
public:
    explicit DropTable(const char* name) {
        tbName = name;
    }

    void run() override {
        Database::instance() ->dropTableByName(tbName);
    }
};

class DescTable : public Node {
    std::string tbName;
public:
    explicit DescTable(const char* name) {
        tbName = name;
    }

    void run() override {
        Database::instance() ->getTableByName(tbName)->showTable();
    }
};

class AddPri : public Node {
    std::string tbName;
    std::string priName;
    PrimaryField* field;
public:
    explicit AddPri(const char* name, const char* m_name, IdentList* colList) {
        tbName = name;
        priName = m_name;
        field = new PrimaryField(colList);
    }

    void run() override {
        field->setPri(Database::instance() ->getTableByName(tbName), priName);
    }

    ~AddPri() override {
        delete field;
    }
};

class AddIndex : public Node {
    std::string tbName;
    std::string ixName;
    IdentList* identList;
    std::string ixClass = "";
public:
    explicit AddIndex(const char* name, const char* m_name, IdentList* colList, const char* c = nullptr) {
        tbName = name;
        ixName = m_name;
        identList = colList;
        if(c){
            ixClass = c;
        }
    }

    void run() override {
        Database::instance() ->getTableByName(tbName)->addIndex(identList->list, ixName, ixClass);
    }

    ~AddIndex(){
        delete identList;
    }
};

class DropIndex : public Node {
    std::string tbName;
    std::string ixName;
public:
    explicit DropIndex(const char* name, const char* m_name) {
        tbName = name;
        ixName = m_name;
    }

    void run() override {
        Database::instance() ->getTableByName(tbName)->dropIndex(ixName);
    }
};

class AddColumn : public Node {
    std::string tbName;
    Field* __field;
public:
    explicit AddColumn(const char* name, Field* field) {
        tbName = name;
        __field = field;
    }

    void run() override {
        __field->addColumn(Database::instance() ->getTableByName(tbName));
    }

    ~AddColumn() override {
        delete __field;
    }
};

class DropColumn : public Node {
    std::string tbName;
    char __colName[MAX_NAME_LEN];
public:
    explicit DropColumn(const char* name, const char* colName) {
        tbName = name;
        memcpy(__colName, colName, MAX_NAME_LEN);
    }

    void run() override {
        Database::instance() ->getTableByName(tbName)->dropColumn(__colName);
    }
};

class ChangeColumn : public Node {
    std::string tbName;
    char __colName[MAX_NAME_LEN];
    Field* __field;
public:
    explicit ChangeColumn(const char* name, const char* colName, Field* field) {
        tbName = name;
        memcpy(__colName, colName, MAX_NAME_LEN);
        __field = field;
    }

    void run() override {
        __field->changeColumn(Database::instance() ->getTableByName(tbName), __colName);
    }

    ~ChangeColumn() override {
        delete __field;
    }
};

class RenameTable : public Node {
    std::string tbName;
    std::string newTbName;
public:
    explicit RenameTable(const char* name, const char* newName) {
        tbName = name;
        newTbName = newName;
    }

    void run() override {
        Database::instance() ->getTableByName(tbName)->renameTable(newTbName);
    }
};

class DropPri : public Node {
    std::string tbName;
public:
    explicit DropPri(const char* name) {
        tbName = name;
    }

    void run() override {
        Database::instance() ->getTableByName(tbName)->dropPrimary();
    }
};

class AddForeign : public Node {
    std::string tbName;
    std::string ixName;
    IdentList* identList;
    std::string refTbName;
    IdentList* refKeys;
public:
    explicit AddForeign(const char* name, const char* m_name, IdentList* colList, const char* m_refTbName, IdentList* m_refKeys) {
        tbName = name;
        ixName = m_name;
        identList = colList;
        refTbName = m_refTbName;
        refKeys = m_refKeys;
    }

    void run() override {
        Table* table = Database::instance()->getTableByName(refTbName);
        Database::instance() ->getTableByName(tbName)->addForeignKey(identList->list, ixName, refTbName, refKeys->list, table);
    }

    ~AddForeign() override {
        delete identList;
        delete refKeys;
    }
};
# endif