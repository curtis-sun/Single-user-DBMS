# include "QueryManager.h"

static bool checkNotNulDefault(AttrVal& val, Table* table, int colId){
    if (val.type == NO_TYPE){
        if (table->header->constraints[colId] & 1){
            val = recordToAttr(table->rm->defaultRow + table->header->columnOffsets[colId], table->header->columnOffsets[colId + 1] - table->header->columnOffsets[colId], table->header->columnTypes[colId]);
        }
        else{
            if (table->header->constraints[colId] & 4){
                printf("warning: null value cannot insert into column %s\n", table->header->columnNames[colId]);
                return false;
            }
        }
    }
    return true;
}

static bool checkIndex(const std::vector<AttrVal>& valList, Table* table){
    for (int i = 0; i < table->ims.size(); i ++){
        if (table->ims[i]->ixClass == "unique" || table->ims[i]->ixClass == "pri"){
            Entry entry;
            for (int k = 0; k < table->ims[i]->keys.size(); k ++){
                std::string colName = table->ims[i]->keys[k];
                int colId = table->__colId(colName);
                entry.vals[k] = valList[colId - 1];
            }
            table->ims[i]->indexScan->openScan(entry, table->ims[i]->keys.size(), EQ_OP);
            RID_t rid;
            if(!table->ims[i]->indexScan->getNextEntry(rid)){
                printf("warning: unique index cannot maintain %s\n", table->ims[i]->ixName.c_str());
                return false;
            }
        }
        else if (table->ims[i]->ixClass == "foreign"){
            Table* refTb = Database::instance()->getTableByName(table->ims[i]->refTbName);
            Entry entry;
            for (int k = 0; k < table->ims[i]->keys.size(); k ++){
                std::string colName = table->ims[i]->keys[k];
                int colId = table->__colId(colName);
                entry.vals[k] = valList[colId - 1];
            }
            refTb->priIx->indexScan->openScan(entry, table->ims[i]->keys.size(), GE_OP);
            RID_t rid;
            if(refTb->priIx->indexScan->getNextEntry(rid)){
                printf("warning: foreign key reference to %s cannot maintain %s\n", table->ims[i]->refTbName.c_str(), refTb->priIx->ixName.c_str());
                return false;
            }
            else{
                char data[refTb->header->pminlen];
                refTb->rm->getRecord(rid, data);
                for (int k = 0; k < table->ims[i]->refKeys.size(); k ++){
                    std::string colName = table->ims[i]->refKeys[k];
                    int colId = refTb->__colId(colName);
                    AttrVal refVal = recordToAttr(data + refTb->header->columnOffsets[colId], refTb->header->columnOffsets[colId + 1] - refTb->header->columnOffsets[colId], refTb->header->columnTypes[colId]);
                    AttrVal cmp = compare(entry.vals[k], refVal, EQ_OP);
                    if (cmp.type != INTEGER || !cmp.val.i){
                        printf("warning: foreign key reference to %s cannot maintain %s\n", table->ims[i]->refTbName.c_str(), refTb->priIx->ixName.c_str());
                        return false;
                    }
                }
            }
        }
    }
    return true;
}

static bool checkIndex(const char* data, Table* table){
    std::vector<AttrVal> attrValList;
    for (int i = 1; i < table->header->columnCnt; i ++){
        attrValList.push_back(recordToAttr(data + table->header->columnOffsets[i], table->header->columnOffsets[i + 1] - table->header->columnOffsets[i], table->header->columnTypes[i]));
    }
    return checkIndex(attrValList, table);
}

void Insert::run(){
    Table* table = Database::instance()->getTableByName(tbName);
    for (int i = 0; i < lists->list.size(); i ++){
        std::vector<AttrVal> attrValList;
        ValueList* valueList = lists->list[i];
        char temp[table->header->pminlen];
        for (int j = 0; j < valueList->list.size(); j ++){
            AttrVal val = valueList->list[j]->calc(nullptr, table);
            if (!checkNotNulDefault(val, table, j + 1)){
                return;
            }
            attrValList.push_back(val);
            restoreAttr(temp + table->header->columnOffsets[j + 1], table->header->columnOffsets[j + 2] - table->header->columnOffsets[j + 1], attrValList[j]);
        }
        if (!checkIndex(attrValList, table)){
            return;
        }
        RID_t rid;
        table->rm->insertRecord(temp, rid);
        for (int j = 0; j < table->ims.size(); j ++){
            Entry entry;
            entry.rid = rid;
            for (int k = 0; k < table->ims[j]->keys.size(); k ++){
                std::string colName = table->ims[j]->keys[k];
                int colId = table->__colId(colName);
                entry.vals[k] = attrValList[colId - 1];
            }
            table->ims[j]->insertEntry(entry);
        }
    }
}

