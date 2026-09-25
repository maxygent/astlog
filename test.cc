#include "logger.h"
#include "membuf.h"
#include <iostream>
#include "logtime.h"

int main(){
    details::inlineBuffer<details::FORMATTED_RECORD_INLINE_CAPACITY> buffer;
    details::smallStreamBuffer<details::FORMATTED_RECORD_INLINE_CAPACITY>  smb{buffer};
    std::ostream os{&smb};
    os << details::time::getLocalTime();
    std::cout<< buffer.data();
    
}