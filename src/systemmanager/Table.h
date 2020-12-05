# ifndef TABLE
# define TABLE

# include "../recordmanager/RecordManager.h"
# include "../index/IX_Manager.h"
# include "../bufmanager/BufPageManager.h"
# include <vector>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <dirent.h>
# include <fcntl.h>
# include <stdlib.h>

class Table{
    std::string dbname, name;
    RecordManager* rm;
    TableHeader* header;
    std::vector<void*> ims;
    std::vector<std::vector<std::string>> ikeys;
    int priKey = -1;

    std::string __tableName(std::string name){
        return dbname + "/" + name + "/tb";
    }
    std::string __tablePath(std::string name){
        return dbname + "/" + name;
    }
    std::string __tablePath(std::string name, std::string subName){
        if (subName.length() != 0){
            return dbname + "/" + name + "/" + subName;
        }
        return __tablePath(name);
    }
    std::uint8_t __colId(TableHeader* header, std::string colName);
    void __setNotNull(TableHeader* header, uint8_t colId);
    void __setDefault(TableHeader* header, uint8_t colId, char defaultValue[]);
    void __addIndex(std::vector<std::string>& c_names, std::string ixName = ""){
        char names[c_names.size()][MAX_NAME_LEN];
        for (int i = 0; i < c_names.size(); i ++){
            memcpy(names[i], c_names[i].c_str(), MAX_NAME_LEN);
        }
        ikeys.push_back(c_names);
        if (c_names.size() == 1){
            uint8_t colId = __colId(header, c_names[0]);
            switch(header->columnTypes[colId]){
                case INTEGER: 
                case DATE: {
                    ims.push_back(new IX_Manager<int>(__tablePath(name, ixName), names, c_names.size()));
                    break;
                }
                case STRING: {
                    ims.push_back(new IX_Manager<std::string>(__tablePath(name, ixName), names, c_names.size()));
                    break;
                }
                case FLOAT: {
                    ims.push_back(new IX_Manager<float>(__tablePath(name, ixName), names, c_names.size()));
                    break;
                }
                default: {}
            }
        }
        else{
            ims.push_back(new IX_Manager<MultiCol>(__tablePath(name, ixName), names, c_names.size()));
        }
        if (ixName == "pri"){
            priKey = ikeys.size() - 1;
        }
    }
    int __loadIndexFromTable(int index){
        int columnCnt = ikeys[index].size();
        uint8_t colIds[columnCnt];
        for (int i = 0; i < columnCnt; i ++){
            colIds[i] = __colId(header, ikeys[index][i]);
        }
        if (columnCnt > 1){
            rm ->fileScan->openScan(STRING, header->pminlen, 0, NO_OP, rm->defaultRow);
            char data[header->pminlen];
            char temp[header->pminlen];
            RID_t rid;
            while(!rm->fileScan->getNextEntry(data, rid)){
                AttrVal vals[columnCnt];
                MultiCol multiCol;
                multiCol.len = columnCnt;
                for (int i = 0; i < columnCnt; i ++){
                    vals[i].type = header->columnTypes[colIds[i]];
                    memcpy(temp, data + header->columnOffsets[colIds[i]], header->columnOffsets[colIds[i] + 1] - header->columnOffsets[colIds[i]]);
                    switch(vals[i].type){
                        case INTEGER: 
                        case DATE: {
                            vals[i].val.i = stoi(temp);
                            break;
                        }
                        case STRING: {
                            memcpy(vals[i].s, temp, sizeof(temp));
                            break;
                        }
                        case FLOAT: {
                            vals[i].val.f = stof(temp);
                            break;
                        }
                        default: {}
                    }
                }
                memcpy(multiCol.vals, vals, sizeof(vals));
                ((IX_Manager<MultiCol>*)ims[index]) -> insertEntry(rid, multiCol);
            }
        }
        else{
            AttrType type = header->columnTypes[colIds[0]];
            int colLen = header->columnOffsets[colIds[0] + 1] - header->columnOffsets[colIds[0]];
            rm ->fileScan->openScan(type, colLen, header->columnOffsets[colIds[0]], NO_OP, rm->defaultRow);
            char data[colLen];
            RID_t rid;
            while(!rm->fileScan->getNextEntry(data, rid)){
                switch(type){
                    case DATE:
                    case INTEGER:{
                        int op = *(int *)(data);
                        ((IX_Manager<int>*)ims[index]) -> insertEntry(rid, op);
                        break;
                    }
                    case FLOAT:{
                        float op = *(float *)(data);
                        ((IX_Manager<float>*)ims[index]) -> insertEntry(rid, op);
                        break;
                    }
                    case STRING:{
                        std::string op = data;
                        ((IX_Manager<std::string>*)ims[index]) -> insertEntry(rid, op);
                        break;
                    }
                    default: {}
                }
            }
        }
        return 0;
    }
    int __loadIndex(std::string subName = ""){
        DIR *p_dir = NULL;
        struct dirent *p_entry = NULL;
        struct stat statbuf;
        std::string path = __tablePath(name, subName);

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
                    if (subName == "pri"){
                        __loadIndex("pri");
                    }
                } else {
                    std::vector<std::string> names;
                    std::string name = p_entry->d_name;
                    int oldPosition = name.find("_");
                    int position = oldPosition + 1;
                    while((position = name.find("_", position)) != std::string::npos){
                        names.push_back(name.substr(oldPosition + 1, position - oldPosition - 1));
                        oldPosition = position;
                        position ++;
                    }
                    __addIndex(names, subName);
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
public:
    void setNotNull(uint8_t colId);
    void setDefault(uint8_t colId, char defaultValue[]);
    void setPrimary(std::vector<std::string>& c_names){
        mkdir(__tablePath(name, "pri").c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
        addIndex(c_names, "pri");
    }

    void dropPrimary(){
        dropIndex(ikeys[priKey]);
        priKey = -1;
        rmdir(__tablePath(name, "pri").c_str());
    }
    int createTable(AttrType types[], int colLens[], char names[][MAX_NAME_LEN], int colNum);
    int destroyTable();
    int openTable(){
        int err = rm ->openFile();
        if (err){
            throw "error: open table " + name + " fail";
        }
        __loadIndex();
        return err;
    }
    int closeTable();
    void showTable();

    int addColumn(AttrType type, int colLen, char colName[MAX_NAME_LEN], bool notNull = false);
    int dropColumn(char colName[MAX_NAME_LEN]);
    int changeColumn(char colName[MAX_NAME_LEN], AttrType type, int colLen, char newColName[MAX_NAME_LEN], bool notNull = false);
    int reorganize(TableHeader* newHeader);

    int addIndex(std::vector<std::string>& c_names, std::string ixName = ""){
        __addIndex(c_names, ixName);
        return __loadIndexFromTable(ims.size() - 1);
    }

    void dropIndex(std::vector<std::string>& c_names){
        for (int i = 0; i < ikeys.size(); i ++){
            if (ikeys[i] == c_names){
                if (c_names.size() == 1){
                    uint8_t colId = __colId(header, c_names[0]);
                    switch(header->columnTypes[colId]){
                        case INTEGER: 
                        case DATE: {
                            ((IX_Manager<int>*)ims[i]) ->destroyIndex();
                            break;
                        }
                        case STRING: {
                            ((IX_Manager<std::string>*)ims[i]) ->destroyIndex();
                            break;
                        }
                        case FLOAT: {
                            ((IX_Manager<float>*)ims[i]) ->destroyIndex();
                            break;
                        }
                        default: {}
                    }
                }
                else{
                    ((IX_Manager<MultiCol>*)ims[i]) ->destroyIndex();
                }
                ikeys.erase(ikeys.begin() + i);
                ims.erase(ims.begin() + i);
                break;
            }
        }
    }
    
    Table(char m_dbname[MAX_NAME_LEN], char m_name[MAX_NAME_LEN]);
    ~Table(){
        delete header;
        delete rm;
    }
};
# endif