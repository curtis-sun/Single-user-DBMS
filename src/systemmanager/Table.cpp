# include "Table.h"
# include <cstring>
# include <cassert>
# include <string>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <dirent.h>
# include <fcntl.h>
# include <stdlib.h>
# include <cmath>
# include "../utils/compare.cpp"

static std::string ixNames[] = {"", "pri", "foreign"};

std::uint8_t Table::__colId(const std::string& colName){
    assert(header != nullptr);
    for (int i = 0; i < header->columnCnt; i ++){
        if (colName == header->columnNames[i]){
            return i;
        }
    }
    return (uint8_t)-1;
}

void Table::__setNotNull(uint8_t colId){
    header->constraints[colId] |= 4;
}

void Table::__setDefault(uint8_t colId, char defaultValue[]){
    header->constraints[colId] |= 1;
    memcpy(rm ->defaultRow + header->columnOffsets[colId], defaultValue, header->columnOffsets[colId + 1] - header->columnOffsets[colId]);
}

void Table::__addIndex(const std::vector<std::string>& c_names,  const std::string& ixName, const std::string& ixClass){
    if (ixClass == "pri"){
        if (priIx){
            throw "error: more than one primary key defined";
        }
    }
    ims.push_back(new IX_Manager(__tablePath(ixClass), ixName, c_names));
    if (ixClass == "pri"){
        priIx = ims[ims.size() - 1];
    }
}

void Table:: __addForeignKey(const std::vector<std::string>& c_names,  const std::string& ixName, const std::string& refTbName, const std::vector<std::string>& refKeys){
    ims.push_back(new IX_Manager(__tablePath("foreign"), ixName, c_names, refTbName, refKeys));
    foreignIxs.push_back(ims[ims.size() - 1]);
}

int Table::__loadIndexFromTable(int index){
    int columnCnt = ims[index]->keys.size();
    uint8_t colIds[columnCnt];
    for (int i = 0; i < columnCnt; i ++){
        colIds[i] = __colId(ims[index]->keys[i]);
    }
    rm ->fileScan->openScan();
    char data[header->pminlen];
    RID_t rid;
    while(!rm->fileScan->getNextEntry(data, rid)){
        Entry entry;
        entry.rid = rid;
        for (int i = 0; i < columnCnt; i ++){
            entry.vals[i] = recordToAttr(data + header->columnOffsets[colIds[i]], header->columnOffsets[colIds[i] + 1] - header->columnOffsets[colIds[i]], header->columnTypes[colIds[i]]);
        }
        ims[index] -> insertEntry(entry);
    }
    return 0;
}

void Table::__loadFiles(const std::string& subName){
    DIR *p_dir = NULL;
    struct dirent *p_entry = NULL;
    struct stat statbuf;
    std::string path = subName;
    if (subName.length() == 0){
        path = __tablePath();
        files.clear();
    }

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
            std::string ixClass = p_entry->d_name;  
            if (ixClass == "." || ixClass == "..")
                continue;
            if (ixClass == "pri"){
                __loadFiles("pri");
            }
            else if (ixClass == "foreign"){
                __loadFiles("foreign");
            }
        } else {
            files.push_back(__tablePath(subName) + "/" + p_entry->d_name);
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
}

