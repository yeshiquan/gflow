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

namespace general_processor {

//DEFINE_int32(round, 2, "round");

using Clock = std::chrono::high_resolution_clock;

using namespace gflow;

class GeneralProcessorTest : public ::testing::Test {
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

class RecallProcessor4 : public GraphFunction {
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

        qq_channel->clear();
        // 在往队列push数据之前就发布数据，从而下游节点可以提前ready
        RELEASE(qq_channel);

        qishui_channel->clear();
        // 在往队列push数据之前就发布数据，从而下游节点可以提前ready
        RELEASE(qishui_channel);

        // 往队列push数据
        for (int batch = 0; batch < 10; ++batch) {
            std::vector<Item> items(100);

            for (int i = 0; i < 100; ++i) {
                items[i].id = batch*100 + i;
                items[i].name = "qq_" + std::to_string(items[i].id);
                LOG(TRACE) << "RecallProcessor emit a item:" << items[i].name;
            }
            qq_channel->push_n(items.begin(), items.end());

            for (int i = 0; i < 100; ++i) {
                items[i].id = batch*100 + i;
                items[i].name = "qishui_" + std::to_string(items[i].id);
                LOG(TRACE) << "RecallProcessor emit a item:" << items[i].name;
            }            
            qishui_channel->push_n(items.begin(), items.end());

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }

        qq_channel->close();
        qishui_channel->close();

        LOG(TRACE) << "RecallProcessor finish...";
        return 0;
    }
    GRAPH_DECLARE(
        EMIT(std::vector<std::string>, QQ_RESULT, mids)
        EMIT(Channel<Item>, QQ_ITEM_CHANNEL, qq_channel)
        EMIT(Channel<Item>, QISHUI_ITEM_CHANNEL, qishui_channel)
    );    
};
REGISTER_PROCESSOR(RecallProcessor4);

// 通用算子，只需声明局部变量
// 变量和GraphData的绑定在建图的时候来指定
class StrategyProcessor4 : public GraphFunction {
   public:
    int setup(std::any& option) {
        return 0;
    }
    int operator()() {
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        LOG(TRACE) << "StrategyProcessor run...";

        while (true) {
            std::vector<Item> items(100);
            size_t num = queue->pop_n(items.begin(), items.end());
            for (int i = 0; i < num; ++i) {
                LOG(TRACE) << "StrategyProcessor got a item:" << items[i].name;
            }
            if (num < 100) {
                break;
            }
        }

        LOG(TRACE) << "StrategyProcessor finish...";
        return 0;
    }
    VAR_DECLARE(
        DEPEND_VAR(std::vector<std::string>, mids)
        DEPEND_VAR(Channel<Item>, queue) 
        EMIT_VAR(int32_t, response)
    );    
};
REGISTER_PROCESSOR(StrategyProcessor4);

TEST_F(GeneralProcessorTest, test_mapreduce_channel) {
    std::cout << "\n\n============= test_channel =================\n";
    auto create_graph = []() -> Graph *{
        Graph *g = new Graph;
        GraphVertex* qq = g->add_vertex("RecallProcessor4");
        qq->emit("QQ_RESULT");
        qq->emit("ITEM_CHANNEL");

        GraphVertex *cp = g->add_vertex("StrategyProcessor4");
        cp->depend_and_bind("QQ_RESULT", "mids");
        cp->depend_and_bind("QQ_ITEM_CHANNEL", "queue");
        cp->emit_and_bind("QQ_RESPONSE", "response");

        GraphVertex *cp2 = g->add_vertex("StrategyProcessor4");
        cp2->depend_and_bind("QISHUI_RESULT", "mids");
        cp2->depend_and_bind("QISHUI_ITEM_CHANNEL", "queue");
        cp2->emit_and_bind("QISHUI_RESPONSE", "response");

        g->build();
        return g;
    };
    auto g = create_graph();
    GraphData *response = g->get_data("QQ_RESPONSE");
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