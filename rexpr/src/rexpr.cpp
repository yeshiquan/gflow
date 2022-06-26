#include "rexpr/config.hpp"
#include "rexpr/rexpr_def.hpp"

namespace rexpr::parser {

BOOST_SPIRIT_INSTANTIATE(RexprType, IteratorType, ContextType);

}