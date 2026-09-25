#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <source_location>
#include <thread>



#include "pattern_formatter.h"
#include "common.h"
#include "logmsg.h"
#include "logtime.h"
#include "membuf.h"




namespace details{



class scopedPadder {
public:

    scopedPadder(size_t wrapped_size, const paddingInfo &padinfo, formatterBuf &dest)
        : m_padinfo(padinfo),
          m_dest(dest) {
        // 对齐大小 - 内容大小
        m_remainedPadding = static_cast<long>(padinfo.m_width) - static_cast<long>(wrapped_size);
        if (m_remainedPadding <= 0) {
            return;
        }

        if (m_padinfo.m_side == paddingInfo::padSide::left) {
            pad_it(m_remainedPadding);
            m_remainedPadding = 0;
        } else if (m_padinfo.m_side == paddingInfo::padSide::center) {
            auto half_pad = m_remainedPadding / 2;  // 考虑可能截断
            auto reminder = m_remainedPadding & 1;  // 补上一个保证总宽度一致
            pad_it(half_pad);
            m_remainedPadding = half_pad + reminder;  // for the right side
        }
    }

    ~scopedPadder() {
        if (m_remainedPadding >= 0) {
            pad_it(m_remainedPadding);
        } else if (m_padinfo.m_truncate) {
            // 强制截断   remaining_pad是负数 强制截断
            long new_size = m_padinfo.m_width;
            if (new_size < 0) {
                new_size = 0;
            }
            m_dest.erase(new_size);
        }
    }

private:
    void pad_it(long count) {
        m_dest.append(m_spaces.data(),count);
    }

    const paddingInfo &m_padinfo;
    formatterBuf &m_dest;
    int m_remainedPadding;
    std::string_view m_spaces{"                                                                ", 64};
};

struct nullScopedPadder {
    nullScopedPadder(size_t /*wrapped_size*/,
                       const paddingInfo & /*padinfo*/,
                       formatterBuf & /*dest*/) {}

