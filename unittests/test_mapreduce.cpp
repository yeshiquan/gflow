#include <gtest/gtest.h>
#include "gflags/gflags.h"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <memory>
#include <any>

#define DCHECK_IS_ON 1

#include "data.h"
#include "dependency.h"
#include "vertex.h"
#include "graph.h"
#include "expr_processor.h"
#include "channel.h"
#include "mapreduce.h"

//DEFINE_int32(round, 2, "round");

using Clock = std::chrono::high_resolution_clock;

using namespace gflow;

class MapReduceTest : public ::testing::Test {
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

struct Item {
    int id = 0;
};

struct Forward {
    std::string title;
};

TEST_F(MapReduceTest, test_map_reduce) {
    Channel<Item*> queue; 
    std::vector<Item*> items(100);
    for (int i = 0; i < items.size(); ++i) {
        items[i] = new Item;
        items[i]->id = i+1;
    }
    queue.push_n(items.begin(), items.end());
    queue.close();

    using MR = MapReducer<Item*, Forward*, Channel>;
    using InputBatchPtr = MR::InputBatchPtr;
    using OutputBatchPtr = MR::OutputBatchPtr;
    MR mr;
    mr.input(queue).batch_size(10).map([&](InputBatchPtr batch) -> OutputBatchPtr {
        OutputBatchPtr result = mr.make_output_batch();
        for (auto item : *batch) {
            auto* fwd = new Forward;
            fwd->title = "title_" + std::to_string(item->id);
            result->push_back(fwd);
        }
        return result;
    }).reduce([&](MR::OutputBatchPtr batch) {
        std::cout << "reduce..." << std::endl;
        for (auto fwd : *batch) {
            std::cout << "forward title: " << fwd->title << std::endl;
        }
    }).run();
}

TEST_F(MapReduceTest, test_no_reduce) {
    Channel<Item*> queue; 
    std::vector<Item*> items(100);
    for (int i = 0; i < items.size(); ++i) {
        items[i] = new Item;
        items[i]->id = i+1;
    }
    queue.push_n(items.begin(), items.end());
    queue.close();

    using MR = MapReducer<Item*, Forward*, Channel>;
    using InputBatchPtr = MR::InputBatchPtr;
    using OutputBatchPtr = MR::OutputBatchPtr;
    MR mr;
    mr.input(queue).batch_size(10).map([&](InputBatchPtr batch) -> OutputBatchPtr {
        OutputBatchPtr result = mr.make_output_batch();
        for (auto item : *batch) {
            auto* fwd = new Forward;
            fwd->title = "title_" + std::to_string(item->id);
            result->push_back(fwd);
        }
        return result;
    }).run();
}