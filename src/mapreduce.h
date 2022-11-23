#pragma once

#include <string>
#include <memory>
#include <deque>
#include <mutex>
#include <iostream>
#include <condition_variable>
//#include <base/logging.h>

#include "concurrent/thread_pool.h"

// #include <baidu/feed/mlarch/babylon/bthread_executor.h>
// #include <baidu/feed/mlarch/babylon/executor.h>
// using ::baidu::feed::mlarch::babylon::Executor;
// using ::baidu::feed::mlarch::babylon::Future;
// using ::baidu::feed::mlarch::babylon::BthreadExecutor;
// using baidu::feed::mlarch::babylon::bthread_async;

using gflow::concurrent::thread_pool_async;

namespace gflow {

template<typename T>
using Batch = std::vector<T>;

template<typename T>
using BatchPtr = std::unique_ptr<Batch<T>>;

template<typename InputType, typename OutputType,
                template<typename Elem> class CHANNEL>
class MapReducer {
public:
    using InputBatch = Batch<InputType>;
    using OutputBatch = Batch<OutputType>;
    using InputBatchPtr = BatchPtr<InputType>;
    using OutputBatchPtr = BatchPtr<OutputType>;
public:
    MapReducer() = default;
    MapReducer(MapReducer const&) = delete;             // Copy construct
    MapReducer(MapReducer&&) = delete;                  // Move construct
    MapReducer& operator=(MapReducer const&) = delete;  // Copy assign
    MapReducer& operator=(MapReducer &&) = delete;      // Move assign

    void run() {
        if (_parallel_map_callback) {
#ifdef USE_BTHREAD
            std::vector<Future<OutputBatchPtr, bthread::Mutex>> futures;
#else
            std::vector<std::future<OutputBatchPtr>> futures;
#endif
            while (true) {
                InputBatchPtr batch = std::make_unique<InputBatch>(_batch_size);
                size_t num = _channel->pop_n(batch->begin(), batch->end());
                if (num > 0) {
#ifdef USE_BTHREAD
                    auto future = bthread_async(_parallel_map_callback, std::move(batch));
#else
                    auto future = thread_pool_async(_parallel_map_callback, std::move(batch));
#endif
                    futures.emplace_back(std::move(future));
                }
                if (num < _batch_size) {
                    break;
                }
            }
            for (auto& future : futures) {
                if (future.valid()) {
                    OutputBatchPtr result = std::move(future.get());
                    if (_reduce_callback) {
                        _reduce_callback(std::move(result));
                    }
                }
            }
        } else if (_map_callback) {
            while (true) {
                InputBatchPtr batch = std::make_unique<InputBatch>(_batch_size);
                size_t num = _channel->pop_n(batch->begin(), batch->end());
                if (num > 0) {
                    OutputBatchPtr result = _map_callback(std::move(batch));
                    if (_reduce_callback) {
                        _reduce_callback(std::move(result));                    
                    }
                }
                if (num < _batch_size) {
                    break;
                }
            }
                   
        }
    }

    OutputBatchPtr make_output_batch() {
        return std::make_unique<OutputBatch>();
    }

    template<typename C>
    MapReducer& parallel_map(C&& callback) { 
        _parallel_map_callback = callback; 
        return *this;
    }

    template<typename C>
    MapReducer& map(C&& callback) { 
        _map_callback = callback; 
        return *this;
    }    

    template<typename C>
    MapReducer& reduce(C&& callback) { 
        _reduce_callback = callback; 
        return *this;
    }

    MapReducer& batch_size(size_t batch_size) { 
        _batch_size = batch_size; 
        return *this;
    }
    MapReducer& input(CHANNEL<InputType>& queue) { 
        _channel = &queue; 
        return *this;
    }
private:
    size_t _batch_size = 0;
    CHANNEL<InputType>* _channel = nullptr;
    std::function<OutputBatchPtr(InputBatchPtr)> _parallel_map_callback;
    std::function<OutputBatchPtr(InputBatchPtr)> _map_callback;
    std::function<void(OutputBatchPtr)> _reduce_callback;
};

} // namespace