int Table::__loadIndex(const std::string& subName){
    // initialize index
    ims.clear();
    priIx = nullptr;
    foreignIxs.clear();

    DIR *p_dir = NULL;
    struct dirent *p_entry = NULL;
    struct stat statbuf;
    std::string path = subName;
    if (subName.length() == 0){
        path = __tablePath();
    }

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
            std::string ixClass = p_entry->d_name;  
            if (ixClass == "." || ixClass == "..")
                continue;
            if (ixClass == "pri"){
                __loadIndex("pri");
            }
            else if (ixClass == "foreign"){
                __loadIndex("foreign");
            }
        } else {
            std::vector<std::string> names;
            std::string name = p_entry->d_name;
            if (subName == "foreign"){
                int midPosition = name.find("#");
                int oldPosition = name.find("%");
                std::string ixName = name.substr(0, oldPosition);
                int position = oldPosition + 1;
                while((position = name.find("%", position)) < midPosition){
                    names.push_back(name.substr(oldPosition + 1, position - oldPosition - 1));
                    oldPosition = position;
                    position ++;
                }
                names.push_back(name.substr(oldPosition + 1, midPosition - oldPosition - 1));
                std::string refTbName = name.substr(midPosition + 1, position - midPosition - 1);
                oldPosition = position;
                position ++;
                std::vector<std::string> refKeys;
                while((position = name.find("%", position)) != std::string::npos){
                    refKeys.push_back(name.substr(oldPosition + 1, position - oldPosition - 1));
                    oldPosition = position;
                    position ++;
                }
                refKeys.push_back(name.substr(oldPosition + 1));
                __addForeignKey(names, ixName, refTbName, refKeys);
            }
            else{
                if (name == "tb" || name == "tb_temp"){
                    continue;
                }
                int oldPosition = name.find("%");
                std::string ixName = name.substr(0, oldPosition);
                int position = oldPosition + 1;
                while((position = name.find("%", position)) != std::string::npos){
                    names.push_back(name.substr(oldPosition + 1, position - oldPosition - 1));
                    oldPosition = position;
                    position ++;
                }
                names.push_back(name.substr(oldPosition + 1));
                __addIndex(names, ixName, subName);
            }
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
    return err;
}

void Table::setNotNull(uint8_t colId){
    __setNotNull(colId);
}

void Table::setDefault(uint8_t colId, char defaultValue[]){
    __setDefault(colId, defaultValue);
}

void Table::setPrimary(const std::vector<std::string>& c_names, std::string priName){
    __addIndex(c_names, priName, "pri");
    __loadIndexFromTable(ims.size() - 1);
}

void Table::dropPrimary(){
    if (!priIx){
        throw "error: primary key is undefined";
    }
    dropIndex(priIx->ixName);
}

int Table::addForeignKey(const std::vector<std::string>& c_names, const std::string& ixName, const std::string& refTbName, const std::vector<std::string>& refKeys){
    __addForeignKey(c_names, ixName, refTbName, refKeys);
    return __loadIndexFromTable(ims.size() - 1);
}

