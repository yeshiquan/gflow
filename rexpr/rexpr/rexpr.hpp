#include "ast.hpp"

#include <boost/spirit/home/x3.hpp>

namespace rexpr::parser {

namespace x3 = boost::spirit::x3;

///////////////////////////////////////////////////////////////////////////
// rexpr public interface
///////////////////////////////////////////////////////////////////////////
struct RexprClass;
using RexprType = x3::rule<RexprClass, rexpr::ast::Rexpr>;
BOOST_SPIRIT_DECLARE(RexprType);


} // namespace rexpr::parser


namespace rexpr {

rexpr::parser::RexprType const& rexpr();

} // namespace rexpr
