#pragma once


#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>

#include <map>

namespace rexpr::ast {

///////////////////////////////////////////////////////////////////////////
//  The AST
///////////////////////////////////////////////////////////////////////////
namespace x3 = boost::spirit::x3;

struct Rexpr;

struct RexpValue : x3::variant<std::string, x3::forward_ast<Rexpr>> {
    using base_type::base_type;
    using base_type::operator=;
};

using RexprKeyValue = std::pair<std::string, RexpValue> ;

struct Rexpr : x3::position_tagged {
    std::map<std::string, RexpValue> entries;
};


}

