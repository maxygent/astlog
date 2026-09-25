#include "loglevel.h"
#include <string_view>
namespace astlog{

std::string_view toStr(LEVEL level){
    char res[32] = {'\0'};
    #define LEVELTOSTR(x) case LEVEL::x:{ strcpy(res,#x); break;}
    switch(level){
    FOREACH(LEVELTOSTR)
    }
    #undef LEVELTOSTR
    return std::string_view{res,sizeof(res)};
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

}