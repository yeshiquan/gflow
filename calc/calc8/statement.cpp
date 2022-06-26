#include "statement_def.hpp"
#include "config.hpp"

namespace client {
namespace parser {
BOOST_SPIRIT_INSTANTIATE(statement_type, iterator_type, context_type);
}
}