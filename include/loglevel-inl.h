#include "loglevel.h"
#include <string_view>


std::string toStr(LEVEL level){
    std::string res;
    #define LEVELTOSTR(x) case LEVEL::x:{ res = #x; break;}
    switch(level){
    FOREACH(LEVELTOSTR)
    }
    #undef LEVELTOSTR
    return std::move(res);
}


LEVEL toLevel(std::string_view level){
    char res[32] = {'\0'};
    #define LEVELTOSTR(x) if(level == #x) { return LEVEL::x;}
    FOREACH(LEVELTOSTR)
    #undef LEVELTOSTR
    return LEVEL::INFO;
}

LEVEL toLevel(std::string level){
    return toLevel(std::string_view{level.data(),level.size()});
}

