#include <boost/spirit/home/x3.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/io.hpp>
#include <iostream>
#include <vector>

namespace x3 = boost::spirit::x3;
using x3::double_;
using x3::_attr;
using x3::space;
using x3::phrase_parse;
using x3::_val;

int test1()
{
    std::string source = "1.2 , 1.3 , 1.4 , 1.5";
    auto itr = source.cbegin();
    auto end = source.cend();

    auto const def = x3::float_ >> *(',' >> x3::float_);

    std::vector<float> result;
    auto r = phrase_parse(itr, end, def, x3::ascii::space, result);
    for (auto& v : result) {
        std::cout << v << std::endl;
    }
    return 0;
}

struct user_defined {
    float              value;
    std::string        name;
};
BOOST_FUSION_ADAPT_STRUCT(user_defined, value, name)

void test2() {
    std::string source = "1.2, Hello World";
    auto itr = source.cbegin();
    auto end = source.cend();

    auto const def = x3::float_ >> ',' >> x3::lexeme[*x3::char_];

    user_defined data;
    auto r = phrase_parse(itr, end, def, x3::ascii::space, data);
    std::cout << data.name << " " << data.value << std::endl;
}

template <typename Iterator>
bool parse_complex(Iterator first, Iterator last, std::complex<double>& c)
{
    using boost::spirit::x3::double_;
    using boost::spirit::x3::_attr;
    using boost::spirit::x3::phrase_parse;
    using boost::spirit::x3::ascii::space;

    double rN = 0.0;
    double iN = 0.0;
    auto fr = [&](auto& ctx){ rN = _attr(ctx); };
    auto fi = [&](auto& ctx){ iN = _attr(ctx); };

    bool r = phrase_parse(first, last,
        //  Begin grammar
        (
                '(' >> double_[fr]
                    >> -(',' >> double_[fi]) >> ')'
            |   double_[fr]
        ),
        //  End grammar
        space);

    if (!r || first != last) // fail if we did not get a full match
        return false;
    c = std::complex<double>(rN, iN);
    return r;
}

void test3() {
    std::string src = "(2.4,5.3)";
    std::complex<double> res;
    parse_complex(src.begin(), src.end(), res);
    std::cout << res << std::endl;
}

template <typename Iterator>
bool adder(Iterator first, Iterator last, double& n)
{
    auto assign = [&](auto& ctx){ n = _attr(ctx); };
    auto add = [&](auto& ctx){ n += _attr(ctx); };

    bool r = x3::phrase_parse(first, last,

        //  Begin grammar
        (
            double_[assign] >> *(',' >> double_[add])
        )
        ,
        //  End grammar

        space);

    if (first != last) // fail if we did not get a full match
        return false;
    return r;
}

void test4() {
    std::string src = "1.1,2.2,3.3,4.5";
    double res = 0.0;
    adder(src.begin(), src.end(), res);
    std::cout << res << std::endl;
}

template <typename Iterator>
bool parse_numbers(Iterator first, Iterator last, std::vector<double>& v)
{
    using x3::double_;
    using x3::phrase_parse;
    using x3::_attr;
    using x3::space;

    auto push_back = [&](auto& ctx){ v.push_back(_attr(ctx)); };

    bool r = phrase_parse(first, last,

        //  Begin grammar
        (
            double_[push_back]
                >> *(',' >> double_[push_back])
        )
        ,
        //  End grammar

        space);

    if (first != last) // fail if we did not get a full match
        return false;
    return r;
}

void test5() {
    std::string src = "1.1,2.2,3.3,4.5";
    std::vector<double> res;
    parse_numbers(src.begin(), src.end(), res);
    for (auto& num : res) {
        std::cout << num << std::endl;
    }
}

int main() {
    test1();
    test2();
    test3();
    test4();
    test5();
}