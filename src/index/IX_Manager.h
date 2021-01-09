# ifndef IX_MANAGER
# define IX_MANAGER

# include "../utils/pagedef.h"
# include "btree_set.h"
# include <vector>

class AttrList{
public:
    RID_t rid;
    std::vector<AttrVal> vals;

    AttrList(){}
    AttrList(const AttrList& attrList, int len){
        rid = attrList.rid;
        if (len > attrList.vals.size()){
            len = attrList.vals.size();
        }
        for (int i = 0; i < len; i ++){
            vals.push_back(attrList.vals[i]);
        }
    }
    bool operator< (const AttrList &b) const {
        for(int i = 0; i < vals.size(); i ++){
            if (vals[i].type != NO_TYPE && b.vals[i].type == NO_TYPE){
                return false;
            }
            if (vals[i].type != POS_TYPE && b.vals[i].type == POS_TYPE){
                return true;
            }
            switch(vals[i].type){
                case INTEGER: 
                case DATE: {
                    if (vals[i].val.i < b.vals[i].val.i){
                        return true;
                    }
                    if (b.vals[i].val.i < vals[i].val.i){
                        return false;
                    }
                    break;
                }
                case STRING: {
                    if (strcmp(vals[i].s, b.vals[i].s) < 0){
                        return true;
                    }
                    if (strcmp(b.vals[i].s, vals[i].s) < 0){
                        return false;
                    }
                    break;
                }
                case FLOAT: {
                    if (vals[i].val.f < b.vals[i].val.f){
                        return true;
                    }
                    if (b.vals[i].val.f < vals[i].val.f){
                        return false;
                    }
                    break;
                }
                case NO_TYPE: {
                    if (b.vals[i].type != NO_TYPE){
                        return true;
                    }
                    break;
                }
                case POS_TYPE: {
                    if (b.vals[i].type != POS_TYPE){
                        return false;
                    }
                    break;
                }
                default: {
                    throw "error: unknown attrlist value type " + std::to_string(vals[i].type);
                }
            }
        }
        return rid < b.rid;
    }
};

struct IxElement{
    char val[MAX_ATTR_LEN];
    IxElement& operator=(const IxElement& el){
        memcpy(val, el.val, sizeof(val));
        return *this;
    }
};

struct IxCol{
    std::string name;
    int offset;
    AttrType type;
    bool operator< (const IxCol &b) const {
        return name < b.name;
    }
    bool operator== (const IxCol &b) const {
        return name == b.name;
    }
};

class IX_Manager;

class Comparator{
public:
    IX_Manager* im;
    Comparator(IX_Manager* i){
        im = i;
    }
    bool operator()(const IxElement& x, const IxElement& y) const;
};

class IX_IndexScan { 
    CalcOp compOp;
    
    stx::btree_set<IxElement, Comparator>* btree;
    IX_Manager* im;

    IX_IndexScan(stx::btree_set<IxElement, Comparator>* b, IX_Manager* i): btree(b), im(i){}  
    void __lowestFill(AttrList& entry);
    void __uppestFill(AttrList& entry);
public: 
    stx::btree_set<IxElement, Comparator>::iterator iter, lowerMid, upperMid, upperBound;                                                                
    void openScan(const AttrList& data, int num, CalcOp m_compOp);           
    int getNextEntry(RID_t& rid);                   
    //void closeScan();   
    friend IX_Manager;                           
};

class IX_Manager{
    stx::btree_set<IxElement, Comparator>* btree;
    std::string ixPath;

    std::string __indexName();
public:  
    void destroyIndex();
    void openIndex();
    void closeIndex();

    void insertEntry(const AttrList& data);
    void deleteEntry(const AttrList& data);

    void restoreAttrList(const AttrList& list, char* data);
    void recordToAttrList(const char* record, AttrList& list);

    IX_Manager(const std::string& path, const std::string& name, const std::string& c, const std::vector<IxCol>& c_names);
    IX_Manager(const std::string& path, const std::string& name, const std::vector<IxCol>& c_names, const std::string& m_refTbName, const std::vector<std::string>& m_refKeys);
    ~IX_Manager(){
        delete indexScan;
    }

    IX_IndexScan* indexScan;
    std::vector<IxCol> keys;
    std::string ixName;
    std::string ixClass;
    std::string refTbName;
    std::vector<std::string> refKeys;
};

#endif