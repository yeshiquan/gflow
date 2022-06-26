#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <iostream>
#include <string>
#include <list>
#include <numeric>
#include <map>

namespace x3 = boost::spirit::x3;

namespace client {
namespace ast {
///////////////////////////////////////////////////////////////////////////
//  The AST
///////////////////////////////////////////////////////////////////////////
struct nil {};
struct Signed_;
struct Program;

struct Operand
    : x3::variant<nil, unsigned int, double, std::string,
                  x3::forward_ast<Signed_>, x3::forward_ast<Program>> {
    using base_type::base_type;
    using base_type::operator=;
};

struct Signed_ {
    char sign;
    Operand operand_;
};

struct Operation {
    char operator_;
    Operand operand_;
};

struct Program {
    Operand first;
    std::list<Operation> rest;
};
}
}

BOOST_FUSION_ADAPT_STRUCT(client::ast::Signed_, sign, operand_)

BOOST_FUSION_ADAPT_STRUCT(client::ast::Operation, operator_, operand_)

BOOST_FUSION_ADAPT_STRUCT(client::ast::Program, first, rest)

namespace client {
namespace ast {
///////////////////////////////////////////////////////////////////////////
//  The AST Printer
///////////////////////////////////////////////////////////////////////////
struct Printer {
    typedef void result_type;

    void operator()(nil) const {}
    void operator()(unsigned int n) const { std::cout << n; }
    void operator()(double n) const { std::cout << n; }

    void operator()(const std::string &var) const { std::cout << var; }

    void operator()(Operation const &x) const {
        boost::apply_visitor(*this, x.operand_);
        switch (x.operator_) {
            case '+':
                std::cout << " add";
                break;
            case '-':
                std::cout << " subt";
                break;
            case '*':
                std::cout << " mult";
                break;
            case '/':
                std::cout << " div";
                break;
        }
    }

    void operator()(Signed_ const &x) const {
        boost::apply_visitor(*this, x.operand_);
        switch (x.sign) {
            case '-':
                std::cout << " neg";
                break;
            case '+':
                std::cout << " pos";
                break;
        }
    }

    void operator()(Program const &x) const {
        boost::apply_visitor(*this, x.first);
        for (Operation const &oper : x.rest) {
            std::cout << ' ';
            (*this)(oper);
        }
    }
};

///////////////////////////////////////////////////////////////////////////
//  The AST evaluator
///////////////////////////////////////////////////////////////////////////
struct Eval {
    typedef double result_type;
    Eval(std::map<std::string, double> st) : _st(std::move(st)) {}

    int operator()(nil) const {
        BOOST_ASSERT(0);
        return 0;
    }
    int operator()(unsigned int n) const { return n; }
    double operator()(double n) const { return n; }

    double operator()(const std::string &var) const {
        auto iter = _st.find(var);
        if (iter == _st.end()) {
            return 0.0;
        }
        return iter->second;
    }

    double operator()(double lhs, Operation const &x) const {
        double rhs = boost::apply_visitor(*this, x.operand_);
        switch (x.operator_) {
            case '+':
                return lhs + rhs;
            case '-':
                return lhs - rhs;
            case '*':
                return lhs * rhs;
            case '/':
                return lhs / rhs;
        }
        BOOST_ASSERT(0);
        return 0;
    }

    double operator()(Signed_ const &x) const {
        double rhs = boost::apply_visitor(*this, x.operand_);
        switch (x.sign) {
            case '-':
                return -rhs;
            case '+':
                return +rhs;
        }
        BOOST_ASSERT(0);
        return 0;
    }

    double operator()(Program const &x) const {
        return std::accumulate(x.rest.begin(), x.rest.end(),
                               boost::apply_visitor(*this, x.first), *this);
    }
    std::map<std::string, double> _st;
};
}
}

namespace client {
///////////////////////////////////////////////////////////////////////////////
//  The calculator grammar
///////////////////////////////////////////////////////////////////////////////
namespace calculator_grammar {
using x3::uint_;
using x3::char_;
using x3::double_;

x3::rule<class expression, ast::Program> const expression("expression");
x3::rule<class term, ast::Program> const term("term");
x3::rule<class factor, ast::Operand> const factor("factor");
x3::rule<class variable, std::string> const variable("variable");

auto const expression_def = term >>
                            *((char_('+') >> term) | (char_('-') >> term));

auto const term_def = factor >>
                      *((char_('*') >> factor) | (char_('/') >> factor));
auto const variable_def = x3::raw[x3::lexeme[x3::alpha >> *(x3::alnum | '_')]];
auto const factor_def = uint_ | double_ | variable | '(' >> expression >> ')' |
                        (char_('-') >> factor) | (char_('+') >> factor);

BOOST_SPIRIT_DEFINE(expression, term, factor, variable);

auto calculator = expression;
}

using calculator_grammar::calculator;
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main() {
    std::cout
        << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Expression parser...\n\n";
    std::cout
        << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    typedef std::string::const_iterator iterator_type;
    typedef client::ast::Program ast_program;
    typedef client::ast::Printer ast_print;
    typedef client::ast::Eval ast_eval;

    std::string str;
    while (std::getline(std::cin, str)) {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q') {
            break;
        }

        std::map<std::string, double> st;
        st.emplace("a", 3);
        st.emplace("b", 4);
        st.emplace("c", 5);

        auto &calc = client::calculator;  // Our grammar
        ast_program program;              // Our program (AST)
        ast_print print;                  // Prints the program
        ast_eval eval(st);                // Evaluates the program

        iterator_type iter = str.begin();
        iterator_type end = str.end();
        boost::spirit::x3::ascii::space_type space;
        bool r = phrase_parse(iter, end, calc, space, program);

        if (r && iter == end) {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            print(program);
            std::cout << "\nResult: " << eval(program) << std::endl;
            std::cout << "-------------------------\n";
        } else {
            std::string rest(iter, end);
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "stopped at: \"" << rest << "\"\n";
            std::cout << "-------------------------\n";
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}