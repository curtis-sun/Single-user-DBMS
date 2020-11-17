# ifndef TABLE_HEADER
# define TABLE_HEADER

# include "../utils/pagedef.h"

struct TableHeader{
    RID_t nextAvailable;
    uint32_t pageCnt;
    uint16_t pminlen, slotCntPerPage;
    uint8_t columnCnt;
    uint16_t columnOffsets[MAX_COL_NUM];
    AttrType columnTypes[MAX_COL_NUM];
    char columnNames[MAX_COL_NUM][MAX_NAME_LEN];
};
# endif