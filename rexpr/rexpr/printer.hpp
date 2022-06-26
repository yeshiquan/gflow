#pragma once

#include "ast.hpp"

#include <ostream>

namespace rexpr::ast {

///////////////////////////////////////////////////////////////////////////
//  Print out the rexpr tree
///////////////////////////////////////////////////////////////////////////
int const tabsize = 4;

struct RexprPrinter {
    using result_type = void ;

    RexprPrinter(std::ostream& out, int indent = 0)
        : out(out), indent(indent) {}

    void operator()(rexpr::ast::Rexpr const& ast) const {
        out << '{' << std::endl;
        for (auto const& entry : ast.entries) {
            tab(indent+tabsize);
            out << '"' << entry.first << "\" = ";
            RexprPrinter next_printer(out, indent+tabsize);
            boost::apply_visitor(next_printer, entry.second);
        }
        tab(indent);
        out << '}' << std::endl;
    }

    void operator()(std::string const& text) const {
        out << '"' << text << '"' << std::endl;
    }

    void tab(int spaces) const {
        for (int i = 0; i < spaces; ++i)
            out << ' ';
    }

    std::ostream& out;
    int indent;
};

} // namespace rexpr::ast