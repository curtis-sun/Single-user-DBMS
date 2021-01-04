# ifndef DATABASE
# define DATABASE

# include "Table.h"
# include <vector>

class Database{
    std::string __path(){
        return "data/" + dbname;
    }
public:
    static Database* instance(){
        if (!database){
            throw "error: no database used";
        }
		return database;
	}
    int createDatabase();
    int destroyDatabase();
    int openDatabase();
    int closeDatabase();
    void showTables();
    
    Table* createTable(const std::string& name);
    int dropTableByName(const std::string& name);
    Table* getTableByName(const std::string& name);

    Database(const std::string& name);

    std::string dbname;
    static Database* database;
    std::vector<Table*> tables;
};
# endif