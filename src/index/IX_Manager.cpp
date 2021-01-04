# include "IX_Manager.h"
# include <cassert>
# include <iostream>
# include <fstream>
# include <cstdio>

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
    std::ifstream fin(__indexName().c_str());
    btree.restore(fin);
}

void IX_Manager::closeIndex(){
    std::ofstream fout(__indexName().c_str());
    btree.dump(fout);
}

void IX_Manager::destroyIndex(){
    btree.clear();
    int err = remove(__indexName().c_str());
    if (err){
        printf("warning: remove file %s fail %d\n", __indexName().c_str(), errno);
    }
}

void IX_Manager::insertEntry(const Entry& data){
    btree.insert(data);
}

void IX_Manager::deleteEntry(const Entry& data){
    btree.erase(data);
}

IX_Manager::IX_Manager(const std::string& path, const std::string& name, const std::vector<std::string>& c_names){
    ixName = name;
    ixPath = path;
    keys = c_names;
    indexScan = new IX_IndexScan(btree);
}

IX_Manager::IX_Manager(const std::string& path, const std::string& name, const std::vector<std::string>& c_names, const std::string& m_refTbName, const std::vector<std::string>& m_refKeys){
    ixName = name;
    ixPath = path;
    keys = c_names;
    refTbName = m_refTbName;
    refKeys = m_refKeys;
    indexScan = new IX_IndexScan(btree);
}

void IX_IndexScan::openScan(const Entry& data, CompOp m_compOp){
    compOp = m_compOp;
    switch(m_compOp){
        case LT_OP:{
            iter = btree.begin();
            upperBound = btree.lower_bound(data);
            break;
        }
        case GT_OP:{
            iter = btree.upper_bound(data);
            upperBound = btree.end();
            break;
        }
        case LE_OP:{
            iter = btree.begin();
            upperBound = btree.upper_bound(data);
            break;
        }
        case GE_OP:{
            iter = btree.lower_bound(data);
            upperBound = btree.end();
            break;
        }
        case EQ_OP:{
            std::pair<typename stx::btree_set<Entry>::iterator, typename stx::btree_set<Entry>::iterator> pair = btree.equal_range(data);
            iter = pair.first;
            upperBound = pair.second;
            break;
        }
        case NE_OP:{
            std::pair<typename stx::btree_set<Entry>::iterator, typename stx::btree_set<Entry>::iterator> pair = btree.equal_range(data);
            iter = btree.begin();
            lowerBound = pair.first;
            upperBound = pair.second;
            break;
        }
        default: {
            iter = btree.begin();
            upperBound = btree.end();
        }
    }
}

int IX_IndexScan::getNextEntry(RID_t& rid){
    switch(compOp){
        case NE_OP:{
            if (iter == lowerBound){
                iter = upperBound;
            }
            if (iter == btree.end()){
                return -1;
            }
            break;
        }
        default: {
            if (iter == upperBound){
                return -1;
            }
        }
    }
    rid = iter->rid;
    iter ++;
    return 0;
}