#pragma once

#include "ast.hpp"
#include "ast_adapted.hpp"
#include "error_handler.hpp"
#include "rexpr.hpp"

#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/utility/annotate_on_success.hpp>

namespace rexpr::parser {

namespace x3 = boost::spirit::x3;
namespace ascii = boost::spirit::x3::ascii;

using x3::lit;
using x3::lexeme;

using ascii::char_;
using ascii::string;

///////////////////////////////////////////////////////////////////////////
// Rule IDs
///////////////////////////////////////////////////////////////////////////

struct RexprValueClass;
struct RexprKeyValueClass;
struct RexprInnerClass;

///////////////////////////////////////////////////////////////////////////
// Rules
///////////////////////////////////////////////////////////////////////////

x3::rule<RexprValueClass, rexpr::ast::RexpValue> const rexpr_value = "rexpr_value";

x3::rule<RexprKeyValueClass, rexpr::ast::RexprKeyValue> const rexpr_key_value = "rexpr_key_value";

x3::rule<RexprInnerClass, rexpr::ast::Rexpr> const rexpr_inner = "rexpr";

x3::rule<RexprClass, rexpr::ast::Rexpr> const rexpr = "rexpr";

///////////////////////////////////////////////////////////////////////////
// Grammar
///////////////////////////////////////////////////////////////////////////

auto const quoted_string = lexeme['"' >> *(char_ - '"') >> '"'];

auto const rexpr_value_def = quoted_string | rexpr_inner;

auto const rexpr_key_value_def = quoted_string > '=' > rexpr_value;

auto const rexpr_inner_def = '{' > *rexpr_key_value > '}';

auto const rexpr_def = rexpr_inner_def;

BOOST_SPIRIT_DEFINE(rexpr_value, rexpr, rexpr_inner, rexpr_key_value);

///////////////////////////////////////////////////////////////////////////
// Annotation and Error handling
///////////////////////////////////////////////////////////////////////////

// We want these to be annotated with the iterator position.
struct RexprValueClass : x3::annotate_on_success {};
struct RexprKeyValueClass : x3::annotate_on_success {};
struct RexprInnerClass : x3::annotate_on_success {};

// We want error-handling only for the start (outermost) rexpr
// rexpr is the same as rexpr_inner but without error-handling (see error_handler.hpp)
struct RexprClass : x3::annotate_on_success, ErrorHandlerBase {};

} // namesapce rexpr::parser

namespace rexpr {

rexpr::parser::RexprType const& rexpr() {
    return rexpr::parser::rexpr;
}

} // namespace rexpr