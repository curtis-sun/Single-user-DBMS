# ifndef RECORD_MANAGER
# define RECORD_MANAGER

# include "../utils/pagedef.h"
# include "../bufmanager/BufPageManager.h"
# include "../systemmanager/TableHeader.h"
# include <string>

class RM_FileScan;

class RecordManager{
    TableHeader* header;
    BufPageManager* bpm;
    FileManager* fm;
    int fileID = -1;
    uint16_t slotCntPerPage;


public: 
    int createFile();
    int destroyFile();
    int openFile();
    int closeFile();

    void allocPage();

    void insertRecord(const char* data, RID_t& rid);
    //void insertRecord(const char* data, RID_t rid);
    void deleteRecord(RID_t rid);
    void updateRecord(RID_t rid, const char* data);
    void getRecord(RID_t rid, char* data);

    RecordManager(const std::string& path, TableHeader* m_header, const std::string& name);
    ~RecordManager(){
        delete fileScan;
    }

    std::string tablePath;
    std::string tableName;
    RM_FileScan* fileScan;
    char defaultRow[PAGE_SIZE];
    friend RM_FileScan;
};

class RM_FileScan { 
    RecordManager* rm;
    uint32_t pageId;
    uint16_t offset;
    char* pagePos;
    int index;

    RM_FileScan(RecordManager* m_rm):rm(m_rm){}
public:                                                                 
    void openScan();           
    int getNextEntry(char* data, RID_t& rid);                   
    //void closeScan();
    friend RecordManager;                                 
};
# endif