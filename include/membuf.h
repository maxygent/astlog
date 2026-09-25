#ifndef __ASTLOG_MEMBUF_H
#define __ASTLOG_MEMBUF_H
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <streambuf>
#include <string_view>
#include <vector>





namespace details{

template <size_t INLINE_CAPACITY>
class inlineBuffer
{
public:
    void append(const char* data, size_t size)
    {
        if(size == 0)
        {
            return;
        }
        if(m_overflow.empty() && m_size + size <= INLINE_CAPACITY)
        {
            std::memcpy(m_inline.data() + m_size, data, size);
            m_size += size;
            return;
        }
        if(m_overflow.empty())
        {
            const size_t required_capacity = m_size + size;
            m_overflow.reserve(std::max(INLINE_CAPACITY * 2, required_capacity));
            m_overflow.insert(m_overflow.end(), m_inline.data(), m_inline.data() + m_size);
        }
        m_overflow.insert(m_overflow.end(), data, data + size);
        m_size = m_overflow.size();
    }
    
    void append(std::string_view value)
    {
        append(value.data(), value.size());
    }
    
    void append(char value)
    {
        append(&value, 1);
    }
    void erase(size_t size)
    {
        if(size >= 0){
            m_size = size;
            if(m_overflow.empty())
                m_inline[m_size] = '\0';
            else
            {
                m_overflow.resize(m_size);
                m_overflow[m_size] = '\0';
            }
        }
    }
    [[nodiscard]] const char* data() const noexcept
    {
        return m_overflow.empty() ? m_inline.data() : m_overflow.data();
    }
    
    [[nodiscard]] size_t size() const noexcept
    {
        return m_size;
    }
    
    [[nodiscard]] std::string_view view() const noexcept
    {
        return {data(), size()};
    }

private:
    std::array<char, INLINE_CAPACITY> m_inline{};
    std::vector<char> m_overflow;
    size_t m_size = 0;
};

// 继承std::streambuf是为了能够能够继续使用流式输入接口，兼容ostream生态
template <size_t INLINE_CAPACITY>
class smallStreamBuffer final : public std::streambuf
{
public:
    explicit smallStreamBuffer(inlineBuffer<INLINE_CAPACITY>& buffer) noexcept
        : m_buffer(buffer){}

protected:
    std::streamsize xsputn(const char* data, std::streamsize size) override
    {
        if(size <= 0)
        {
            return 0;
        }
        m_buffer.append(data, static_cast<size_t>(size));
        return size;
    }

    int_type overflow(int_type character) override
    {
        // overflow，除了可以接受一般字符，还可能会接收流终止符EOF
        // 如果我们遇到了EOF, 把它转成非EOF值向上报告写入成功，实际上不向Buffer里写入任何值。
        if(traits_type::eq_int_type(character, traits_type::eof()))
        {
            return traits_type::not_eof(character);
        }

        m_buffer.append(traits_type::to_char_type(character));
        return character;
    }
    
private:
    inlineBuffer<INLINE_CAPACITY>& m_buffer;
};

inline constexpr size_t FORMATTED_RECORD_INLINE_CAPACITY = 256;

inline constexpr size_t LOG_MESSAGE_INLINE_CAPACITY = 128;

inline constexpr size_t PRINTF_FORMAT_INLINE_CAPACITY = 128;

using formatterBuf = inlineBuffer<FORMATTED_RECORD_INLINE_CAPACITY>;
using logBuf = inlineBuffer<LOG_MESSAGE_INLINE_CAPACITY>;
using printfBuf = inlineBuffer<PRINTF_FORMAT_INLINE_CAPACITY>;

}
#endif