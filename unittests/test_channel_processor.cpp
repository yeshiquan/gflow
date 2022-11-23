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

//DEFINE_int32(round, 2, "round");

using Clock = std::chrono::high_resolution_clock;

using namespace gflow;

class ChannelTest : public ::testing::Test {
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

class RecallProcessor : public GraphFunction {
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
REGISTER_PROCESSOR(RecallProcessor);

class StrategyProcessor : public GraphFunction {
   public:
    int setup(std::any& option) {
        return 0;
    }
    int operator()() {
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        LOG(TRACE) << "StrategyProcessor run...";

        while (true) {
            std::vector<Item> items(100);
            size_t num = channel->pop_n(items.begin(), items.end());
            for (int i = 0; i < num; ++i) {
                LOG(TRACE) << "StrategyProcessor got a item:" << items[i].id;
            }
            if (num < 100) {
                break;
            }
        }

        LOG(TRACE) << "StrategyProcessor finish...";
        return 0;
    }
    GRAPH_DECLARE(
        DEPEND(std::vector<std::string>, QQ_RESULT, mids) 
        DEPEND(Channel<Item>, ITEM_QUEUE, channel) 
        EMIT(int32_t, RESPONSE, response)
    );    
};
REGISTER_PROCESSOR(StrategyProcessor);

TEST_F(ChannelTest, test_channel) {
    std::cout << "\n\n============= test_channel =================\n";
    auto create_graph = []() -> Graph *{
        Graph *g = new Graph;
        GraphVertex* qq = g->add_vertex("RecallProcessor");
        qq->emit("QQ_RESULT");
        qq->emit("ITEM_QUEUE");

        GraphVertex *cp = g->add_vertex("StrategyProcessor");
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