void Delete::run(){
    Table* table = Database::instance()->getTableByName(tbName);
    std::map<std::string, RID_t> rids;
    table->rm->fileScan->openScan();
    RID_t rid;
    char data[table->header->pminlen];
    while(!table->rm->fileScan->getNextEntry(data, rid)){
        rids[tbName] = rid;
        AttrVal val = clause->calc(&rids, table);
        if (val.type == INTEGER && val.val.i){
            table->rm->deleteRecord(rid);
            for (int j = 0; j < table->ims.size(); j ++){
                Entry entry;
                entry.rid = rid;
                for (int k = 0; k < table->ims[j]->keys.size(); k ++){
                    std::string colName = table->ims[j]->keys[k];
                    int colId = table->__colId(colName);
                    entry.vals[k] = recordToAttr(data + table->header->columnOffsets[colId], table->header->columnOffsets[colId + 1] - table->header->columnOffsets[colId], table->header->columnTypes[colId]);
                }
                table->ims[j]->deleteEntry(entry);
            }
        }
    }
}

void Update::run(){
    Table* table = Database::instance()->getTableByName(tbName);
    std::map<std::string, RID_t> rids;
    table->rm->fileScan->openScan();
    RID_t rid;
    char data[table->header->pminlen];
    char newData[table->header->pminlen];
    while(!table->rm->fileScan->getNextEntry(data, rid)){
        memcpy(newData, data, table->header->pminlen);
        rids[tbName] = rid;
        AttrVal val = clause ->calc(&rids, table);
        if (val.type == INTEGER && val.val.i){
            for (int i = 0; i < list->identList.list.size(); i ++){
                AttrVal val = list->valueList.list[i]->calc(&rids, table);
                int colId = table->__colId(list->identList.list[i]);
                if (!checkNotNulDefault(val, table, colId)){
                    return;
                }
                restoreAttr(newData + table->header->columnOffsets[colId], table->header->columnOffsets[colId + 1] - table->header->columnOffsets[colId], val);
            }
            if (!checkIndex(newData, table)){
                return;
            }
            table->rm->updateRecord(rid, newData);
            for (int j = 0; j < table->ims.size(); j ++){
                Entry oldEntry, newEntry;
                oldEntry.rid = newEntry.rid = rid;
                for (int k = 0; k < table->ims[j]->keys.size(); k ++){
                    std::string colName = table->ims[j]->keys[k];
                    int colId = table->__colId(colName);
                    oldEntry.vals[k] = recordToAttr(data + table->header->columnOffsets[colId], table->header->columnOffsets[colId + 1] - table->header->columnOffsets[colId], table->header->columnTypes[colId]);
                    newEntry.vals[k] = recordToAttr(newData + table->header->columnOffsets[colId], table->header->columnOffsets[colId + 1] - table->header->columnOffsets[colId], table->header->columnTypes[colId]);
                }
                table->ims[j]->deleteEntry(oldEntry);
                table->ims[j]->insertEntry(newEntry);
            }
        }
    }
}

class Row{
public:
    std::vector<RID_t> rowList;
    Row(){
        rowList.reserve(MAX_COL_NUM);
    }
    Row(const Row& r){
        rowList.reserve(MAX_COL_NUM);
        for (int k = 0; k < r.rowList.size(); k ++){
            rowList.push_back(r.rowList[k]);
        }
    }
    bool operator<(const Row& r) const{
        int i;
        for (i = 0; i < min(rowList.size(), r.rowList.size()); i ++){
            if (rowList[i] < r.rowList[i]){
                return true;
            }
            if (r.rowList[i] < rowList[i]){
                return false;
            }
        }
        return i < r.rowList.size();
    }
};

