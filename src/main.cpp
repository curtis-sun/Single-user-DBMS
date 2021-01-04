# include <cstdio>
# include <string>
# include "utils/MyBitMap.h"
extern char start_parse(const char *expr_input);

extern int yyparse();

int main(){
    while(true){
        try{
            int rc = yyparse();
            if (!rc){
                break;
            }
        }
        catch(char const* msg){
            printf("%s\n", msg);
        }
        catch(std::string msg){
            printf("%s\n", msg.c_str());
        }
    }
}