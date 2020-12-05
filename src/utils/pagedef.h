#ifndef PAGE_DEF
#define PAGE_DEF
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <cstdint>
#include <cstring>
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
#define MAX_COL_NUM 31
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

#define MAX_ATTR_LEN 255

enum AttrType {
    INTEGER, STRING, FLOAT, DATE
};

enum CompOp {
    EQ_OP, LT_OP, GT_OP, LE_OP, GE_OP, NE_OP, NO_OP
};

struct AttrVal{
    union {
        int i;
        float f;
    } val;
    AttrType type;
    char s[];
};

struct MultiCol{
    int len;
    AttrVal vals[];
    friend bool operator< (const MultiCol &a, const MultiCol &b) {
        for(int i = 0; i < a.len; i ++){
            switch(a.vals[i].type){
                case INTEGER: 
                case DATE: {
                    if (a.vals[i].val.i < b.vals[i].val.i){
                        return true;
                    }
                    if (b.vals[i].val.i < a.vals[i].val.i){
                        return false;
                    }
                    break;
                }
                case STRING: {
                    if (strcmp(a.vals[i].s, b.vals[i].s) < 0){
                        return true;
                    }
                    if (strcmp(b.vals[i].s, a.vals[i].s) < 0){
                        return false;
                    }
                    break;
                }
                case FLOAT: {
                    if (a.vals[i].val.f < b.vals[i].val.f){
                        return true;
                    }
                    if (b.vals[i].val.f < a.vals[i].val.f){
                        return false;
                    }
                    break;
                }
                default: {}
            }
        }
    }
};
#endif
