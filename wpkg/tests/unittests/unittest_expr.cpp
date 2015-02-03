/*    unittest_expr.cpp
 *    Copyright (C) 2013-2015  Made to Order Software Corporation
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License along
 *    with this program; if not, write to the Free Software Foundation, Inc.,
 *    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 *    Authors
 *    Alexis Wilke   alexis@m2osw.com
 */

#include "unittest_expr.h"
#include "libexpr/expr.h"

#include <cppunit/config/SourcePrefix.h>
#include <string.h>
#include <math.h>
#include <time.h>

// warning C4554: '<<' : check operator precedence for possible error; use parentheses to clarify precedence
#pragma warning(disable: 4554)
// warning C4805: '==' : unsafe mix of type 'long' and type 'bool' in operation
#pragma warning(disable: 4805)

CPPUNIT_TEST_SUITE_REGISTRATION( ExprUnitTests );

#ifdef _MSC_VER
namespace
{
long lrint(double f)
{
    // XXX at some point libexpr may check for over/under-flow...
    return static_cast<long>(f < 0.0 ? ceil(f - 0.5) : floor(f + 0.5));
}

double acosh(double flt)
{
	return log(flt + sqrt(flt * flt - 1.0));
}

double asinh(double flt)
{
	return log(flt + sqrt(flt * flt + 1.0));
}

double atanh(double flt)
{
	return log((1.0 + flt) / (1.0 - flt)) / 2.0;
}

double rint(double flt)
{
	if(flt < 0.0)
	{
		return ceil(flt - 0.5);
	}
	else
	{
		return floor(flt + 0.5);
	}
}

} // noname namespace
#endif


void ExprUnitTests::setUp()
{
}

long compute_long(const char *op)
{
//printf("compute long [%s]\n", op);
    libexpr::expr_evaluator e;
    libexpr::variable result;
    e.eval(op, result);
    long value;
    result.get(value);
    return value;
}

double compute_double(const char *op, double flt)
{
    libexpr::expr_evaluator e;
    libexpr::variable result;
    e.eval(op, result);
    double value;
    result.get(value);
    return abs(value - flt) < 0.00001;
}

bool compute_string(const char *op, const char *expected)
{
    libexpr::expr_evaluator e;
    libexpr::variable result;
//printf("compute [%s]\n", op);
    e.eval(op, result);
    std::string value;
    result.get(value);
//printf("\nvalue = [");
//for(const char *s(value.c_str()); *s != '\0'; ++s) static_cast<unsigned char>(*s) < ' ' ? printf("^%c", *s + '@') : (static_cast<unsigned char>(*s) > 0x7E ? printf("/0x%02X/", static_cast<unsigned char>(*s)) : printf("%c", *s));
//printf("] [");
//for(const char *s(expected); *s != '\0'; ++s) static_cast<unsigned char>(*s) < ' ' ? printf("^%c", *s + '@') : (static_cast<unsigned char>(*s) > 0x7E ? printf("/0x%02X/", static_cast<unsigned char>(*s)) : printf("%c", *s));
//printf("]\n");
    return value == expected;
}

