#ifndef ASTLOG_CIRCLEBUFFER_H
#define ASTLOG_CIRCLEBUFFER_H

#include <cstddef>
#include <array>
#include <atomic>
#include <stdexcept>



namespace details{
template <class DataType,size_t Size = 128>
class circleBuffer{
public:
    circleBuffer(){}
    
    void append(DataType &data){
        auto index = m_head.exchange(m_head+1) % Size;
        m_data[index] = std::move(data);
    }
    
    void pop(){
        auto index = m_tail.exchange(m_tail+1) % Size;
        return m_data[index];
    }



    template<class T>
    requires std::convertible_to<T,size_t>
    const DataType& operator[](T t){
        if(t < Size)
            return m_data[t];
        else 
            throw std::runtime_error("access out of range");
    }   
private:
    std::atomic<size_t> m_head{0};
    std::atomic<size_t> m_tail{0};

    std::array<DataType,Size> m_data;
};

}


#endif