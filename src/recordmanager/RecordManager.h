# ifndef RECORD_MANAGER
# define RECORD_MANAGER

# include "../utils/pagedef.h"
# include "../bufmanager/BufPageManager.h"
# include "../systemmanager/TableHeader.h"

class RecordManager{
    TableHeader* header;
    std::string tableName, tablePath;
    BufPageManager* bpm;
    FileManager* fm;
    int fileID = -1;

public:  
    void createFile();
    void dropFile();
    void openFile();
    void closeFile();

    void allocPage();

    void insertRecord(const char* data);
    void deleteRecord(RID_t rid);
    void updateRecord(RID_t rid, const char* data);
    void selectRecord(RID_t rid, char* data);

    RecordManager(std::string m_name, std::string m_path, BufPageManager* m_bpm);
};
# endif