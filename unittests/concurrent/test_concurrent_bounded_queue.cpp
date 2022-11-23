#include "gtest/gtest.h"
#include "gflags/gflags.h"
#include <thread>
#include <chrono>
#include <random>
#include <vector>
#include <mutex>
#include <bitset>

#include <assert.h>
#include "baidu/streaming_log.h"
#include "base/comlog_sink.h"
#include "base/strings/stringprintf.h"
#include "com_log.h"
#include "cronoapd.h"

#define  DCHECK_IS_ON 1

#define private public
#define protected public
#include "concurrent/concurrent_bounded_queue.h"
#undef private
#undef protected

using gflow::concurrent::ConcurrentBoundedQueue;
using gflow::concurrent::ConcurrentBoundedQueueOption;
using gflow::concurrent::FutexInterface;

int intRand(const int & min, const int & max) {
    static thread_local std::mt19937 generator;
    std::uniform_int_distribution<int> distribution(min,max);
    return distribution(generator);
}

size_t get_steady_clock_millisecond() {
	return std::chrono::time_point_cast<std::chrono::milliseconds>(
    	std::chrono::steady_clock::now()).time_since_epoch().count();
}

class ConcurrentBoundedQueueTest : public ::testing::Test {
private:
    virtual void SetUp() {
        //p_obj.reset(new ExprBuilder);
    }
    virtual void TearDown() {
        //if (p_obj != nullptr) p_obj.reset();
    }
protected:
    //std::shared_ptr<ExprBuilder>  p_obj;
};

TEST_F(ConcurrentBoundedQueueTest, test_case1) {
    using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    Queue vec(8);
    for (size_t index = 0; index < 24; ++index) {
        uint16_t push_expected_version = vec.push_version_for_index(index);
        uint16_t pop_expected_version = vec.pop_version_for_index(index);
        std::cout << "index:" << index
                  << " push_expected_version:" << push_expected_version
                  << " pop_expected_version:" << pop_expected_version
                  << " round_num:" << (index >> vec._round_bits)
                  << " slot_id:" << (index & (vec.capacity() - 1))
                  << std::endl;
    }

    for (uint32_t current_version = 1; current_version < 21; ++current_version) {
        uint32_t tagged_version = gflow::concurrent::make_tagged_version(current_version);
        uint16_t raw_version = gflow::concurrent::get_raw_version(tagged_version);
        ASSERT_EQ(raw_version, current_version);
        bool ret1 = gflow::concurrent::is_tagged_version(current_version);
        bool ret2 = gflow::concurrent::is_tagged_version(tagged_version);
        ASSERT_EQ(ret1, false);
        ASSERT_EQ(ret2, true);
        std::cout << "current_version:" << current_version << " " << std::bitset<32>(current_version)
                  << "\ttagged_version:" << tagged_version << " " << std::bitset<32>(tagged_version)
                  << "\traw_version:" << raw_version << " " << std::bitset<16>(raw_version)
                  << "\tis_tagged_version(current_version)=" << ret1
                  << "\tis_tagged_version(tagged_version)=" << ret2
                  << std::endl;
    }
}

TEST_F(ConcurrentBoundedQueueTest, test_basic) {
    //using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    Queue vec(53);

    std::cout << "test_basic" << std::endl;
    for (int i = 0; i < 10; ++i) {
        vec.push(i);
    }
    ASSERT_EQ(vec.size(), 10);

    for (int i = 0; i < 5; ++i) {
        int v;
        vec.pop(v);
        ASSERT_EQ(v, i);
    }
    ASSERT_EQ(vec.size(), 5);

    for (int i = 10; i < 20; ++i) {
        vec.push(i);
    }
    ASSERT_EQ(vec.size(), 15);

    for (int i = 5; i < 20; ++i) {
        int v;
        vec.pop(v);
        ASSERT_EQ(v, i);
    }
    ASSERT_EQ(vec.size(), 0);

    vec.push([](int& value) {
        value = 111;
    });
    vec.pop([](int& value) {
        ASSERT_EQ(value, 111);
    });
}

TEST_F(ConcurrentBoundedQueueTest, test_push_n_case1) {
    using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    Queue vec(8);
    // 测试push_n, index没有超过单轮
    vec.clear();
    std::vector<int> src;
    for (int i = 0; i < 5; ++i) {
        src.push_back(i);
    }
    vec.push_n(src.begin(), src.end());
    ASSERT_EQ(vec.size(), 5);
    for (int i = 0; i < 5; ++i) {
        vec.pop([i](int& value) { ASSERT_EQ(value, i); });
    }
}

TEST_F(ConcurrentBoundedQueueTest, test_push_n_case2) {
    using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    std::vector<int> src;
    Queue vec(8);
    // 测试push_n，index跨越了2个轮次
    ASSERT_EQ(vec.size(), 0);
    for (int i = 0; i < 5; ++i) {
        vec.push(-1);
        vec.pop([i](int& value) { ASSERT_EQ(value, -1); });
    }

    src.clear();
    for (int i = 0; i < 5; ++i) {
        src.push_back(i+1);
    }

    ASSERT_EQ(vec.size(), 0);
    vec.print();
    vec.push_n(src.begin(), src.end());
    vec.print();
    for (int i = 0; i < 5; ++i) {
        vec.pop([i](int& value) { 
            ASSERT_EQ(value, i+1); 
        });
    }
    ASSERT_EQ(vec.size(), 0);
}

TEST_F(ConcurrentBoundedQueueTest, test_push_n_case3) {
    using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    std::vector<int> src;
    Queue vec(8);
    // 测试push_n，n超过了队列容量
    src.clear();
    vec.clear();
    for (int i = 0; i < 20; ++i) {
        src.push_back(i);
    }
    vec.push_n(src.begin(), src.end());
    ASSERT_EQ(vec.size(), vec.capacity());
}

