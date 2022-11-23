#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <thread>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <future>
#include <functional>
#include <stdexcept>

#include "common.h"
//#include "baidu/streaming_log.h"

#include "concurrent_bounded_queue.h"
#include "move_only_function.h"

namespace gflow::concurrent {

class ThreadPool {
private:
    static constexpr size_t DEFAULT_WORKER_NUM = 30;
    static constexpr size_t DEFAULT_QUEUE_CAPACITY = 300000;
public:
    explicit ThreadPool(uint32_t, uint32_t) noexcept;

    ThreadPool(ThreadPool const&) = delete;             // Copy construct
    ThreadPool(ThreadPool&&) = delete;                  // Move construct
    ThreadPool& operator=(ThreadPool const&) = delete;  // Copy assign
    ThreadPool& operator=(ThreadPool &&) = delete;      // Move assign

    static ThreadPool& instance() {
        static ThreadPool ins(DEFAULT_WORKER_NUM, DEFAULT_QUEUE_CAPACITY);
        return ins;
    }

    ~ThreadPool() noexcept;

    template<class F, class... Args>
    auto enqueue(F&& f, Args&&... args) noexcept -> std::future<typename std::result_of_t<F(Args...)>>;
    int32_t enqueue(MoveOnlyFunction<void(void)>&& function) noexcept;
    size_t task_size() noexcept;
    void wait_task_finish() noexcept;
    void stop_and_wait() noexcept;
private:
    std::vector< std::thread > workers;
    std::atomic<bool> is_stopped{false};
    ConcurrentBoundedQueue<MoveOnlyFunction<void(void)>> tasks;
};


inline size_t ThreadPool::task_size() noexcept
{
    return tasks.size();
}
 
inline ThreadPool::ThreadPool(uint32_t threads, uint32_t queue_size) noexcept
{
    constexpr size_t batch_num = 10;
    using IT = ConcurrentBoundedQueue<std::function<void(void)>>::Iterator;
    tasks.reserve_and_clear(queue_size);
    for(uint32_t i = 0; i < threads; ++i) {
        workers.emplace_back([this] {
            while (true) {
                MoveOnlyFunction<void(void)> task;
                this->tasks.pop(task);
                if (task) { // task表示function是否可以调用, task->bool();
                    task();
                } else {
                    // 收到空任务的标记，优雅退出，所有任务处理完才退出
                    break;
                }
            }
        });
    }
}

template<class F, class... Args>
inline auto ThreadPool::enqueue(F&& f, Args&&... args) noexcept
        -> std::future<typename std::result_of_t<F(Args...)>>
{
    using return_type = typename std::result_of_t<
            typename ::std::decay<F>::type(
                typename ::std::decay<Args>::type...)>;

    std::promise<return_type> promise;
    auto future = promise.get_future();

    using RawFunc = typename std::decay<F>::type;
    // 增加一层传引用的代理函数，它负责把binder对象保存的参数值掏空，move到func参数中
    auto proxy_callback = [](std::promise<return_type>& promise, RawFunc& func, typename std::decay<Args>::type&... args) {
        if constexpr (std::is_member_function_pointer_v<F>) {
            if constexpr (std::is_same_v<void, return_type>) {
                std::mem_fn(func)(std::move(args)...);
                promise.set_value();
            } else {
                promise.set_value(std::mem_fn(func)(std::move(args)...));
            }
        } else {
            if constexpr (std::is_same_v<void, return_type>) {
                func(std::move(args)...);
                promise.set_value();
            } else {
                promise.set_value(func(std::move(args)...));
            }
        }
    };

    auto binder = std::bind(proxy_callback, std::move(promise), std::forward<F>(f), std::forward<Args>(args)...);

    // 不能使用std::function，因为binder对象可能是不可拷贝的，例如参数里面有unique_ptr。
    // std::function要求可调用对象是可拷贝的，否则编译出错，就算使用std::move(binder)也会出错
    // std::function<void(void)> function(std::move(binder));  // ERROR
    MoveOnlyFunction<void(void)> function(std::move(binder));

    tasks.push(std::move(function));
    return future;
}

inline int32_t ThreadPool::enqueue(MoveOnlyFunction<void(void)>&& function) noexcept 
{
    tasks.push(std::move(function));
    return 0;
}


inline void ThreadPool::stop_and_wait() noexcept
{
    bool expected = false;
    if (!is_stopped.compare_exchange_strong(expected, 
                                            true, 
                                            std::memory_order_seq_cst)) {
        return;
    }
    LOG(NOTICE) << "ThreadPool is going to stop...left task_size:" << tasks.size();

    // 发送n个空任务来标记退出
    for (uint32_t i = 0; i < workers.size(); ++i) {
        enqueue({});
    }
    for(std::thread& worker: workers) {
        if (worker.joinable()) {
            worker.join();
        }
    }
    
    LOG(NOTICE) << "ThreadPool stopped, left task_size:" << tasks.size();
}

inline ThreadPool::~ThreadPool() noexcept
{
    LOG(NOTICE) << "~ThreadPool";
    stop_and_wait();
}

// 使用线程池执行异步任务, 语义更明确的异步语法糖
template <typename F, typename... Args>
auto thread_pool_async(F &&f, Args &&...args) -> std::future<typename std::result_of_t<F(Args...)>> {
    return ThreadPool::instance().enqueue(std::forward<F>(f), std::forward<Args>(args)...);
}

} // namespace