#define ASSERT_LONG_OPERATION(operation) \
    CPPUNIT_ASSERT(compute_long(#operation) == (operation))

#define ASSERT_DOUBLE_OPERATION(operation) \
    CPPUNIT_ASSERT(compute_double(#operation, (operation)))


void ExprUnitTests::bad_literals()
{
    // bad hex
    CPPUNIT_ASSERT_THROW(compute_long("(0x) * 2"), libexpr::syntax_error);

    // bad octal
    CPPUNIT_ASSERT_THROW(compute_long("03 + 08"), libexpr::syntax_error);
    CPPUNIT_ASSERT_THROW(compute_long("033 + 09"), libexpr::syntax_error);

    // bad float
    CPPUNIT_ASSERT_THROW(compute_long("0.3e++"), libexpr::syntax_error);
    CPPUNIT_ASSERT_THROW(compute_long("0.3ee3"), libexpr::syntax_error);
    CPPUNIT_ASSERT_THROW(compute_long("0.3e-a"), libexpr::syntax_error);

    // bad character
    CPPUNIT_ASSERT_THROW(compute_long("'h + 3"), libexpr::syntax_error);
    CPPUNIT_ASSERT_THROW(compute_long("'h"), libexpr::syntax_error);
    CPPUNIT_ASSERT_THROW(compute_long("'\\x74 + 3"), libexpr::syntax_error);
    CPPUNIT_ASSERT_THROW(compute_long("'\\x74"), libexpr::syntax_error);

    // bad string
    CPPUNIT_ASSERT_THROW(compute_long("\"hello world"), libexpr::syntax_error);
    CPPUNIT_ASSERT_THROW(compute_long("\"hello\\xqaworld\""), libexpr::syntax_error);

    // bad conditional
    CPPUNIT_ASSERT_THROW(compute_long("(a = 3, b = 55, 3 > 0 ? a b)"), libexpr::syntax_error);
}

void ExprUnitTests::bad_variables()
{
    CPPUNIT_ASSERT_THROW(compute_long("a"), libexpr::undefined_variable);

    CPPUNIT_ASSERT_THROW(compute_long("a++"), libexpr::undefined_variable);
    CPPUNIT_ASSERT_THROW(compute_long("++a"), libexpr::undefined_variable);
    CPPUNIT_ASSERT_THROW(compute_long("a--"), libexpr::undefined_variable);
    CPPUNIT_ASSERT_THROW(compute_long("--a"), libexpr::undefined_variable);

    CPPUNIT_ASSERT_THROW(compute_long("a = b;"), libexpr::undefined_variable); // cannot get an undefined variable
    CPPUNIT_ASSERT_THROW(compute_long("a *= 5;"), libexpr::undefined_variable); // cannot get an undefined variable
    CPPUNIT_ASSERT_THROW(compute_long("a /= 5;"), libexpr::undefined_variable); // cannot get an undefined variable
    CPPUNIT_ASSERT_THROW(compute_long("a %= 5;"), libexpr::undefined_variable); // cannot get an undefined variable
    CPPUNIT_ASSERT_THROW(compute_long("a += 5;"), libexpr::undefined_variable); // cannot get an undefined variable
    CPPUNIT_ASSERT_THROW(compute_long("a -= 5;"), libexpr::undefined_variable); // cannot get an undefined variable
    CPPUNIT_ASSERT_THROW(compute_long("a >>= 5;"), libexpr::undefined_variable); // cannot get an undefined variable
    CPPUNIT_ASSERT_THROW(compute_long("a <<= 5;"), libexpr::undefined_variable); // cannot get an undefined variable
    CPPUNIT_ASSERT_THROW(compute_long("a &= 5;"), libexpr::undefined_variable); // cannot get an undefined variable
    CPPUNIT_ASSERT_THROW(compute_long("a ^= 5;"), libexpr::undefined_variable); // cannot get an undefined variable
    CPPUNIT_ASSERT_THROW(compute_long("a |= 5;"), libexpr::undefined_variable); // cannot get an undefined variable
}

void ExprUnitTests::bad_expressions()
{
    // misc.
    CPPUNIT_ASSERT_THROW(compute_long("(a = 3, b = \"abc\", a->b = 5)"), libexpr::syntax_error); // arrow not supported
    CPPUNIT_ASSERT_THROW(compute_long("(a = 5, a++"), libexpr::syntax_error); // ')' missing
    CPPUNIT_ASSERT_THROW(compute_long("(a = 5 a)"), libexpr::syntax_error); // comma missing
    CPPUNIT_ASSERT_THROW(compute_long("a = 5, a)"), libexpr::syntax_error); // '(' missing
    CPPUNIT_ASSERT_THROW(compute_long("a = 5 a"), libexpr::syntax_error); // ',' missing
    CPPUNIT_ASSERT_THROW(compute_long(")a)"), libexpr::syntax_error); // ')' instead of '('
    CPPUNIT_ASSERT_THROW(compute_long("lrint("), libexpr::syntax_error); // missing argument
    CPPUNIT_ASSERT_THROW(compute_long("lrint(3.4"), libexpr::syntax_error); // ')' missing
    CPPUNIT_ASSERT_THROW(compute_long("lrint(3.4,"), libexpr::syntax_error); // expect only one parameter, missing ')'
    CPPUNIT_ASSERT_THROW(compute_long("lrint(3.4, 5)"), libexpr::function_args); // expects only one parameter
    CPPUNIT_ASSERT_THROW(compute_long("3(75)"), libexpr::syntax_error); // not a function name
    CPPUNIT_ASSERT_THROW(compute_long("unknown_function_name(1, 2, 3)"), libexpr::undefined_function); // function not defined
#if !defined(WINDOWS) && !defined(_WINDOWS)
    // shell true and false are inverted!
    CPPUNIT_ASSERT_THROW(compute_long("shell( \"true\" , \"magic\" )"), libexpr::function_args);
    CPPUNIT_ASSERT_THROW(compute_long("shell( \"totally-unknown-command\" )"), libexpr::libexpr_runtime_error);
#endif
    CPPUNIT_ASSERT_THROW(compute_long("a = 5, a++++"), libexpr::syntax_error); // ++ only applicable to lvalue
    CPPUNIT_ASSERT_THROW(compute_long("a = 5, a----"), libexpr::syntax_error); // -- only applicable to lvalue
    CPPUNIT_ASSERT_THROW(compute_long("a = 5, a++--"), libexpr::syntax_error); // -- only applicable to lvalue
    CPPUNIT_ASSERT_THROW(compute_long("a = 5, a--++"), libexpr::syntax_error); // ++ only applicable to lvalue
    CPPUNIT_ASSERT_THROW(compute_long("5--"), libexpr::expected_a_variable); // ++ only applicable to lvalue
    CPPUNIT_ASSERT_THROW(compute_long("5++"), libexpr::expected_a_variable); // ++ only applicable to lvalue
    CPPUNIT_ASSERT_THROW(compute_long("++5"), libexpr::expected_a_variable); // ++ only applicable to lvalue
    CPPUNIT_ASSERT_THROW(compute_long("--5"), libexpr::expected_a_variable); // ++ only applicable to lvalue
    CPPUNIT_ASSERT_THROW(compute_long("5.3--"), libexpr::expected_a_variable); // ++ only applicable to lvalue
    CPPUNIT_ASSERT_THROW(compute_long("5.2++"), libexpr::expected_a_variable); // ++ only applicable to lvalue
    CPPUNIT_ASSERT_THROW(compute_long("++5.1"), libexpr::expected_a_variable); // ++ only applicable to lvalue
    CPPUNIT_ASSERT_THROW(compute_long("--7.9"), libexpr::expected_a_variable); // ++ only applicable to lvalue
    CPPUNIT_ASSERT_THROW(compute_long("3 = 5;"), libexpr::expected_a_variable); // cannot set a number
    CPPUNIT_ASSERT_THROW(compute_long("3 *= 5;"), libexpr::expected_a_variable); // cannot get/set a number
    CPPUNIT_ASSERT_THROW(compute_long("3 /= 5;"), libexpr::expected_a_variable); // cannot get/set a number
    CPPUNIT_ASSERT_THROW(compute_long("3 %= 5;"), libexpr::expected_a_variable); // cannot get/set a number
    CPPUNIT_ASSERT_THROW(compute_long("3 += 5;"), libexpr::expected_a_variable); // cannot get/set a number
    CPPUNIT_ASSERT_THROW(compute_long("3 -= 5;"), libexpr::expected_a_variable); // cannot get/set a number
    CPPUNIT_ASSERT_THROW(compute_long("3 >>= 5;"), libexpr::expected_a_variable); // cannot get/set a number
    CPPUNIT_ASSERT_THROW(compute_long("3 <<= 5;"), libexpr::expected_a_variable); // cannot get/set a number
    CPPUNIT_ASSERT_THROW(compute_long("3 &= 5;"), libexpr::expected_a_variable); // cannot get/set a number
    CPPUNIT_ASSERT_THROW(compute_long("3 ^= 5;"), libexpr::expected_a_variable); // cannot get/set a number
    CPPUNIT_ASSERT_THROW(compute_long("3 |= 5;"), libexpr::expected_a_variable); // cannot get/set a number

    // string problems
    CPPUNIT_ASSERT_THROW(compute_long("-\"neg\""), libexpr::incompatible_type); // -"string" not valid
    CPPUNIT_ASSERT_THROW(compute_long("+\"neg\""), libexpr::incompatible_type); // +"string" not valid
    CPPUNIT_ASSERT_THROW(compute_long("3 * \"mul\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3 % \"mod\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3 / \"div\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3 - \"div\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_double("3.5 * \"mul\"", 0.0), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_double("17.3 / \"div\"", 0.0), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_double("3.9 % \"mod\"", 0.0), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_double("3.9 - \"mod\"", 0.0), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"mul\" * 3"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"div\" / 3"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"min\" - 3"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"min\" & 3"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"min\" ^ 3"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"min\" | 3"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3 & \"min\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3 ^ \"min\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3 | \"min\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("~\"min\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3 == \"str\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3.5 == \"str\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3 != \"str\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3.5 != \"str\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3 < \"str\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3.5 < \"str\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3 <= \"str\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3.5 <= \"str\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3 > \"str\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3.5 > \"str\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3 >= \"str\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("3.5 >= \"str\""), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"str\" == 56"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"str\" == 895.3"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"str\" != 56"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"str\" != 895.3"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"str\" < 56"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"str\" < 895.3"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"str\" <= 56"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"str\" <= 895.3"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"str\" > 56"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"str\" > 895.3"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"str\" >= 56"), libexpr::incompatible_type); // "string" operation not valid
    CPPUNIT_ASSERT_THROW(compute_long("\"str\" >= 895.3"), libexpr::incompatible_type); // "string" operation not valid

    // floating point problems
    CPPUNIT_ASSERT_THROW(compute_double("-3.5 % 2.4", 0.0), libexpr::incompatible_type); // % not valid with float
    CPPUNIT_ASSERT_THROW(compute_double("-3 % 2.4", 0.0), libexpr::incompatible_type); // % not valid with float
    CPPUNIT_ASSERT_THROW(compute_double("-3.5 % 2", 0.0), libexpr::incompatible_type); // % not valid with float
    CPPUNIT_ASSERT_THROW(compute_double("-3.5 << 2.4", 0.0), libexpr::incompatible_type); // << not valid with float
    CPPUNIT_ASSERT_THROW(compute_double("-3 << 2.4", 0.0), libexpr::incompatible_type); // << not valid with float
    CPPUNIT_ASSERT_THROW(compute_double("-3.5 << 2", 0.0), libexpr::incompatible_type); // << not valid with float
    CPPUNIT_ASSERT_THROW(compute_double("-3.5 >> 2.4", 0.0), libexpr::incompatible_type); // >> not valid with float
    CPPUNIT_ASSERT_THROW(compute_double("-3 >> 2.4", 0.0), libexpr::incompatible_type); // >> not valid with float
    CPPUNIT_ASSERT_THROW(compute_double("-3.5 >> 2", 0.0), libexpr::incompatible_type); // >> not valid with float
    CPPUNIT_ASSERT_THROW(compute_double("-3.5 & 2", 0.0), libexpr::incompatible_type); // & not valid with float
    CPPUNIT_ASSERT_THROW(compute_double("-3.5 ^ 2", 0.0), libexpr::incompatible_type); // ^ not valid with float
    CPPUNIT_ASSERT_THROW(compute_double("-3.5 | 2", 0.0), libexpr::incompatible_type); // | not valid with float
    CPPUNIT_ASSERT_THROW(compute_double("~3.5", 0.0), libexpr::incompatible_type); // ~ not valid with float
    CPPUNIT_ASSERT_THROW(compute_double("~-9.3", 0.0), libexpr::incompatible_type); // ~ not valid with float

    // incompatible data type for our get
    CPPUNIT_ASSERT_THROW(compute_double("3", 0.0), libexpr::invalid_type); // 3 is a long, not a float
    CPPUNIT_ASSERT_THROW(compute_double("\"hello\"", 0.0), libexpr::invalid_type); // "hello" is a string, not a float
    CPPUNIT_ASSERT_THROW(compute_long("3.3"), libexpr::invalid_type); // 3.3 is a float, not a long
    CPPUNIT_ASSERT_THROW(compute_long("\"hello\""), libexpr::invalid_type); // "hello" is a string, not a long
    CPPUNIT_ASSERT_THROW(compute_string("3", "3"), libexpr::invalid_type); // 3 is a long, not a string
    CPPUNIT_ASSERT_THROW(compute_string("4.5", "4.5"), libexpr::invalid_type); // 4.5 is a float, not a string
}

void ExprUnitTests::additions()
{
#if defined( __GNUC__ )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wsign-compare"
#endif
    // integers
    ASSERT_LONG_OPERATION(3 + 7 + 2 + 0X8f);
    ASSERT_LONG_OPERATION(90 - 45 + 33 + 0xC1);
    ASSERT_LONG_OPERATION(-45 + 33 - 193 + 34 + 1000 + 3276 - 9);
    ASSERT_LONG_OPERATION(3 + 7 - +0 + 2 + 0XFABCD899);
    ASSERT_LONG_OPERATION(-3 + +7 - +0 + -2 + +0XFABCD899);

    // characters (same as integers, just not the same value)
    ASSERT_LONG_OPERATION(3 + '7' - 0 + 2 + 0XFABCD899 - 'a');
#if defined( __GNUC__ )
#   pragma GCC diagnostic pop
#endif

    // floating points
    ASSERT_DOUBLE_OPERATION(3.3 + 7.1 - 0.9 + 2.2 + 123.001);
    ASSERT_DOUBLE_OPERATION(.33e1 + .71e+1 - 9.E-1 + .22E1 + 123001.0e-3 );
    ASSERT_DOUBLE_OPERATION(+.33e1 + -.71e+1 - -9.E-1 + +.22E1 + -123001.0e-3 );
    ASSERT_DOUBLE_OPERATION(-3.99 - +43 - +0.0 + -2 + +0XD899);

    // test newlines and carriage returns
    // although at this point we cannot check the f_line (not accessible)
    CPPUNIT_ASSERT(compute_double("3.5\n*\r\n7.2", 3.5 * 7.2));
    CPPUNIT_ASSERT(compute_double("13.5 // this is an approximation\n+\r\n /* multiplye is * but we use + here and luckily we found this number: */ 7.05", 13.5 + 7.05));

    // string concatenation
    CPPUNIT_ASSERT(compute_string("\"this\" + \"that\"", "thisthat"));
    CPPUNIT_ASSERT(compute_string("\"\\x74hi\\163\\7\" + \".\\40.\" + \"that\\x07\"", "this\a. .that\a"));
    CPPUNIT_ASSERT(compute_string("\"escapes: \\a\\b\\e\\f\" + \"\\n\\r\\t\\v\\?\"", "escapes: \a\b\33\f\n\r\t\v\?"));
    CPPUNIT_ASSERT(compute_string("\"\\xaa\\XFF\\XFQ\" + \"\\xBe\\xDc\" \"auto-concat\";;;", "\xaa\xff\xfQ\xbe\xdc" "auto-concat"));
    CPPUNIT_ASSERT(compute_string("\"this\" + 3", "this3"));
    CPPUNIT_ASSERT(compute_string("3 + \"this\"", "3this"));
    CPPUNIT_ASSERT(compute_string("3 + \"this\" + 3", "3this3"));
    CPPUNIT_ASSERT(compute_string("3.35 + \"this\" + 3.35", "3.35this3.35"));
}

void ExprUnitTests::shifts()
{
    ASSERT_LONG_OPERATION(1 << 0xD);
    ASSERT_LONG_OPERATION(0x8000 >> 05);
    ASSERT_LONG_OPERATION(1 << 2 << 3 << 4);

    ASSERT_LONG_OPERATION(0x3000 << 7 + 2 >> 1);
    ASSERT_LONG_OPERATION(0x3000 << 7 - 2 >> 1);
    ASSERT_LONG_OPERATION(0x3000 >> 7 * 3 << 1);
    ASSERT_LONG_OPERATION(0x3000 >> 7 % 3 << 1);
    ASSERT_LONG_OPERATION(0x3000 >> 7 / 3 << 1);

    ASSERT_LONG_OPERATION(0x3000 >> 7 == 3 << 1);
    ASSERT_LONG_OPERATION(0x3000 >> 7 != 3 << 1);
    ASSERT_LONG_OPERATION(0x3000 >> 7 > 3 << 1);
    ASSERT_LONG_OPERATION(0x3000 >> 7 >= 3 << 1);
    ASSERT_LONG_OPERATION(0x3000 >> 7 < 3 << 1);
    ASSERT_LONG_OPERATION(0x3000 >> 7 <= 3 << 1);

    ASSERT_LONG_OPERATION(((0x3000 >> 7) | (0x3000 << 7)) & 0xFFFF);
}

void ExprUnitTests::increments() // postfix/prefix
{
    // note that we support multiple ++ and -- in a row, but that
    // is not valid C++ (i.e. a++++ is not valid because the second
    // ++ would otherwise be applied to the result of a++ which is
    // not an lvalue.)
    long a, _a;
    ASSERT_LONG_OPERATION((a = 3, a++));
    ASSERT_LONG_OPERATION((a = 3, a++, a));
    ASSERT_LONG_OPERATION((a = 78, a--));
    ASSERT_LONG_OPERATION((a = 78, a--, a));
    ASSERT_LONG_OPERATION((a = 234, ++a));
    ASSERT_LONG_OPERATION((a = 234, ++a, a));
    ASSERT_LONG_OPERATION((a = 934, --a));
    ASSERT_LONG_OPERATION((_a = 934, --_a, _a));
}

void ExprUnitTests::multiplications()
{
    // integer operations
    ASSERT_LONG_OPERATION(3 + 7 + 2 * 143);
    ASSERT_LONG_OPERATION(-90 * 45 + 33 + 193);
    ASSERT_LONG_OPERATION(0x3002 + 1 + 45 + 33 * 193);
    ASSERT_LONG_OPERATION(3702 / 9 + 45 * 7 + 33 / 193 + 30491 / 129 / 2);
    ASSERT_LONG_OPERATION(7 + 3 + 5 + 9 / 0x2);
    ASSERT_LONG_OPERATION(111 + 7 + 3 + 5 + 0x09 * 2 - 003);
    ASSERT_LONG_OPERATION(56 + 7 + 3 + 05 + 9 % 2 + 34);
    ASSERT_LONG_OPERATION(3 + 7 + 3804 % 5 + 9 % 2 * 13 % 27);

    // some floating point operations
    ASSERT_DOUBLE_OPERATION(3 * 1.34e0 + 1);
    ASSERT_DOUBLE_OPERATION(3.34 * 34 + 24);
    ASSERT_DOUBLE_OPERATION(3.34e-0 * 34 + 24);
    ASSERT_DOUBLE_OPERATION(3 / 1.34e0 + 1);
    ASSERT_DOUBLE_OPERATION(3.34 / 34 + 24);
    ASSERT_DOUBLE_OPERATION(3.34e-0 / 34 + 24);
    ASSERT_DOUBLE_OPERATION(3 * 34 / 2.4);
    ASSERT_DOUBLE_OPERATION(3 * 34 + 2.4);
    ASSERT_DOUBLE_OPERATION(3 * 34 - 2.4);
}

void ExprUnitTests::bitwise()
{
    ASSERT_LONG_OPERATION(3 | +4);
    ASSERT_LONG_OPERATION(255 & -4);
    ASSERT_LONG_OPERATION(0xAA^0x55);

    ASSERT_LONG_OPERATION(~3 | +4);
    ASSERT_LONG_OPERATION(255 & ~-4);
    ASSERT_LONG_OPERATION(0xAA^~0x55);

    ASSERT_LONG_OPERATION(3 | ~4);
    ASSERT_LONG_OPERATION(~255 & -4);
    ASSERT_LONG_OPERATION(~0xAA^0x55);

    ASSERT_LONG_OPERATION(~3 | ~4);
    ASSERT_LONG_OPERATION(~255 & ~-4);
    ASSERT_LONG_OPERATION(~0xAA^~0x55);

    // priority stuff
    ASSERT_LONG_OPERATION('a' ^ 0x55 | 071 & 0xEF);
    ASSERT_LONG_OPERATION('a' ^ 0x55 & 071 | 0xEF);
    ASSERT_LONG_OPERATION('a' | 0x55 & 071 ^ 0xEF);
    ASSERT_LONG_OPERATION('a' | 0x55 ^ 071 & 0xEF);
    ASSERT_LONG_OPERATION('a' & 0x55 ^ 071 | 0xEF);
    ASSERT_LONG_OPERATION('a' & 0x55 | 071 ^ 0xEF);
}

void ExprUnitTests::comparisons()
{
    // integers
    ASSERT_LONG_OPERATION(7 != 9);
    ASSERT_LONG_OPERATION(132817291 == 132817291);
    ASSERT_LONG_OPERATION(!(132817291 == 13281729));
    ASSERT_LONG_OPERATION(3 * 7 < 9 * 47);
    ASSERT_LONG_OPERATION(3 * 7 < 9 * 47 < true);
    ASSERT_LONG_OPERATION(9 * 47 < 3 * 7);
    ASSERT_LONG_OPERATION(3 * 7 <= 9 * 47);
    ASSERT_LONG_OPERATION(9 * 47 <= 3 * 7);
    ASSERT_LONG_OPERATION(3 * 7 < 9 * 47 <= false);
    ASSERT_LONG_OPERATION(3 * 7 > 9 * 47);
    ASSERT_LONG_OPERATION(9 * 47 > 3 * 7);
    ASSERT_LONG_OPERATION(3 * 7 >= 9 * 47);
    ASSERT_LONG_OPERATION(9 * 47 >= 3 * 7);
    ASSERT_LONG_OPERATION(9 * 47 >= 3 * 7 < true);
    ASSERT_LONG_OPERATION(9 * 47 >= 3 * 7 <= true);
    ASSERT_LONG_OPERATION(9 * 47 >= 3 * 7 < false);
    ASSERT_LONG_OPERATION(9 * 47 >= 3 * 7 <= false);

    // floating point
    ASSERT_LONG_OPERATION(9.01 * 47 == 3.1 * 7);
    ASSERT_LONG_OPERATION(9 * 47.1 == 3 * 7.2222);
    ASSERT_LONG_OPERATION(9.2 * 47 != 3.1 * 7);
    ASSERT_LONG_OPERATION(9 * 47.1 != 3 * 7.111);
    ASSERT_LONG_OPERATION(9.01 * 47 > 3.1 * 7);
    ASSERT_LONG_OPERATION(9 * 47.1 > 3 * 7.2222);
    ASSERT_LONG_OPERATION(9.2 * 47 >= 3.1 * 7);
    ASSERT_LONG_OPERATION(9 * 47.1 >= 3 * 7.111);
    ASSERT_LONG_OPERATION(9.3 * 47 < 3.1 * 7);
    ASSERT_LONG_OPERATION(9 * 47.1 < 3 * 7);
    ASSERT_LONG_OPERATION(9.3 * 47 <= 3.1 * 7.0102);
    ASSERT_LONG_OPERATION(9 * 47.1 <= 3 * 7);

    ASSERT_LONG_OPERATION(9 * 47 == 3.1 * 7);
    ASSERT_LONG_OPERATION(9 * 47.1 == 3 * 7);
    ASSERT_LONG_OPERATION(9 * 47 != 3.1 * 7);
    ASSERT_LONG_OPERATION(9 * 47.1 != 3 * 7);
    ASSERT_LONG_OPERATION(9 * 47 > 3.1 * 7);
    ASSERT_LONG_OPERATION(9 * 47.1 > 3 * 7);
    ASSERT_LONG_OPERATION(9 * 47 >= 3.1 * 7);
    ASSERT_LONG_OPERATION(9 * 47.1 >= 3 * 7);
    ASSERT_LONG_OPERATION(9 * 47 < 3.1 * 7);
    ASSERT_LONG_OPERATION(9 * 47.1 < 3 * 7);
    ASSERT_LONG_OPERATION(9 * 47 <= 3.1 * 7);
    ASSERT_LONG_OPERATION(9 * 47.1 <= 3 * 7);

    // string
    CPPUNIT_ASSERT(compute_long("\"this\" == \"th\" \"is\"") == true);
    CPPUNIT_ASSERT(compute_long("\"th\" + \"is\" == \"th\" \"is\"") == true);
    CPPUNIT_ASSERT(compute_long("\"9 * 47\" == \"3.1 * 7\"") == false);
    CPPUNIT_ASSERT(compute_long("\"9 * 47.1\" == \"3 * 7\"") == false);
    CPPUNIT_ASSERT(compute_long("\"9 * 47\" != \"3.1 * 7\"") == true);
    CPPUNIT_ASSERT(compute_long("\"9 * 47.1\" != \"3 * 7\"") == true);
    CPPUNIT_ASSERT(compute_long("\"th\" + \"is\" != \"th\" \"is\"") == false);
    CPPUNIT_ASSERT(compute_long("\"9 * 47\" > \"3.1 * 7\"") == true);
    CPPUNIT_ASSERT(compute_long("\"9 * 47.1\" > \"3 * 7\"") == true);
    CPPUNIT_ASSERT(compute_long("\"9 * 47\" >= \"3.1 * 7\"") == true);
    CPPUNIT_ASSERT(compute_long("\"9 * 47.1\" >= \"3 * 7\"") == true);
    CPPUNIT_ASSERT(compute_long("\"9 * 47\" < \"3.1 * 7\"") == false);
    CPPUNIT_ASSERT(compute_long("\"9 * 47.1\" < \"3 * 7\"") == false);
    CPPUNIT_ASSERT(compute_long("\"9 * 47\" <= \"3.1 * 7\"") == false);
    CPPUNIT_ASSERT(compute_long("\"9 * 47.1\" <= \"3 * 7\"") == false);

    {
    // proves we can redefine e along the way
    long a, b, c, d, e, f;
    ASSERT_LONG_OPERATION((a = 9, b = 47, c = 3, d = 7, e = 33, f = 45, a * b >= c * d && e > f));
    ASSERT_LONG_OPERATION((a = 888, a = 333, b = 123, c = 00003, d = 0x7AFE, ++b, e = '\33', e++, f = 9945, c -= 32, a * b >= c * d || e > f));
    }

    // check the not
    ASSERT_LONG_OPERATION(!0);
    ASSERT_LONG_OPERATION(!7);
    ASSERT_LONG_OPERATION(!-7);
    CPPUNIT_ASSERT(compute_long("!\"\"") == true);  // we return true but C++ says false here!
    ASSERT_LONG_OPERATION(!"not empty");
    ASSERT_LONG_OPERATION(!3.5);
    ASSERT_LONG_OPERATION(!-3.5);
    ASSERT_LONG_OPERATION(!0.0);

    // test our addition (^^)
    CPPUNIT_ASSERT(compute_long("true ^^ true") == false);
    CPPUNIT_ASSERT(compute_long("true ^^ false") == true);
    CPPUNIT_ASSERT(compute_long("false ^^ true") == true);
    CPPUNIT_ASSERT(compute_long("false ^^ false") == false);
    CPPUNIT_ASSERT(compute_long("3 ^^ 3.3") == false);
    CPPUNIT_ASSERT(compute_long("3 ^^ \"\"") == true);
    CPPUNIT_ASSERT(compute_long("3.3 ^^ 3") == false);
    CPPUNIT_ASSERT(compute_long("3.3 ^^ \"\"") == true);
    CPPUNIT_ASSERT(compute_long("\"\" ^^ 3") == true);
    CPPUNIT_ASSERT(compute_long("\"\" ^^ 5.4") == true);

    CPPUNIT_ASSERT(compute_long("true && true ^^ true && true") == false);
    CPPUNIT_ASSERT(compute_long("true && true ^^ true && false") == true);
    CPPUNIT_ASSERT(compute_long("true && false ^^ true && true") == true);
    CPPUNIT_ASSERT(compute_long("true && true ^^ false && true") == true);
    CPPUNIT_ASSERT(compute_long("false && true ^^ true && true") == true);
    CPPUNIT_ASSERT(compute_long("true && false ^^ true && false") == false);
    CPPUNIT_ASSERT(compute_long("false && false ^^ true && false") == false);

    // some priority checks
    ASSERT_LONG_OPERATION(true && true || true);
    ASSERT_LONG_OPERATION(true && true || false);
    ASSERT_LONG_OPERATION(false && true || false);
    ASSERT_LONG_OPERATION(false && false || false);
    ASSERT_LONG_OPERATION(false && true || true);
    ASSERT_LONG_OPERATION(false && false || true);

    ASSERT_LONG_OPERATION(true && true == true || true);
    ASSERT_LONG_OPERATION(true && false == true || false);
    ASSERT_LONG_OPERATION(false && true == false || true);
    ASSERT_LONG_OPERATION(false && false == false || false);

    ASSERT_LONG_OPERATION(true && true != true || true);
    ASSERT_LONG_OPERATION(true && false != true || false);
    ASSERT_LONG_OPERATION(false && true != false || true);
    ASSERT_LONG_OPERATION(false && false != false || false);

    // operations on other types
    ASSERT_LONG_OPERATION(33 && 35);
    ASSERT_LONG_OPERATION(33 && 3.5);
    ASSERT_LONG_OPERATION(5.5 && 35);
    ASSERT_LONG_OPERATION(5.5 && "35");
    ASSERT_LONG_OPERATION("35" && 5.5);
    CPPUNIT_ASSERT(compute_long("\"\" && 5.4") == false);
    CPPUNIT_ASSERT(compute_long("5.4 && \"\"") == false);
    ASSERT_LONG_OPERATION(33 || 35);
    ASSERT_LONG_OPERATION(33 || 3.5);
    ASSERT_LONG_OPERATION(5.5 || 35);
    ASSERT_LONG_OPERATION(5.5 || "35");
    ASSERT_LONG_OPERATION("35" || 5.5);
    CPPUNIT_ASSERT(compute_long("\"\" || 5.4") == true);
    CPPUNIT_ASSERT(compute_long("5.4 || \"\"") == true);

    // conditional
    {
    long a, b, c, d;
    ASSERT_LONG_OPERATION((a = 34, b = 123, 3 > 9 ? a : b));
    ASSERT_LONG_OPERATION((a = 9444, b = 23, c = -33, d = 55, c < d ? a : b));
    }
}

void ExprUnitTests::assignments()
{
    long a, b;
    ASSERT_LONG_OPERATION((a = 9444, b = a, b + 3));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a *= b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a *= b, a));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a *= b, b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a /= b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a /= b, a));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a /= b, b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a %= b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a %= b, a));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a %= b, b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a += b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a += b, a));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a += b, b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a -= b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a -= b, a));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a -= b, b));
    // WARNING: large shifts do not give us the same results under Linux & Windows...
    //          (it looks like MS-Windows, even in 64 bits, may apply an AND 0x1F?)
    ASSERT_LONG_OPERATION((a = 9444, b = 29, a <<= b));
    ASSERT_LONG_OPERATION((a = 9444, b = 29, a <<= b, a));
    ASSERT_LONG_OPERATION((a = 9444, b = 29, a <<= b, b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4, a >>= b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4, a >>= b, a));
    ASSERT_LONG_OPERATION((a = 9444, b = 4, a >>= b, b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a &= b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a &= b, a));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a &= b, b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a ^= b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a ^= b, a));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a ^= b, b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a |= b));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a |= b, a));
    ASSERT_LONG_OPERATION((a = 9444, b = 4531, a |= b, b));
}

