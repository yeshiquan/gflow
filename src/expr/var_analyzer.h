#pragma once

#include "ast.hpp"
#include "error_handler.hpp"
#include <vector>
#include <map>
#include <unordered_map>
#include <boost/function.hpp>
#include <boost/phoenix/core.hpp>
#include <boost/phoenix/function.hpp>
#include <boost/phoenix/operator.hpp>

namespace client {
namespace code_gen {

struct VarAnalyzer {
    typedef bool result_type;
    VarAnalyzer(std::vector<std::string> *varnames);

    bool operator()(ast::nil) const;
    bool operator()(unsigned int x) const;
    bool operator()(bool x) const;
    bool operator()(std::string const &x) const;
    bool operator()(ast::operation const &x) const;
    bool operator()(ast::unary const &x) const;
    bool operator()(ast::expression const &x) const;
    bool operator()(ast::condition const &x) const;

    bool analyze(ast::FinalResult const &x) const;

    std::vector<std::string> *variable_names = nullptr;
};
}
}