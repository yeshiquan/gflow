#pragma once

#include <tuple>
#include <bthread.h>
#include <bthread/mutex.h>
#include <bthread/condition_variable.h>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <future>

#include "count_down_latch.h"

namespace gflow {

class ClosureContext {
   public:
    // 禁止拷贝和移动
    inline ClosureContext(ClosureContext &&) = delete;
    inline ClosureContext(const ClosureContext &) = delete;
    inline ClosureContext &operator=(ClosureContext &&) = delete;
    inline ClosureContext &operator=(const ClosureContext &) = delete;

    ClosureContext(int32_t executor_type_) { 
        LOG(TRACE) << std::this_thread::get_id() << " Constructor ClosureContext()"; 
        executor_type = executor_type_;
	}

    int32_t wait_finish() {
        if (executor_type == 1) {
            latch1.await();
        } else {
            latch2.await();
        }
        return _error_code;
    }

    void mark_finish(int32_t error_code) {
        bool expected = false;
        if (_is_mark_finished.compare_exchange_strong(
                expected, true, std::memory_order_acq_rel)) {
            _error_code = error_code;
            LOG(TRACE) << std::this_thread::get_id() << " mark_finish success error_code:" << error_code;
            if (executor_type == 1) {
                latch1.notify_all();
            } else {
                latch2.notify_all();
            }
        } else {
            LOG(TRACE) << "mark_finish failed error_code:" << error_code;
        }
    }

    bool is_mark_finished() {
        return _is_mark_finished.load(std::memory_order_relaxed);
    }

    void one_vertex_finished() {
        if (executor_type == 1) {
            latch1.count_down();
        } else {
            latch2.count_down();
        }
    }

    void add_wait_vertex_num(size_t num) {
        if (executor_type == 1) {
            latch1.add_count(num);
        } else {
            latch2.add_count(num);
        }
    }

    ~ClosureContext() { 
		LOG(TRACE) << this << " Destructor ~ClosureContext()"; 
		wait_finish();
	}

	std::atomic<bool> is_delete{false};
    int32_t executor_type = 1;
   private:
    CountDownLatch<bthread::Mutex, bthread::ConditionVariable> latch1;
    CountDownLatch<std::mutex, std::condition_variable> latch2;
    int32_t _error_code = 0;
    std::atomic<bool> _is_mark_finished{false};
};

}  // namespace gflow
