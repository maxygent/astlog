#include "logger.h"
#include "membuf.h"
#include <iostream>
#include "logtime.h"

// int main(){
//     details::inlineBuffer<details::FORMATTED_RECORD_INLINE_CAPACITY> buffer;
//     details::smallStreamBuffer<details::FORMATTED_RECORD_INLINE_CAPACITY>  smb{buffer};
//     std::ostream os{&smb};
//     os << details::time::getLocalTime();
//     std::cout<< buffer.data();
    
// }


#include "sink/file_sink.h"
#include <mutex>
#include "pattern_formatter.h"
#include "logmsg.h"
int main(){
  sink::fileSink<std::mutex> filesink{"log.txt",std::make_unique<details::patternFormatter>("%d%m %^20!s [%t] [%l] [%n] [%e] [%v]")};
  details::logmsg msg{
    .m_payload = "hello world"
  };

  filesink.log(msg);
  filesink.flush();
  filesink.sync();
}