TEST_F(ConcurrentBoundedQueueTest, test_Iterator) {
    using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    using SlotVector = Queue::SlotVector;
    using Iterator = Queue::Iterator;

    SlotVector slot_vec;
    slot_vec.init(8);
    int data = 0;
    for (auto& item : slot_vec) {
        item = data++;
    }

    data = 0;
    for (const auto& item : slot_vec) {
        std::cout << "slot value -> " << item << std::endl;
        ASSERT_EQ(item, data++);
    }
}

TEST_F(ConcurrentBoundedQueueTest, test_pop_wait) {
    using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    std::vector<int> src;
    Queue vec(8);

    std::thread thd([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        vec.push(5);
    });
    
    uint64_t start = get_steady_clock_millisecond();
    int value;
    vec.pop(value);
    uint64_t end = get_steady_clock_millisecond();
    std::cout << "wait " << (end-start) << std::endl;
    ASSERT_TRUE( (end-start) >= 50);

    thd.join();
}

TEST_F(ConcurrentBoundedQueueTest, test_push_wait) {
    using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    std::vector<int> src;
    Queue vec(8);

    std::thread thd([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        int value;
        vec.pop(value);
    });

    std::vector<int> items1{1,2,3,4};
    vec.push_n(items1.begin(), items1.end());
    
    std::vector<int> items2{6,7,8,9,10};
    uint64_t start = get_steady_clock_millisecond();
    // 6,7,8,9顺利push后队列满了，10需要等待，直到另外一个pop一个元素出来
    vec.push_n(items2.begin(), items2.end());
    uint64_t end = get_steady_clock_millisecond();
    std::cout << "wait " << (end-start) << std::endl;
    ASSERT_TRUE( (end-start) >= 50);

    thd.join();
}

TEST_F(ConcurrentBoundedQueueTest, test_pop_n_basic) {
    using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    Queue vec(8);
    
    std::vector<int> items1{1,2,3,4};
    vec.push_n(items1.begin(), items1.end());
    ASSERT_EQ(items1.size(), vec.size());

    vec.print();
    auto callback = [&](Queue::Iterator begin, Queue::Iterator end) {
        int i = 0;
        for (auto it = begin; it != end; ++it) {
            ASSERT_EQ(items1[i++], *it);
            std::cout << "pop_n value -> " << *it << std::endl;
        }
    };
    vec.pop_n(callback, 4);
}

TEST_F(ConcurrentBoundedQueueTest, test_try_pop_n) {
    using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    Queue vec(8);
    
    std::vector<int> items1{1,2,3,4};
    vec.push_n(items1.begin(), items1.end());
    ASSERT_EQ(items1.size(), vec.size());

    vec.print();
    auto callback = [&](Queue::Iterator begin, Queue::Iterator end) {
        ASSERT_EQ(std::distance(begin, end), items1.size());
        int i = 0;
        for (auto it = begin; it != end; ++it) {
            ASSERT_EQ(items1[i++], *it);
            std::cout << "pop_n value -> " << *it << std::endl;
        }
    };
    // 想pop 7个，但是只有4个，最终只能pop 4个
    vec.try_pop_n(callback, 7);
}

TEST_F(ConcurrentBoundedQueueTest, test_try_push_n) {
    using Queue = ConcurrentBoundedQueue<int, FutexInterface, ConcurrentBoundedQueueOption::CACHELINE_ALIGNED>;
    Queue vec(8);
    
    std::vector<int> items1{1,2,3,4,5, 6};
    std::vector<int> items2{7, 8, 9, 10};
    vec.push_n(items1.begin(), items1.end());
    ASSERT_EQ(items1.size(), vec.size());
    vec.try_push_n(items2.begin(), items2.end());
    ASSERT_EQ(vec.capacity(), vec.size());

    auto callback = [&](Queue::Iterator begin, Queue::Iterator end) {
        int i = 1;
        for (auto it = begin; it != end; ++it) {
            ASSERT_EQ(i++, *it);
            std::cout << "pop value -> " << *it << std::endl;
        }
    };
    vec.try_pop_n(callback, 10);
}

TEST_F(ConcurrentBoundedQueueTest, test_stress) {
    ConcurrentBoundedQueue<int> vec(54);

    std::vector<std::thread> threads;

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&] {
            for (int k = 0; k < 100000; ++k) {
                int v = intRand(1, 1000000);
                //std::lock_guard<std::mutex> guard(mutex);
                vec.push(v);
                //std::cout << "push v:" << v << std::endl;
                //std::this_thread::sleep_for(std::chrono::milliseconds(500));
            }
        });
    }

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([&] {
            std::this_thread::sleep_for(std::chrono::milliseconds(48));
            for (int k = 0; k < 100000; ++k) {
                //std::lock_guard<std::mutex> guard(mutex);
                int value;
                vec.pop(value);
                //std::cout << "pop v:" << v << std::endl;
                //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }
} 

// int main(int argc, char* argv[]) {
//     std::string log_conf_file = "./conf/log_afile.conf";

//     com_registappender("CRONOLOG", comspace::CronoAppender::getAppender,
//                 comspace::CronoAppender::tryAppender);

//     auto logger = logging::ComlogSink::GetInstance();
//     if (0 != logger->SetupFromConfig(log_conf_file.c_str())) {
//         LOG(FATAL) << "load log conf failed";
//         return 0;
//     }

//     testing::InitGoogleTest(&argc, argv);
//     google::ParseCommandLineFlags(&argc, &argv, false);
//     return RUN_ALL_TESTS();
// }
