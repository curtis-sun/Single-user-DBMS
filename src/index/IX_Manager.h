# ifndef IX_MANAGER
# define IX_MANAGER

# include "../utils/pagedef.h"
# include "btree_set.h"
# include <vector>

struct Entry{
    RID_t rid;
    AttrVal vals[MAX_IX_NUM];
    bool operator< (const Entry &b) const {
        for(int i = 0; i < MAX_IX_NUM; i ++){
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
                    throw "error: unknown entry value type " + std::to_string(vals[i].type);
                }
            }
        }
        return rid < b.rid;
    }
};

class IX_Manager;

class IX_IndexScan { 
    CalcOp compOp;
    stx::btree_set<Entry>* btree;
    IX_Manager* im;

    IX_IndexScan(stx::btree_set<Entry>* b, IX_Manager* i): btree(b), im(i){}  
    void __lowestFill(Entry& entry, int len);
    void __uppestFill(Entry& entry, int len);
public: 
    stx::btree_set<Entry>::iterator iter, lowerMid, upperMid, upperBound;                                                                
    void openScan(const Entry& data, int num, CalcOp m_compOp);           
    int getNextEntry(RID_t& rid);                   
    //void closeScan();   
    friend IX_Manager;                           
};

class IX_Manager{
    stx::btree_set<Entry>* btree;
    std::string ixPath;

    std::string __indexName();
public:  
    void destroyIndex();
    void openIndex();
    void closeIndex();

    void insertEntry(const Entry& data);
    void deleteEntry(const Entry& data);

    IX_Manager(const std::string& path, const std::string& name, const std::string& c, const std::vector<std::string>& c_names);
    IX_Manager(const std::string& path, const std::string& name, const std::vector<std::string>& c_names, const std::string& m_refTbName, const std::vector<std::string>& m_refKeys);
    ~IX_Manager(){
        delete indexScan;
    }

    IX_IndexScan* indexScan;
    std::vector<std::string> keys;
    std::string ixName;
    std::string ixClass;
    std::string refTbName;
    std::vector<std::string> refKeys;
};

#endif