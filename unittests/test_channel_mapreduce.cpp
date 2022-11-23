#include <gtest/gtest.h>
#include "gflags/gflags.h"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <any>

#define DCHECK_IS_ON 1

#include "data.h"
#include "dependency.h"
#include "vertex.h"
#include "graph.h"
#include "expr_processor.h"
#include "channel.h"
#include "mapreduce.h"

namespace mapreduce_channel {

//DEFINE_int32(round, 2, "round");

using Clock = std::chrono::high_resolution_clock;

using namespace gflow;

class MapReduceChannelTest : public ::testing::Test {
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
    Item() = default;
    Item(int id_) : id(id_) {}
    int id = 0;
    std::string name;
};

struct Forward {
    std::string title;
};

class RecallProcessor3 : public GraphFunction {
   public:
    int setup(std::any& option) {
        return 0;
    }
    int operator()() {
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        LOG(TRACE) << "RecallProcessor run...";
        mids->clear();
        mids->emplace_back("QQ_003R");
        mids->emplace_back("QQ_003R42424");
        RELEASE(mids);

        channel->clear();
        // 在往队列push数据之前就发布数据，从而下游节点可以提前ready
        RELEASE(channel);

        // 往队列push数据
        for (int batch = 0; batch < 10; ++batch) {
            std::vector<Item> items(100);
            for (int i = 0; i < 100; ++i) {
                items[i].id = batch*100 + i;
                LOG(TRACE) << "RecallProcessor emit a item:" << items[i].id;
            }
            channel->push_n(items.begin(), items.end());
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        channel->close();

        LOG(TRACE) << "RecallProcessor finish...";
        return 0;
    }
    GRAPH_DECLARE(
        EMIT(std::vector<std::string>, QQ_RESULT, mids)
        EMIT(Channel<Item>, ITEM_QUEUE, channel)
    );    
};
REGISTER_PROCESSOR(RecallProcessor3);

class StrategyProcessor3 : public GraphFunction {
   public:
    int setup(std::any& option) {
        return 0;
    }
    int operator()() {
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        LOG(TRACE) << "StrategyProcessor run...";

        using MR = MapReducer<Item, Forward, Channel>;
        using InputBatchPtr = MR::InputBatchPtr;
        using OutputBatchPtr = MR::OutputBatchPtr;
        MR mr;

        auto map_func = [&](InputBatchPtr batch) -> OutputBatchPtr {
            OutputBatchPtr result = mr.make_output_batch();
            for (auto& item : *batch) {
                Forward fwd;
                fwd.title = "title_" + std::to_string(item.id);
                result->push_back(fwd);
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            return result;
        };

        auto reduce_func = [&](OutputBatchPtr batch) {
            std::cout << "reduce..." << std::endl;
            for (auto& fwd : *batch) {
                std::cout << "reduce forward title: " << fwd.title << std::endl;
            }
        };

        mr.input(*channel).batch_size(100).parallel_map(map_func).reduce(reduce_func).run();

        LOG(TRACE) << "StrategyProcessor finish...";
        return 0;
    }
    GRAPH_DECLARE(
        DEPEND(std::vector<std::string>, QQ_RESULT, mids) 
        DEPEND(Channel<Item>, ITEM_QUEUE, channel) 
        EMIT(int32_t, RESPONSE, response)
    );    
};
REGISTER_PROCESSOR(StrategyProcessor3);

TEST_F(MapReduceChannelTest, test_mapreduce_channel) {
    std::cout << "\n\n============= test_channel =================\n";
    auto create_graph = []() -> Graph *{
        Graph *g = new Graph;
        GraphVertex* qq = g->add_vertex("RecallProcessor3");
        qq->emit("QQ_RESULT");
        qq->emit("ITEM_QUEUE");

        GraphVertex *cp = g->add_vertex("StrategyProcessor3");
        cp->depend("QQ_RESULT");
        cp->depend("ITEM_QUEUE");
        cp->emit("RESPONSE");
        g->build();
        return g;
    };
    auto g = create_graph();
    GraphData *response = g->get_data("RESPONSE");
    auto closure_context = g->run(response);
    int32_t error_code = closure_context->wait_finish();
    if (error_code != 0) {
        std::cout << "graph run failed error_code:" << error_code <<
        std::endl;
    }
    std::cout << "response -> " << response->raw<int32_t>() << std::endl;
    g->reset();
    delete g;
}

} // namespace