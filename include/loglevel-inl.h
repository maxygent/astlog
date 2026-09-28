#include "loglevel.h"
#include <string_view>


constexpr std::string toStr(LEVEL level){
    #define LEVELTOSTR(x) case LEVEL::x:{return #x;}
    switch(level){
    FOREACH(LEVELTOSTR)
    default:
        return "";
    }
    #undef LEVELTOSTR
    
}


constexpr LEVEL toLevel(std::string_view level){
    if(level == "MAX")
        return LEVEL::INFO;
    #define LEVELTOSTR(x) if(level == #x) { return LEVEL::x;}
    FOREACH(LEVELTOSTR)
    #undef LEVELTOSTR
    return LEVEL::INFO;
}

constexpr LEVEL toLevel(std::string level){
    return toLevel(std::string_view{level.data(),level.size()});
}


constexpr size_t toIndex(LEVEL level){
    return static_cast<size_t>(level);
}


