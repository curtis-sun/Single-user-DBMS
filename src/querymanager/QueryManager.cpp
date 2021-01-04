# include "QueryManager.h"

void Insert::run(){
    Table* table = Database::instance()->getTableByName(tbName);
    for (int i = 0; i < lists->list.size(); i ++){
        std::vector<AttrVal> attrValList;
        ValueList* valueList = lists->list[i];
        char temp[table->header->pminlen];
        for (int j = 0; j < valueList->list.size(); j ++){
            AttrVal val = valueList->list[j]->calc(nullptr, table);
            attrValList.push_back(val);
            restoreAttr(temp + table->header->columnOffsets[j + 1], table->header->columnOffsets[j + 2] - table->header->columnOffsets[j + 1], val);
        }
        RID_t rid;
        table->rm->insertRecord(temp, rid);
        for (int j = 0; j < table->ims.size(); j ++){
            Entry entry;
            entry.rid = rid;
            for (int k = 0; k < table->ims[j]->keys.size(); k ++){
                std::string colName = table->ims[j]->keys[k];
                int colId = table->__colId(colName);
                entry.vals[k] = attrValList[colId];
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
        }
    }
}

void Update::run(){
    Table* table = Database::instance()->getTableByName(tbName);
    std::map<std::string, RID_t> rids;
    table->rm->fileScan->openScan();
    RID_t rid;
    char data[table->header->pminlen];
    while(!table->rm->fileScan->getNextEntry(data, rid)){
        rids[tbName] = rid;
        AttrVal val = clause ->calc(&rids, table);
        if (val.type == INTEGER && val.val.i){
            for (int i = 0; i < list->identList.list.size(); i ++){
                AttrVal val = list->valueList.list[i]->calc(&rids, table);
                int colId = table->__colId(list->identList.list[i]);
                restoreAttr(data + table->header->columnOffsets[colId], table->header->columnOffsets[colId + 1] - table->header->columnOffsets[colId], val);
            }
            table->rm->updateRecord(rid, data);
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
        std::map<std::string, RID_t> rids;
        for (int i = 0; i < tableList->list.size(); i ++){
            newRows = new set<Row>;
            Table* table = Database::instance()->getTableByName(tableList->list[i]);
            table->rm->fileScan->openScan();
            RID_t rid;
            char data[table->header->pminlen];
            while(!table->rm->fileScan->getNextEntry(data, rid)){
                rids[tableList->list[i]] = rid;
                if (!oldRows){
                    AttrVal val = clause->exprList[g]->calc(&rids, nullptr);
                    if (val.type == INTEGER && val.val.i){
                        Row row;
                        row.rowList.push_back(rid);
                        newRows->insert(row);
                    }
                }
                else{
                    for (auto it = oldRows->begin(); it != oldRows->end(); it ++){
                        for (int j = 0; j < i; j ++){
                            rids[tableList->list[j]] = it->rowList[j];
                        }
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