void Select::run(){
    if (tableList->list.size() == 1){
        std::map<std::string, RID_t> rids;
        Table* table = Database::instance()->getTableByName(tableList->list[0]);
        table->rm->fileScan->openScan();
        RID_t rid;
        char data[table->header->pminlen];
        while(!table->rm->fileScan->getNextEntry(data, rid)){
            rids[table->name] = rid;
            AttrVal val;
            if (clause){
                val = clause ->calc(&rids, table);
            }
            if (!clause || val.type == INTEGER && val.val.i){
                if (!selector){
                    for (int i = 1; i < table->header->columnCnt; i ++){
                        int colLen = table->header->columnOffsets[i + 1] - table->header->columnOffsets[i];
                        AttrVal val = recordToAttr(data + table->header->columnOffsets[i], colLen, table->header->columnTypes[i]);
                        std::string str = attrToString(val);
                        printf(" %-*s ", str.length(), str.c_str());
                        if (i < table->header->columnCnt - 1){
                            printf("|");
                        }
                        else{
                            printf("\n");
                        }
                    }
                }
                else{
                    for (int i = 0; i < selector->list.size(); i ++){
                        AttrVal val = selector->list[i]->calc(&rids, table);
                        std::string str = attrToString(val);
                        printf(" %-*s ", str.length(), str.c_str());
                        if (i < selector->list.size() - 1){
                            printf("|");
                        }
                        else{
                            printf("\n");
                        }
                    }
                }
            }
        }
        return;
    }
    std::set<Row> rows;
    for (int g = 0; g < clause->exprList.size(); g ++){
        std::set<Row>* oldRows = nullptr, *newRows;
        for (int i = 0; i < tableList->list.size(); i ++){
            newRows = new set<Row>;
            Table* table = Database::instance()->getTableByName(tableList->list[i]);
            if (!oldRows){
                std::map<std::string, RID_t> rids;
                clause->exprList[g]->judgeScan(&rids, table);
                clause->exprList[g]->openScan(&rids, table);
                RID_t rid;
                while(!clause->exprList[g]->getNextEntry(rid, table)){
                    rids[tableList->list[i]] = rid;
                    AttrVal val = clause->exprList[g]->calc(&rids, nullptr);
                    if (val.type == INTEGER && val.val.i){
                        Row row;
                        row.rowList.push_back(rid);
                        newRows->insert(row);
                    }
                }
            }
            else{
                for (auto it = oldRows->begin(); it != oldRows->end(); it ++){
                    std::map<std::string, RID_t> rids;
                    for (int j = 0; j < i; j ++){
                        rids[tableList->list[j]] = it->rowList[j];
                    }
                    if (it == oldRows->begin()){
                        clause->exprList[g]->judgeScan(&rids, table);
                    }
                    clause->exprList[g]->openScan(&rids, nullptr);
                    RID_t rid;
                    while(!clause->exprList[g]->getNextEntry(rid, table)){
                        rids[tableList->list[i]] = rid;
                        AttrVal val = clause->exprList[g]->calc(&rids, nullptr);
                        if (val.type == INTEGER && val.val.i){
                            Row row(*it);
                            row.rowList.push_back(rid);
                            newRows->insert(row);
                        }
                    }
                }
            }
            delete oldRows;
            oldRows = newRows;
            
            if (oldRows->empty()){
                break;
            }
        }
        for (auto it = oldRows->begin(); it != oldRows->end(); it ++){
            rows.insert(*it);
        }
        delete oldRows;
    }
    for (auto it = rows.begin(); it != rows.end(); it ++){
        if (!selector){
            for (int j = 0; j < tableList->list.size(); j ++){
                Table* table = Database::instance()->getTableByName(tableList->list[j]);
                char data[table->header->pminlen];
                table->rm->getRecord(it->rowList[j], data);
                for (int i = 1; i < table->header->columnCnt; i ++){
                    int colLen = table->header->columnOffsets[i + 1] - table->header->columnOffsets[i];
                    AttrVal val = recordToAttr(data + table->header->columnOffsets[i], colLen, table->header->columnTypes[i]);
                    std::string str = attrToString(val);
                    printf(" %-*s ", str.length(), str.c_str());
                    if (i < table->header->columnCnt - 1 || j < tableList->list.size() - 1){
                        printf("|");
                    }
                    else{
                        printf("\n");
                    }
                }
            }
        }
        else{
            std::map<std::string, RID_t> rids;
            for (int j = 0; j < tableList->list.size(); j ++){
                rids[tableList->list[j]] = it->rowList[j];
            }
            for (int i = 0; i < selector->list.size(); i ++){
                AttrVal val = selector->list[i]->calc(&rids, nullptr);
                std::string str = attrToString(val);
                printf(" %-*s ", str.length(), str.c_str());
                if (i < selector->list.size() - 1){
                    printf("|");
                }
                else{
                    printf("\n");
                }
            }
        }
    }
}