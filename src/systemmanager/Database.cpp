# include "Database.h"

int Database::destroyDatabase(){
    for (int i = 0; i < tables.size(); i ++){
        tables[i]->destroyTable();
    }
    return rmdir(dbname.c_str());
}

int Database::openDatabase(){
    DIR *p_dir = NULL;
    struct dirent *p_entry = NULL;
    struct stat statbuf;
    std::string path = dbname;

    if((p_dir = opendir(path.c_str())) == NULL){
        throw "error: opendir " +  path + " fail";
    }

    int err = chdir(path.c_str());
    if (err){
        throw "error: chdir " +  path + " fail";
    }

    while(NULL != (p_entry = readdir(p_dir))) { // 获取下一级目录信息
        lstat(p_entry->d_name, &statbuf);   // 获取下一级成员属性
        if(S_IFDIR & statbuf.st_mode) {      // 判断下一级成员是否是目录
            std::string subName = p_entry->d_name;  
            if (subName == "." || subName == "..")
                continue;
            Table* temp = new Table(dbname, subName);
            temp->openTable();
            tables.push_back(temp);
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
    return err;
}
int Database::closeDatabase(){
    for (int i = 0; i < tables.size(); i ++){
        tables[i]->closeTable();
    }
}

Table* Database::getTableByName(const std::string& name) {
    for (int i = 0; i < tables.size(); i++){
        if (tables[i]->name == name){
            return tables[i];
        }
    }
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

Database::Database(char m_dbname[MAX_NAME_LEN]):dbname(m_dbname){
    int err = mkdir(dbname.c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
    if (err){
        throw "error: mkdir " + dbname + " fail";
    }
}