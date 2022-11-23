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


#include "vertex.h"

#define private public
#define protected public

#include "closure.h"
#include "graph_executor.h"
#include "processor.h"
#include "graph.h"

#include "dependency.h"
#undef private
#undef protected


using namespace gflow;

class DependencyTest : public ::testing::Test {
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
        return 0;
    }
    GRAPH_DECLARE(
        DEPEND(int32_t, QQ_STATUS, qq_status)
        EMIT(std::vector<std::string>, MERGE_RESULT, mids)
    );    
};
REGISTER_PROCESSOR(DummyProcessor);

#define INIT_GRAPH() \
    auto closure_context = std::make_unique<ClosureContext>(); \
    auto pg = std::make_shared<Graph>(); \
    Graph* g = pg.get(); \
    GraphVertex *v = g->add_vertex("DummyProcessor"); \
    v->set_closure_context(closure_context.get()); \
    GraphData *data = g->create_data("QQ_STATUS"); \
    GraphData *condition = g->create_data("CONDITION"); \
    auto* dep = v->depend("QQ_STATUS"); \
    dep->_condition_data = condition; \
    v->add_waiting_num(); \
    closure_context->add_wait_vertex_num(1); \
    std::vector<GraphVertex *> actived_vertexs; \
    int32_t wait_num = INT_MAX;

#define INIT_GRAPH_ONLY_DATA() \
    auto closure_context = std::make_unique<ClosureContext>(); \
    auto pg = std::make_shared<Graph>(); \
    Graph* g = pg.get(); \
    GraphVertex *v = g->add_vertex("DummyProcessor"); \
    v->set_closure_context(closure_context.get()); \
    v->add_waiting_num(); \
    GraphData *data = g->create_data("QQ_STATUS"); \
    auto* dep = v->depend("QQ_STATUS"); \
    closure_context->add_wait_vertex_num(1); \
    std::vector<GraphVertex *> actived_vertexs; \
    int32_t wait_num = INT_MAX;    

// ================== 有condition和data, condition是true ===========================
TEST_F(DependencyTest, test_case1) {
    std::cout << "------------------ case1 ----------------\n";
    INIT_GRAPH();
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, 2);
    wait_num = dep->fire_condition(true);
    ASSERT_EQ(wait_num, 1);
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, 0);
    closure_context->wait_finish();
}

TEST_F(DependencyTest, test_case2) {
    std::cout << "------------------ case2 ----------------\n";
    INIT_GRAPH();
    wait_num = dep->fire_condition(true);
    ASSERT_EQ(wait_num, -1);
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, 1);
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, 0);
    closure_context->wait_finish();
}

TEST_F(DependencyTest, test_case3) {
    std::cout << "------------------ case3 ----------------\n";
    INIT_GRAPH();
    wait_num = dep->fire_condition(true);
    ASSERT_EQ(wait_num, -1);
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, -2);
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, 0);
    closure_context->wait_finish();
}

TEST_F(DependencyTest, test_case4) {
    std::cout << "------------------ case4 ----------------\n";
    INIT_GRAPH();
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, -1);
    wait_num = dep->fire_condition(true);
    ASSERT_EQ(wait_num, -2);
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, 0);
    closure_context->wait_finish();
}

TEST_F(DependencyTest, test_case5) {
    std::cout << "------------------ case5 ----------------\n";
    INIT_GRAPH();
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, -1);
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, 1);
    wait_num = dep->fire_condition(true);
    ASSERT_EQ(wait_num, 0);
    closure_context->wait_finish();
}

TEST_F(DependencyTest, test_case6) {
    std::cout << "------------------ case6 ----------------\n";
    INIT_GRAPH();
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, 2);
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, 1);
    wait_num = dep->fire_condition(true);
    ASSERT_EQ(wait_num, 0);
    closure_context->wait_finish();
}

// ================== 有condition和data, condition是false ===========================
TEST_F(DependencyTest, test_case11) {
    std::cout << "------------------ case11 ----------------\n";
    INIT_GRAPH();
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, 2);
    wait_num = dep->fire_condition(false);
    ASSERT_EQ(wait_num, 0);
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, -1);
    closure_context->wait_finish();
}

TEST_F(DependencyTest, test_case12) {
    std::cout << "------------------ case12 ----------------\n";
    INIT_GRAPH();
    wait_num = dep->fire_condition(false);
    ASSERT_EQ(wait_num, -2);
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, 0);
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, -1);
    closure_context->wait_finish();
}

TEST_F(DependencyTest, test_case13) {
    std::cout << "------------------ case13 ----------------\n";
    INIT_GRAPH();
    wait_num = dep->fire_condition(false);
    ASSERT_EQ(wait_num, -2);
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, -3);
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, -1);
    closure_context->wait_finish();
}

TEST_F(DependencyTest, test_case14) {
    std::cout << "------------------ case14 ----------------\n";
    INIT_GRAPH();
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, -1);
    wait_num = dep->fire_condition(false);
    ASSERT_EQ(wait_num, -3);
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, -1);
    closure_context->wait_finish();
}

TEST_F(DependencyTest, test_case15) {
    std::cout << "------------------ case15 ----------------\n";
    INIT_GRAPH();
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, -1);
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, 1);
    wait_num = dep->fire_condition(false);
    ASSERT_EQ(wait_num, 0);
    closure_context->wait_finish();
}

TEST_F(DependencyTest, test_case16) {
    std::cout << "------------------ case16 ----------------\n";
    INIT_GRAPH();
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, 2);
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, 1);
    wait_num = dep->fire_condition(false);
    ASSERT_EQ(wait_num, 0);
    closure_context->wait_finish();
}

// ================= 只有data，没有condition =================================
TEST_F(DependencyTest, test_case21) {
    std::cout << "------------------ case21 ----------------\n";
    INIT_GRAPH_ONLY_DATA();
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, 1);
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, 0);
    closure_context->wait_finish();
}

TEST_F(DependencyTest, test_case22) {
    std::cout << "------------------ case22 ----------------\n";
    INIT_GRAPH_ONLY_DATA();
    wait_num = dep->fire_data();
    ASSERT_EQ(wait_num, -1);
    wait_num = dep->activate(actived_vertexs, v->get_closure_context());
    ASSERT_EQ(wait_num, 0);
    closure_context->wait_finish();
}