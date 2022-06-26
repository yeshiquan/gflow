#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/home/x3/support/ast/variant.hpp>
#include <boost/fusion/include/adapt_struct.hpp>

#include <iostream>
#include <string>
#include <list>
#include <numeric>

namespace x3 = boost::spirit::x3;

namespace client {
///////////////////////////////////////////////////////////////////////////////
//  The calculator grammar
///////////////////////////////////////////////////////////////////////////////
namespace calculator_grammar {
using x3::uint_;
using x3::char_;

x3::rule<class expression> const expression("expression");
x3::rule<class term> const term("term");
x3::rule<class factor> const factor("factor");

auto const expression_def = term >> *(('+' >> term) | ('-' >> term));

auto const term_def = factor >> *(('*' >> factor) | ('/' >> factor));

auto const factor_def = uint_ | '(' >> expression >> ')' | ('-' >> factor) |
                        ('+' >> factor);

BOOST_SPIRIT_DEFINE(expression, term, factor);

auto calculator = expression;
}

using calculator_grammar::calculator;
}

///////////////////////////////////////////////////////////////////////////////
//  Main program
///////////////////////////////////////////////////////////////////////////////
int main() {
    std::cout
        << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Expression parser...\n\n";
    std::cout
        << "/////////////////////////////////////////////////////////\n\n";
    std::cout << "Type an expression...or [q or Q] to quit\n\n";

    typedef std::string::const_iterator iterator_type;

    std::string str;
    while (std::getline(std::cin, str)) {
        if (str.empty() || str[0] == 'q' || str[0] == 'Q') break;

        auto &calc = client::calculator;  // Our grammar

        iterator_type iter = str.begin();
        iterator_type end = str.end();
        boost::spirit::x3::ascii::space_type space;
        bool r = phrase_parse(iter, end, calc, space);

        if (r && iter == end) {
            std::cout << "-------------------------\n";
            std::cout << "Parsing succeeded\n";
            std::cout << "-------------------------\n";
        } else {
            std::string rest(iter, end);
            std::cout << "-------------------------\n";
            std::cout << "Parsing failed\n";
            std::cout << "stopped at: \"" << rest << "\"\n";
            std::cout << "-------------------------\n";
        }
    }

    std::cout << "Bye... :-) \n\n";
    return 0;
}