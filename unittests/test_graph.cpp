#include <gtest/gtest.h>
#include "gflags/gflags.h"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <any>

#include "data.h"
#include "dependency.h"
#include "vertex.h"
#include "graph.h"
#include "expr_processor.h"

DEFINE_int32(round, 1, "round");
DEFINE_int32(executor_type, gflow::BthreadExecutorType, "executor_type");

using Clock = std::chrono::high_resolution_clock;

using namespace gflow;

class GraphTest : public ::testing::Test {
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

class DummyProcessor : public GraphProcessor {
   public:
    int process() {
        std::cout << "DummyProcessor run..." << std::endl;
        return 1;
    }
};
REGISTER_PROCESSOR(DummyProcessor);

class QqProcessor : public GraphFunction {
   public:
    int setup(std::any& option) {
        return 0;
    }
    int operator()() {
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        LOG(TRACE) << "QqProcessor run...";
        mids->clear();
        mids->emplace_back("QQ_003R");
        mids->emplace_back("QQ_003R42424");
        return 0;
    }
    GRAPH_DECLARE(
        EMIT(std::vector<std::string>, QQ_RESULT, mids)
    );    
};
REGISTER_PROCESSOR(QqProcessor);

class EsProcessor : public GraphFunction {
   public:
    int setup(std::any& option) {
        return 0;
    }
    int operator()() {
        //std::this_thread::sleep_for(std::chrono::milliseconds(500));
        LOG(TRACE) << "EsProcessor run...";
        mids->clear();
        mids->emplace_back("ES_003RZq1D1SiH9a");
        mids->emplace_back("ES_003R");
        LOG(TRACE) << "EsProcessor done. mids_size:" << mids->size();
        return 0;
    }
    GRAPH_DECLARE(
        EMIT(std::vector<std::string>, ES_RESULT, mids)
    );
};
REGISTER_PROCESSOR(EsProcessor);

class MergeProcessor : public GraphFunction {
   public:
    int setup(std::any& option) {
        return 0;
    }
    int operator()() {
        std::string sample_id = vertex().get_option<std::string>();
        LOG(TRACE) << "MergeProcessor run...sample_id:" << sample_id;
        mids->clear();
        // std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        int n_dep = 0;
        int total_mid_size = 0;
        for (GraphDependency *dep : vertex().get_optional_dependencys()) {
            auto &depend_mids = dep->get_data()->raw<std::vector<std::string>>();
            LOG(TRACE) << "merge ndep: " << n_dep << " depend data mids size:" << depend_mids.size();
            total_mid_size += depend_mids.size();
            for (auto &mid : depend_mids) {
                mids->emplace_back(mid);
                LOG(TRACE) << "ndep:" << n_dep << " mid:" << mid;
            }
            n_dep++;
        }
        LOG(TRACE) << "merge processor total mids size:" << total_mid_size;
        for (auto &mid : *self_mids) {
            mids->emplace_back(mid);
        }

        return 0;
    }
    GRAPH_DECLARE(
        DEPEND(std::vector<std::string>, SELF_RESULT, self_mids)
        EMIT(std::vector<std::string>, MERGE_RESULT, mids)
    );
};
REGISTER_PROCESSOR(MergeProcessor);

class FilterProcessor : public GraphFunction {
   public:
    int setup(std::any& option) {
        return 0;
    }
    int operator()() {
        LOG(TRACE) << "FilterProcessor run...";
        response->clear();

        for (auto &mid : *merge_mids) {
            // 过滤长度比较小的id
            if (mid.length() < 8) {
                continue;
            }
            response->emplace_back(mid);
        }
        return 0;
    }
    GRAPH_DECLARE(
        DEPEND(std::vector<std::string>, MERGE_RESULT, merge_mids)
        EMIT(std::vector<std::string>, RESPONSE, response)
    );
};
REGISTER_PROCESSOR(FilterProcessor);

TEST_F(GraphTest, test_vertex_success) {
    std::cout << "\n\n============= test_vertex_success =================\n";
    auto create_graph = [](int flag2, int flag3) -> Graph *{
        Graph *g = new Graph(FLAGS_executor_type);
        GraphVertex *dummy_function = g->add_vertex("DummyProcessor");

        GraphVertex *qq_function = g->add_vertex("QqProcessor");
        qq_function->emit("QQ_RESULT");

        GraphVertex *es_function = g->add_vertex("EsProcessor");
        es_function->emit("ES_RESULT");

        GraphVertex *merge_function = g->add_vertex("MergeProcessor");
        std::string sample_id = "452424";
        merge_function->set_option(sample_id);

        if (flag2 == 1) {
            merge_function->optional_depend("QQ_RESULT")->if_true("Sid_1234 && Sid_5678");
        } else if (flag2 == 2) {
            merge_function->optional_depend("QQ_RESULT")->if_true("Sid_1234 || Sid_5678");
        } else {
            merge_function->optional_depend("QQ_RESULT");
        }
        merge_function->optional_depend("ES_RESULT");
        merge_function->depend("SELF_RESULT");
        merge_function->emit("MERGE_RESULT");

        GraphVertex *filter_function = g->add_vertex("FilterProcessor");

        if (flag3 == 1) {
            filter_function->depend("MERGE_RESULT")->if_true("Sid_1234 && Sid_5678");
        } else if (flag3 == 2) {
            filter_function->depend("MERGE_RESULT")->if_true("Sid_1234 || Sid_5678");
        } else {
            filter_function->depend("MERGE_RESULT");
        }

        filter_function->emit("RESPONSE");
        g->build();
        return g;
    };

    auto run_graph = [](Graph *g) -> std::vector<std::string>& {
        // 发布 SELF_RESULT
        GraphData *self_data = g->ensure_data("SELF_RESULT");
        auto self_mids = self_data->commiter<std::vector<std::string>>();
        self_mids->emplace_back("SELF_22580");
        self_mids.commit();
        // 发布Sid_1234
        g->ensure_data("Sid_1234")->emit_value<int>(true);
        // 发布Sid_5678
        g->ensure_data("Sid_5678")->emit_value<int>(false);

        GraphData *response = g->get_data("RESPONSE");
        LOG(TRACE) << "--------------------- start run graph -------------";        
        auto* closure_context = g->run(response);
        int32_t error_code = closure_context->wait_finish();
        if (error_code != 0) {
            g->reset();
        }

        std::vector<std::string> &result = response->raw<std::vector<std::string>>();
        std::string line = ">>> response -> mids:[";
        for (auto &mid : result) {
            line += mid + ",";
        }
        line += "]";
        LOG(TRACE) << line;
        g->reset();
        return result;
    };

    Graph *g1 = create_graph(0, 0);
    Graph *g3 = create_graph(1, 0);
    Graph *g4 = create_graph(0, 1);
    for (int k = 0; k < FLAGS_round; ++k) {
        auto start_time = Clock::now(); 
        LOG(TRACE) << "--------------- round " << k << " ----------------\n";
        {
            LOG(TRACE) << "\n\n===========================  Test Depend All\n";
            std::vector<std::string>& res = run_graph(g1);
            ASSERT_EQ(res.size(), 3);
        }
        {
            // 不要依赖QQ Result
            LOG(TRACE) << "\n\n===========================  Test Depend No QQ\n";
            std::vector<std::string>& res = run_graph(g3);
            ASSERT_EQ(res.size(), 2);
        }
        {
            // 不要依赖Merge Result
            LOG(TRACE) << "\n\n===========================  Test Depend No Merge\n";
            std::vector<std::string>& res = run_graph(g4);
            ASSERT_EQ(res.size(), 0);
        }
        auto end_time = Clock::now(); 
        auto use_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time).count();
        LOG(NOTICE) << "round " << k << " cost_ns:" << use_ns;       
    }
}

