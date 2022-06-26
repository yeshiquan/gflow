#pragma once

#include <boost/variant/recursive_variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <boost/optional.hpp>
#include <list>

namespace client {
namespace ast {
///////////////////////////////////////////////////////////////////////////
//  The AST
///////////////////////////////////////////////////////////////////////////
struct tagged {
    int id;  // Used to annotate the AST with the iterator position.
             // This id is used as a key to a map<int, Iterator>
             // (not really part of the AST.)
};

struct nil {};
struct unary;
struct expression;
struct condition;

struct variable : tagged {
    variable(std::string const &name = "") : name(name) {}
    std::string name;
};

using operand = boost::variant<
    bool, unsigned int, std::string, boost::recursive_wrapper<unary>,
    boost::recursive_wrapper<expression>, boost::recursive_wrapper<condition>>;

using FinalResult = boost::variant<boost::recursive_wrapper<expression>,
                                   boost::recursive_wrapper<condition>>;

enum optoken {
    op_plus,
    op_minus,
    op_times,
    op_divide,
    op_positive,
    op_negative,
    op_not,
    op_equal,
    op_not_equal,
    op_less,
    op_less_equal,
    op_greater,
    op_greater_equal,
    op_and,
    op_or
};

struct unary {
    optoken operator_;
    operand operand_;
};

struct operation {
    optoken operator_;
    operand operand_;
};

struct expression {
    operand first;
    std::list<operation> rest;
};

struct TwoExpr {
    operand expr1;
    operand expr2;
};

struct condition {
    operand cond;
    operand expr1;
    operand expr2;
};

// print functions for debugging
inline std::ostream &operator<<(std::ostream &out, nil) {
    out << "nil";
    return out;
}
inline std::ostream &operator<<(std::ostream &out, variable const &var) {
    out << var.name;
    return out;
}
}
}

BOOST_FUSION_ADAPT_STRUCT(client::ast::unary,
                          (client::ast::optoken,
                           operator_)(client::ast::operand, operand_))

BOOST_FUSION_ADAPT_STRUCT(client::ast::operation,
                          (client::ast::optoken,
                           operator_)(client::ast::operand, operand_))

BOOST_FUSION_ADAPT_STRUCT(client::ast::expression,
                          (client::ast::operand,
                           first)(std::list<client::ast::operation>, rest))

BOOST_FUSION_ADAPT_STRUCT(client::ast::TwoExpr,
                          (client::ast::operand, expr1)(client::ast::operand,
                                                        expr2))

BOOST_FUSION_ADAPT_STRUCT(client::ast::condition,
                          (client::ast::operand,
                           cond)(client::ast::operand,
                                 expr1)(client::ast::operand, expr2))
