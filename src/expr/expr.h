#pragma once

#include "expression.hpp"
#include "vm.hpp"
#include "compiler.hpp"
#include <unordered_map>
#include "var_analyzer.h"

namespace gflow {

class Expr {
   public:
    using iterator_type = std::string::const_iterator;

    bool evaluate(const std::string& expr_string,
                  std::unordered_map<std::string, int>* var_values,
                  int& result) {
        client::vmachine vm;                // Our virtual machine
        client::code_gen::program program;  // Our VM program
        client::ast::FinalResult ast;       // Our AST
        iterator_type iter = expr_string.begin();
        iterator_type end = expr_string.end();
        client::error_handler<iterator_type> error_handler(
            iter, end);  // Our error handler
        client::code_gen::compiler compile(program, error_handler,
                                           var_values);  // Our compiler

        if (!parse(expr_string, ast)) {
            return false;
        }

        if (compile.start(ast)) {
            vm.execute(program());
            // program.print_assembler();
            result = vm.get_result();
        }

        return true;
    }

    bool evaluate(const std::string& expr_string,
                  std::unordered_map<std::string, int>* var_values,
                  client::ast::FinalResult& ast, int& result) {
        client::vmachine vm;                // Our virtual machine
        client::code_gen::program program;  // Our VM program
        iterator_type iter = expr_string.begin();
        iterator_type end = expr_string.end();
        client::error_handler<iterator_type> error_handler(
            iter, end);  // Our error handler
        client::code_gen::compiler compile(program, error_handler,
                                           var_values);  // Our compiler

        if (!parse(expr_string, ast)) {
            return false;
        }

        if (compile.start(ast)) {
            vm.execute(program());
            // program.print_assembler();
            result = vm.get_result();
        }

        return true;
    }

    bool parse(const std::string& expr_string, client::ast::FinalResult& ast) {
        iterator_type iter = expr_string.begin();
        iterator_type end = expr_string.end();

        client::error_handler<iterator_type> error_handler(
            iter, end);  // Our error handler
        client::parser::expression<iterator_type> parser(
            error_handler);  // Our parser

        boost::spirit::ascii::space_type space;
        bool success = phrase_parse(iter, end, parser, space, ast);

        if (success && iter == end) {
            return true;
        } else {
            return false;
        }

        return true;
    }

    bool var_analyze(client::ast::FinalResult& ast,
                     std::vector<std::string>* var_names) {
        client::code_gen::VarAnalyzer analyzer(var_names);
        return analyzer.analyze(ast);
    }
};

}  // namespace gflow