class KuwoProcessor : public GraphFunction {
   public:
    int setup(std::any& option) {
        return 0;
    }
    int operator()() {
        LOG(TRACE) << "KuwoProcessor run...";
        mids->clear();
        mids->emplace_back("ES_003RZq1D1SiH9a");
        mids->emplace_back("ES_003R");
        return -1;  // 故意返回错误
    }
    GRAPH_DECLARE(
        EMIT(std::vector<std::string>, KUWO_RESULT, mids)
    );
};
REGISTER_PROCESSOR(KuwoProcessor);

class CheckProcessor : public GraphFunction {
   public:
    int setup(std::any& option) {
        return 0;
    }
    int operator()() {
        LOG(TRACE) << "CheckProcessor run...";
        for (auto& mid : *kuwo_mids) {
            if (true) {
                mids->emplace_back(mid);
            }
        }
        return 0;
    }
    GRAPH_DECLARE(
        DEPEND(std::vector<std::string>, KUWO_RESULT, kuwo_mids)
        EMIT(std::vector<std::string>, RESPONSE, mids)
    );
};
REGISTER_PROCESSOR(CheckProcessor);

TEST_F(GraphTest, test_vertex_failed) {
    std::cout << "\n\n============= test_vertex_failed =================\n";
    auto create_graph = []() -> Graph *{
        Graph *g = new Graph;
        GraphVertex *kuwo_function = g->add_vertex("KuwoProcessor");
        kuwo_function->emit("KUWO_RESULT");
        GraphVertex *check_function = g->add_vertex("CheckProcessor");
        check_function->depend("KUWO_RESULT");
        check_function->emit("RESPONSE");
        g->build();
        return g;
    };
    auto g = create_graph();
    GraphData *response = g->get_data("RESPONSE");

    // auto closure_context = g->run(response);
    // int32_t error_code = closure_context->wait_finish();
    // if (error_code != 0) {
    //     std::cout << "graph run failed error_code:" << error_code <<
    //     std::endl;
    // }
    // g->reset();
}

TEST_F(GraphTest, test_expression_processor) {
    std::string result_name = "EXPR_RESULT";
    std::string expr_string = "if (Sid_1 > Sid_2) Sid_3+8 else Sid_4*5";
    auto create_graph = [&]() -> Graph *{
        Graph *g = new Graph;
        GraphData *result_data = g->ensure_data(result_name);
        ExpressionProcessor *processor = new gflow::ExpressionProcessor;
        GraphVertex *expr_function =
            g->add_vertex(static_cast<GraphProcessor *>(processor));
        processor->init(expr_string, result_name);
        result_data->set_producer(expr_function);
        expr_function->depend("Sid_1");
        expr_function->depend("Sid_2");
        expr_function->depend("Sid_3");
        expr_function->depend("Sid_4");
        expr_function->emit(result_name);
        g->build();
        return g;
    };
    auto *g = create_graph();

    g->ensure_data("Sid_1")->emit_value<int>(5);
    g->ensure_data("Sid_2")->emit_value<int>(7);
    g->ensure_data("Sid_3")->emit_value<int>(8);
    g->ensure_data("Sid_4")->emit_value<int>(9);

    GraphData *response = g->get_data(result_name);
    auto* closure_context = g->run(response);
    int32_t error_code = closure_context->wait_finish();
    if (error_code != 0) {
        std::cout << "graph run failed error_code:" << error_code << std::endl;
    }
    GraphData *result = g->get_data(result_name);
    int res = result->raw<int>();
    std::cout << "expression result -> " << res << std::endl;
    ASSERT_EQ(res, 45);
    g->reset();
}
