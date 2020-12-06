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

std::uint8_t Table::__colId(TableHeader* header, const std::string& colName){
    assert(header != nullptr);
    for (int i = 0; i < header->columnCnt; i ++){
        if (colName == header->columnNames[i]){
            return i;
        }
    }
    return (uint8_t)-1;
}

void Table::__setNotNull(TableHeader* header, uint8_t colId){
    header->constraints[colId] |= 4;
}

void Table::__setDefault(TableHeader* header, uint8_t colId, char defaultValue[]){
    header->constraints[colId] |= 1;
    memcpy(rm ->defaultRow + header->columnOffsets[colId], defaultValue, header->columnOffsets[colId + 1] - header->columnOffsets[colId]);
}

void Table::__addIndex(std::vector<std::string>& c_names, const std::string& ixName){
    ims.push_back(new IX_Manager(__tablePath(name, ixName), c_names));
    if (ixName == "pri"){
        priKey = ims.size() - 1;
    }
}

int Table::__loadIndexFromTable(int index){
    int columnCnt = ims[index]->keys.size();
    uint8_t colIds[columnCnt];
    for (int i = 0; i < columnCnt; i ++){
        colIds[i] = __colId(header, ims[index]->keys[i]);
    }
    rm ->fileScan->openScan(STRING, header->pminlen, 0, NO_OP, rm->defaultRow);
    char data[header->pminlen];
    char temp[header->pminlen];
    RID_t rid;
    while(!rm->fileScan->getNextEntry(data, rid)){
        Entry entry;
        entry.rid = rid;
        for (int i = 0; i < columnCnt; i ++){
            memcpy(temp, data + header->columnOffsets[colIds[i]], header->columnOffsets[colIds[i] + 1] - header->columnOffsets[colIds[i]]);
            if (temp[0] == 0){
                continue;
            }
            entry.vals[i].type = header->columnTypes[colIds[i]];
            switch(entry.vals[i].type){
                case INTEGER: 
                case DATE: {
                    entry.vals[i].val.i = stoi(temp + 1);
                    break;
                }
                case STRING: {
                    memcpy(entry.vals[i].s, temp + 1, header->columnOffsets[colIds[i] + 1] - header->columnOffsets[colIds[i]] - 1);
                    break;
                }
                case FLOAT: {
                    entry.vals[i].val.f = stof(temp + 1);
                    break;
                }
                default: {}
            }
        }
        ims[index] -> insertEntry(entry);
    }
    return 0;
}

int Table::__loadIndex(const std::string& subName){
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
    delete p_entry;
    delete p_dir;
    return err;
}

void Table::setNotNull(uint8_t colId){
    __setNotNull(header, colId);
}

void Table::setDefault(uint8_t colId, char defaultValue[]){
    __setDefault(header, colId, defaultValue);
}

void Table::setPrimary(std::vector<std::string>& c_names){
    mkdir(__tablePath(name, "pri").c_str(), S_IRUSR | S_IWUSR | S_IXUSR | S_IRWXG | S_IRWXO);
    addIndex(c_names, "pri");
}

void Table::dropPrimary(){
    dropIndex(ims[priKey]->keys);
    priKey = -1;
    rmdir(__tablePath(name, "pri").c_str());
}

