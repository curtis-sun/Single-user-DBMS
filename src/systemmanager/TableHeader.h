# ifndef TABLE_HEADER
# define TABLE_HEADER

# include "../utils/pagedef.h"

struct TableHeader{
    RID_t nextAvailable;
    uint32_t pageCnt;
    uint16_t columnOffsets[MAX_COL_NUM + 1];
    uint16_t pminlen;
    AttrType columnTypes[MAX_COL_NUM];
    char columnNames[MAX_COL_NUM][MAX_NAME_LEN];
    uint8_t constraints[MAX_COL_NUM]; // 末3位表示not null / / default
    uint8_t columnCnt;
};
# endif