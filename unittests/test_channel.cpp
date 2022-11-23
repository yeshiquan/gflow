#include <gtest/gtest.h>
#include "gflags/gflags.h"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <tuple>

// #include <bthread.h>
// #include <bthread/mutex.h>

#define private public
#define protected public
#include "channel.h"
#undef private
#undef protected


inline size_t get_steady_clock_millisecond() {
	return std::chrono::time_point_cast<std::chrono::milliseconds>(
    	std::chrono::steady_clock::now()).time_since_epoch().count();
}

using namespace gflow;

class SimpleChannelTest : public ::testing::Test {
   private:
    virtual void SetUp() {
        // p_obj.reset(new ExprBuilder);
    }
    virtual void TearDown() {
        // if (p_obj != nullptr) p_obj.reset();
    }

   protected:
    // std::shared_ptr<ExprBuilder>  p_obj;
};


TEST_F(SimpleChannelTest, test_channel_1) {
    Channel<int> q;
    for (int i = 0; i < 10; ++i) {
        q.push(i);
    }
    for (int i = 0; i < 10; ++i) {
        int value;
        q.pop(value);
        ASSERT_EQ(value, i);
    }    
}

TEST_F(SimpleChannelTest, test_channel_2) {
    Channel<int> q;
    std::vector<int> items{1, 2, 3, 4, 5};
    q.push_n(items.begin(), items.end());

    std::vector<int> items2(items.size());
    size_t num = q.pop_n(items2.begin(), items2.end());
    ASSERT_EQ(num, items.size());
    for (int i = 0; i < items.size(); ++i) {
        ASSERT_EQ(items[i], items2[i]);
    }
}

TEST_F(SimpleChannelTest, test_channel_3) {
    Channel<int> q;
    std::vector<int> items{1, 2, 3, 4, 5};
    q.push_n(items.begin(), items.end());

    std::vector<int> items2(3);
    size_t num = q.pop_n(items2.begin(), items2.end());
    ASSERT_EQ(num, items2.size());
    for (int i = 0; i < num; ++i) {
        ASSERT_EQ(items[i], items2[i]);
    }
}

TEST_F(SimpleChannelTest, test_channel_4) {
    Channel<int> q;

    std::thread thd([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        q.push(5);
    });
    
    uint64_t start = get_steady_clock_millisecond();
    int value;
    q.pop(value);
    uint64_t end = get_steady_clock_millisecond();
    std::cout << "wait " << (end-start) << std::endl;
    ASSERT_TRUE( (end-start) >= 50);

    thd.join();
}

TEST_F(SimpleChannelTest, test_channel_5) {
    Channel<int> q;
    std::vector<int> items{1, 2, 3, 4, 5};
    q.push_n(items.begin(), items.end());

    std::thread thd([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        q.close();
    });

    uint64_t start = get_steady_clock_millisecond();
    std::vector<int> items2(10);
    // 想pop 10个，队列不够，一直阻塞
    size_t num = q.pop_n(items2.begin(), items2.end());

    // 被另一个线程close了，pop返回的数量不够10个
    ASSERT_EQ(num, items.size());
    for (int i = 0; i < num; ++i) {
        ASSERT_EQ(items[i], items2[i]);
    }
    uint64_t end = get_steady_clock_millisecond();
    std::cout << "wait " << (end-start) << std::endl;
    ASSERT_TRUE( (end-start) >= 50);    

    thd.join();
}