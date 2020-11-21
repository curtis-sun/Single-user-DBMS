# ifndef TABLE
# define TABLE

# include "../recordmanager/RecordManager.h"
# include "../bufmanager/BufPageManager.h"

class Table{
    std::string dbname, name;
    RecordManager* rm;
    TableHeader* header;

    std::string __tableName(std::string dbname, std::string name){
        return dbname + "/" + name + "/tb";
    }
    std::string __tablePath(std::string dbname, std::string name){
        return dbname + "/" + name;
    }
    std::uint8_t __colId(TableHeader* header, char colName[MAX_NAME_LEN]);
public:
    int createTable(AttrType types[], int colLens[], char names[][MAX_NAME_LEN], int colNum);
    int destroyTable();
    int openTable();
    int closeTable();
    void showTable();

    int addColumn(AttrType type, int colLen, char colName[MAX_NAME_LEN]);
    int dropColumn(char colName[MAX_NAME_LEN]);
    int changeColumn(char colName[MAX_NAME_LEN], AttrType type, int colLen, char newColName[MAX_NAME_LEN]);
    int reorganize(TableHeader* newHeader);
    Table(char m_dbname[MAX_NAME_LEN], char m_name[MAX_NAME_LEN]);
    ~Table(){
        delete header;
        delete rm;
    }
};
# endif