#pragma once

#include <string>
#include <memory>
#include <deque>
#include <mutex>
#include <iostream>
#include <condition_variable>
// #include <bthread/mutex.h>
// #include <bthread/condition_variable.h>
//#include <base/logging.h>

// 一种简单的管道实现，线程安全，使用mutex同步，适用于并发量不大的场景
// 支持close操作, close之后，阻塞的操作会立马返回，从而实现channel的效果

namespace gflow {

template<typename T>
class Channel {
public:
    Channel() {}
    Channel(Channel const&) = delete;             // Copy construct
    Channel(Channel&&) = delete;                  // Move construct
    Channel& operator=(Channel const&) = delete;  // Copy assign
    Channel& operator=(Channel &&) = delete;      // Move assign

    void close() noexcept {
        std::unique_lock lk(_mutex);
        if (!_is_closed) {
            _is_closed = true;
            _cv.notify_all();
        }
    }

    // 迭代器[begin, end]是数据源，把这些数据源批量插入到队列尾部
    template<typename IT>
    void push_n(IT begin, IT end) noexcept {
        std::unique_lock lk(_mutex);
        if (_is_closed) {
            return;
        }        
        for (auto it = begin; it != end; ++it) {
            _queue.push_back(*it);
        }
        _cv.notify_one();
    }

    // 迭代器[begin, end]是目的地，假如目的地的迭代器的大小是n
    // 那么pop_n会把队列头部前n个元素弹出然后拷贝到目的地迭代器，如果队列数据不够n个，有两种情况
    // 1) 队列没有关闭，操作会阻塞一直等到元素达到n个。
    // 2) 队列关闭，操作立即返回，弹出元素的个数少于n个。
    // return: 弹出元素的个数
    template<typename IT>
    size_t pop_n(IT begin, IT end) noexcept {
        size_t poped_num = 0;
        auto num = std::distance(begin, end);
        std::unique_lock lk(_mutex);

        // _cv.wait(lk, [this, num]{ 
        //     return this->_queue.size() >= num || _is_closed;
        // });
        while (_queue.size() < num && !_is_closed) {
            _cv.wait(lk);
        }

        for (auto iter = begin; iter != end; ++iter) {
            if (_queue.size() == 0) {
                break;
            }
            *iter = _queue.front();
            _queue.pop_front();
            poped_num++;
        }
        return poped_num;
    }

    void push(const T& value) noexcept {
        std::unique_lock lk(_mutex);
        if (_is_closed) {
            return;
        }
        _queue.push_back(value);
        _cv.notify_one();
    }

    bool pop(T& value) noexcept {
        std::unique_lock lk(_mutex);

        // _cv.wait(lk, [this]{ 
        //     return this->_queue.size() > 0 || _is_closed;
        // });
        while (_queue.size() == 0 && !_is_closed) {
            _cv.wait(lk);
        }

        if (_queue.size() > 0) {
            value = _queue.front();
            _queue.pop_front();
            return true;
        }
        return false;
    }

    void clear() noexcept {
        std::unique_lock lk(_mutex);
        _is_closed = false;
        _queue.clear();
    }

    size_t size() const noexcept { return _queue.size(); }
private:
    std::deque<T> _queue;    
#ifdef USE_BTHREAD
    bthread::Mutex _mutex;
    bthread::ConditionVariable _cv;
#else
    std::mutex _mutex;
    std::condition_variable _cv;
#endif
    bool _is_closed = false;
};

} // namespace
