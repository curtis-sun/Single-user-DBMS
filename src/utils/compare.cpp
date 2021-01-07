#ifndef COMPARE
#define COMPARE

#include "pagedef.h"
#include <cstring>
#include <sstream>
#include <string>
#include <regex>
# include <cmath>

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
		case STRING:{
			return val.s;
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
	val.s[len - 1] = 0; // string last '\0'
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
		memcpy(val.s, temp.c_str(), temp.length() + 1);
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
static bool compare(const T& a, const T& b, CalcOp compOp){
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
			throw "error: Ts cannot be operated";
		}
	}
}

template<typename T> 
static T calculate(const T& a, const T& b, CalcOp op){
	switch(op){
		case ADD_OP:{
			return a + b;
		}
		case SUB_OP:{
			return a - b;
		}
		case MUL_OP:{
			return a * b;
		}
		case DIV_OP:{
			return a / b;
		}
		default:{
			throw "error: Ts cannot be calculated";
		}
	}
}

static bool strlike(const char *s1, const char *s2)
{
	// See: https://docs.microsoft.com/en-us/sql/t-sql/language-elements/like-transact-sql?view=sql-server-2017
	enum {
		STATUS_A,
		STATUS_SLASH,
	} status = STATUS_A;
	std::string pattern;

	for(int i = 0; s2[i]; ++i)
	{
		switch(status)
		{
			case STATUS_A:
				switch(s2[i])
				{
					case '\\':
						status = STATUS_SLASH;
						break;
					case '%':
						pattern += ".*";
						break;
					case '_':
						pattern.push_back('.');
						break;
					default:
						pattern.push_back(s2[i]);
						break;
				}
				break;
			case STATUS_SLASH:
				switch(s2[i])
				{
					case '%':
					case '_':
						pattern.push_back(s2[i]);
						break;
					default:
						pattern.push_back('\\');
						pattern.push_back(s2[i]);
						break;
				}
				status = STATUS_A;
				break;
		}
	}

	return std::regex_match(s1, std::regex(pattern));
}

static AttrVal compare(AttrVal a, AttrVal b, CalcOp compOp){
	AttrVal val;
	val.type = INTEGER;
	if (a.type == NO_TYPE || b.type == NO_TYPE){
		return AttrVal();
	}
	if (!attrConvert(a, b.type) && !attrConvert(b, a.type)){
		throw "error: attributes cannot be operated";
		val.val.i = 0;
		return val;
	}
	switch(a.type){
		case DATE:
		case INTEGER:{
			val.val.i = compare<int>(a.val.i, b.val.i, compOp);
			return val;
		}
		case FLOAT:{
			val.val.i = compare<float>(a.val.f, b.val.f, compOp);
			return val;
		}
		case STRING:{
			switch(compOp){
				case EQ_OP:{
					val.val.i = strcmp(a.s, b.s) == 0;
					return val;
				}
				case LT_OP:{
					val.val.i = strcmp(a.s, b.s) < 0;
					return val;
				}
				case GT_OP:{
					val.val.i = strcmp(a.s, b.s) > 0;
					return val;
				}
				case LE_OP:{
					val.val.i = strcmp(a.s, b.s) <= 0;
					return val;
				}
				case GE_OP:{
					val.val.i = strcmp(a.s, b.s) >= 0;
					return val;
				}
				case NE_OP:{
					val.val.i = strcmp(a.s, b.s) != 0;
					return val;
				}
				default:{
					throw "error: unknown compare operation " + std::to_string(compOp);
				}
			}
		}
		default:{
			throw "error: attributes " + attrToString(a) + ", " + attrToString(b) + " cannot be compared";
		}
	}
}

static AttrVal calculate(AttrVal a, AttrVal b, CalcOp op){
	if (a.type == NO_TYPE || b.type == NO_TYPE){
		return AttrVal();
	}
	if (a.type != INTEGER && a.type != FLOAT || b.type != INTEGER && b.type != FLOAT){
		throw "error: attributes " + attrToString(a) + ", " + attrToString(b) + " cannot be calculated";
	}
	if (!attrConvert(a, b.type) && !attrConvert(b, a.type)){
		throw "error: attributes cannot be operated";
	}
	switch(a.type){
		case INTEGER:{
			if (op == DIV_OP && b.val.i == 0){
				printf("warning: attributes %s divided by zero\n", attrToString(a).c_str());
				return AttrVal();
			}
			AttrVal val;
			val.type = INTEGER;
			val.val.i = calculate<int>(a.val.i, b.val.i, op);
			return val;
		}
		case FLOAT:{
			if (op == DIV_OP && fabs(b.val.f) < 1e-8){
				printf("warning: attributes %s divided by zero\n", attrToString(a).c_str());
				return AttrVal();
			}
			AttrVal val;
			val.type = FLOAT;
			val.val.f = calculate<float>(a.val.f, b.val.f, op);
			return val;
		}
		default:{
			throw "error: attributes " + attrToString(a) + ", " + attrToString(b) + " cannot be calculated";
		}
	}
}

static AttrVal like(const AttrVal& a, const AttrVal& b){
	if (a.type == NO_TYPE || b.type == NO_TYPE){
		return AttrVal();
	}
	if (a.type != STRING || b.type != STRING){
		throw "error: attributes " + attrToString(a) + ", " + attrToString(b) + " cannot be liked";
	}
	AttrVal val;
	val.type = INTEGER;
	val.val.i = strlike(a.s, b.s);
	return val;
}

static AttrVal logic(const AttrVal& a, const AttrVal& b, CalcOp op){
	AttrVal val;
	val.type = INTEGER;
	if (a.type == NO_TYPE || b.type == NO_TYPE){
		return AttrVal();
	}
	if (a.type != INTEGER || b.type != INTEGER){
		throw "error: attributes " + attrToString(a) + ", " + attrToString(b) + " cannot be logically operated";
	}
	switch(op){
		case AND_OP:{
			val.val.i = a.val.i && b.val.i;
			return val;
		}
		case OR_OP:{
			val.val.i = a.val.i || b.val.i;
			return val;
		}
		default:{
			throw "error: unknown binary operation " + std::to_string(op);
		}
	}
}

static AttrVal judge(const AttrVal& a, CalcOp compOp){
	AttrVal val;
	val.type = INTEGER;
	switch(compOp){
		case IS_NULL:{
			val.val.i = a.type == NO_TYPE;
			return val;
		}
		case NOT_NULL:{
			val.val.i = a.type != NO_TYPE;
			return val;
		}
		default:{
			throw "error: unknown unary operation " + std::to_string(compOp);
		}
	}
}

#endif
