#include <cstdio>
#include <unordered_map>
#include "ansicolor_sink.h"
#include "basic_sink.h"
#include "common.h"
#include "logmsg.h"
#include "membuf.h"
#include "sink.h"

namespace sink {

template<Lockable Mutex>
ansicolorSink<Mutex>::ansicolorSink(){}      


template<Lockable Mutex>
ansicolorSink<Mutex>::ansicolorSink(std::unordered_map<LEVEL,std::string> colorMap,std::unique_ptr<details::formatter> formatter,FILE* target_file):
m_colorMap(std::move(colorMap)),m_file(target_file),basicSink<Mutex>(std::move(formatter)){
    if(!m_file)
    {
        perror("file can not be openned!  check if accessible");
    } 
}


template<Lockable Mutex>
void ansicolorSink<Mutex>::setColor(LEVEL level, std::string color){
    m_colorMap[level] = color;
}

template<Lockable Mutex>
void ansicolorSink<Mutex>::setColor(std::initializer_list<std::pair<LEVEL,std::string>> list){
    for(auto &[level,color]:list)
    {
        setColor(level,color);
    }
}

template<Lockable Mutex>
void ansicolorSink<Mutex>::sinkIt(const details::logmsg& msg){
    details::formatterBuf buffer;
    buffer.append(m_colorMap[msg.m_level]);
    basicSink<Mutex>::m_formatter->format(msg,buffer);
    buffer.append(reset);
    std::fwrite(buffer.data(),sizeof(char),buffer.size(),m_file);
    std::fwrite(details::EOL, sizeof(char),1, m_file);
}

template<Lockable Mutex>
void ansicolorSink<Mutex>::flush_(){
    std::fflush(m_file);
}


template<Lockable Mutex>
void ansicolorSink<Mutex>::sync_(){
    ::fsync(::fileno(m_file));
}


}
