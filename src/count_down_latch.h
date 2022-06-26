#pragma once

#include <mutex>

namespace gflow {

template<typename M, typename C>
class CountDownLatch {
public:
    CountDownLatch() = default;
    CountDownLatch(uint32_t count);
    void await();
    void count_down();
    uint32_t get_count(); 
    void add_count(uint32_t count);
    void notify_all();

private:
  C cv;
  M lock;
  uint32_t count = 0;
  
  CountDownLatch(const CountDownLatch& other) = delete;
  CountDownLatch& operator=(const CountDownLatch& opther) = delete;
};

} // namespace

#include "count_down_latch.hpp"
