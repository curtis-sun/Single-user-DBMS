# include <cstdio>
# include <string>
# include "utils/MyBitMap.h"
# include "querymanager/QueryManager.h"
extern char start_parse(const char *expr_input);

int main(){
    while(true){
        try{
            char* input = nullptr;
            do{
                start_parse(input);
                if (Copy::inserts.size() > 0){
                    delete[] input;
                    input = Copy::inserts[0];
                    Copy::inserts.erase(Copy::inserts.begin());
                }
                else{
                    input = nullptr;
                }
            }
            while(input);
        }
        catch(char const* msg){
            printf("%s\n", msg);
        }
        catch(std::string msg){
            printf("%s\n", msg.c_str());
        }
    }
}