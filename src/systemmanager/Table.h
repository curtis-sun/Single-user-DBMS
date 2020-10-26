# ifndef TABLE
# define TABLE

# include "../recordmanager/RecordManager.h"
# include "../bufmanager/BufPageManager.h"

class Table{
    std::string name;
    RecordManager* rm;

public:
    Table(std::string m_name, std::string path, BufPageManager* m_bpm){
        name = m_name;
        rm = new RecordManager(m_name, path, m_bpm);
    }
};
# endif