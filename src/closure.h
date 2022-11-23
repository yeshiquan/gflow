#pragma once

#include <tuple>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>

#include "count_down_latch.h"
#include "common.h"

// #include <bthread.h>
// #include <bthread/mutex.h>
// #include <bthread/condition_variable.h>

namespace gflow {

class ClosureContext {
   public:
    // 禁止拷贝和移动
    inline ClosureContext(ClosureContext &&) = delete;
    inline ClosureContext(const ClosureContext &) = delete;
    inline ClosureContext &operator=(ClosureContext &&) = delete;
    inline ClosureContext &operator=(const ClosureContext &) = delete;

    ClosureContext() { 
        LOG(TRACE) << std::this_thread::get_id() << " Constructor ClosureContext()"; 
	}

    int32_t wait_finish() {
        latch.await();
        return _error_code;
    }

    void mark_finish(int32_t error_code) {
        bool expected = false;
        if (_is_mark_finished.compare_exchange_strong(
                expected, true, std::memory_order_acq_rel)) {
            _error_code = error_code;
            LOG(TRACE) << std::this_thread::get_id() << " mark_finish success error_code:" << error_code;
            latch.notify_all();
        } else {
            LOG(TRACE) << "mark_finish failed error_code:" << error_code;
        }
    }

    bool is_mark_finished() {
        return _is_mark_finished.load(std::memory_order_relaxed);
    }

    void one_vertex_finished() {
        latch.count_down();
    }

    void add_wait_vertex_num(size_t num) {
        latch.add_count(num);
    }

    ~ClosureContext() { 
		LOG(TRACE) << this << " Destructor ~ClosureContext()"; 
		wait_finish();
	}

	std::atomic<bool> is_delete{false};
   private:
#ifdef USE_BTHREAD
    CountDownLatch<bthread::Mutex, bthread::ConditionVariable> latch;
#else
    CountDownLatch<std::mutex, std::condition_variable> latch;
#endif
    int32_t _error_code = 0;
    std::atomic<bool> _is_mark_finished{false};
};

}  // namespace gflow
