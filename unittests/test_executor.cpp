#include <gtest/gtest.h>
#include "gflags/gflags.h"

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <tuple>
#include <bthread.h>
#include <bthread/mutex.h>

#include "vertex.h"
#include "executor.h"
#include "processor.h"
#include "graph.h"
#include "closure.h"

using namespace gflow;

class ExecutorTest : public ::testing::Test {
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

struct Foobar {};

class DummyProcessor : public GraphProcessor {
   public:
    int process() {
        std::cout << "DummyProcessor run..." << std::endl;
        return -1;
    }
};

void* execute_vertex(void* args) {
    auto* params = reinterpret_cast<std::tuple<Foobar*, int*>*>(args);
    Foobar* foobar = std::get<0>(*params);
    int* value = std::get<1>(*params);
    std::cout << "execute_vertex -> value:" << *value << std::endl;
    return NULL;
}

TEST_F(ExecutorTest, test_bthread_and_tuple) {
    Foobar* foobar = new Foobar;
    int* value = new int(111);
    std::tuple<Foobar*, int*>* params =
        new std::tuple<Foobar*, int*>(foobar, value);
    bthread_t th;
    bthread_start_background(&th, NULL, execute_vertex, params);
    bthread_join(th, NULL);
}

// TEST_F(ExecutorTest, case2) {
//     Graph graph;
//     auto closure_context = std::make_unique<ClosureContext>();
//     DummyProcessor processor;
//     auto* vertex = new GraphVertex(&graph, &processor);
//     vertex->set_closure_context(closure_context.get());
//     auto* executor = BthreadGraphExecutor::instance();
//     executor->execute(vertex, closure_context.get());
//     std::this_thread::sleep_for(std::chrono::milliseconds(1000));
// }