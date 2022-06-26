#include "ast.hpp"
#include "var_analyzer.h"

namespace client::code_gen {

    VarAnalyzer::VarAnalyzer(std::vector<std::string> * varnames)
        : variable_names(varnames) {}

    bool VarAnalyzer::operator()(ast::nil) const {
        BOOST_ASSERT(0);
        return false;
    }
    bool VarAnalyzer::operator()(unsigned int x) const { return true; }
    bool VarAnalyzer::operator()(bool x) const { return true; }

    bool VarAnalyzer::operator()(std::string const &x) const {
        variable_names->emplace_back(x);
        return true;
    }
    bool VarAnalyzer::operator()(ast::operation const &x) const {
        return boost::apply_visitor(*this, x.operand_);
    }
    bool VarAnalyzer::operator()(ast::unary const &x) const {
        return boost::apply_visitor(*this, x.operand_);
    }
    bool VarAnalyzer::operator()(ast::expression const &x) const {
        if (!boost::apply_visitor(*this, x.first)) return false;
        for (ast::operation const &oper : x.rest) {
            if (!(*this)(oper)) return false;
        }
        return true;
    }
    bool VarAnalyzer::operator()(ast::condition const &x) const {
        if (!boost::apply_visitor(*this, x.cond)) return false;
        if (!boost::apply_visitor(*this, x.expr1)) return false;
        if (!boost::apply_visitor(*this, x.expr2)) return false;
        return true;
    }

    bool VarAnalyzer::analyze(ast::FinalResult const &x) const {
        return boost::apply_visitor(*this, x);
    }

}  // namespace client::code_gen