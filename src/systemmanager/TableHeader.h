# ifndef TABLE_HEADER
# define TABLE_HEADER

# include "../utils/pagedef.h"

struct TableHeader{
    char checks[MAX_CHECK_NUM][MAX_CHECK_LEN];
    char columnNames[MAX_COL_NUM][MAX_NAME_LEN];
    uint16_t columnOffsets[MAX_COL_NUM + 1];
    AttrType columnTypes[MAX_COL_NUM];
    uint8_t constraints[MAX_COL_NUM]; // 末3位表示not null / / default
    RID_t nextAvailable;
    uint32_t pageCnt;
    uint16_t pminlen;
    uint8_t columnCnt;
    uint8_t checkCnt;
};
# endif