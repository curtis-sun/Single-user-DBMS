# include "IX_Manager.h"
# include <cassert>
# include <iostream>
# include <fstream>
# include <cstdio>
# include "../utils/compare.cpp"

std::string IX_Manager::__indexName(){
    std::string tableName =  ixPath + "/" + ixName;
    for(uint8_t i = 0; i < keys.size(); i ++){
        tableName = tableName + "%" + keys[i].name;
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

void IX_Manager::restoreAttrList(const AttrList& list, char* data){
    memcpy(data, &list.rid, 8);
    for (int i = 0; i < list.vals.size(); i ++){
        if (i == 0){
            restoreAttr(data + 8, keys[i].offset, list.vals[i]);
        }
        else{
            restoreAttr(data + 8 + keys[i - 1].offset, keys[i].offset - keys[i - 1].offset, list.vals[i]);
        }
    }
}

void IX_Manager::recordToAttrList(const char* record, AttrList& list){
    memcpy(&list.rid, record, 8);
    list.vals.clear();
    for (int i = 0; i < keys.size(); i ++){
        if (i == 0){
            list.vals.push_back(recordToAttr(record + 8, keys[i].offset, keys[i].type));
        }
        else{
            list.vals.push_back(recordToAttr(record + 8 + keys[i - 1].offset, keys[i].offset - keys[i - 1].offset, keys[i].type));
        }
    }
}

void IX_Manager::insertEntry(const AttrList& data){
    IxElement temp;
    //char temp[keys[keys.size() - 1].offset + 8];
    restoreAttrList(data, temp.val);
    btree->insert(temp);
}

void IX_Manager::deleteEntry(const AttrList& data){
    IxElement temp;
    //char temp[keys[keys.size() - 1].offset + 8];
    restoreAttrList(data, temp.val);
    btree->erase(temp);
}

IX_Manager::IX_Manager(const std::string& path, const std::string& name, const std::string& c, const std::vector<IxCol>& c_names){
    ixName = name;
    ixPath = path;
    keys = c_names;
    ixClass = c;
    Comparator comparator(this);
    btree = new stx::btree_set<IxElement, Comparator>(comparator);
    indexScan = new IX_IndexScan(btree, this);
}

IX_Manager::IX_Manager(const std::string& path, const std::string& name, const std::vector<IxCol>& c_names, const std::string& m_refTbName, const std::vector<std::string>& m_refKeys){
    ixName = name;
    ixPath = path;
    keys = c_names;
    refTbName = m_refTbName;
    refKeys = m_refKeys;
    ixClass = "foreign";
    Comparator comparator(this);
    btree = new stx::btree_set<IxElement, Comparator>(comparator);
    indexScan = new IX_IndexScan(btree, this);
}

void IX_IndexScan::__lowestFill(AttrList& entry){
    entry.rid = 0;
    for (int i = entry.vals.size(); i < im->keys.size(); i ++){
        entry.vals.push_back(AttrVal());
    }
}
void IX_IndexScan::__uppestFill(AttrList& entry){
    entry.rid = -1;
    AttrVal val;
    val.type = POS_TYPE;
    for (int i = entry.vals.size(); i < im->keys.size(); i ++){
        entry.vals.push_back(val);
    }
}

void IX_IndexScan::openScan(const AttrList& data, int num, CalcOp m_compOp){
    IxElement lowRecord, upRecord;
    if (num > 1 && m_compOp != EQ_OP){
        AttrList lowerData(data, num - 1);
        __lowestFill(lowerData);
        im->restoreAttrList(lowerData, lowRecord.val);
        iter = btree->lower_bound(lowRecord);
        AttrList upperData(data, num - 1);
        __uppestFill(upperData);
        im->restoreAttrList(upperData, upRecord.val);
        upperBound = btree->upper_bound(upRecord);
    }
    else{
        iter = btree->begin();
        upperBound = btree->end();
    }
    AttrList lowerData(data, num);
    __lowestFill(lowerData);
    im->restoreAttrList(lowerData, lowRecord.val);
    AttrList upperData(data);
    __uppestFill(upperData);
    im->restoreAttrList(upperData, upRecord.val);
    compOp = m_compOp;
    switch(m_compOp){
        case LT_OP:{
            upperBound = btree->lower_bound(lowRecord);
            break;
        }
        case GT_OP:{
            iter = btree->upper_bound(upRecord);
            break;
        }
        case LE_OP:{
            upperBound = btree->upper_bound(upRecord);
            break;
        }
        case GE_OP:{
            iter = btree->lower_bound(lowRecord);
            break;
        }
        case EQ_OP:{
            iter = btree->lower_bound(lowRecord);
            upperBound = btree->upper_bound(upRecord);
            break;
        }
        case NE_OP:{
            lowerMid = btree->lower_bound(lowRecord);
            upperMid = btree->upper_bound(upRecord);
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
    memcpy(&rid, iter->val, 8);
    iter ++;
    return 0;
}

bool Comparator::operator()(const IxElement& x, const IxElement& y) const{ 
    AttrList a, b;
    im->recordToAttrList(x.val, a);
    im->recordToAttrList(y.val, b);
    return a < b;
}