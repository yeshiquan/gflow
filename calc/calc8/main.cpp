#include "ast.hpp"
#include "vm.hpp"
#include "compiler.hpp"
#include "statement.hpp"
#include "error_handler.hpp"
#include "config.hpp"
#include <iostream>

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main() {
    std::cout
        << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Statement parser...\n\n";
    std::cout
        << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type some statements... ";
    std::cout
        << "An empty line ends input, compiles, runs and prints results\n\n";
    std::cout << "Example:\n\n";
    std::cout << "    var a = 123;\n";
    std::cout << "    var b = 456;\n";
    std::cout << "    var c = a + b * 2;\n\n";
    std::cout << "-------------------------\n";

    std::string str;
    std::string source;
    while (std::getline(std::cin, str)) {
        if (str.empty()) break;
        source += str + '\n';
    }

    using client::parser::iterator_type;
    iterator_type iter(source.begin());
    iterator_type end(source.end());

    client::vmachine vm;                // Our virtual machine
    client::code_gen::program program;  // Our VM program
    client::ast::statement_list ast;    // Our AST

    using boost::spirit::x3::with;
    using client::parser::error_handler_type;
    using client::parser::error_handler_tag;
    error_handler_type error_handler(iter, end,
                                     std::cerr);  // Our error handler

    // Our compiler
    client::code_gen::compiler compile(program, error_handler);

    // Our parser
    auto const parser =
        // we pass our error handler to the parser so we can access
        // it later on in our on_error and on_sucess handlers
        with<error_handler_tag>(std::ref(error_handler))[client::statement()];

    using boost::spirit::x3::ascii::space;
    bool success = phrase_parse(iter, end, parser, space, ast);

    std::cout << "-------------------------\n";

    if (success && iter == end) {
        if (compile(ast)) {
            std::cout << "Success\n";
            std::cout << "-------------------------\n";
            vm.execute(program());

            std::cout << "-------------------------\n";
            std::cout << "Assembler----------------\n\n";
            program.print_assembler();

            std::cout << "-------------------------\n";
            std::cout << "Results------------------\n\n";
            program.print_variables(vm.get_stack());
        } else {
            std::cout << "Compile failure\n";
        }
    } else {
        std::cout << "Parse failure\n";
    }

    std::cout << "-------------------------\n\n";
    return 0;
}