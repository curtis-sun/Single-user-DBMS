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
                default: {
                    return false;
                }
            }
        }
        return false;
    }
};

class IX_IndexScan;

class IX_Manager{
    stx::btree_set<Entry> btree;
    std::string ixPath;

    std::string __indexName();
public:  
    void destroyIndex();
    void openIndex();
    void closeIndex();

    void insertEntry(const Entry& data);
    void deleteEntry(const Entry& data);

    IX_Manager(const std::string& path, const std::string& name, const std::vector<std::string>& c_names);
    IX_Manager(const std::string& path, const std::string& name, const std::vector<std::string>& c_names, const std::string& m_refTbName, const std::vector<std::string>& m_refKeys);
    ~IX_Manager(){
        delete indexScan;
    }

    IX_IndexScan* indexScan;
    std::vector<std::string> keys;
    std::string ixName;
    std::string refTbName;
    std::vector<std::string> refKeys;
};

class IX_IndexScan { 
    CompOp compOp;
    stx::btree_set<Entry>& btree;

    IX_IndexScan(stx::btree_set<Entry>& b): btree(b){}  
public: 
    typename stx::btree_set<Entry>::iterator iter, lowerBound, upperBound;                                                                
    void openScan(const Entry& data, CompOp m_compOp);           
    int getNextEntry(RID_t& rid);                   
    //void closeScan();   
    friend IX_Manager;                           
};

#endif