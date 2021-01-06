# ifndef TABLE
# define TABLE

# include "../recordmanager/RecordManager.h"
# include "../index/IX_Manager.h"
# include "../bufmanager/BufPageManager.h"
# include <vector>

class Table{
    std::vector<std::string> files;

    std::string __tableName(){
        return __tablePath() + "/tb";
    }
    std::string __tableTemp(){
        return __tablePath() + "/tb_temp";
    }
    std::string __tablePath(){
        return "data/" + dbname + "/" + name;
    }
    std::string __tablePath(const std::string& subName){
        if (subName.length() != 0){
            return "data/" + dbname + "/" + name + "/" + subName;
        }
        return __tablePath();
    }
    void __setNotNull(uint8_t colId);
    void __setDefault(uint8_t colId, char defaultValue[]);
    void __addIm(IX_Manager* im);
    void __addIndex(const std::vector<std::string>& c_names,  const std::string& ixName, const std::string& ixClass);
    void __addForeignKey(const std::vector<std::string>& c_names,  const std::string& ixName, const std::string& refTbName, const std::vector<std::string>& refKeys);
    int __loadIndexFromTable(int index);
    int __loadIndex(const std::string& subName = "");
    void __loadFiles(const std::string& subName = "");
public:
    int __colId(const std::string& colName);
    void setNotNull(uint8_t colId);
    void setDefault(uint8_t colId, char defaultValue[]);
    void setPrimary(const std::vector<std::string>& c_names, std::string priName);
    void dropPrimary();
    int addForeignKey(const std::vector<std::string>& c_names, const std::string& ixName, const std::string& refTbName, const std::vector<std::string>& refKeys, Table* refTb);

    int createTable(const std::vector<AttrType>& types, const std::vector<int>& colLens, const std::vector<std::string>& names, const std::vector<bool>& notNulls, const std::vector<char*>& defaults);
    int destroyTable();
    int openTable();
    int closeTable();
    void showTable();

    int addColumn(AttrType type, int colLen, char colName[MAX_NAME_LEN], bool notNull = false, char defaultValue[] = nullptr);
    int dropColumn(char colName[MAX_NAME_LEN]);
    int changeColumn(char colName[MAX_NAME_LEN], AttrType type, int colLen, char newColName[MAX_NAME_LEN], bool notNull = false, char defaultValue[] = nullptr);
    int reorganize(TableHeader* newHeader);

    int addIndex(const std::vector<std::string>& c_names, const std::string& ixName, const std::string& ixClass);
    void dropIndex(std::string ixName);
    void dropIndex(int index);

    void renameTable(const std::string& newName);
    
    std::string dbname, name;
    TableHeader* header;
    RecordManager* rm;
    std::vector<IX_Manager*> ims;
    IX_Manager* priIx;
    
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