void ExprUnitTests::functions()
{
    double a, b, pi(3.14159265358979323846), e(2.7182818284590452354);
    const char *s;

    // acos
    ASSERT_DOUBLE_OPERATION((a = 0.0, acos(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.03, acos(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.123, acos(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.245, acos(a)));

    // acosh
    ASSERT_DOUBLE_OPERATION((a = 1.0, acosh(a)));
    ASSERT_DOUBLE_OPERATION((a = pi, acosh(a)));
    ASSERT_DOUBLE_OPERATION((a = pi / 2.0, acosh(a)));
    ASSERT_DOUBLE_OPERATION((a = 2.45, acosh(a)));

    // asin
    ASSERT_DOUBLE_OPERATION((a = 0.0, asin(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.03, asin(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.123, asin(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.245, asin(a)));

    // asinh
    ASSERT_DOUBLE_OPERATION((a = 0.0, asinh(a)));
    ASSERT_DOUBLE_OPERATION((a = pi, asinh(a)));
    ASSERT_DOUBLE_OPERATION((a = pi / 2.0, asinh(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.245, asinh(a)));

    // atan
    ASSERT_DOUBLE_OPERATION((a = 0.0, atan(a)));
    ASSERT_DOUBLE_OPERATION((a = pi, atan(a)));
    ASSERT_DOUBLE_OPERATION((a = pi / 2.0, atan(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.245, atan(a)));

    // atan2
    ASSERT_DOUBLE_OPERATION((a = 0.0, b = 0.0, atan2(a, b)));
    ASSERT_DOUBLE_OPERATION((a = 10.0, b = 0.0, atan2(a, b)));
    ASSERT_DOUBLE_OPERATION((a = 0.0, b = 10.0, atan2(a, b)));
    ASSERT_DOUBLE_OPERATION((a = 10.0, b = 10.0, atan2(a, b)));

    // atanh
    ASSERT_DOUBLE_OPERATION((a = 0.0, atanh(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.999, atanh(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.5, atanh(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.245, atanh(a)));

    // ceil
    ASSERT_DOUBLE_OPERATION((a = 0.0, ceil(a)));
    ASSERT_DOUBLE_OPERATION((a = 3.245, ceil(a)));
    ASSERT_DOUBLE_OPERATION((a = -3.245, ceil(a)));
    ASSERT_DOUBLE_OPERATION((a = 3.6245, ceil(a)));
    ASSERT_DOUBLE_OPERATION((a = -3.6245, ceil(a)));
    ASSERT_DOUBLE_OPERATION((a = 3.5, ceil(a)));
    ASSERT_DOUBLE_OPERATION((a = -3.5, ceil(a)));

    // cos
    ASSERT_DOUBLE_OPERATION((a = 0.0, cos(a)));
    ASSERT_DOUBLE_OPERATION((a = pi, cos(a)));
    ASSERT_DOUBLE_OPERATION((a = pi / 2.0, cos(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.245, cos(a)));

    // cosh
    ASSERT_DOUBLE_OPERATION((a = 0.0, cosh(a)));
    ASSERT_DOUBLE_OPERATION((a = pi, cosh(a)));
    ASSERT_DOUBLE_OPERATION((a = pi / 2.0, cosh(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.245, cosh(a)));

    // ctime
	time_t the_time(1234123412);
    std::string the_ctime(ctime(&the_time));
    std::string::size_type p(the_ctime.find_first_of("\r\n"));
    if(p != std::string::npos)
    {
        the_ctime = the_ctime.substr(0, p);
    }
    CPPUNIT_ASSERT(compute_string("a = 1234123412, ctime(a)", the_ctime.c_str()));

    // exp
    ASSERT_DOUBLE_OPERATION((a = 0.0, exp(a)));
    ASSERT_DOUBLE_OPERATION((a = e, exp(a)));
    ASSERT_DOUBLE_OPERATION((a = 10.0, exp(a)));

    // fabs
    ASSERT_DOUBLE_OPERATION((a = 0.0, fabs(a)));
    ASSERT_DOUBLE_OPERATION((a = -0.0, fabs(a)));
    ASSERT_DOUBLE_OPERATION((a = e, fabs(a)));
    ASSERT_DOUBLE_OPERATION((a = -pi, fabs(a)));

    // floor
    ASSERT_DOUBLE_OPERATION((a = 0.0, floor(a)));
    ASSERT_DOUBLE_OPERATION((a = 3.245, floor(a)));
    ASSERT_DOUBLE_OPERATION((a = -3.245, floor(a)));
    ASSERT_DOUBLE_OPERATION((a = 3.6245, floor(a)));
    ASSERT_DOUBLE_OPERATION((a = -3.6245, floor(a)));
    ASSERT_DOUBLE_OPERATION((a = 3.5, floor(a)));
    ASSERT_DOUBLE_OPERATION((a = -3.5, floor(a)));

    // fmod
    ASSERT_DOUBLE_OPERATION((a = 0.0, b = 3.0, fmod(a, b)));
    ASSERT_DOUBLE_OPERATION((a = 10.0, b = 3.0, fmod(a, b)));
    ASSERT_DOUBLE_OPERATION((a = e * 45, b = 3.0, fmod(a, b)));
    ASSERT_DOUBLE_OPERATION((a = pi * 143.4, b = 3.0, fmod(a, b)));

    // log
    ASSERT_DOUBLE_OPERATION((a = 0.00001, log(a)));
    ASSERT_DOUBLE_OPERATION((a = 10.0, log(a)));
    ASSERT_DOUBLE_OPERATION((a = e * 45, log(a)));
    ASSERT_DOUBLE_OPERATION((a = pi * 143.4, log(a)));

    // log10
    ASSERT_DOUBLE_OPERATION((a = 0.00005, log10(a)));
    ASSERT_DOUBLE_OPERATION((a = 10.0, log10(a)));
    ASSERT_DOUBLE_OPERATION((a = e * 45, log10(a)));
    ASSERT_DOUBLE_OPERATION((a = pi * 143.4, log10(a)));

    // lrint
    ASSERT_LONG_OPERATION((a = 9444.32, lrint(a)));
    ASSERT_LONG_OPERATION((a = -744.66, lrint(a)));

    // pow
    ASSERT_DOUBLE_OPERATION((a = 0.0, b = 3.0, pow(a, b)));
    ASSERT_DOUBLE_OPERATION((a = 10.0, b = 0.0, pow(a, b)));
    ASSERT_DOUBLE_OPERATION((a = e * 45, b = 7.0, pow(a, b)));
    ASSERT_DOUBLE_OPERATION((a = pi * 143.4, b = 23.0, pow(a, b)));

    // rint
    ASSERT_DOUBLE_OPERATION((a = 9444.32, rint(a)));
    ASSERT_DOUBLE_OPERATION((a = -744.66, rint(a)));

#if !defined(WINDOWS) && !defined(_WINDOWS)
    // shell true and false are inverted!
    CPPUNIT_ASSERT(compute_long("shell( \"true\" , \"exitcode\" )") == 0);
    CPPUNIT_ASSERT(compute_long("shell(\"false\", \"exitcode\")") == 256);
    CPPUNIT_ASSERT(compute_string("shell(\"echo true\", \"output\")", "true\n"));
    CPPUNIT_ASSERT(compute_string("shell(\"echo true\")", "true\n"));
#endif

    // sin
    ASSERT_DOUBLE_OPERATION((a = 0.0, sin(a)));
    ASSERT_DOUBLE_OPERATION((a = pi, sin(a)));
    ASSERT_DOUBLE_OPERATION((a = pi / 2.0, sin(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.245, sin(a)));

    // sinh
    ASSERT_DOUBLE_OPERATION((a = 0.0, sinh(a)));
    ASSERT_DOUBLE_OPERATION((a = pi, sinh(a)));
    ASSERT_DOUBLE_OPERATION((a = pi / 2.0, sinh(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.245, sinh(a)));

    // sqrt
    ASSERT_DOUBLE_OPERATION((a = 0.0, sqrt(a)));
    ASSERT_DOUBLE_OPERATION((a = pi, sqrt(a)));
    ASSERT_DOUBLE_OPERATION((a = pi / 2.0, sqrt(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.245, sqrt(a)));

#if defined( __GNUC__ )
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wsign-compare"
#endif
    // strlen
    // DO NOT USE static_cast<>() for the strlen(), add -W... on the command line in CMakeLists.txt to turn off those warnings
    ASSERT_LONG_OPERATION((s = "9444.32", strlen(s)));
    ASSERT_LONG_OPERATION((s = "", strlen(s)));
    ASSERT_LONG_OPERATION((s = "con" "cat", (strlen(s))));
#if defined( __GNUC__ )
#   pragma GCC diagnostic pop
#endif

    // tan
    ASSERT_DOUBLE_OPERATION((a = 0.0, tan(a)));
    ASSERT_DOUBLE_OPERATION((a = pi, tan(a)));
    ASSERT_DOUBLE_OPERATION((a = pi / 2.0, tan(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.245, tan(a)));

    // tanh
    ASSERT_DOUBLE_OPERATION((a = 0.0, tanh(a)));
    ASSERT_DOUBLE_OPERATION((a = pi, tanh(a)));
    ASSERT_DOUBLE_OPERATION((a = pi / 2.0, tanh(a)));
    ASSERT_DOUBLE_OPERATION((a = 0.245, tanh(a)));

    // time
    time_t now(time(NULL));
    time_t expr_now(compute_long("time()"));
    CPPUNIT_ASSERT((expr_now - now) <= 1);
}

void ExprUnitTests::misc()
{
    libexpr::variable v, undefined, result;
    std::string str;
    undefined.to_string(str);
    CPPUNIT_ASSERT(str == "undefined");

    v.set(128394L);
    CPPUNIT_ASSERT_THROW(result.add(v, undefined), libexpr::incompatible_type);
    v.set(128.394);
    CPPUNIT_ASSERT_THROW(result.add(v, undefined), libexpr::incompatible_type);
    v.set("128394");
    CPPUNIT_ASSERT_THROW(result.add(v, undefined), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.add(undefined, v), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.lt(undefined, v), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.lt(v, undefined), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.le(undefined, v), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.le(v, undefined), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.gt(undefined, v), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.gt(v, undefined), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.ge(undefined, v), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.ge(v, undefined), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.eq(undefined, v), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.eq(v, undefined), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.ne(undefined, v), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.ne(v, undefined), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.logic_and(undefined, v), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.logic_and(v, undefined), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.logic_or(undefined, v), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.logic_or(v, undefined), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.logic_xor(undefined, v), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.logic_xor(v, undefined), libexpr::incompatible_type);
    CPPUNIT_ASSERT_THROW(result.logic_not(undefined), libexpr::incompatible_type);

    v.set(std::string("string"));
    v.to_string(str);
    CPPUNIT_ASSERT(str == "string");
    v.set(5.509);
    v.to_string(str);
    CPPUNIT_ASSERT(str == "5.509");
    v.set("string");
    v.to_string(str);
    CPPUNIT_ASSERT(str == "string");
    std::wstring wstr(L"wide-string");
    v.set(wstr);
    v.to_string(str);
    CPPUNIT_ASSERT(str == "wide-string");
    v.set(L"wc-string");
    v.to_string(str);
    CPPUNIT_ASSERT(str == "wc-string");

}

// vim: ts=4 sw=4 et
