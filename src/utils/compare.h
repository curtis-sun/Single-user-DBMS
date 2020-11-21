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
	switch(attrType){
		case INTEGER:{
			int op1 = *(const int *)(a);
			int op2 = *(const int *)(b);
			return compare<int>(op1, op2, compOp);
		}
		case FLOAT:{
			float op1 = *(const float *)(a);
			float op2 = *(const float *)(b);
			return compare<float>(op1, op2, compOp);
		}
		case STRING:{
			switch(compOp){
				case EQ_OP:{
					return strcmp(a, b) == 0;
				}
				case LT_OP:{
					return strcmp(a, b) < 0;
				}
				case GT_OP:{
					return strcmp(a, b) > 0;
				}
				case LE_OP:{
					return strcmp(a, b) <= 0;
				}
				case GE_OP:{
					return strcmp(a, b) >= 0;
				}
				case NE_OP:{
					return strcmp(a, b) != 0;
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