    template <typename T>
    static unsigned int count_digits(T /* number */) {
        return 0;
    }
};

template <class ScopedPadder>
class nameFormatter final : public flagFormatter {
public:
    explicit nameFormatter(paddingInfo padinfo)
    : flagFormatter(padinfo) {}
    void format(const logmsg &msg, formatterBuf &dest) override {
        ScopedPadder _(msg.m_loggerName.size(), m_padInfo, dest);
        dest.append(msg.m_loggerName);
    }
};



template<class scopedPadder>
class  timeFormatter final: public flagFormatter{
public:
    explicit timeFormatter(paddingInfo padinfo, std::string timeformat = "%Y-%m-%d  %H:%M:%S")
    : flagFormatter(padinfo),m_timeFormat(timeformat){}
    void format(const logmsg &msg,formatterBuf &dest) override {
        // scopedPadder
        static thread_local clock::time_point lastTime;
        static thread_local char cachedTime[32] = {'\0'};
        const auto currentTime = std::chrono::time_point_cast<std::chrono::seconds>(msg.m_timeStamp);
        if(currentTime != lastTime)
        {   
            std::time_t now_c = std::chrono::system_clock::to_time_t(currentTime);
            std::tm* local_tm;
            std::tm tmBuf;
            local_tm = localtime_r(&now_c,&tmBuf);
            if(local_tm != nullptr)
                std::strftime(cachedTime,sizeof(cachedTime),m_timeFormat.data(),local_tm);
            lastTime = currentTime;
        }
        scopedPadder _(sizeof(cachedTime), m_padInfo,dest);
        dest.append(cachedTime);
    }
    private:
        std::string m_timeFormat;
};

template<class scopedPadder>
class  millisecondsFormatter final: public flagFormatter{
public:
    explicit millisecondsFormatter(paddingInfo padinfo)
    : flagFormatter(padinfo){}
    void format(const logmsg &msg,formatterBuf &dest) override {
        // scopedPadder
        uint64_t ms = std::chrono::duration_cast
                    <std::chrono::milliseconds>(msg.m_timeStamp.time_since_epoch()).count();
        ms %= 1000;
        scopedPadder _(countDigit(ms) + 1, m_padInfo,dest);
        dest.append('.' + std::to_string(ms));
    }
};
template<class scopedPadder>
class  timeElapsedFormatter final: public flagFormatter{
public:
    explicit timeElapsedFormatter(paddingInfo padinfo)
    : flagFormatter(padinfo){}
    void format(const logmsg &msg,formatterBuf &dest) override {
        // scopedPadder
        uint64_t ms = std::chrono::duration_cast
                    <std::chrono::milliseconds>(clock::now() - msg.m_timeStart).count();
        scopedPadder _(countDigit(ms),m_padInfo,dest);
        dest.append( std::to_string(ms));
    }
};
template<class scopedPadder>
class  threadIdFormatter final: public flagFormatter{
public:
    explicit threadIdFormatter(paddingInfo padinfo)
    : flagFormatter(padinfo){}
    void format(const logmsg &msg,formatterBuf &dest) override {
        // scopedPadder
        scopedPadder _(countDigit(msg.m_threadId),m_padInfo,dest);
        dest.append( std::to_string(msg.m_threadId));
    }
};

template<class scopedPadder>
class  locationFormatter final: public flagFormatter{
public:
    explicit locationFormatter(paddingInfo padinfo , uint8_t mode = 0b110)
    : flagFormatter(padinfo),m_mode(mode){}
    void format(const logmsg &msg,formatterBuf &dest) override {
        // scopedPadder
        std::string buf;
        if(msg.m_loc.line() == 0)
            return;
        if(0b100 & m_mode)
        {
            std::string filename{msg.m_loc.file_name()};
            filename = filename.substr(filename.rfind('/') + 1);
            buf.append('[' + filename + ']');
        }
        if(0b010 & m_mode)
            buf.append('[' + std::to_string(msg.m_loc.line()) + ']');
        if(0b001 & m_mode)
            buf.append(msg.m_loc.function_name());

        scopedPadder _(buf.size(),m_padInfo,dest);
        dest.append( buf);
    }
private:
    uint8_t m_mode;
};
template<class scopedPadder>
class msgFormatter final : public flagFormatter{
    public:
    explicit msgFormatter(paddingInfo padinfo):flagFormatter(padinfo){}
    void format(const logmsg &msg,formatterBuf &dest) override {
        scopedPadder _(msg.m_payload.size(),m_padInfo,dest);
        dest.append(msg.m_payload);
    }
};
template<class scopedPadder>
class strFormatter final : public flagFormatter{
    public:
    explicit strFormatter(paddingInfo padinfo,std::string str):flagFormatter(padinfo),m_str(str){}
    void format(const logmsg &msg,formatterBuf &dest) override {
        scopedPadder _(m_str.size(),m_padInfo,dest);
        dest.append(m_str);
    }
    private:
    std::string m_str;
};

class colorStartFormatter final : public flagFormatter {
public:
    explicit colorStartFormatter(paddingInfo padinfo)
        : flagFormatter(padinfo) {}

    void format(const logmsg &msg, formatterBuf &dest) override {
        msg.m_colorRangeStart = dest.size();
    }
};

class colorStopFormatter final : public flagFormatter {
public:
    explicit colorStopFormatter(paddingInfo padinfo)
        : flagFormatter(padinfo) {}

    void format(const logmsg &msg, formatterBuf &dest) override {
        msg.m_colorRangeStop = dest.size();
    }
};




}
#include <iostream>
using namespace details;




int main()
{
    paddingInfo pad {10,paddingInfo::padSide::center,true};
    locationFormatter<nullScopedPadder> tim{pad,0b110};
    logmsg msg;
    
    
    details::inlineBuffer<256> buf;
    tim.format(msg, buf);
    std::cout << buf.data();
    return 0;
}




