# ifndef TABLE
# define TABLE

# include "../recordmanager/RecordManager.h"
# include "../index/IX_Manager.h"
# include "../bufmanager/BufPageManager.h"
# include <vector>

class Table{
    RecordManager* rm;
    TableHeader* header;
    std::vector<IX_Manager*> ims;
    int priKey = -1;

    std::string __tableName(const std::string& name){
        return dbname + "/" + name + "/tb";
    }
    std::string __tablePath(const std::string& name){
        return dbname + "/" + name;
    }
    std::string __tablePath(const std::string& name, const std::string& subName){
        if (subName.length() != 0){
            return dbname + "/" + name + "/" + subName;
        }
        return __tablePath(name);
    }
    std::uint8_t __colId(TableHeader* header, const std::string& colName);
    void __setNotNull(TableHeader* header, uint8_t colId);
    void __setDefault(TableHeader* header, uint8_t colId, char defaultValue[]);
    void __addIndex(std::vector<std::string>& c_names, const std::string& ixName = "");
    int __loadIndexFromTable(int index);
    int __loadIndex(const std::string& subName = "");
public:
    void setNotNull(uint8_t colId);
    void setDefault(uint8_t colId, char defaultValue[]);
    void setPrimary(std::vector<std::string>& c_names);
    void dropPrimary();

    int createTable(AttrType types[], int colLens[], char names[][MAX_NAME_LEN], int colNum);
    int destroyTable();
    int openTable();
    int closeTable();
    void showTable();

    int addColumn(AttrType type, int colLen, char colName[MAX_NAME_LEN], bool notNull = false);
    int dropColumn(char colName[MAX_NAME_LEN]);
    int changeColumn(char colName[MAX_NAME_LEN], AttrType type, int colLen, char newColName[MAX_NAME_LEN], bool notNull = false);
    int reorganize(TableHeader* newHeader);

    int addIndex(std::vector<std::string>& c_names, const std::string& ixName = "");
    void dropIndex(std::vector<std::string>& c_names);
    
    std::string dbname, name;
    Table(const std::string& m_dbname, const std::string& m_name);
    ~Table(){
        delete header;
        delete rm;
        for (int i = 0; i < ims.size(); i ++){
            delete ims[i];
        }
    }
};
# endif