int Table::createTable(const std::vector<AttrType>& types, const std::vector<int>& colLens, const std::vector<std::string>& names){
    for (int i = 0; i < 3; i ++){
        int err = mkdir(__tablePath(ixNames[i]).c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
        if (err){
            throw "error: create table dir fail " + to_string(errno);
        }
    }
    header->nextAvailable = (RID_t) -1;
    header->pageCnt = 0;
    header->columnCnt = 1 + names.size();
    header->columnOffsets[0] = 0;
    header->columnOffsets[1] = 8;
    header->columnTypes[0] = INTEGER;
    header->constraints[0] = 4; // not null
    for (int i = 0; i < names.size(); i ++){
        header->columnOffsets[i + 2] = header->columnOffsets[i + 1] + colLens[i];
        memcpy(header->columnNames[i + 1], names[i].c_str(), MAX_NAME_LEN);
        header->columnTypes[i + 1] = types[i];
    }
    header->pminlen = header->columnOffsets[header->columnCnt];
    int err = rm ->createFile();
    if (err){
        throw "error: create table " + name + " fail";
    }
    return err;
}

int Table::destroyTable(){
    for (int i = 0; i < ims.size(); i ++){
        ims[i]->destroyIndex();
    }
    rmdir(__tablePath("pri").c_str());
    rmdir(__tablePath("foreign").c_str());
    int err = rm ->destroyFile();
    if (err){
        throw "error: drop table " + name + " fail";
    }
    err = rmdir(__tablePath().c_str());
    if (err){
        throw "error: rmdir " + __tablePath() + " fail";
    }
    return err;
}

int Table::openTable(){
    int err = rm ->openFile();
    if (err){
        throw "error: open table " + name + " fail";
    }
    return __loadIndex();
}

int Table::closeTable(){
    for (int i = 0; i < ims.size(); i ++){
        ims[i]->closeIndex();
    }
    int err = rm -> closeFile();
    if (err){
        throw "error: open table " + name + " fail";
    }
    return err;
}

void Table::showTable(){
    int maxNameLen = 6, maxDefaultLen = 7;
    for (int i = 0; i < header->columnCnt; i ++){
        int nameLen = strlen(header->columnNames[i]);
        if (nameLen > maxNameLen){
            maxNameLen = nameLen;
        }
        int valLen = header->columnOffsets[i + 1] - header->columnOffsets[i] - 1;
        if (valLen > maxDefaultLen){
            maxDefaultLen = valLen;
        }
    }
    printf(" Table %s.%s\n", dbname.c_str(), name.c_str());
    printf(" %-*s ", maxNameLen, "Column");
    printf("| Type        | Nullable |");
    printf(" %-*s\n-", maxDefaultLen, "Default");
    for(int i = 0; i < maxNameLen; i ++){
        printf("-");
    }
    printf("-+-------------+----------+-");
    for(int i = 0; i < maxDefaultLen; i ++){
        printf("-");
    }
    printf("\n");
    std::string type; 
    char defaultValue[maxDefaultLen];
    for (int i = 1; i < header->columnCnt; i ++){
        printf(" %-*s ", maxNameLen, header->columnNames[i]);
        switch(header->columnTypes[i]){
            case INTEGER: {
                if (header->columnOffsets[i + 1] - header->columnOffsets[i] == 5){
                    type = "int";
                }
                else{
                    type = "int(" + to_string(header->columnOffsets[i + 1] - header->columnOffsets[i] - 1) + ")";
                }
                break;
            }
            case STRING: {
                type = "varchar(" + to_string(header->columnOffsets[i + 1] - header->columnOffsets[i] - 1) + ")";
                break;
            }
            case FLOAT: {
                type = "float";
                break;
            }
            case DATE: {
                type = "date";
                break;
            }
            default: {
                throw "error: unknown value type";
            }
        }
        printf("| %-11s |", type.c_str());
        if (header->constraints[i] & 4){
            printf(" not null |");
        }
        else{
            printf("          |");
        }
        if (header->constraints[i] & 1){
            memcpy(defaultValue, rm ->defaultRow + header->columnOffsets[i] + 1, header->columnOffsets[i + 1] - header->columnOffsets[i] - 1);
            printf(" %-*s\n", maxDefaultLen, defaultValue);
        }
        else{
            printf(" %-*s\n", maxDefaultLen, "");
        }
    }
}

int Table::addIndex(const std::vector<std::string>& c_names, const std::string& ixName){
    __addIndex(c_names, ixName);
    return __loadIndexFromTable(ims.size() - 1);
}

void Table::dropIndex(std::string ixName){
    for (int i = 0; i < ims.size(); i ++){
        if (ims[i]->ixName == ixName){
            dropIndex(i);
            return;
        }
    }
    throw "error: no index named " + ixName;
}

void Table::dropIndex(int i){
    IX_Manager* im = ims[i];
    ims.erase(ims.begin() + i);
    if (im == priIx){
        priIx = nullptr;
    }
    else{
        for (int j = 0; j < foreignIxs.size(); j ++){
            if (foreignIxs[j] == im){
                foreignIxs.erase(foreignIxs.begin() + j);
                break;
            }
        }
    }
    im ->destroyIndex();
}

int Table::addColumn(AttrType type, int colLen, char colName[MAX_NAME_LEN], bool notNull, char defaultValue[]){
    if (notNull && !defaultValue){
        throw "error: add column not null must have default value";
    }
    TableHeader* newHeader = new TableHeader(*header);
    newHeader->pageCnt = 0;
    newHeader->nextAvailable = (RID_t) -1;
    memcpy(newHeader->columnNames[newHeader->columnCnt], colName, MAX_NAME_LEN);
    newHeader->columnOffsets[newHeader->columnCnt + 1] = newHeader->columnOffsets[newHeader->columnCnt] + colLen;
    newHeader->columnTypes[newHeader->columnCnt] = type;
    newHeader->columnCnt ++;
    newHeader->pminlen = newHeader->columnOffsets[newHeader->columnCnt];
    
    RecordManager* newRm = new RecordManager(__tablePath(), newHeader, __tableTemp());
    int err = newRm ->createFile();
    if (err){
        throw "error: reorganize table " + name + " fail to create new table";
    }

    rm ->fileScan->openScan();
    char data[header->pminlen];
    char newData[newHeader->pminlen];
    RID_t rid;
    while(!rm->fileScan->getNextEntry(data, rid)){
        memcpy(newData, data, header->pminlen);
        if (defaultValue){
            memcpy(newData + header->pminlen, defaultValue, colLen);
        }
        else{
            newData[header->pminlen] = 0;
        }
        RID_t tempRid;
        newRm ->insertRecord(newData, tempRid);
    }
    err = newRm->closeFile();
    if (err){
        throw "error: reorganize table " + name + " fail to close new table";
    }
    err = rm ->destroyFile();
    if (err){
        throw "error: reorganize table " + name + " fail to drop old table";
    }
    delete rm;
    delete header;

    err = rename(__tableTemp().c_str(), __tableName().c_str());
    if (err){
        throw "error: reorganize table " + name + " fail to rename new table";
    }
    newRm->tableName = __tableName();
    err = newRm->openFile();
    if (err){
        throw "error: reorganize table " + name + " fail to open new table";
    }
    rm = newRm;
    header = newHeader;
    if (notNull){
        __setNotNull(newHeader->columnCnt - 1);
    }
    if (defaultValue){
        __setDefault(header->columnCnt - 1, defaultValue);
    }

    for (int i = 0; i < ims.size(); i ++){
        ims[i] ->destroyIndex();
        __loadIndexFromTable(i);
    }
    return 0;
}

int Table::dropColumn(char colName[MAX_NAME_LEN]){
    TableHeader* newHeader = new TableHeader(*header);
    newHeader->pageCnt = 0;
    newHeader->nextAvailable = (RID_t) -1;
    uint8_t colId = __colId(colName);
    newHeader->columnCnt --;
    uint16_t colLen = newHeader->columnOffsets[colId + 1] - newHeader->columnOffsets[colId];
    for (int i = colId; i < newHeader->columnCnt; i ++){
        memcpy(newHeader->columnNames[i], newHeader->columnNames[i + 1], MAX_NAME_LEN);
        newHeader->columnTypes[i] = newHeader->columnTypes[i + 1];
        newHeader->columnOffsets[i + 1] = newHeader->columnOffsets[i + 2] - colLen;
    }
    newHeader->pminlen = newHeader->columnOffsets[newHeader->columnCnt];
    
    RecordManager* newRm = new RecordManager(__tablePath(), newHeader,  __tableTemp());
    int err = newRm ->createFile();
    if (err){
        throw "error: reorganize table " + name + " fail to create new table";
    }

    rm ->fileScan->openScan();
    char data[header->pminlen];
    char newData[newHeader->pminlen];
    RID_t rid;
    while(!rm->fileScan->getNextEntry(data, rid)){
        memcpy(newData, data, newHeader->columnOffsets[colId]);
        memcpy(newData + newHeader->columnOffsets[colId], data + header->columnOffsets[colId + 1], header->pminlen - header->columnOffsets[colId + 1]);
        RID_t tempRid;
        newRm ->insertRecord(newData, tempRid);
    }
    err = newRm->closeFile();
    if (err){
        throw "error: reorganize table " + name + " fail to close new table";
    }
    err = rm ->destroyFile();
    if (err){
        throw "error: reorganize table " + name + " fail to drop old table";
    }

    err = rename(__tableTemp().c_str(), __tableName().c_str());
    if (err){
        throw "error: reorganize table " + name + " fail to rename new table";
    }
    newRm->tableName = __tableName();
    err = newRm->openFile();
    if (err){
        throw "error: reorganize table " + name + " fail to open new table";
    }

    delete rm;
    delete header;
    rm = newRm;
    header = newHeader;

    for (int i = 0; i < ims.size(); i ++){
        if (std::find(ims[i]->keys.begin(), ims[i]->keys.end(), colName) != ims[i]->keys.end()){
            dropIndex(i);
            i--;
        }
        else{
            ims[i]->destroyIndex();
            __loadIndexFromTable(i);
        }
    }
    return 0;
}

int Table::changeColumn(char colName[MAX_NAME_LEN], AttrType newType, int newColLen, char newColName[MAX_NAME_LEN], bool notNull, char defaultValue[]){
    TableHeader* newHeader = new TableHeader(*header);
    uint8_t colId = __colId(colName);
    memcpy(newHeader->columnNames[colId], newColName, MAX_NAME_LEN);
    // 检查类型转换是否可行
    if (!isAttrConvert(header->columnTypes[colId], newType)){
        delete newHeader;
        throw "error: type cast is impossible";
        return 2;
    }
    newHeader->columnTypes[colId] = newType;
    uint16_t colLen = newHeader->columnOffsets[colId + 1] - newHeader->columnOffsets[colId];
    newHeader->pageCnt = 1;
    newHeader->nextAvailable = (RID_t) -1;
    for (int i = colId; i < newHeader->columnCnt; i ++){
        newHeader->columnOffsets[i + 1] = newHeader->columnOffsets[i + 1] + newColLen - colLen;
    }
    newHeader->pminlen = newHeader->columnOffsets[newHeader->columnCnt];

    RecordManager* newRm = new RecordManager(__tablePath(), newHeader, __tableTemp());
    int err = newRm ->createFile();
    if (err){
        throw "error: reorganize table " + name + " fail to create new table";
    }

    rm ->fileScan->openScan();
    char data[header->pminlen];
    char newData[newHeader->pminlen];
    RID_t rid;
    while(!rm->fileScan->getNextEntry(data, rid)){
        memcpy(newData, data, newHeader->columnOffsets[colId]);
        // 类型转换
        if (data[header->columnOffsets[colId]] == 1){
            AttrVal val = recordToAttr(data + header->columnOffsets[colId], colLen, header->columnTypes[colId]);
            attrConvert(val, newType);
            restoreAttr(newData + newHeader->columnOffsets[colId], newColLen, val);
        }
        else{
            // 检查现有数据是否有空数据？
            if (notNull){
                if (defaultValue){
                    memcpy(newData + newHeader->columnOffsets[colId], defaultValue, newColLen);
                }
                else{
                    delete newHeader;
                    delete newRm;
                    throw "error: null exist at row " + to_string(rid);
                }
            }
        }
        memcpy(newData + newHeader->columnOffsets[colId + 1], data + header->columnOffsets[colId + 1], header->pminlen - header->columnOffsets[colId + 1]);
        RID_t tempRid;
        newRm ->insertRecord(newData, tempRid);
    }
    err = newRm->closeFile();
    if (err){
        throw "error: reorganize table " + name + " fail to close new table";
    }
    err = rm ->destroyFile();
    if (err){
        throw "error: reorganize table " + name + " fail to drop old table";
    }

    err = rename(__tableTemp().c_str(), __tableName().c_str());
    if (err){
        throw "error: reorganize table " + name + " fail to rename new table";
    }
    newRm->tableName = __tableName();
    err = newRm->openFile();
    if (err){
        throw "error: reorganize table " + name + " fail to open new table";
    }

    delete rm;
    delete header;
    rm = newRm;
    header = newHeader;

    if (notNull){
        __setNotNull(colId);
    }
    if (defaultValue){
        __setDefault(colId, defaultValue);
    }

    // 相应索引改名
    for (int i = 0; i < ims.size(); i ++){
        ims[i] ->destroyIndex();
        std::vector<std::string>::iterator iter = std::find(ims[i]->keys.begin(), ims[i]->keys.end(), colName);
        if (iter != ims[i]->keys.end()){
            *iter = newColName;
        }
        __loadIndexFromTable(i);
    }
    return 0;
}

void Table::renameTable(const std::string& newName){
    closeTable();
    __loadFiles();
    std::string oldPath = __tablePath();
    name = newName;
    std::string newPath = __tablePath();
    for (int i = 0; i < 3; i ++){
        int err = mkdir(__tablePath(ixNames[i]).c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
        if (err){
            throw "error: create table dir fail " + to_string(errno);
        }
    }

    for (int i = 0; i < files.size(); i ++){
        std::string newFileName = newPath + files[i].substr(oldPath.size());
        int err = rename(files[i].c_str(), newFileName.c_str());
        if (err){
            throw "error: rename table fail " + to_string(errno);
        }
    }
    rmdir((oldPath + "/pri").c_str());
    rmdir((oldPath + "/foreign").c_str());
    int err = rmdir(oldPath.c_str());
    if (err){
        throw "error: rmdir " + oldPath + " fail " + to_string(errno);
    }
    rm->tablePath = __tablePath();
    rm->tableName = __tableName();
    openTable();
}

Table::Table(const std::string& m_dbname, const std::string& m_name):dbname(m_dbname), name(m_name){
    header = new TableHeader;
    memset(header, 0, sizeof(*header));
    rm = new RecordManager(__tablePath(), header, __tableName());
}