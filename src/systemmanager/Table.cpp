# include "Table.h"
# include <cstring>
# include <cassert>

std::uint8_t Table::__colId(TableHeader* header, char colName[MAX_NAME_LEN]){
    assert(header != nullptr);
    for (int i = 0; i < header->columnCnt; i ++){
        if (!memcmp(header->columnNames[i], colName, MAX_NAME_LEN)){
            return i;
        }
    }
    return (uint8_t)-1;
}

int Table::createTable(AttrType types[], int colLens[], char names[][MAX_NAME_LEN], int colNum){
    header->nextAvailable = (RID_t) -1;
    header->pageCnt = 1;
    header->columnCnt = 1 + colNum;
    header->columnOffsets[0] = 0;
    header->columnOffsets[1] = 8;
    header->columnNames[0][0] = 0;
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
    return err;
}

int Table::closeTable(){
    int err = rm -> closeFile();
    if (err){
        throw "error: open table " + name + " fail";
    }
    return err;
}

void Table::showTable(){
    int maxNameLen = 6, maxDefaultLen = 7;
    char data[header->pminlen];
    for (int i = 0; i < header->columnCnt; i ++){
        int nameLen = strlen(header->columnNames[i]);
        if (nameLen > maxNameLen){
            maxNameLen = nameLen;
        }
        int valLen = header->columnOffsets[i + 1] - header->columnOffsets[i];
        if (valLen > maxDefaultLen){
            maxDefaultLen = valLen;
        }
    }
    printf(" Table %s.%s\n ", dbname, name);
    printf("%-*s", maxNameLen, "Column");
    printf(" | Type        | Nullable | ");
    printf("%-*s\n", maxDefaultLen, "Default");
}

int Table::addColumn(AttrType type, int colLen, char colName[MAX_NAME_LEN]){
    TableHeader* newHeader = new TableHeader(*header);
    newHeader->pageCnt = 1;
    newHeader->nextAvailable = (RID_t) -1;
    memcpy(newHeader->columnNames[newHeader->columnCnt], colName, MAX_NAME_LEN);
    newHeader->columnOffsets[newHeader->columnCnt + 1] = newHeader->columnOffsets[newHeader->columnCnt] + colLen;
    newHeader->columnTypes[newHeader->columnCnt] = type;
    newHeader->columnCnt ++;
    newHeader->pminlen = newHeader->columnOffsets[newHeader->columnCnt];
    return reorganize(newHeader);
}

int Table::dropColumn(char colName[MAX_NAME_LEN]){
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
    return reorganize(newHeader);
}

int Table::changeColumn(char colName[MAX_NAME_LEN], AttrType newType, int newColLen, char newColName[MAX_NAME_LEN]){
    TableHeader* newHeader = new TableHeader(*header);
    newHeader->pageCnt = 1;
    newHeader->nextAvailable = (RID_t) -1;
    uint8_t colId = __colId(newHeader, colName);
    memcpy(newHeader->columnNames[colId], newColName, MAX_NAME_LEN);
    newHeader->columnTypes[colId] = newType;
    uint16_t colLen = newHeader->columnOffsets[colId + 1] - newHeader->columnOffsets[colId];
    for (int i = colId; i < newHeader->columnCnt; i ++){
        newHeader->columnOffsets[i + 1] = newHeader->columnOffsets[i + 1] + newColLen - colLen;
    }
    newHeader->pminlen = newHeader->columnOffsets[newHeader->columnCnt];
    return reorganize(newHeader);
}

int Table::reorganize(TableHeader* newHeader){
    RecordManager* newRm = new RecordManager(__tablePath(dbname, name + "_temp"), newHeader);
    int err = newRm ->createFile();
    if (err){
        throw "error: reorganize table " + name + " fail to create new table";
    }

    rm ->fileScan->openScan(STRING, header->pminlen, 0, NO_OP, rm->defaultRow);
    char data[header->pminlen];
    while(!rm->fileScan->getNextEntry(data)){
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

    err = rename(__tablePath(dbname, name + "_temp").c_str(), __tablePath(dbname, name).c_str());
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

Table::Table(char m_dbname[MAX_NAME_LEN], char m_name[MAX_NAME_LEN]):dbname(m_dbname), name(m_name){
        header = new TableHeader;
        rm = new RecordManager(__tablePath(m_dbname, m_name), header);
    }