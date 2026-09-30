#include <cstddef>
#include "membuf.h"
#include "sink/sink.h"
#include "thread_pool.h"




namespace details{








struct threadPool::impl{
    impl(size_t num){
        
    }
    void postmsg(const async_msg & msg)
    {

    }



private:
    
    DataBuf<async_msg> buffer;
};


threadPool::threadPool(size_t num) : m_impl(std::make_shared<impl>(num)){
    
}

void threadPool::postmsg(sink::sinkPtr sinker,const logmsg& msg){
    async_msg asmsg{sinker,std::move(msg)};
    m_impl->postmsg(asmsg);
}






}