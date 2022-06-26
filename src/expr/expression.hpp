#pragma once

#include <boost/spirit/include/qi.hpp>
#include "ast.hpp"
#include "error_handler.hpp"
#include <vector>

namespace client {
namespace parser {
namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;

///////////////////////////////////////////////////////////////////////////////
//  The expression grammar
///////////////////////////////////////////////////////////////////////////////
template <typename Iterator>
struct expression
    : qi::grammar<Iterator, ast::FinalResult(), ascii::space_type> {
    expression(error_handler<Iterator> &error_handler);

    qi::rule<Iterator, ast::FinalResult(), ascii::space_type> expr;
    qi::rule<Iterator, ast::condition(), ascii::space_type> condition_expr;
    qi::rule<Iterator, ast::expression(), ascii::space_type> equality_expr,
        relational_expr, logical_expr, additive_expr, multiplicative_expr;
    qi::rule<Iterator, ast::operand(), ascii::space_type> unary_expr;
    qi::rule<Iterator, ast::operand(), ascii::space_type> primary_expr;
    qi::rule<Iterator, std::string(), ascii::space_type> identifier;
    qi::symbols<char, ast::optoken> equality_op, relational_op, logical_op,
        additive_op, multiplicative_op, unary_op;
    qi::symbols<char> keywords;
};
}
}