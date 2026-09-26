#ifndef __ASTLOG_PATTERN_FORMATTER_H
#define __ASTLOG_PATTERN_FORMATTER_H
#include <cstddef>
#include <unordered_map>

#include "common.h"
#include "membuf.h"
#include "logmsg.h"
#include "loglevel.h"

namespace details{




struct paddingInfo{
    enum class padSide { left, right, center };

    paddingInfo() = default;
    paddingInfo(size_t width, paddingInfo::padSide side, bool truncate)
        : m_width(width),
          m_side(side),
          m_truncate(truncate),
          m_enabled(true) {}

    bool enabled() const { return m_enabled; }
    size_t m_width = 0;   // padding width
    padSide m_side = padSide::left;  // padding side
    bool m_truncate = false;    // 超出截断
    bool m_enabled = false;    
};

class formatter {
public:
    virtual ~formatter() = default;
    virtual void format(const logmsg &msg, formatterBuf &dest) = 0;
    virtual std::unique_ptr<formatter> clone() const = 0;
};


class  flagFormatter {
public:
    explicit flagFormatter(paddingInfo padInfo):m_padInfo(padInfo){}
    flagFormatter() = default;
    virtual ~flagFormatter() = default;
    virtual void format(const details::logmsg &msg, formatterBuf &dest) = 0;

protected:
    paddingInfo m_padInfo;
};


class  customFormatter : public flagFormatter {
public:
    virtual std::unique_ptr<customFormatter> clone() const = 0;

    void setPaddingInfo(const paddingInfo &padding) {
        flagFormatter::m_padInfo = padding;
    }
};

class patternFormatter final : public formatter {
public:
    using customFlags = std::unordered_map<char, std::unique_ptr<customFormatter>>;

    explicit patternFormatter(std::string pattern,
                               customFlags custom_user_flags = customFlags());                              

    // use default pattern is not given
    explicit patternFormatter();

    patternFormatter(const patternFormatter &other) = delete;
    patternFormatter &operator=(const patternFormatter &other) = delete;

    patternFormatter(patternFormatter &&) = default;
    patternFormatter &operator=(patternFormatter &&) = default;

    std::unique_ptr<formatter> clone() const override;
    void format(const logmsg &msg, formatterBuf &dest) override;

    template <typename T, typename... Args>
    patternFormatter &add_flag(char flag, Args &&...args) {
        m_customHandlers[flag] = std::make_unique<T>(std::forward<Args>(args)...);
        return *this;
    }
    void set_pattern(std::string pattern);
    void need_localtime(bool need = true);

private:
    std::string m_pattern;

    bool need_localtime_;
    // std::chrono::seconds last_log_secs_;
    std::vector<std::unique_ptr<flagFormatter>> m_formatters;
    customFlags m_customHandlers;

    std::tm getTime(const logmsg &msg) const;
    // template <typename Padder>
    // void handleFlag(char flag, details::paddingInfo padding);
    template <class Padder, class...Args>
    void handleFlag(char flag, details::paddingInfo padding, Args... args);

    // Extract given pad spec (e.g. %8X)
    // Advance the given it pass the end of the padding spec found (if any)
    // Return padding.
    static details::paddingInfo handlePadspec(std::string::const_iterator &it,
                                              std::string::const_iterator end);

    void compilePattern(const std::string &pattern);
};









}  // namespace details







#endif