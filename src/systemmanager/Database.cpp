# include "Database.h"

int Database::createDatabase(){
    return mkdir(__path().c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
}

int Database::destroyDatabase(){
    for (int i = 0; i < tables.size(); i ++){
        tables[i]->destroyTable();
    }
    return rmdir(__path().c_str());
}

int Database::openDatabase(){
    tables.clear();
    std::vector<std::string> tbNames;
    DIR *p_dir = NULL;
    struct dirent *p_entry = NULL;
    struct stat statbuf;
    std::string path = __path();

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
            std::string subName = p_entry->d_name;  
            if (subName == "." || subName == "..")
                continue;
            tbNames.push_back(subName);
        }
    }
    for (int i = 0; i < path.length(); i ++){
        if (path[i] == '/'){
            err = chdir(".."); // 回到上级目录 
            if (err){
                throw "error: chdir .. fail";
            } 
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

    for (int i = 0; i < tbNames.size(); i ++){
        Table* temp = new Table(dbname, tbNames[i]);
        temp->openTable();
        printf("info: table %s has been opened\n", temp->name.c_str());
        tables.push_back(temp);
    }
    printf("info: database %s has been opened\n", dbname.c_str());
    return err;
}
int Database::closeDatabase(){
    for (int i = 0; i < tables.size(); i ++){
        tables[i]->closeTable();
        printf("info: table %s has been closed\n", tables[i]->name.c_str());
    }
    printf("info: database %s has been closed\n", dbname.c_str());
}

void Database::showTables(){
    int maxNameLen = 4;
    for (int i = 0; i < tables.size(); i ++){
        int nameLen = tables[i]->name.length();
        if (nameLen > maxNameLen){
            maxNameLen = nameLen;
        }
    }
    printf(" %-*s\n-", maxNameLen, "Name");
    for(int i = 0; i < maxNameLen; i ++){
        printf("-");
    }
    printf("\n");
    for (int i = 0; i < tables.size(); i ++){
        printf(" %-*s\n", maxNameLen, tables[i]->name.c_str());
    }
}

Table* Database::getTableByName(const std::string& name) {
    for (int i = 0; i < tables.size(); i++){
        if (tables[i]->name == name){
            return tables[i];
        }
    }
    throw "error: no table named " + name + " in database " + dbname;
    return nullptr;
}

Table* Database::createTable(const std::string& name) {
    Table* temp = new Table(dbname, name);
    tables.push_back(temp);
    return temp;
}

int Database::dropTableByName(const std::string& name) {
    int i;
    for (i = 0; i < tables.size(); i++){
        if (tables[i]->name == name){
            break;
        }
    }
    if (i >= tables.size()){
        throw "error: table " +  name + " not found";
        return -1;
    }
    tables[i]->destroyTable();
    tables.erase(tables.begin() + i);
    return 0;
}

Database::Database(const std::string& m_dbname):dbname(m_dbname){}

Database* Database::database = nullptr;