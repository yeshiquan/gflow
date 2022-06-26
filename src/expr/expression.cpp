#include "expression.hpp"
#include "expression_def.hpp"
#include <string>

namespace client {
namespace parser {

using iterator_type = std::string::const_iterator;
template struct client::parser::expression<iterator_type>;
}
}