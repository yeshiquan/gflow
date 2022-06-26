#pragma once


#include "error_handler.hpp"

#include <boost/spirit/home/x3.hpp>

namespace rexpr::parser {

// Our Iterator Type
using IteratorType = std::string::const_iterator ;

// The Phrase Parse Context
using PhraseContextType = x3::phrase_parse_context<x3::ascii::space_type>::type
;

// Our Error Handler
using ErrorHandlerType = error_handler<IteratorType> ;

// Combined Error Handler and Phrase Parse Context
using ContextType = x3::context<error_handler_tag, 
                                 std::reference_wrapper<ErrorHandlerType>, 
                                 PhraseContextType>;

}

