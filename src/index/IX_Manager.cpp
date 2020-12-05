# include "IX_Manager.h"
# include <cassert>
# include <iostream>
# include <fstream>
# include <cstdio>

template<typename T>
void IX_Manager<T>::openIndex(){
    std::ifstream fin(tableName.c_str());
    btree.restore(fin);
}

template<typename T>
void IX_Manager<T>::closeIndex(){
    std::ofstream fout(tableName.c_str());
    btree.dump(fout);
}

template<typename T>
void IX_Manager<T>::destroyIndex(){
    remove(tableName.c_str());
}

template<typename T>
void IX_Manager<T>::insertEntry(RID_t rid, const T& data){
    btree.insert(Entry<T>(rid, data));
}

template<typename T>
void IX_Manager<T>::deleteEntry(RID_t rid, const T& data){
    btree.erase(Entry<T>(rid, data));
}

template<typename T>
IX_Manager<T>::IX_Manager(std::string path, char c_names[][MAX_NAME_LEN], int colLen){
    tableName = path + "/ix";
    for(uint8_t i = 0; i < colLen; i ++){
        tableName = tableName + "_" + c_names[i];
    }

    indexScan = new IX_IndexScan<T>(btree);
}

template<typename T>
void IX_IndexScan<T>::openScan(const T& data, CompOp m_compOp){
    compOp = m_compOp;
    switch(m_compOp){
        case LT_OP:{
            iter = btree.begin();
            upperBound = btree.lower_bound(Entry<T>(0, data));
        }
        case GT_OP:{
            iter = btree.upper_bound(Entry<T>(0, data));
            upperBound = btree.end();
        }
        case LE_OP:{
            iter = btree.begin();
            upperBound = btree.upper_bound(Entry<T>(0, data));
        }
        case GE_OP:{
            iter = btree.lower_bound(Entry<T>(0, data));
            upperBound = btree.end();
        }
        case EQ_OP:{
            std::pair<typename stx::btree_set<Entry<T>>::iterator, typename stx::btree_set<Entry<T>>::iterator> pair = btree.equal_range(Entry<T>(0, data));
            iter = pair.first;
            upperBound = pair.second;
        }
        case NE_OP:{
            std::pair<typename stx::btree_set<Entry<T>>::iterator, typename stx::btree_set<Entry<T>>::iterator> pair = btree.equal_range(Entry<T>(0, data));
            iter = btree.begin();
            lowerBound = pair.first;
            upperBound = pair.second;
        }
        default: {
            iter = btree.begin();
            upperBound = btree.end();
        }
    }
}

template<typename T>
int IX_IndexScan<T>::getNextEntry(RID_t& rid){
    switch(compOp){
        case NE_OP:{
            if (iter == lowerBound){
                iter = upperBound;
            }
            if (iter == btree.end()){
                return -1;
            }
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