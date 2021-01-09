#ifndef PAGE_DEF
#define PAGE_DEF
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <stdint.h>
#include <string.h>
/*
 * 一个页面中的字节数
 */
#define PAGE_SIZE 8192
/*
 * 一个页面中的整数个数
 */
#define PAGE_INT_NUM 2048
/*
 * 页面字节数以2为底的指数
 */
#define PAGE_SIZE_IDX 13
#define MAX_FMT_INT_NUM 128
//#define BUF_PAGE_NUM 65536
#define MAX_FILE_NUM 128
#define MAX_TYPE_NUM 256
/*
 * 缓存中页面个数上限
 */
#define CAP 60000
/*
 * hash算法的模
 */
#define MOD 60000
#define IN_DEBUG 0
#define DEBUG_DELETE 0
#define DEBUG_ERASE 1
#define DEBUG_NEXT 1
/*
 * 一个表中列的上限
 */
#define MAX_COL_NUM 32
#define MAX_NAME_LEN 63
/*
 * 数据库中表的个数上限
 */
#define MAX_TB_NUM 32
#define RELEASE 1
typedef unsigned int* BufType;
typedef unsigned int uint;
typedef unsigned short ushort;
typedef unsigned char uchar;
typedef unsigned long long ull;
typedef long long ll;
typedef double db;
typedef int INT;
typedef int(cf)(uchar*, uchar*);
typedef uint64_t RID_t;
//int current = 0;
//int tt = 0;

#define MAX_ATTR_LEN 256
#define MAX_IX_NUM 8

enum AttrType {
    INTEGER, STRING, FLOAT, DATE, NO_TYPE, POS_TYPE
};

enum CalcOp {
    ADD_OP, SUB_OP, MUL_OP, DIV_OP, LIKE_OP, OR_OP, AND_OP, EQ_OP, LT_OP, GT_OP, LE_OP, GE_OP, NE_OP, IS_NULL, NOT_NULL, IN_OP
};

struct AttrVal{
    union {
        int i;
        float f;
    } val;
    AttrType type = NO_TYPE;
    char s[MAX_ATTR_LEN];
};
#endif
