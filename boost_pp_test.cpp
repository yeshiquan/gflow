#include <iostream>
#include <boost/any.hpp>
#include <vector>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/punctuation/comma_if.hpp>
#include <boost/preprocessor/logical/bool.hpp>
#include <boost/preprocessor/facilities/overload.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/array/data.hpp>
#include <boost/preprocessor/repetition/enum.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/enum_params_with_a_default.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/seq.hpp>
#include <type_traits>
#include <boost/mpl/int.hpp>
#include <boost/none.hpp>
#include <boost/preprocessor/repetition.hpp>
#include <boost/preprocessor/arithmetic/sub.hpp>

template <class T>
struct is_integral : std::false_type {}; 

// a seq of integral types with unsigned counterparts
#define BOOST_TT_basic_ints            (char)(short)(int)(long)
// generate a seq containing "signed t" and "unsigned t"
#define BOOST_TT_int_pair(r,data,t)      (signed t)(unsigned t)
// a seq of all the integral types
#define BOOST_TT_ints                                           \
    (bool)(char)                                                \
    BOOST_PP_SEQ_FOR_EACH(BOOST_TT_int_pair, ~, BOOST_TT_basic_ints)
// generate an is_integral specialization for type t
#define BOOST_TT_is_integral_spec(r,data,t) \
   template <>                              \
   struct is_integral<t> : std::true_type {}; 
BOOST_PP_SEQ_FOR_EACH(BOOST_TT_is_integral_spec, ~, BOOST_TT_ints)
void test5() {
    int a = 5;
    std::cout << "is_integral<int> -> " << is_integral<int>::value << std::endl;
    std::cout << "is_integral<std::string> -> " << is_integral<std::string>::value << std::endl;
    std::cout << "is_integral<bool> -> " << is_integral<bool>::value << std::endl;
    std::cout << "is_integral<char> -> " << is_integral<char>::value << std::endl;
    std::cout << "is_integral<unsigned char> -> " << is_integral<unsigned char>::value << std::endl;
    std::cout << "is_integral<signed char> -> " << is_integral<signed char>::value << std::endl;
}


/*
template<typename T, typename A0> 
T* alloc(const A0& a0) { 
	return new T(a0);
}
template<typename T, typename A0, typename A1> 
T* alloc(const A0& a0, const A1& a1) { 
	return new T(a0, a1);
}
template<typename T, typename A0, typename A1, typename A2> 
T* alloc(const A0& a0, const A1& a1, const A2& a2) { 
	return new T(a0, a1, a2);
}
*/
#define FUNCTION_ALLOC(z, N, _)  \
  template<typename T, BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(N), typename A)>  \
  T* alloc(BOOST_PP_ENUM_BINARY_PARAMS_Z(z, BOOST_PP_INC(N), const A, &a)) {  \
     return new T(  \
       BOOST_PP_ENUM_PARAMS_Z(z, BOOST_PP_INC(N), a)  \
     );  \
  }
BOOST_PP_REPEAT(3, FUNCTION_ALLOC, ~)
// 这些带有z后缀的都是用来进行多层枚举的，可以配合BOOST_PP_REPEAT或其他重复或迭代类宏使用.
// 比如定义0~N个不同输入的构造函数。两层循环，一层是内部的XXX_Z来重复参数个数，外层是重复函数
// BOOST_PP_ENUM_BINARY_PARAMS_Z用于同步枚举两个目标
// BOOST_PP_REPEAT, a higher-order macro that repeatedly 
// invokes the macro named by its second argument (FUNCTION_ALLOC). 
// The first argument specifies the number of repeated invocations, 
// and the third one can be any data; it is passed on unchanged to the 
// macro being invoked. In this case, FUNCTION_ALLOC doesn't use that data, so the choice to pass ~ was arbitrary
struct Foobar {
	Foobar(int x) {}
	Foobar(int x, int y) {}
	Foobar(int x, int y, std::string z) {}
};
void test4() {
	auto* p1 = alloc<Foobar>(4);	
	auto* p2 = alloc<Foobar>(4,4);	
	auto* p3 = alloc<Foobar>(4,4,"Hello");	
}


// void testfunc( int p0 = 1 , int p1 = 1 , int p2 = 1 ){};
void test3(BOOST_PP_ENUM_PARAMS_WITH_A_DEFAULT(3, int p, 1)){
    std::cout << "p0:" << p0 
              << " p1:" << p1
              << " p2:" << p2
              << std::endl;
};


#define DECL(z, n, text) text ## n = n;
BOOST_PP_REPEAT(5, DECL, int x)
// int x0 = 0; int x1 = 1; int x2 = 2; int x3 = 3; int x4 = 4;
void test2() {
    std::cout << "x0:" << x0 
              << " x1:" << x1
              << " x2:" << x2
              << " x3:" << x3
              << " x4:" << x4
              << std::endl;
}

#define N 5
std::vector<int> vec{1 BOOST_PP_COMMA_IF(N) 2 BOOST_PP_COMMA_IF(N) 3};

int a = 1;
int b = 2;
int c = 3;
int d = 4;

void hello() {
    #define TUPLE (a, b, c, d)
    int z = BOOST_PP_TUPLE_ELEM(3, TUPLE); // d
    std::cout << "z -> " << z << std::endl; 
}

void test1() {
    int Mn = 5;
    int c = BOOST_PP_CAT(M, n);
    std::cout << "c -> " << c << std::endl;

    std::string s = BOOST_PP_STRINGIZE(text);
    std::cout << "s -> " << s << std::endl;

    BOOST_PP_EMPTY()
    int X = 1;
    int Y = 2;
    auto e = BOOST_PP_IF(1, X, Y);
    auto f = BOOST_PP_IF(0, X, Y);
    std::cout << "e:" << e << " f:" << f << std::endl;

    for (auto e : vec) {
        std::cout << "vec -> " << e << std::endl;
    }
}

#define DECLARE(type, name, ...) ((type, name, 0, 0, ##__VA_ARGS__))

#define __DECLARE(r, data, args) \
    BOOST_PP_TUPLE_ELEM(0, args);

#define INTERFACE(members) \
    BOOST_PP_SEQ_FOR_EACH(__DECLARE, _, members)

#define SEQ (w)(x)(y)(z)

#define MACRO(r, data, elem) BOOST_PP_CAT(elem, data)

BOOST_PP_SEQ_FOR_EACH(MACRO, _, SEQ) // expands to w_ x_ y_ z_

void test6() {
    INTERFACE(
        (std::string)
    );
}

int main() {
    std::cout << "------------------" << std::endl;
    test1();
    std::cout << "------------------" << std::endl;
    hello();
    std::cout << "------------------" << std::endl;
    test2();
    std::cout << "------------------" << std::endl;
    test3();
    std::cout << "------------------" << std::endl;
    test4();
    std::cout << "------------------" << std::endl;
    test5();
    std::cout << "------------------" << std::endl;
    test6();
    return 0;
}

