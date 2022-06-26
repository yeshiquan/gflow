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

#include "expr/expr.h"
#include "expr/ast.hpp"
#include "expr/var_analyzer.h"
#include "expr_processor.h"

using namespace gflow;

class ExprTest : public ::testing::Test {
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

TEST_F(ExprTest, test_expr) {
    std::string expr_string = "if (A > B) C+2 else D*3";
    std::unordered_map<std::string, int> variables;
    variables.emplace("A", 5);
    variables.emplace("B", 6);
    variables.emplace("C", 7);
    variables.emplace("D", 8);
    gflow::Expr expr;
    int result;
    bool ret = expr.evaluate(expr_string, &variables, result);
    std::cout << expr_string << " -> result:" << result << std::endl;
    ASSERT_EQ(result, 24);
}

TEST_F(ExprTest, test_var_analyzer) {
    std::string expr_string = "if (A > B) C+2 else D*3";
    client::ast::FinalResult ast;
    gflow::Expr expr;
    std::vector<std::string> varnames;
    bool ret = expr.parse(expr_string, ast);
    ASSERT_TRUE(ret);
    ret = expr.var_analyze(ast, &varnames);
    ASSERT_TRUE(ret);
    // expect:  A B C D
    for (auto& var : varnames) {
        std::cout << "var -> " << var << std::endl;
    }
    ASSERT_EQ(varnames.size(), 4);
}