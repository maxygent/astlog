#include <chrono>
// #include <cstddef>
#include <iostream>
#include "log_line.h"
#include "thread_pool.h"
int main(){
  
  auto task = []{
    DEBUG("hello thread");
  };
  std::chrono::nanoseconds duration1;
  std::chrono::nanoseconds duration2;
  {
    auto start = std::chrono::system_clock::now();  
    {
      for(size_t i = 0;i<10000;i++)
      {
        details::threadPool<10>::getInstance().appendTask(task);
      }
    }
    duration1 = std::chrono::system_clock::now() - start;
  }
  {
    auto start = std::chrono::system_clock::now();  
    {
      for(size_t i = 0;i<10000;i++)
      {
        task();
      }
    }
    duration2 = std::chrono::system_clock::now() - start;
  }
  std::cout << "async:" << duration1 << "\n";
  std::cout << "sync:" << duration2 << "\n";
}



