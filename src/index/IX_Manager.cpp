# include "IX_Manager.h"
# include <cassert>
# include <iostream>
# include <fstream>
# include <cstdio>

void IX_Manager::openIndex(){
    std::ifstream fin(tableName.c_str());
    btree.restore(fin);
}

void IX_Manager::closeIndex(){
    std::ofstream fout(tableName.c_str());
    btree.dump(fout);
}

void IX_Manager::destroyIndex(){
    remove(tableName.c_str());
}

void IX_Manager::insertEntry(const Entry& data){
    btree.insert(data);
}

void IX_Manager::deleteEntry(const Entry& data){
    btree.erase(data);
}

IX_Manager::IX_Manager(std::string path, std::vector<std::string>& c_names){
    tableName = path + "/ix";
    for(uint8_t i = 0; i < c_names.size(); i ++){
        tableName = tableName + "_" + c_names[i];
    }
    keys = c_names;
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