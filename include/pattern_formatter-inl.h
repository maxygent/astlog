#include "pattern_formatter.h"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <memory>
#include <source_location>
#include <thread>
#include <utility>

#include "common.h"
#include "logmsg.h"
#include "logtime.h"
#include "membuf.h"

namespace details {

class scopedPadder {
   public:
    scopedPadder(size_t wrapped_size, const paddingInfo &padinfo,
                 formatterBuf &dest)
        : m_padinfo(padinfo), m_dest(dest),m_truncate(dest.size()) {
        // 对齐大小 - 内容大小
        m_remainedPadding = static_cast<long>(padinfo.m_width) -
                            static_cast<long>(wrapped_size);
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
            long new_size = m_padinfo.m_width + m_truncate;
            if (new_size < 0) {
                new_size = 0;
            }
            m_dest.erase(new_size);
        }
    }

   private:
    void pad_it(long count) { m_dest.append(m_spaces.data(), count); }
    size_t m_truncate;
    const paddingInfo &m_padinfo;
    formatterBuf &m_dest;
    int m_remainedPadding;
    std::string_view m_spaces{
        "                                                                ", 64};
};

struct nullScopedPadder {
    nullScopedPadder(size_t /*wrapped_size*/, const paddingInfo & /*padinfo*/,
                     formatterBuf & /*dest*/) {}

    template <class T>
    static unsigned int count_digits(T /* number */) {
        return 0;
    }
};

template <class ScopedPadder>
class nameFormatter final : public flagFormatter {
   public:
    explicit nameFormatter(paddingInfo padinfo) : flagFormatter(padinfo) {}
    void format(const logmsg &msg, formatterBuf &dest) override {
        ScopedPadder _(msg.m_loggerName.size(), m_padInfo, dest);
        dest.append(msg.m_loggerName);
    }
};

template <class ScopedPadder>
class levelFormatter final : public flagFormatter {
   public:
    explicit levelFormatter(paddingInfo padinfo) : flagFormatter(padinfo) {}
    void format(const logmsg &msg, formatterBuf &dest) override {
        auto level = astlog::toStr(msg.m_level);
        ScopedPadder _(level.size(), m_padInfo, dest);
        dest.append(level);
    }
};

template <class scopedPadder>
class timeFormatter final : public flagFormatter {
   public:
    explicit timeFormatter(paddingInfo padinfo,
                           std::string timeformat = "[%Y-%m-%d] %H:%M:%S")
        : flagFormatter(padinfo), m_timeFormat(timeformat) {}
    void format(const logmsg &msg, formatterBuf &dest) override {
        // scopedPadder
        static thread_local clock::time_point lastTime;
        static thread_local char cachedTime[32] = {'\0'};
        const auto currentTime =
            std::chrono::time_point_cast<std::chrono::seconds>(msg.m_timeStamp);
        if (currentTime != lastTime) {
            std::time_t now_c =
                std::chrono::system_clock::to_time_t(currentTime);
            std::tm *local_tm;
            std::tm tmBuf;
            local_tm = localtime_r(&now_c, &tmBuf);
            if (local_tm != nullptr)
                std::strftime(cachedTime, sizeof(cachedTime),
                              m_timeFormat.data(), local_tm);
            lastTime = currentTime;
        }
        scopedPadder _(sizeof(cachedTime), m_padInfo, dest);
        dest.append(cachedTime);
    }

   private:
    std::string m_timeFormat;
};

