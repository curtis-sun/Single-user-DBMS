#ifndef COMPARE
#define COMPARE


#include "pagedef.h"
#include <cstring>

template<typename T> 
bool compare(const T& a, const T& b, CompOp compOp){
	switch(compOp){
		case EQ_OP:{
			return a == b;
		}
		case LT_OP:{
			return a < b;
		}
		case GT_OP:{
			return a > b;
		}
		case LE_OP:{
			return a <= b;
		}
		case GE_OP:{
			return a >= b;
		}
		case NE_OP:{
			return a != b;
		}
		case NO_OP:{
			return true;
		}
		default:{
			return false;
		}
	}
}

bool compare(const char* a, const char* b, AttrType attrType, CompOp compOp){
	if (a[0] == 0 || b[0] == 0){
		return false;
	}
	switch(attrType){
		case DATE:
		case INTEGER:{
			int op1 = *(const int *)(a + 1);
			int op2 = *(const int *)(b + 1);
			return compare<int>(op1, op2, compOp);
		}
		case FLOAT:{
			float op1 = *(const float *)(a + 1);
			float op2 = *(const float *)(b + 1);
			return compare<float>(op1, op2, compOp);
		}
		case STRING:{
			switch(compOp){
				case EQ_OP:{
					return strcmp(a + 1, b + 1) == 0;
				}
				case LT_OP:{
					return strcmp(a + 1, b + 1) < 0;
				}
				case GT_OP:{
					return strcmp(a + 1, b + 1) > 0;
				}
				case LE_OP:{
					return strcmp(a + 1, b + 1) <= 0;
				}
				case GE_OP:{
					return strcmp(a + 1, b + 1) >= 0;
				}
				case NE_OP:{
					return strcmp(a + 1, b + 1) != 0;
				}
				case NO_OP:{
					return true;
				}
				default:{
					return false;
				}
			}
		}
		default:{
			return false;
		}
	}
}

#endif
