#include "compiler.hpp"
#include "vm.hpp"
#include <boost/variant/apply_visitor.hpp>
#include <boost/assert.hpp>
#include <boost/lexical_cast.hpp>
#include <set>

namespace client {
namespace code_gen {
void program::op(int a) { code.push_back(a); }

void program::op(int a, int b) {
    code.push_back(a);
    code.push_back(b);
}

void program::op(int a, int b, int c) {
    code.push_back(a);
    code.push_back(b);
    code.push_back(c);
}

void program::print_assembler() const {
    std::vector<int>::const_iterator pc = code.begin();

    std::map<std::size_t, std::string> lines;
    std::set<std::size_t> jumps;

    while (pc != code.end()) {
        std::string line;
        std::size_t address = pc - code.begin();

        switch (*pc++) {
            case op_neg:
                line += "      op_neg";
                break;

            case op_not:
                line += "      op_not";
                break;

            case op_add:
                line += "      op_add";
                break;

            case op_sub:
                line += "      op_sub";
                break;

            case op_mul:
                line += "      op_mul";
                break;

            case op_div:
                line += "      op_div";
                break;

            case op_eq:
                line += "      op_eq";
                break;

            case op_neq:
                line += "      op_neq";
                break;

            case op_lt:
                line += "      op_lt";
                break;

            case op_lte:
                line += "      op_lte";
                break;

            case op_gt:
                line += "      op_gt";
                break;

            case op_gte:
                line += "      op_gte";
                break;

            case op_and:
                line += "      op_and";
                break;

            case op_or:
                line += "      op_or";
                break;

            case op_int:
                line += "      op_int      ";
                line += boost::lexical_cast<std::string>(*pc++);
                break;

            case op_true:
                line += "      op_true";
                break;

            case op_false:
                line += "      op_false";
                break;

            case op_jump: {
                line += "      op_jump     ";
                std::size_t pos = (pc - code.begin()) + *pc++;
                if (pos == code.size())
                    line += "end";
                else
                    line += boost::lexical_cast<std::string>(pos);
                jumps.insert(pos);
            } break;

            case op_jump_if: {
                line += "      op_jump_if  ";
                std::size_t pos = (pc - code.begin()) + *pc++;
                if (pos == code.size())
                    line += "end";
                else
                    line += boost::lexical_cast<std::string>(pos);
                jumps.insert(pos);
            } break;

            case op_stk_adj:
                line += "      op_stk_adj  ";
                line += boost::lexical_cast<std::string>(*pc++);
                break;
        }
        lines[address] = line;
    }

    std::cout << "start:" << std::endl;
    typedef std::pair<std::size_t, std::string> line_info;
    for (line_info const &l : lines) {
        std::size_t pos = l.first;
        if (jumps.find(pos) != jumps.end())
            std::cout << pos << ':' << std::endl;
        std::cout << l.second << std::endl;
    }

    std::cout << "end:" << std::endl;
}

bool compiler::operator()(unsigned int x) const {
    program.op(op_int, x);
    return true;
}

bool compiler::operator()(bool x) const {
    program.op(x ? op_true : op_false);
    return true;
}

bool compiler::operator()(std::string const &x) const {
    auto iter = var_values->find(x);
    if (iter != var_values->end()) {
        auto var_value = iter->second;
        program.op(op_int, var_value);
        return true;
    }
    return false;
}

bool compiler::operator()(ast::operation const &x) const {
    if (!boost::apply_visitor(*this, x.operand_)) return false;
    switch (x.operator_) {
        case ast::op_plus:
            program.op(op_add);
            break;
        case ast::op_minus:
            program.op(op_sub);
            break;
        case ast::op_times:
            program.op(op_mul);
            break;
        case ast::op_divide:
            program.op(op_div);
            break;

        case ast::op_equal:
            program.op(op_eq);
            break;
        case ast::op_not_equal:
            program.op(op_neq);
            break;
        case ast::op_less:
            program.op(op_lt);
            break;
        case ast::op_less_equal:
            program.op(op_lte);
            break;
        case ast::op_greater:
            program.op(op_gt);
            break;
        case ast::op_greater_equal:
            program.op(op_gte);
            break;

        case ast::op_and:
            program.op(op_and);
            break;
        case ast::op_or:
            program.op(op_or);
            break;
        default:
            BOOST_ASSERT(0);
            return false;
    }
    return true;
}

bool compiler::operator()(ast::unary const &x) const {
    if (!boost::apply_visitor(*this, x.operand_)) return false;
    switch (x.operator_) {
        case ast::op_negative:
            program.op(op_neg);
            break;
        case ast::op_not:
            program.op(op_not);
            break;
        case ast::op_positive:
            break;
        default:
            BOOST_ASSERT(0);
            return false;
    }
    return true;
}

bool compiler::operator()(ast::expression const &x) const {
    if (!boost::apply_visitor(*this, x.first)) return false;
    for (ast::operation const &oper : x.rest) {
        if (!(*this)(oper)) return false;
    }
    return true;
}

bool compiler::operator()(ast::condition const &x) const {
    if (!boost::apply_visitor(*this, x.cond)) return false;

    program.op(op_jump_if, 0);
    std::size_t skip = program.size() - 1;
    if (!boost::apply_visitor(*this, x.expr1)) return false;
    program[skip] = program.size() - skip;

    program[skip] += 2;
    program.op(op_jump, 0);
    std::size_t exit = program.size() - 1;
    if (!boost::apply_visitor(*this, x.expr2)) return false;
    program[exit] = program.size() - exit;

    return true;
}

bool compiler::start(ast::FinalResult const &x) const {
    // return (*this)(x);
    if (!boost::apply_visitor(*this, x)) return false;
    return true;
}
}
}
