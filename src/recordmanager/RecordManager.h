# ifndef RECORD_MANAGER
# define RECORD_MANAGER

# include "../utils/pagedef.h"
# include "../bufmanager/BufPageManager.h"
# include "../systemmanager/TableHeader.h"

class RM_FileScan;

class RecordManager{
    TableHeader header;
    std::string tablePath;
    BufPageManager* bpm;
    FileManager* fm;
    int fileID = -1;

public:  
    void createFile();
    void destroyFile();
    void openFile();
    void closeFile();

    void allocPage();

    void insertRecord(const char* data);
    void deleteRecord(RID_t rid);
    void updateRecord(RID_t rid, const char* data);
    void getRecord(RID_t rid, char* data);

    RecordManager(std::string m_name, std::string m_path, BufPageManager* m_bpm);

    RM_FileScan* fileScan;
    friend RM_FileScan;
};

class RM_FileScan { 
    AttrType attrType;
    CompOp compOp;
    char value[MAX_ATTR_LEN];

    int attrLength;
    int attrOffset;
    RecordManager* rm;
    uint32_t pageID;
    uint16_t offset;
    BufType pagePos;
    int index;
    char tempData[MAX_ATTR_LEN];

    RM_FileScan(RecordManager* m_rm):rm(m_rm){}
public:                                                                  
    void openScan(AttrType m_attrType, int m_attrLength, int m_attrOffset, CompOp m_compOp, const char* m_value);           
    int getNextEntry(char* data);                   
    //void closeScan();
    friend RecordManager;                                 
};
# endif