#include <tuple>
#include <bthread.h>
#include <bthread/mutex.h>
#include <future>
#include <mutex>
#include <vector>
#include <random>
#include <map>
#include <assert.h>

#include "gflags/gflags.h"

#include "baidu/streaming_log.h"
#include "base/comlog_sink.h"
#include "base/strings/stringprintf.h"
#include "com_log.h"
#include "cronoapd.h"

DEFINE_int32(ops_per_thread, 100, "ops_per_thread");
DEFINE_int32(times, 3, "bench times");
DEFINE_int32(executor_type, 1, "executor_type");
DEFINE_int32(concurrent, 10, "concurrent");
DEFINE_int32(is_delete_closure, 1, "delete closure");
DEFINE_bool(use_log, true, "use_log");

#include "closure.h"
#include "bench_common.h"

using gflow::ClosureContext;

class Vertex {
public:
    void run() {
        //LOG(NOTICE) << "run()...";
        int sum = 0;
        for (int i = 0; i < 100000; ++i) {
            sum += i;
        }
        closure_context->one_vertex_finished();
    }
    ClosureContext* get_closure_context() {
        return closure_context;
    }
public:
    ClosureContext* closure_context = nullptr;
};

struct Params {
    Params(Vertex* vertex_, ClosureContext* closure_context_) : 
                    vertex(vertex_), 
                    closure_context(closure_context_) {}
    Vertex* vertex = nullptr;
    ClosureContext* closure_context = nullptr;
};

class GraphExecutor {
   public:
    GraphExecutor() = default;
    GraphExecutor(GraphExecutor const&) = delete;             // Copy construct
    GraphExecutor(GraphExecutor&&) = delete;                  // Move construct
    GraphExecutor& operator=(GraphExecutor const&) = delete;  // Copy assign
    GraphExecutor& operator=(GraphExecutor&&) = delete;       // Move assign

    virtual ClosureContext* create_closure_context() = 0;

    virtual int32_t execute(Vertex* vertex,
                            ClosureContext* closure_context) = 0;
};

class BthreadGraphExecutor : public GraphExecutor {
   public:
    static GraphExecutor* instance() {
        static BthreadGraphExecutor ins;
        return &ins;
    }
    ClosureContext* create_closure_context() override;
    int32_t execute(Vertex* vertex,
                    ClosureContext* closure_context) override;
};

class AsyncGraphExecutor : public GraphExecutor {
   public:
    static GraphExecutor* instance() {
        static AsyncGraphExecutor ins;
        return &ins;
    }
    ClosureContext* create_closure_context() override;
    int32_t execute(Vertex* vertex,
                    ClosureContext* closure_context) override;
};

inline void* run_vertex(void* args) {
    auto* params = reinterpret_cast<Params*>(args);
    Vertex* vertex = params->vertex;
    ClosureContext* closure_context = params->closure_context;
    assert(closure_context != nullptr);
    assert(closure_context == vertex->get_closure_context());
    //assert(closure_context->is_delete.load() == false);
    vertex->run();
    delete params;
    return NULL;
}

ClosureContext* BthreadGraphExecutor::create_closure_context() {
    //auto* p = ClosreFactory<1>::instance().get();
    auto* p = new ClosureContext(1);
    return p;
}

int32_t BthreadGraphExecutor::execute(Vertex* vertex,
                ClosureContext* closure_context) {
    auto* params = new Params(vertex, closure_context);
    bthread_t th;
    //LOG(NOTICE) << "execute()...";
    if (bthread_start_background(&th, NULL, run_vertex, params) != 0) {
        LOG(WARNING) << "bthread start to run vertex failed";
        delete params;
        return -1;
    }
    //LOG(NOTICE) << "bthread execute return from vertex:" << vertex;
    return 0;
}

ClosureContext* AsyncGraphExecutor::create_closure_context() {
    //auto* p = ClosreFactory<2>::instance().get();
    auto* p = new ClosureContext(2);
    return p;
}

int32_t AsyncGraphExecutor::execute(Vertex* vertex,
                ClosureContext* closure_context) {
    auto* params = new Params(vertex, closure_context);
    std::packaged_task<void*()> task([params]() -> void*{
        return run_vertex(params); 
    });
    task();
    return 0;
}

class Graph {
public:
    Graph(uint32_t vertex_num) {
        if (FLAGS_executor_type == 1) {
            executor = std::make_shared<BthreadGraphExecutor>();
        } else {
            executor = std::make_shared<AsyncGraphExecutor>();
        }
        for (int i = 0; i < vertex_num; ++i) {
            auto v = std::make_shared<Vertex>();
            vertexs.emplace_back(std::move(v));
        }
    }
    ClosureContext* run() {
        auto* closure_context = executor->create_closure_context();
        LOG(TRACE) << "Graph run() this:" << this << " closure_context:" << closure_context;
        closure_context->add_wait_vertex_num(vertexs.size());
        for (auto& v : vertexs) {
            v->closure_context = closure_context;
            executor->execute(v.get(), closure_context);
        }
        return closure_context;
    }
public:
    std::vector<std::shared_ptr<Vertex>> vertexs;
    std::shared_ptr<GraphExecutor> executor;
};

void bench_executor(std::string name, int concurrent) {
    int ops_each_time = FLAGS_ops_per_thread * concurrent;
    // bench一次
    auto benchFn = [&]() -> uint64_t {
        std::vector<std::shared_ptr<Graph>> graphes;
        auto initFn = [&] {
            for (int i = 0; i < concurrent; ++i) {
                auto g = std::make_shared<Graph>(100);
                graphes.emplace_back(std::move(g));
            }
        };

        // 定义每个线程干的活
        auto fn = [&](int thread_idx) {
            auto g = graphes[thread_idx];
            for (int i = 0; i < FLAGS_ops_per_thread; ++i) {
                auto* closure_context = g->run();
                closure_context->wait_finish();
                closure_context->is_delete = true;
                if (FLAGS_is_delete_closure) {
                    delete closure_context;
                }
            }
        };
        auto endFn = [&] {
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

void test_closure() {
	ClosureContext closure(2);
	closure.add_wait_vertex_num(100);

	std::vector<std::thread> threads;
	for (size_t i = 0; i < 100; ++i) {
		threads.emplace_back([&, i] {
            closure.one_vertex_finished();
		});
        threads[i].detach();
	}
    std::cout << "closure_wait_finish...\n";
    closure.wait_finish();
    std::cout << "closure_wait_finish OK\n";

}


// 分别用多少种并发来压测
//std::vector<int> concurrent_list = {1, 5, 10, 20};
//std::vector<int> concurrent_list = {20};
int32_t run_bench() {
    //for (auto concurrent : concurrent_list) {
    auto concurrent = FLAGS_concurrent;
    {
        std::cout << "concurrent:" << concurrent << " threads -------------" << std::endl;
        bench_executor("bench_executor", concurrent); 
        //bench_mutex("bench_mutex", concurrent); 
    }
    //std::this_thread::sleep_for(std::chrono::milliseconds(1000));
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

    test_closure();
    return 0;
}
