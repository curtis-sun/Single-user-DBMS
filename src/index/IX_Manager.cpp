# include "IX_Manager.h"
# include <cassert>
# include <iostream>
# include <fstream>
# include <cstdio>
# include "../utils/compare.cpp"

std::string IX_Manager::__indexName(){
    std::string tableName =  ixPath + "/" + ixName;
    for(uint8_t i = 0; i < keys.size(); i ++){
        tableName = tableName + "%" + keys[i];
    }
    if (!refKeys.empty()){
        tableName = tableName + "#" + refTbName;
        for(uint8_t i = 0; i < refKeys.size(); i ++){
            tableName = tableName + "%" + refKeys[i];
        }
    }
    return tableName;
}

void IX_Manager::openIndex(){
    std::ifstream fin(__indexName().substr(ixPath.length() + 1));
    if (!btree->restore(fin)){
        fin.close();
        throw "warning: index restore fail " + ixName;
    }
    else{
        fin.close();
    }
    /*
    indexScan->openScan(Entry(), ADD_OP);
    RID_t rid;
    while(!indexScan->getNextEntry(rid)){
        printf("%d\n", rid);
    }*/
}

void IX_Manager::closeIndex(){
    std::ofstream fout(__indexName());
    btree->dump(fout);
    fout.close();
}

void IX_Manager::destroyIndex(){
    btree->clear();
    int err = remove(__indexName().c_str());
    if (err){
        printf("warning: remove file %s fail %d\n", __indexName().c_str(), errno);
    }
}

void IX_Manager::insertEntry(const Entry& data){
    btree->insert(data);
}

void IX_Manager::deleteEntry(const Entry& data){
    btree->erase(data);
}

IX_Manager::IX_Manager(const std::string& path, const std::string& name, const std::string& c, const std::vector<std::string>& c_names){
    ixName = name;
    ixPath = path;
    keys = c_names;
    ixClass = c;
    btree = new stx::btree_set<Entry>();
    indexScan = new IX_IndexScan(btree, this);
}

IX_Manager::IX_Manager(const std::string& path, const std::string& name, const std::vector<std::string>& c_names, const std::string& m_refTbName, const std::vector<std::string>& m_refKeys){
    ixName = name;
    ixPath = path;
    keys = c_names;
    refTbName = m_refTbName;
    refKeys = m_refKeys;
    ixClass = "foreign";
    btree = new stx::btree_set<Entry>();
    indexScan = new IX_IndexScan(btree, this);
}

void IX_IndexScan::__lowestFill(Entry& entry, int len){
    entry.rid = 0;
    for (int i = len; i < im->keys.size(); i ++){
        entry.vals[i] = AttrVal();
    }
}
void IX_IndexScan::__uppestFill(Entry& entry, int len){
    entry.rid = -1;
    AttrVal val;
    val.type = POS_TYPE;
    for (int i = len; i < im->keys.size(); i ++){
        entry.vals[i] = val;
    }
}

void IX_IndexScan::openScan(const Entry& data, int num, CalcOp m_compOp){
    if (num > 1 && m_compOp != EQ_OP){
        Entry lowerData(data);
        __lowestFill(lowerData, num - 1);
        iter = btree->lower_bound(lowerData);
        Entry upperData(data);
        __uppestFill(upperData, num - 1);
        upperBound = btree->upper_bound(upperData);
    }
    else{
        iter = btree->begin();
        upperBound = btree->end();
    }
    Entry lowerData(data);
    __lowestFill(lowerData, num);
    Entry upperData(data);
    __uppestFill(upperData, num);
    compOp = m_compOp;
    switch(m_compOp){
        case LT_OP:{
            upperBound = btree->lower_bound(lowerData);
            break;
        }
        case GT_OP:{
            iter = btree->upper_bound(upperData);
            break;
        }
        case LE_OP:{
            upperBound = btree->upper_bound(upperData);
            break;
        }
        case GE_OP:{
            iter = btree->lower_bound(lowerData);
            break;
        }
        case EQ_OP:{
            iter = btree->lower_bound(lowerData);
            upperBound = btree->upper_bound(upperData);
            break;
        }
        case NE_OP:{
            lowerMid = btree->lower_bound(lowerData);
            upperMid = btree->upper_bound(upperData);
            break;
        }
        default: {}
    }
}

int IX_IndexScan::getNextEntry(RID_t& rid){
    if (compOp == NE_OP){
        if (iter == lowerMid){
            iter = upperMid;
        }
    }
    if (iter == upperBound){
        return -1;
    }
    rid = iter->rid;
    iter ++;
    return 0;
}