#pragma once

#if !defined(BOOST_SPIRIT_X3_CALC9_STATEMENT_DEF_HPP)
#define BOOST_SPIRIT_X3_CALC9_STATEMENT_DEF_HPP

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>
#include "ast.hpp"
#include "ast_adapted.hpp"
#include "expression.hpp"
#include "common.hpp"
#include "error_handler.hpp"
#include "config.hpp"

namespace client {
namespace parser {
using x3::raw;
using x3::lexeme;
using namespace x3::ascii;

struct statement_list_class;
struct variable_declaration_class;
struct assignment_class;
struct variable_class;
struct statement_class;
struct if_statement_class;

typedef x3::rule<statement_list_class, ast::statement_list> statement_list_type;
typedef x3::rule<variable_declaration_class, ast::variable_declaration>
    variable_declaration_type;
typedef x3::rule<assignment_class, ast::assignment> assignment_type;
typedef x3::rule<variable_class, ast::variable> variable_type;
typedef x3::rule<statement_class, ast::statement> statement_type;
typedef x3::rule<if_statement_class, ast::if_statement> if_statement_type;

statement_type const statement("statement");
statement_list_type const statement_list("statement_list");
variable_declaration_type const variable_declaration("variable_declaration");
assignment_type const assignment("assignment");
if_statement_type const if_statement("if_statement");
variable_type const variable("variable");

// Import the expression rule
namespace {
auto const& expression = client::expression();
}

auto const statement_list_def = +(statement);

auto const statement_def = if_statement | variable_declaration | assignment;

auto const variable_declaration_def =
    lexeme["var" >> !(alnum | '_')]  // make sure we have whole words
    > assignment;

auto const assignment_def = variable > '=' > expression > ';';

auto const variable_def = identifier;

auto const if_statement_def = lit("if") >> '(' >> expression >> ')' >>
                              statement >>
                              -(lexeme["else" >> !(alnum | '_')] >> statement);

BOOST_SPIRIT_DEFINE(statement, statement_list, if_statement,
                    variable_declaration, assignment, variable);

struct statement_list_class : error_handler_base, x3::annotate_on_success {};
struct statement_class : x3::annotate_on_success {};
struct assignment_class : x3::annotate_on_success {};
struct variable_class : x3::annotate_on_success {};
struct if_statement_class : x3::annotate_on_success {};

parser::statement_list_type const& get_parser() {
    return parser::statement_list;
}
}
}

#endif