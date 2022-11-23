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

using namespace gflow;

class ClosureTest : public ::testing::Test {
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

TEST_F(ClosureTest, case1) {}