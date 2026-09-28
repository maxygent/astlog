#include <cstdio>
#include "ansicolor_sink.h"
#include "basic_sink.h"
#include "color_sink.h"
#include "common.h"
#include "loglevel.h"
#include "logmsg.h"
#include "membuf.h"


namespace sink {

template<Lockable Mutex>
ansicolorSink<Mutex>::ansicolorSink(){}      


template<Lockable Mutex>
ansicolorSink<Mutex>::ansicolorSink(colorMap color,details::formatterPtr formatter,FILE* target_file):
m_colorMap(color),m_file(target_file),basicSink<Mutex>(std::move(formatter)){
    if(!m_file)
    {
        perror("file can not be openned!  check if accessible");
    } 
}


template<Lockable Mutex>
void ansicolorSink<Mutex>::setColor(LEVEL level, std::string color){
    m_colorMap[toIndex(level)] = color;
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
    buffer.append(m_colorMap[toIndex(msg.m_level)]);
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
