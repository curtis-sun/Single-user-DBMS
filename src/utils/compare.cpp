#ifndef COMPARE
#define COMPARE

#include "pagedef.h"
#include <cstring>
#include <sstream>
#include <string>

static int dateRestore(const std::string& dateFormat){
    int yearMonth = dateFormat.find("-");
    int monthDate = dateFormat.find("-", yearMonth + 1);
    int year = stoi(dateFormat.substr(0, yearMonth));
    int month = stoi(dateFormat.substr(yearMonth + 1, monthDate - yearMonth - 1));
    int date = stoi(dateFormat.substr(monthDate + 1));
    return year * 10000 + month * 100 + date;
}

static std::string attrToString(const AttrVal& val){
	switch(val.type){
		case INTEGER:{
			return std::to_string(val.val.i);
		}
		case FLOAT:{
			return std::to_string(val.val.f);
		}
		case NO_TYPE:{
			return "null";
		}
		case DATE:{
			int year = val.val.i / 10000;
			int month = (val.val.i - year * 10000) / 100;
			int date = val.val.i - year * 10000 - month * 100;
			std::ostringstream sstream;
			sstream.width(4);
			sstream.fill('0');
			sstream << year;
			sstream << '-';
			sstream.width(2);
			sstream.fill('0');
			sstream << month;
			sstream << '-';
			sstream.width(2);
			sstream.fill('0');
			sstream << date;
			return sstream.str();
		}
		default:{
			throw "error: attribute cannot convert to string because of unknown type";
		}
	}
}

static void restoreAttr(char* data, int len, const AttrVal& val){
	switch(val.type){
		case NO_TYPE:{
			data[0] = 0;
			break;
		}
		case DATE:
		case INTEGER:{
			data[0] = 1;
			memcpy(data + 1, &val.val.i, len - 1);
			break;
		}
		case FLOAT:{
			data[0] = 1;
			memcpy(data + 1, &val.val.f, len - 1);
			break;
		}
		case STRING:{
			data[0] = 1;
			memcpy(data + 1, val.s, len - 1);
			break;
		}
		default:{
			throw "error: insert unknown type value";
		}
	}
}

static AttrVal recordToAttr(const char* data, int len, AttrType attrType){
	AttrVal val;
	if (data[0] == 0){
		return val;
	}
	val.type = attrType;
	memcpy(val.s, data + 1, len - 1);
	switch(attrType){
		case INTEGER:
		case DATE: {
			val.val.i = *(const int *)val.s;
			break;
		}
		case STRING: {
			break;
		}
		case FLOAT: {
			val.val.f = *(const float *)val.s;
			break;
		}
		default: {
			throw "error: record cannot convert to attribute because of unknown type";
		}
	}
	return val;
}

static bool isAttrConvert(AttrType type, AttrType newType){
	if (type == newType){
		return true;
	}
	if (type == NO_TYPE || newType == NO_TYPE){
		return false;
	}
	if (newType == STRING){
		return true;
	}
	if (newType == FLOAT && type == INTEGER){
		return true;
	}
	if (newType == INTEGER && type == FLOAT){
		return true;
	}
	return false;
}

static bool attrConvert(AttrVal& val, AttrType newType){
	if (val.type == newType){
		return true;
	}
	if (val.type == NO_TYPE || newType == NO_TYPE){
		return false;
	}
	if (newType == STRING){
		std::string temp = attrToString(val);
		memcpy(val.s, temp.c_str(), temp.length());
		val.type = newType;
		return true;
	}
	if (newType == FLOAT && val.type == INTEGER){
		val.val.f = val.val.i;
		val.type = newType;
		return true;
	}
	if (newType == INTEGER && val.type == FLOAT){
		val.val.i = val.val.f;
		val.type = newType;
		return true;
	}
	return false;
}

template<typename T> 
static bool compare(const T& a, const T& b, CompOp compOp){
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
		default:{
			return false;
		}
	}
}

static bool compare(AttrVal a, AttrVal b, CompOp compOp){
	if (a.type == NO_TYPE || b.type == NO_TYPE){
		return false;
	}
	if (!attrConvert(a, b.type) && !attrConvert(b, a.type)){
		throw "error: attributes cannot be operated";
		return false;
	}
	switch(a.type){
		case DATE:
		case INTEGER:{
			return compare<int>(a.val.i, b.val.i, compOp);
		}
		case FLOAT:{
			return compare<float>(a.val.f, b.val.f, compOp);
		}
		case STRING:{
			switch(compOp){
				case EQ_OP:{
					return strcmp(a.s, b.s) == 0;
				}
				case LT_OP:{
					return strcmp(a.s, b.s) < 0;
				}
				case GT_OP:{
					return strcmp(a.s, b.s) > 0;
				}
				case LE_OP:{
					return strcmp(a.s, b.s) <= 0;
				}
				case GE_OP:{
					return strcmp(a.s, b.s) >= 0;
				}
				case NE_OP:{
					return strcmp(a.s, b.s) != 0;
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

static bool logic(const AttrVal& a, const AttrVal& b, CalcOp op){
	if (a.type == NO_TYPE || b.type == NO_TYPE){
		return false;
	}
	switch(op){
		case AND_OP:{
			return a.val.i && b.val.i;
		}
		case OR_OP:{
			return a.val.i || b.val.i;
		}
		default:{
			throw "error: unknown binary operation " + std::to_string(op);
		}
	}
}

static bool judge(const AttrVal& a, CompOp compOp){
	switch(compOp){
		case IS_NULL:{
			return a.type == NO_TYPE;
		}
		case NOT_NULL:{
			return a.type != NO_TYPE;
		}
		default:{
			throw "error: unknown unary operation " + std::to_string(compOp);
		}
	}
}

#endif
