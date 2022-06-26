#include <iostream>
#include <iomanip>
#include <boost/spirit/home/x3.hpp>
namespace x3 = boost::spirit::x3;

namespace ast {
    using expression     = x3::unused_type;
    using unary_op       = x3::unused_type;
    using binary_op      = x3::unused_type;
    using conditional_op = x3::unused_type;
    using operand        = x3::unused_type;
}
namespace P {
    struct ehbase {
        template <typename It, typename Ctx>
        x3::error_handler_result on_error(It f, It l, x3::expectation_failure<It> const& e, Ctx const& /*ctx*/) const {
            std::cout << std::string(f,l) << "\n"
                      << std::setw(1+std::distance(f, e.where())) << "^"
                      << "-- expected: " << e.which() << "\n";
            return x3::error_handler_result::fail;
        }
    };
    struct expression_class     : ehbase {};
    struct logical_class        : ehbase {};
    struct equality_class       : ehbase {};
    struct relational_class     : ehbase {};
    struct additive_class       : ehbase {};
    struct multiplicative_class : ehbase {};
    struct factor_class         : ehbase {};
    struct primary_class        : ehbase {};
    struct unary_class          : ehbase {};
    struct binary_class         : ehbase {};
    struct conditional_class    : ehbase {};
    struct variable_class       : ehbase {};

    // Rule declarations
    auto const expression     = x3::rule<expression_class    , ast::expression    >{"expression"};
    auto const logical        = x3::rule<logical_class       , ast::expression    >{"logical"};
    auto const equality       = x3::rule<equality_class      , ast::expression    >{"equality"};
    auto const relational     = x3::rule<relational_class    , ast::expression    >{"relational"};
    auto const additive       = x3::rule<additive_class      , ast::expression    >{"additive"};
    auto const multiplicative = x3::rule<multiplicative_class, ast::expression    >{"multiplicative"};
    auto const factor         = x3::rule<factor_class        , ast::expression    >{"factor"};
    auto const primary        = x3::rule<primary_class       , ast::operand       >{"primary"};
    auto const unary          = x3::rule<unary_class         , ast::unary_op      >{"unary"};
    auto const binary         = x3::rule<binary_class        , ast::binary_op     >{"binary"};
    auto const conditional    = x3::rule<conditional_class   , ast::conditional_op>{"conditional"};
    auto const variable       = x3::rule<variable_class      , std::string        >{"variable"};

    template <typename Tag>
    static auto make_sym(std::initializer_list<char const*> elements) {
#ifdef DEBUG_SYMBOLS
        static x3::symbols<x3::unused_type> instance;
        static auto name = boost::core::demangle(typeid(Tag*).name());
        for (auto el : elements)
            instance += el;
        return x3::rule<Tag> {name.c_str()} = instance;
#else
        x3::symbols<x3::unused_type> instance;
        for (auto el : elements)
            instance += el;
        return instance;
#endif
    }

    static const auto logical_op        = make_sym<struct logical_op_>({"and","or","xor"});
    static const auto equality_op       = make_sym<struct equality_op_>({"=","!="});
    static const auto relational_op     = make_sym<struct relational_op_>({"<","<=",">",">="});
    static const auto additive_op       = make_sym<struct additive_op_>({"+","-"});
    static const auto multiplicative_op = make_sym<struct multiplicative_op_>({"*","/"});
    static const auto unary_op          = make_sym<struct unary_op_>({"!","-","~"}); // TODO FIXME interference with binop?
    static const auto ufunc             = make_sym<struct ufunc_>({"gamma","factorial","abs"});
    static const auto bfunc             = make_sym<struct bfunc_>({"pow","min","max"});
    static const auto constant          = make_sym<struct constant_>({"pi","e","tau"});
    static const auto variable_def      = make_sym<struct variable_def_>({"a","b","c","d","x","y","z"});

    // Rule defintions
    auto const expression_def =
        conditional
        ;

    auto const conditional_def =
        logical >> -('?' > expression > ':' > expression)
        ;

    auto const logical_def =
        equality >> *(logical_op > equality)
        ;

    auto const equality_def =
        relational >> *(equality_op > relational)
        ;

    auto const relational_def =
        additive >> *(relational_op > additive)
        ;

    auto const additive_def =
        multiplicative >> *(additive_op > multiplicative)
        ;

    auto const multiplicative_def =
        factor >> *(multiplicative_op > factor)
        ;

    auto const factor_def =
        primary >> *( '^' > factor )
        ;

    auto const unary_def =
        ufunc > '(' > expression > ')'
        ;

    auto const binary_def =
        bfunc > '(' > expression > ',' > expression > ')'
        ;

    auto const primary_def =
        x3::double_
        | ('(' > expression > ')')
        | (unary_op > primary)
        | binary
        | unary
        | constant
        | variable
        ;

    BOOST_SPIRIT_DEFINE(
            expression,
            logical,
            equality,
            relational,
            additive,
            multiplicative,
            factor,
            primary,
            unary,
            binary,
            conditional,
            variable
        )
}

int main() {
    for (std::string const input : {
           "x+(3^pow(2,8))",
           "1 + (2 + abs(x))",
           "min(x,1+y)",
           "(x > y ? 1 : 0) * (y - z)",
           "min(3^4,7))",
           "3^^4",
           "(3,4)",
        })
    {
        std::cout << " ===== " << std::quoted(input) << " =====\n";
        auto f = begin(input), l = end(input);
        ast::expression out;
        if (phrase_parse(f, l, P::expression, x3::space, out)) {
            std::cout << "Success\n";
        } else {
            std::cout << "Failed\n";
        }
        if (f!=l) {
            std::cout << "Unparsed: " << std::quoted(std::string(f,l)) << "\n";
        }
    }
}