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
///////////////////////////////////////////////////////////////////////////
//  The Program
///////////////////////////////////////////////////////////////////////////
struct program {
    void op(int a);
    void op(int a, int b);
    void op(int a, int b, int c);

    int &operator[](std::size_t i) { return code[i]; }
    int const &operator[](std::size_t i) const { return code[i]; }
    void clear() {
        code.clear();
        variables.clear();
    }
    std::size_t size() const { return code.size(); }
    std::vector<int> const &operator()() const { return code; }

    void print_assembler() const;

   private:
    std::map<std::string, int> variables;
    std::vector<int> code;
};

///////////////////////////////////////////////////////////////////////////
//  The Compiler
///////////////////////////////////////////////////////////////////////////
struct compiler {
    typedef bool result_type;

    template <typename ErrorHandler>
    compiler(client::code_gen::program &program, ErrorHandler &error_handler_,
             std::unordered_map<std::string, int> *var_values)
        : program(program), var_values(var_values) {
        using namespace boost::phoenix::arg_names;
        namespace phx = boost::phoenix;
        using boost::phoenix::function;

        error_handler = function<ErrorHandler>(error_handler_)(
            "Error! ", _2, phx::cref(error_handler_.iters)[_1]);
    }

    bool operator()(ast::nil) const {
        BOOST_ASSERT(0);
        return false;
    }
    bool operator()(unsigned int x) const;
    bool operator()(bool x) const;
    bool operator()(std::string const &x) const;
    bool operator()(ast::operation const &x) const;
    bool operator()(ast::unary const &x) const;
    bool operator()(ast::expression const &x) const;
    bool operator()(ast::condition const &x) const;

    bool start(ast::FinalResult const &x) const;

    client::code_gen::program &program;
    std::unordered_map<std::string, int> *var_values;
    boost::function<void(int tag, std::string const &what)> error_handler;
};
}
}