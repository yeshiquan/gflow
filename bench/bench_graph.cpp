#include "gflags/gflags.h"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <any>

#include <assert.h>
#include "baidu/streaming_log.h"
#include "base/comlog_sink.h"
#include "base/strings/stringprintf.h"
#include "com_log.h"
#include "cronoapd.h"

#include "data.h"
#include "dependency.h"
#include "vertex.h"
#include "graph.h"
#include "expr_processor.h"

DEFINE_int32(ops_per_thread, 100, "ops_per_thread");
DEFINE_int32(times, 10, "bench times");
DEFINE_int32(executor_type, gflow::AsyncExecutorType, "executor_type");
DEFINE_int32(concurrent, 10, "concurrent");
DEFINE_int32(is_delete_closure, 1, "delete closure");
DEFINE_bool(use_log, true, "use_log");

#include "bench_common.h"

using gflow::Graph;
using gflow::GraphVertex;
using gflow::GraphData;
using gflow::GraphDependency;
using gflow::GraphFunction;

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
        //std::this_thread::sleep_for(std::chrono::milliseconds(5));
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

Graph* create_graph(int flag2, int flag3) {
    Graph *g = new Graph(FLAGS_executor_type);

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
}

std::vector<std::string>& run_graph(Graph *g) {
    // 发布 SELF_RESULT
    GraphData *self_data = g->ensure_data("SELF_RESULT");
    auto self_mids = self_data->commiter<std::vector<std::string>>();
    self_mids->clear();
    self_mids->emplace_back("SELF_22580");
    self_mids.commit();
    // 发布Sid_1234
    g->ensure_data("Sid_1234")->emit_value<int>(true);
    // 发布Sid_5678
    g->ensure_data("Sid_5678")->emit_value<int>(false);

    GraphData *response = g->get_data("RESPONSE");
    LOG(TRACE) << "--------------------- start run graph -------------";        

    using Clock = std::chrono::high_resolution_clock;
    auto start_time = Clock::now();

    auto* closure_context = g->run(response);
    int32_t error_code = closure_context->wait_finish();

    auto use_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                            Clock::now() - start_time).count();
    if (error_code != 0) {
        LOG(WARNING) << "closure_context error:" << error_code;
        g->reset();
    }

    std::vector<std::string> &result = response->raw<std::vector<std::string>>();
    //assert(result.size() == 3);
    LOG(NOTICE) << "Graph finish use_ns:" << (use_ns/1000000.0) << " closure_context:" << closure_context << " result:" << result.size();
    std::string line = ">>> response -> mids:[";
    for (auto &mid : result) {
        line += mid + ",";
    }
    line += "]";
    LOG(NOTICE) << line;
    g->reset();
    closure_context->is_delete = true;
    if (FLAGS_is_delete_closure) {
        delete closure_context;
    }
    return result;
}

void bench_graph(std::string name, int concurrent, int flag2, int flag3) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        std::vector<Graph*> graphs;
        auto initFn = [&] {
            for (int i = 0; i < concurrent; ++i) {
                Graph *g = create_graph(flag2, flag3);
                graphs.emplace_back(g);
            }
            int i = 0;
            for (auto& g : graphs) {
                LOG(NOTICE) << name << "idx:" << i << " " << g;
                i++;
            }
        };

        // 定义每个线程干的活
        auto fn = [&](int thread_idx) {
            assert(graphs.size() > thread_idx);
            Graph* g = graphs[thread_idx];
            //LOG(NOTICE) << "bench_graph " << name << " thread_idx:" << thread_idx << " g:" << g;
            for (int i = 0; i < FLAGS_ops_per_thread; i++) {
                std::vector<std::string>& res = run_graph(g);
            }
        };
        auto endFn = [&] {
            //rcu::queue::PRINT_STATS();
        };
		uint64_t cost;
		if (FLAGS_executor_type == 1) {
			cost = bthread_run_concurrent(initFn, fn, endFn, concurrent);
		} else {
			cost = run_concurrent(initFn, fn, endFn, concurrent);
		}
        return cost;
    };

    // bench多次，取最大值、平均值、最小值
    bench_many_times(name, benchFn, ops_each_time, FLAGS_times);    
}

// 分别用多少种并发来压测
//std::vector<int> concurrent_list = {1, 5, 10, 20};
std::vector<int> concurrent_list = {20};
int32_t run_bench() {
    //for (auto concurrent : concurrent_list) {
    auto concurrent = FLAGS_concurrent;
    {
        std::cout << "concurrent:" << concurrent << " threads -------------" << std::endl;
        bench_graph("depend_all", concurrent, 0, 0);
        bench_graph("depend_qq_condition_failed", concurrent, 1, 0);
        bench_graph("depend_qq_condition_ok", concurrent, 2, 1);
        bench_graph("depend_no_merge", concurrent, 0, 1);
    }
    return 0;
}

int main(int argc, char** argv) {
    google::ParseCommandLineFlags(&argc, &argv, true);

    if (FLAGS_use_log) {
        std::string log_conf_file = "./conf/log.conf";

        com_registappender("CRONOLOG", comspace::CronoAppender::getAppender,
                    comspace::CronoAppender::tryAppender);

        auto logger = logging::ComlogSink::GetInstance();
        if (0 != logger->SetupFromConfig(log_conf_file.c_str())) {
            LOG(FATAL) << "load log conf failed";
            return -1;
        }

        // 要在SetupFromConfig之后执行，否则comlog_get_log_level()是错误的
        switch (comlog_get_log_level()) {
            case COMLOG_FATAL:
                ::logging::SetMinLogLevel(::logging::BLOG_FATAL);
                break;
            case COMLOG_WARNING:
                ::logging::SetMinLogLevel(::logging::BLOG_WARNING);
                break;
            case COMLOG_NOTICE:
                ::logging::SetMinLogLevel(::logging::BLOG_NOTICE);
                break;
            case COMLOG_TRACE:
                ::logging::SetMinLogLevel(::logging::BLOG_TRACE);
                break;
            case COMLOG_DEBUG:
                ::logging::SetMinLogLevel(::logging::BLOG_DEBUG);
                break;
            
            default:
                break;
        }
    }

    return run_bench();
}
