#if !defined(BOOST_SPIRIT_X3_CALC8_STATEMENT_HPP)
#define BOOST_SPIRIT_X3_CALC8_STATEMENT_HPP

#include <boost/spirit/home/x3.hpp>
#include "ast.hpp"

namespace client {
namespace x3 = boost::spirit::x3;
namespace parser {
struct statement_class;
typedef x3::rule<statement_class, ast::statement_list> statement_type;
BOOST_SPIRIT_DECLARE(statement_type);
}

parser::statement_type const &statement();
}

#endif