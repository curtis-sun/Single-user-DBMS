# ifndef TABLE
# define TABLE

# include "../recordmanager/RecordManager.h"
# include "../index/IX_Manager.h"
# include "../bufmanager/BufPageManager.h"
# include <vector>

class Table{
    std::string dbname, name;
    RecordManager* rm;
    TableHeader* header;
    std::vector<void*> ims;
    std::vector<char (*)[MAX_NAME_LEN]> ikeys;

    std::string __tableName(std::string name){
        return dbname + "/" + name + "/tb";
    }
    std::string __tablePath(std::string name){
        return dbname + "/" + name;
    }
    std::uint8_t __colId(TableHeader* header, char colName[MAX_NAME_LEN]);
    void __setNotNull(TableHeader* header, uint8_t colId);
    void __setDefault(TableHeader* header, uint8_t colId, char defaultValue[]);
public:
    void setNotNull(uint8_t colId);
    void setDefault(uint8_t colId, char defaultValue[]);
    int createTable(AttrType types[], int colLens[], char names[][MAX_NAME_LEN], int colNum);
    int destroyTable();
    int openTable();
    int closeTable();
    void showTable();

    int addColumn(AttrType type, int colLen, char colName[MAX_NAME_LEN], bool notNull = false);
    int dropColumn(char colName[MAX_NAME_LEN]);
    int changeColumn(char colName[MAX_NAME_LEN], AttrType type, int colLen, char newColName[MAX_NAME_LEN], bool notNull = false);
    int reorganize(TableHeader* newHeader);

    int addIndex(char c_names[][MAX_NAME_LEN], uint8_t columnCnt){
        ikeys.push_back(c_names);
        if (columnCnt == 1){
            uint8_t colId = __colId(header, c_names[0]);
            switch(header->columnTypes[colId]){
                case INTEGER: 
                case DATE: {
                    ims.push_back(new IX_Manager<int>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case STRING: {
                    ims.push_back(new IX_Manager<std::string>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case FLOAT: {
                    ims.push_back(new IX_Manager<float>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                default: {}
            }
        }
        else{
            switch(columnCnt){
                case 2: {
                    ims.push_back(new IX_Manager<MultiCol<2>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 3: {
                    ims.push_back(new IX_Manager<MultiCol<3>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 4: {
                    ims.push_back(new IX_Manager<MultiCol<4>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 5: {
                    ims.push_back(new IX_Manager<MultiCol<5>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 6: {
                    ims.push_back(new IX_Manager<MultiCol<6>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 7: {
                    ims.push_back(new IX_Manager<MultiCol<7>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 8: {
                    ims.push_back(new IX_Manager<MultiCol<8>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 9: {
                    ims.push_back(new IX_Manager<MultiCol<9>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 10: {
                    ims.push_back(new IX_Manager<MultiCol<10>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 11: {
                    ims.push_back(new IX_Manager<MultiCol<11>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 12: {
                    ims.push_back(new IX_Manager<MultiCol<12>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 13: {
                    ims.push_back(new IX_Manager<MultiCol<13>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 14: {
                    ims.push_back(new IX_Manager<MultiCol<14>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 15: {
                    ims.push_back(new IX_Manager<MultiCol<15>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                case 16: {
                    ims.push_back(new IX_Manager<MultiCol<16>>(__tablePath(name), c_names, columnCnt));
                    break;
                }
                default: {
                    throw "error: index column number must <= 16";
                    return -1;
                }
            }
        }
        return 0;
    }
    
    Table(char m_dbname[MAX_NAME_LEN], char m_name[MAX_NAME_LEN]);
    ~Table(){
        delete header;
        delete rm;
    }
};
# endif