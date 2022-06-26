#pragma once

#include "expr/ast.hpp"
#include "expr/expr.h"
#include "processor.h"
#include <vector>
#include <unordered_map>
#include "data.h"

namespace gflow {

class ExpressionProcessor : public GraphProcessor {
   public:
    void init(std::string expr_string_, std::string result_name);
    int setup();
    int process();

   private:
    std::string expr_string;
    client::ast::FinalResult ast;
    std::string result_name;
    std::vector<std::string> varnames;
    gflow::Expr expr;
    GraphData* _result_data = nullptr;
};

}  // namespace gflow