template <class scopedPadder>
class millisecondsFormatter final : public flagFormatter {
   public:
    explicit millisecondsFormatter(paddingInfo padinfo)
        : flagFormatter(padinfo) {}
    void format(const logmsg &msg, formatterBuf &dest) override {
        // scopedPadder
        uint64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          msg.m_timeStamp.time_since_epoch())
                          .count();
        ms %= 1000;
        scopedPadder _(4, m_padInfo, dest);
        auto s = std::to_string(ms);
        dest.append('.' + std::string(3-s.size(),'0') + s);
    }
};
template <class scopedPadder>
class timeElapsedFormatter final : public flagFormatter {
   public:
    explicit timeElapsedFormatter(paddingInfo padinfo,std::string unit = "ms")
        : flagFormatter(padinfo),m_unit(unit){}
    void format(const logmsg &msg, formatterBuf &dest) override {
        // scopedPadder
        clock::duration timeElapsed = clock::now() - msg.m_timeStart;
        uint64_t time = 0;
        if(m_unit == "ms")
            time = std::chrono::duration_cast<std::chrono::milliseconds>(timeElapsed).count();
        else if(m_unit == "s")
            time = std::chrono::duration_cast<std::chrono::seconds>(timeElapsed).count();
        else if(m_unit == "ns")
            time = std::chrono::duration_cast<std::chrono::nanoseconds>(timeElapsed).count();
        else if(m_unit == "us")
            time = std::chrono::duration_cast<std::chrono::microseconds>(timeElapsed).count();
        
        scopedPadder _(countDigit(time), m_padInfo, dest);
        dest.append(std::to_string(time));
    }
    private:
    std::string m_unit;
};
template <class scopedPadder>
class threadIdFormatter final : public flagFormatter {
   public:
    explicit threadIdFormatter(paddingInfo padinfo) : flagFormatter(padinfo) {}
    void format(const logmsg &msg, formatterBuf &dest) override {
        // scopedPadder
        scopedPadder _(countDigit(msg.m_threadId), m_padInfo, dest);
        dest.append(std::to_string(msg.m_threadId));
    }
};

template <class scopedPadder>
class locationFormatter final : public flagFormatter {
   public:
    explicit locationFormatter(paddingInfo padinfo, uint8_t mode = 0b110)
        : flagFormatter(padinfo), m_mode(mode) {}
    void format(const logmsg &msg, formatterBuf &dest) override {
        // scopedPadder
        std::string buf;
        if (msg.m_loc.line() == 0) return;
        if (0b100 & m_mode) {
            std::string filename{msg.m_loc.file_name()};
            filename = filename.substr(filename.rfind('/') + 1);
            buf.append('[' + filename + ']');
        }
        if (0b010 & m_mode)
            buf.append('[' + std::to_string(msg.m_loc.line()) + ']');
        if (0b001 & m_mode) buf.append(msg.m_loc.function_name());

        scopedPadder _(buf.size(), m_padInfo, dest);
        dest.append(buf);
    }

