#pragma once


#include "rexpr.hpp"

#include <boost/spirit/home/x3/support/ast/position_tagged.hpp>
#include <boost/spirit/home/x3/support/utility/error_reporting.hpp>

#include <map>

namespace rexpr::parser {

namespace x3 = boost::spirit::x3;

////////////////////////////////////////////////////////////////////////////
//  Our error handler
////////////////////////////////////////////////////////////////////////////
// X3 Error Handler Utility
template <typename Iterator>
using error_handler = x3::error_handler<Iterator>;

// tag used to get our error handler from the context
using error_handler_tag = x3::error_handler_tag;

struct ErrorHandlerBase {
    ErrorHandlerBase();

    template <typename Iterator, typename Exception, typename Context>
    x3::error_handler_result on_error(Iterator& first, 
                                        Iterator const& last, 
                                        Exception const& x, 
                                        Context const& context);

    std::map<std::string, std::string> id_map;
};

////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////

inline ErrorHandlerBase::ErrorHandlerBase() {
    id_map["rexpr"] = "RExpression";
    id_map["rexpr_value"] = "Value";
    id_map["rexpr_key_value"] = "Key value pair";
}

template <typename Iterator, typename Exception, typename Context>
inline x3::error_handler_result
ErrorHandlerBase::on_error(Iterator& first, 
                             Iterator const& last, 
                             Exception const& x, 
                             Context const& context) {
    std::string which = x.which();
    auto iter = id_map.find(which);
    if (iter != id_map.end()) {
        which = iter->second;
    }

    std::string message = "Error! Expecting: " + which + " here:";
    auto& error_handler = x3::get<error_handler_tag>(context).get();
    error_handler(x.where(), message);
    return x3::error_handler_result::fail;
}

} // namespace rexpr::parser
