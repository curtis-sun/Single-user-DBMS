# ifndef DATABASE
# define DATABASE

# include "Table.h"
# include <vector>

class Database{
    std::string dbname;
    std::vector<Table*> tables;
public:
    int destroyDatabase();
    int openDatabase();
    int closeDatabase();
    
    Table* createTable(const std::string& name);
    int dropTableByName(const std::string& name);
    Table* getTableByName(const std::string& name);

    Database(char m_dbname[MAX_NAME_LEN]);
};
# endif