   private:
    uint8_t m_mode;
};
template <class scopedPadder>
class msgFormatter final : public flagFormatter {
   public:
    explicit msgFormatter(paddingInfo padinfo) : flagFormatter(padinfo) {}
    void format(const logmsg &msg, formatterBuf &dest) override {
        scopedPadder _(msg.m_payload.size(), m_padInfo, dest);
        dest.append(msg.m_payload);
    }
};
template <class scopedPadder>
class strFormatter final : public flagFormatter {
   public:
    explicit strFormatter(paddingInfo padinfo, std::string str)
        : flagFormatter(padinfo), m_str(str) {}
    void add_ch(char ch) { m_str += ch; }
    void format(const logmsg &msg, formatterBuf &dest) override {
        scopedPadder _(m_str.size(), m_padInfo, dest);
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
    explicit colorStopFormatter(paddingInfo padinfo) : flagFormatter(padinfo) {}

    void format(const logmsg &msg, formatterBuf &dest) override {
        msg.m_colorRangeStop = dest.size();
    }
};

std::unique_ptr<formatter> patternFormatter::clone() const {
    patternFormatter::customFlags clonedCustomFormatters;
    for (auto&[key,value] : m_customHandlers) {
        clonedCustomFormatters[key] = value->clone();
    }
    return std::make_unique<patternFormatter>(m_pattern,std::move(clonedCustomFormatters));
}

patternFormatter::patternFormatter(std::string pattern,
                                   customFlags custom_user_flags)
    : m_pattern(pattern == "" ? "  " : pattern),
      m_customHandlers(std::move(custom_user_flags)) {
        compilePattern(m_pattern);
      }
patternFormatter::patternFormatter() { patternFormatter(""); }

void patternFormatter::format(const logmsg &msg, formatterBuf &dest) {
    for (const auto &fmt : m_formatters) {
        fmt->format(msg, dest);
    }
}

template <class Padder, class...Args>
void patternFormatter::handleFlag(char flag, details::paddingInfo padding, Args...args) {
    auto it = m_customHandlers.find(flag);  // 自定义的  需要实现clone方法
    if (it != m_customHandlers.end()) {
        auto custom_handler = it->second->clone();
        custom_handler->setPaddingInfo(padding);
        m_formatters.push_back(std::move(custom_handler));
        return;
    }

 switch (flag) {
        case ('n'):  // logger name
            m_formatters.push_back(std::make_unique<details::nameFormatter<Padder>>(padding));
            break;
        case ('l'):  // level
            m_formatters.push_back(std::make_unique<details::levelFormatter<Padder>>(padding));
            break;
        case ('t'):  // thread id
            m_formatters.push_back(std::make_unique<details::threadIdFormatter<Padder>>(padding));
            break;
        case ('v'):  // the message text
            m_formatters.push_back(std::make_unique<details::msgFormatter<Padder>>(padding));
            break;
        case ('m'):  // milliseconds
            m_formatters.push_back(std::make_unique<details::millisecondsFormatter<Padder>>(padding));
            break;
        case ('e'):  // seconds since epoch
            m_formatters.push_back(std::make_unique<details::timeElapsedFormatter<Padder>>(padding,std::forward<Args>(args)...));
            break;
        case ('d'):  // time
            m_formatters.push_back(std::make_unique<details::timeFormatter<Padder>>(padding,std::forward<Args>(args)...));
            break;
        case ('^'):  // color range start
            m_formatters.push_back(std::make_unique<details::colorStartFormatter>(padding));
            break;
        case ('$'):  // color range end
            m_formatters.push_back(std::make_unique<details::colorStopFormatter>(padding));
            break;
        case ('s'):  // source location (filename:filenumber)
            m_formatters.push_back(std::make_unique<details::locationFormatter<Padder>>(padding,std::forward<Args>(args)...));
            break;
        default:  
            m_formatters.push_back(std::make_unique<details::strFormatter<Padder>>(padding,"unkonwn formatter"));
            break;
    }

}

void patternFormatter::compilePattern(const std::string &pattern){
    auto end = pattern.end();
    std::unique_ptr<details::strFormatter<nullScopedPadder>> user_chars;
    m_formatters.clear();
    for (auto it = pattern.begin(); it != end; ++it) {
        if (*it == '%') {
            if (user_chars)  // append user chars found so far
            {
                m_formatters.push_back(std::move(user_chars));
            }

            auto padding = handlePadspec(++it, end);

            if (it != end) {
                if (padding.enabled()) {
                    handleFlag<details::scopedPadder>(*it, padding);
                } else {
                    handleFlag<details::nullScopedPadder>(*it, padding);
                }
            } else {
                break;
            }
        } else  // chars not following the % sign should be displayed as is
        {
            if (!user_chars) {
                user_chars = std::make_unique<details::strFormatter<nullScopedPadder>>(paddingInfo(),"");
            }
            user_chars->add_ch(*it);
        }
    }
    if (user_chars)  // append raw chars found so far
    {
        m_formatters.push_back(std::move(user_chars));
    }


}


details::paddingInfo patternFormatter::handlePadspec(std::string::const_iterator &it, std::string::const_iterator end)
{
    using details::paddingInfo;
    using details::scopedPadder;
    const size_t max_width = 64;
    if (it == end) {
        return paddingInfo{};
    }

    paddingInfo::padSide side;
    switch (*it) {
        case '<':
            side = paddingInfo::padSide::left;
            ++it;
            break;
        case '^':
            side = paddingInfo::padSide::center;
            ++it;
            break;
        case '>':
            side = paddingInfo::padSide::right;
            ++it;
            break;
        default:
            side = paddingInfo::padSide::center;
            break;
    }

    if (it == end || !std::isdigit(static_cast<unsigned char>(*it))) {
        return paddingInfo{};  // no padding if no digit found here
    }

    auto width = static_cast<size_t>(*it) - '0';
    for (++it; it != end && std::isdigit(static_cast<unsigned char>(*it)); ++it) {
        auto digit = static_cast<size_t>(*it) - '0';
        width = width * 10 + digit;
    }

    // search for the optional truncate marker '!'
    bool truncate;
    if (it != end && *it == '!') {
        truncate = true;
        ++it;
    } else {
        truncate = false;
    }
    return details::paddingInfo{std::min<size_t>(width, max_width), side, truncate};
}



}  // namespace details




// #include <iostream>
// using namespace details;

// int main() {
//     patternFormatter pattern{"%d%m %^20!s [%t] [%l] [%n] [%e] [%v]"};
//     logmsg msg{
//         .m_level = astlog::LEVEL::FATAL,
//         .m_loc = std::source_location::current(),
//         .m_payload = "hello world",
//     };
//     std::this_thread::sleep_for(std::chrono::seconds(2));
//     formatterBuf buf;
//     pattern.format(msg,buf);
//     std::cout << buf.data()<<'\n';

//     return 0;
// }