int Table::createTable(AttrType types[], int colLens[], char names[][MAX_NAME_LEN], int colNum){
    header->nextAvailable = (RID_t) -1;
    header->pageCnt = 1;
    header->columnCnt = 1 + colNum;
    header->columnOffsets[0] = 0;
    header->columnOffsets[1] = 8;
    header->columnTypes[0] = INTEGER;
    header->constraints[0] = 4; // not null
    for (int i = 0; i < colNum; i ++){
        header->columnOffsets[i + 2] = header->columnOffsets[i + 1] + colLens[i];
        memcpy(header->columnNames[i + 1], names[i], MAX_NAME_LEN);
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
    if (priKey >= 0){
        rmdir(__tablePath(name, "pri").c_str());
    }
    int err = rm ->destroyFile();
    if (err){
        throw "error: drop table " + name + " fail";
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
    printf(" Table %s.%s\n", dbname, name);
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
    for (int i = 0; i < header->columnCnt; i ++){
        printf(" %-*s ", maxNameLen, header->columnNames[i]);
        switch(header->columnTypes[i]){
            case INTEGER: {
                type = "int(" + to_string((header->columnOffsets[i + 1] - header->columnOffsets[i] - 1) / 4) + ")";
                break;
            }
            case STRING: {
                type = "char(" + to_string(header->columnOffsets[i + 1] - header->columnOffsets[i] - 1) + ")";
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
                type = "";
            }
        }
        printf("| %-11s |", type);
        if (header->constraints[i] & 4){
            printf(" not null |");
        }
        else{
            printf("          |");
        }
        if (header->constraints[i] & 1){
            memcpy(defaultValue, rm ->defaultRow + header->columnOffsets[i], header->columnOffsets[i + 1] - header->columnOffsets[i]);
            printf(" %-*s\n", maxDefaultLen, defaultValue);
        }
        else{
            printf(" %-*s\n", maxDefaultLen, "");
        }
    }
}

int Table::addIndex(std::vector<std::string>& c_names, const std::string& ixName){
    __addIndex(c_names, ixName);
    return __loadIndexFromTable(ims.size() - 1);
}

void Table::dropIndex(std::vector<std::string>& c_names){
    for (int i = 0; i < ims.size(); i ++){
        if (ims[i]->keys == c_names){
            ims[i] ->destroyIndex();
            ims.erase(ims.begin() + i);
            if (i < priKey){
                priKey --;
            }
            break;
        }
    }
}

int Table::addColumn(AttrType type, int colLen, char colName[MAX_NAME_LEN], bool notNull){
    TableHeader* newHeader = new TableHeader(*header);
    newHeader->pageCnt = 1;
    newHeader->nextAvailable = (RID_t) -1;
    memcpy(newHeader->columnNames[newHeader->columnCnt], colName, MAX_NAME_LEN);
    newHeader->columnOffsets[newHeader->columnCnt + 1] = newHeader->columnOffsets[newHeader->columnCnt] + colLen;
    newHeader->columnTypes[newHeader->columnCnt] = type;
    newHeader->columnCnt ++;
    newHeader->pminlen = newHeader->columnOffsets[newHeader->columnCnt];
    if (notNull){
        __setNotNull(newHeader, newHeader->columnCnt - 1);
    }
    
    RecordManager* newRm = new RecordManager(__tablePath(name + "_temp"), newHeader);
    int err = newRm ->createFile();
    if (err){
        throw "error: reorganize table " + name + " fail to create new table";
    }

    rm ->fileScan->openScan(STRING, header->pminlen, 0, NO_OP, rm->defaultRow);
    char data[header->pminlen];
    RID_t rid;
    while(!rm->fileScan->getNextEntry(data, rid)){
        newRm ->insertRecord(data);
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

    err = rename(__tablePath(name + "_temp").c_str(), __tablePath(name).c_str());
    if (err){
        throw "error: reorganize table " + name + " fail to rename new table";
    }
    err = newRm->openFile();
    if (err){
        throw "error: reorganize table " + name + " fail to open new table";
    }
    rm = newRm;
    header = newHeader;
    return 0;
}

int Table::dropColumn(char colName[MAX_NAME_LEN]){
    if (std::find(ims[priKey]->keys.begin(), ims[priKey]->keys.end(), colName) != ims[priKey]->keys.end()){
        throw strcat(strcat("error: column ", colName), " in primary key cannot drop");
    }
    TableHeader* newHeader = new TableHeader(*header);
    newHeader->pageCnt = 1;
    newHeader->nextAvailable = (RID_t) -1;
    uint8_t colId = __colId(newHeader, colName);
    newHeader->columnCnt --;
    uint16_t colLen = newHeader->columnOffsets[colId + 1] - newHeader->columnOffsets[colId];
    for (int i = colId; i < newHeader->columnCnt; i ++){
        memcpy(newHeader->columnNames[i], newHeader->columnNames[i + 1], MAX_NAME_LEN);
        newHeader->columnTypes[i] = newHeader->columnTypes[i + 1];
        newHeader->columnOffsets[i + 1] = newHeader->columnOffsets[i + 2] - colLen;
    }
    newHeader->pminlen = newHeader->columnOffsets[newHeader->columnCnt];
    
    RecordManager* newRm = new RecordManager(__tablePath(name + "_temp"), newHeader);
    int err = newRm ->createFile();
    if (err){
        throw "error: reorganize table " + name + " fail to create new table";
    }

    rm ->fileScan->openScan(STRING, header->pminlen, 0, NO_OP, rm->defaultRow);
    char data[header->pminlen];
    char newData[newHeader->pminlen];
    RID_t rid;
    while(!rm->fileScan->getNextEntry(data, rid)){
        memcpy(newData, data, newHeader->columnOffsets[colId]);
        memcpy(newData + newHeader->columnOffsets[colId], data + header->columnOffsets[colId + 1], header->pminlen - header->columnOffsets[colId + 1]);
        newRm ->insertRecord(newData);
    }
    err = newRm->closeFile();
    if (err){
        throw "error: reorganize table " + name + " fail to close new table";
    }
    err = rm ->destroyFile();
    if (err){
        throw "error: reorganize table " + name + " fail to drop old table";
    }

    err = rename(__tablePath(name + "_temp").c_str(), __tablePath(name).c_str());
    if (err){
        throw "error: reorganize table " + name + " fail to rename new table";
    }
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
            ims[i] ->destroyIndex();
            ims.erase(ims.begin() + i);
            if (i < priKey){
                priKey --;
            }
        }
    }
    return 0;
}

int Table::changeColumn(char colName[MAX_NAME_LEN], AttrType newType, int newColLen, char newColName[MAX_NAME_LEN], bool notNull){
    TableHeader* newHeader = new TableHeader(*header);
    uint8_t colId = __colId(newHeader, colName);
    memcpy(newHeader->columnNames[colId], newColName, MAX_NAME_LEN);
    // 检查类型转换是否可行
    if (newType != header->columnTypes[colId] && newType != STRING && !(header->columnTypes[colId] == FLOAT && newType == INTEGER) && !(header->columnTypes[colId] == INTEGER && newType == FLOAT)){
        delete newHeader;
        throw "error: type cast is impossible";
        return 2;
    }
    newHeader->columnTypes[colId] = newType;
    if (notNull){
        __setNotNull(newHeader, colId);
    }
    uint16_t colLen = newHeader->columnOffsets[colId + 1] - newHeader->columnOffsets[colId];
    newHeader->pageCnt = 1;
    newHeader->nextAvailable = (RID_t) -1;
    for (int i = colId; i < newHeader->columnCnt; i ++){
        newHeader->columnOffsets[i + 1] = newHeader->columnOffsets[i + 1] + newColLen - colLen;
    }
    newHeader->pminlen = newHeader->columnOffsets[newHeader->columnCnt];

    RecordManager* newRm = new RecordManager(__tablePath(name + "_temp"), newHeader);
    int err = newRm ->createFile();
    if (err){
        throw "error: reorganize table " + name + " fail to create new table";
    }

    rm ->fileScan->openScan(STRING, header->pminlen, 0, NO_OP, rm->defaultRow);
    char data[header->pminlen];
    char newData[newHeader->pminlen];
    char temp[colLen];
    RID_t rid;
    while(!rm->fileScan->getNextEntry(data, rid)){
        memcpy(newData, data, newHeader->columnOffsets[colId]);
        // 类型转换
        newData[newHeader->columnOffsets[colId]] = data[header->columnOffsets[colId]];
        if (data[header->columnOffsets[colId]] == 1){
            if (newType == header->columnTypes[colId]){
                memcpy(newData + newHeader->columnOffsets[colId] + 1, data + header->columnOffsets[colId] + 1, std::min(int(colLen), newColLen) - 1);
            }
            else {
                memcpy(temp, data + header->columnOffsets[colId] + 1, colLen - 1);
                if (newType == STRING){
                    switch(header->columnTypes[colId]){
                        case DATE:
                        case INTEGER:{
                            int op = *(const int *)(temp);
                            memcpy(newData + newHeader->columnOffsets[colId] + 1, to_string(op).c_str(), newColLen - 1);
                            break;
                        }
                        case FLOAT:{
                            float op = *(const float *)(temp);
                            memcpy(newData + newHeader->columnOffsets[colId] + 1, to_string(op).c_str(), newColLen - 1);
                            break;
                        }
                        default: {}
                    }
                }
                else if (newType == FLOAT && header->columnTypes[colId] == INTEGER){
                    float op = *(const int *)(temp);
                    memcpy(newData + newHeader->columnOffsets[colId] + 1, to_string(op).c_str(), newColLen - 1);
                }
                else if (newType == INTEGER && header->columnTypes[colId] == FLOAT){
                    int op = *(const float *)(temp);
                    memcpy(newData + newHeader->columnOffsets[colId] + 1, to_string(op).c_str(), newColLen - 1);
                }
            }
        }
        else{
            // 检查现有数据是否有空数据？
            if (notNull){
                delete newHeader;
                delete newRm;
                throw "error: null exist at row " + to_string(rid);
            }
        }
        memcpy(newData + newHeader->columnOffsets[colId + 1], data + header->columnOffsets[colId + 1], header->pminlen - header->columnOffsets[colId + 1]);
        newRm ->insertRecord(newData);
    }
    err = newRm->closeFile();
    if (err){
        throw "error: reorganize table " + name + " fail to close new table";
    }
    err = rm ->destroyFile();
    if (err){
        throw "error: reorganize table " + name + " fail to drop old table";
    }

    err = rename(__tablePath(name + "_temp").c_str(), __tablePath(name).c_str());
    if (err){
        throw "error: reorganize table " + name + " fail to rename new table";
    }
    err = newRm->openFile();
    if (err){
        throw "error: reorganize table " + name + " fail to open new table";
    }

    delete rm;
    delete header;
    rm = newRm;
    header = newHeader;

    // 相应索引改名
    for (int i = 0; i < ims.size(); i ++){
        std::vector<std::string>::iterator iter = std::find(ims[i]->keys.begin(), ims[i]->keys.end(), colName);
        if (iter != ims[i]->keys.end()){
            ims[i] ->destroyIndex();
            *iter = newColName;
            std::string ixName = "";
            if (priKey == i){
                ixName = "pri";
            }
            ims[i] = new IX_Manager(__tablePath(name, ixName), ims[i]->keys);
            __loadIndexFromTable(i);
        }
    }
    return 0;
}

Table::Table(const std::string& m_dbname, const std::string& m_name):dbname(m_dbname), name(m_name){
    header = new TableHeader;
    memset(header, 0, sizeof(*header));
    rm = new RecordManager(__tablePath(m_name), header);
}