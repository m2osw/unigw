/*    expr.c++ -- an expression evaluator implementation
 *    Copyright (C) 2007-2014  Made to Order Software Corporation
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

/** \file
 * \brief Implementation of the expression parser and interpreter.
 *
 * The expression library actually immediately transforms the input into
 * a result (i.e. it does not support variables that are not yet set to
 * a given value, in other words, an expression such as '3 + x' cannot
 * be returned as '3 + x'. If x is not set, an error occurs.)
 *
 * The parser is 100% compatible with the C/C++ expression parser. The order
 * for all the operators is respected exactly.
 */
#include	"libexpr/expr.h"

#include	<cmath>
#include	<iostream>
#include	<sstream>
#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<time.h>
#if defined(MO_CYGWIN) || defined(MO_FREEBSD)
#include	<sys/wait.h>
#endif


/** \brief The libexpr namespace for the expression library.
 *
 * The libexpr is the namespace that encompasses the expression library
 * declarations and implementations.
 */
namespace libexpr
{


/** \class expr_evaluator
 * \brief The evaluator class handles a user expression and converts it.
 *
 * This class is used to compute a user expression and convert it to
 * a number or a string. The result is returned in a variable.
 *
 * Internally, the evaluator makes use of an expression object which
 * handles the parsing of the expression as in C/C++. The computations
 * are actually implemented in the variables.
 */


#ifdef _MSC_VER
namespace
{
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

long lrint(double flt)
{
	// TODO: check for overflows?
	return static_cast<long>(rint(flt));
}
}
#endif


/** \brief The actual expression parser and execution stack.
 *
 * This class is the one that evaluates an expression and returns its
 * result in a variable. The expression is simply a string. The result
 * is put in a variable because it may be of any one type as supported
 * by the variable class.
 *
 * The evaluator is the class the user creates in order to evaluate
 * expressions. This is used so the user can define its own functions
 * via the call_function() virtual function.
 */
class expression
{
public:
	expression(expr_evaluator& evaluator);

	void eval(const std::string& expr, variable& result);

private:
	static const int TOK_ARROW		= 1000;

	static const int TOK_INCREMENT		= 1001;
	static const int TOK_DECREMENT		= 1002;

	static const int TOK_SHIFT_LEFT		= 1011;
	static const int TOK_SHIFT_RIGHT	= 1012;

	static const int TOK_LOGIC_AND		= 1021;
	static const int TOK_LOGIC_XOR		= 1022;
	static const int TOK_LOGIC_OR		= 1023;

	static const int TOK_LESS_EQUAL		= 1031;
	static const int TOK_GREATER_EQUAL	= 1032;

	static const int TOK_EQUAL		= 1041;
	static const int TOK_NOT_EQUAL		= 1042;

	static const int TOK_ASSIGN_MUL		= 1051;
	static const int TOK_ASSIGN_DIV		= 1052;
	static const int TOK_ASSIGN_MOD		= 1053;
	static const int TOK_ASSIGN_ADD		= 1054;
	static const int TOK_ASSIGN_SUB		= 1055;
	static const int TOK_ASSIGN_SHL		= 1056;
	static const int TOK_ASSIGN_SHR		= 1057;
	static const int TOK_ASSIGN_AND		= 1058;
	static const int TOK_ASSIGN_XOR		= 1059;
	static const int TOK_ASSIGN_OR		= 1060;

	static const int TOK_IDENTIFIER		= 1101;
	static const int TOK_STRING		= 1102;
	static const int TOK_INTEGER		= 1103;
	static const int TOK_FLOAT		= 1104;

	int expr_getc();
	void expr_ungetc(int c);
	void next_token();
	void number(int c);
	int backslash();
	void string();
	void character();
	void identifier(int c);
	void skip_c_comment();
	void skip_cpp_comment();

	void unary(variable& result);
	void array_func(variable& result);
	void postfix(variable& result);
	void prefix(variable& result);
	void multiplicative(variable& result);
	void additive(variable& result);
	void shift(variable& result);
	void relational(variable& result);
	void compare(variable& result);
	void bitwise_and(variable& result);
	void bitwise_xor(variable& result);
	void bitwise_or(variable& result);
	void logical_and(variable& result);
	void logical_xor(variable& result);
	void logical_or(variable& result);
	void conditional(variable& result);
	void assignment(variable& result);
	void comma(variable& result);

	expression& operator = (const expression&)
	{
		return *this;
	}

	expr_evaluator&		f_evaluator;
	int			f_token;	// the current token
	variable		f_var;		// for identifiers, strings, integers and floats
	int			f_unget;	// next character is not EOF
	size_t			f_pos;		// position in f_expr
	std::string		f_expr;
	int			f_line;		// number of lines
	int			f_character;	// character we're on within this line
};



expression::expression(expr_evaluator& evaluator)
	: f_evaluator(evaluator)
	, f_token(EOF)
	//, f_var -- auto-init
	, f_unget(EOF)
	, f_pos(0)
	//, f_expr -- auto-init
	, f_line(0)
	, f_character(0)
{
}


int expression::expr_getc()
{
	int	c;

	if(f_unget != EOF)
	{
		c = f_unget;
		f_unget = EOF;
		return c;
	}

	if(f_pos >= f_expr.size())
	{
		return EOF;
	}

	c = f_expr[f_pos];
	if(c == '\r')
	{
		if(f_expr[f_pos + 1] == '\n')
		{
			++f_pos;
		}
		++f_line;
		f_character = 1;
		c = '\n';
	}
	else if(c == '\n')
	{
		++f_line;
		f_character = 1;
	}
	else
	{
		++f_character;
	}

	++f_pos;

	return c;
}

void expression::expr_ungetc(int c)
{
	f_unget = c;
}


void expression::number(int c)
{
	f_token = TOK_INTEGER;

	long value = long();
	bool first;

	if(c == '0')
	{
		c = expr_getc();
		if(c == 'x' || c == 'X')
		{
			// hexadecimal
			first = true;
			for(;;)
			{
				c = expr_getc();
				if(c >= 'a' && c <= 'f')
				{
					value = (value << 4) | (c - ('a' - 10));
				}
				else if(c >= 'A' && c <= 'F')
				{
					value = (value << 4) | (c - ('A' - 10));
				}
				else if(c >= '0' && c <= '9')
				{
					value = (value << 4) | (c - '0');
				}
				else if(first)
				{
					// would C/C++ consider this as 0 then identifier starting with x?
					throw syntax_error("expected at least one hexadecimal character after 0x");
				}
				else
				{
					expr_ungetc(c);
					f_var.set(value);
					return;
				}
				first = false;
			}
		}
		// floating point?
		if(c != '.')
		{
			// octal
			for(;;)
			{
				if(c >= '0' && c <= '7')
				{
					value = (value << 3) | (c - '0');
				}
				else if(c == '8' || c == '9')
				{
					throw syntax_error("invalid octal digit of 8 or 9");
				}
				else
				{
					// in this case, no extra digits means the value is zero
					expr_ungetc(c);
					f_var.set(value);
					return;
				}
				c = expr_getc();
			}
		}
	}

	while(c >= '0' && c <= '9')
	{
		value = value * 10 + c - '0';
		c = expr_getc();
	}

	if(c != '.')
	{
		expr_ungetc(c);
		f_var.set(value);
		return;
	}

	// this is a floating point value
	f_token = TOK_FLOAT;

	double flt = double(value);

	double divisor = 0.1;

	c = expr_getc();
	while(c >= '0' && c <= '9')
	{
		flt += double(c - '0') * divisor;
		divisor /= 10.0;
		c = expr_getc();
	}

	if(c == 'e' || c == 'E')
	{
		// exponent
		double sign = 1;
		c = expr_getc();
		if(c == '+')
		{
			c = expr_getc();
		}
		else if(c == '-')
		{
			c = expr_getc();
			sign = -1;
		}
		if(c < '0' || c > '9')
		{
			throw syntax_error("invalid floating point exponent");
		}
		long exp = long();
		while(c >= '0' && c <= '9')
		{
			exp = exp * 10 + c - '0';
			c = expr_getc();
		}
		flt *= pow(10.0, exp * sign);
	}

	expr_ungetc(c);

	f_var.set(flt);
}

int expression::backslash()
{
	int c = expr_getc();

	// hex
	if(c == 'x' || c == 'X')
	{
		int x1, x2;
		c = expr_getc();
		if(c >= 'a' && c <= 'f')
		{
			x1 = c - ('a' - 10);
		}
		else if(c >= 'A' && c <= 'F')
		{
			x1 = c - ('A' - 10);
		}
		else if(c >= '0' && c <= '9')
		{
			x1 = c - '0';
		}
		else
		{
			throw syntax_error("unexpected digit for an hexadecimal escape sequence");
		}
		c = expr_getc();
		if(c >= 'a' && c <= 'f')
		{
			x2 = c - ('a' - 10);
		}
		else if(c >= 'A' && c <= 'F')
		{
			x2 = c - ('A' - 10);
		}
		else if(c >= '0' && c <= '9')
		{
			x2 = c - '0';
		}
		else
		{
			expr_ungetc(c);
			return x1;
		}
		return x1 * 16 + x2;
	}

	// octal
	if(c >= '0' && c <= '7')
	{
		int r = c - '0';
		c = expr_getc();
		if(c >= '0' && c <= '7') {
			r = (r << 3) | (c - '0');
			c = expr_getc();
			if(c >= '0' && c <= '7')
			{
				r = (r << 3) | (c - '0');
			}
			else
			{
				expr_ungetc(c);
			}
		}
		else
		{
			expr_ungetc(c);
		}
		return r;
	}

	// char
	switch(c)
	{
	case 'a': return '\a';
	case 'b': return '\b';
	case 'e': return '\033';	// escape
	case 'f': return '\f';
	case 'n': return '\n';
	case 'r': return '\r';
	case 't': return '\t';
	case 'v': return '\v';
	}

	// anything else is returned as is (especially \\ and \")
	return c;
}

void expression::string()
{
	f_token = TOK_STRING;

	std::string result;
	int c;

	for(;;)
	{
		c = expr_getc();
		if(c == EOF)
		{
			throw syntax_error("string not closed (missing \")");
		}
		if(c == '"')
		{
			f_var.set(result);
			return;
		}
		if(c == '\\')
		{
			c = backslash();
		}
		result += static_cast<char>(c);
	}
}

void expression::character()
{
	f_token = TOK_INTEGER;

	long c = expr_getc();
	if(c == '\\')
	{
		c = backslash();
	}

	f_var.set(c);

	c = expr_getc();
	if(c != '\'')
	{
		throw syntax_error("character not closed (missing ')");
	}
}

void expression::identifier(int c)
{
	f_token = TOK_IDENTIFIER;

	std::string result;
	result += static_cast<char>(c);

	for(;;)
	{
		c = expr_getc();
		if((c < '0' || c > '9')
		&& (c < 'a' || c > 'z')
		&& (c < 'A' || c > 'Z')
		&& c != '_')
		{
			expr_ungetc(c);
			break;
		}
		result += static_cast<char>(c);
	}

	// special identifiers that represent a value
	if(result == "true")
	{
		f_token = TOK_INTEGER;
		f_var.set(1L);
		return;
	}
	if(result == "false")
	{
		f_token = TOK_INTEGER;
		f_var.set(0L);
		return;
	}

	// XXX should we forbid all C++ keywords here? (for, if, while, inline, etc.)

	// the identifier is considered to be a variable name
	f_var.set_name(result);
}

void expression::skip_c_comment()
{
	int c;
	do
	{
		c = expr_getc();
		while(c == '*')
		{
			c = expr_getc();
			if(c == '/')
			{
				return;
			}
		}
	}
	while(c != EOF);
}

void expression::skip_cpp_comment()
{
	int c;
	do
	{
		c = expr_getc();
		while(c == '\n')
		{
			return;
		}
	} while(c != EOF);
}

void expression::next_token()
{
	int c;

	for(;;)
	{
		f_token = expr_getc();
		switch(f_token)
		{
		case ' ':
		case '\t':
		case '\n':	// expr_getc() transforms '\r' in '\n'
		case '\f':
			// skip blanks
			continue;

		case '"':
			string();
			return;

		case '\'':
			character();
			return;

		case '=':
			c = expr_getc();
			if(c == '=')
			{
				f_token = TOK_EQUAL;
				return;
			}
			expr_ungetc(c);
			return;

		case '!':
			c = expr_getc();
			if(c == '=')
			{
				f_token = TOK_NOT_EQUAL;
				return;
			}
			expr_ungetc(c);
			return;

		case '<':
			c = expr_getc();
			if(c == '=')
			{
				f_token = TOK_LESS_EQUAL;
				return;
			}
			if(c == '<')
			{
				c = expr_getc();
				if(c == '=')
				{
					f_token = TOK_ASSIGN_SHL;
					return;
				}
				f_token = TOK_SHIFT_LEFT;
			}
			expr_ungetc(c);
			return;

		case '>':
			c = expr_getc();
			if(c == '=')
			{
				f_token = TOK_GREATER_EQUAL;
				return;
			}
			if(c == '>')
			{
				c = expr_getc();
				if(c == '=')
				{
					f_token = TOK_ASSIGN_SHR;
					return;
				}
				f_token = TOK_SHIFT_RIGHT;
			}
			expr_ungetc(c);
			return;

		case '&':
			c = expr_getc();
			if(c == '&')
			{
				f_token = TOK_LOGIC_AND;
				return;
			}
			if(c == '=')
			{
				f_token = TOK_ASSIGN_AND;
				return;
			}
			expr_ungetc(c);
			return;

		case '^':
			c = expr_getc();
			if(c == '^')
			{
				f_token = TOK_LOGIC_XOR;
				return;
			}
			if(c == '=')
			{
				f_token = TOK_ASSIGN_XOR;
				return;
			}
			expr_ungetc(c);
			return;

		case '|':
			c = expr_getc();
			if(c == '|')
			{
				f_token = TOK_LOGIC_OR;
				return;
			}
			if(c == '=')
			{
				f_token = TOK_ASSIGN_OR;
				return;
			}
			expr_ungetc(c);
			return;

		case '+':
			c = expr_getc();
			if(c == '+')
			{
				f_token = TOK_INCREMENT;
				return;
			}
			if(c == '=')
			{
				f_token = TOK_ASSIGN_ADD;
				return;
			}
			expr_ungetc(c);
			return;

		case '-':
			c = expr_getc();
			if(c == '-')
			{
				f_token = TOK_DECREMENT;
				return;
			}
			if(c == '=')
			{
				f_token = TOK_ASSIGN_SUB;
				return;
			}
			if(c == '>')
			{
				f_token = TOK_ARROW;
				return;
			}
			expr_ungetc(c);
			return;

		case '*':
			c = expr_getc();
			if(c == '=')
			{
				f_token = TOK_ASSIGN_MUL;
				return;
			}
			expr_ungetc(c);
			return;

		case '/':
			c = expr_getc();
			if(c == '=')
			{
				f_token = TOK_ASSIGN_DIV;
				return;
			}
			if(c == '*')
			{
				skip_c_comment();
				continue;
			}
			if(c == '/')
			{
				skip_cpp_comment();
				continue;
			}
			expr_ungetc(c);
			return;

		case '%':
			c = expr_getc();
			if(c == '=')
			{
				f_token = TOK_ASSIGN_MOD;
				return;
			}
			expr_ungetc(c);
			return;

		}

		if((f_token >= '0' && f_token <= '9') || f_token == '.')
		{
			number(f_token);
			return;
		}

		if((f_token >= 'a' && f_token <= 'z')
		|| (f_token >= 'A' && f_token <= 'Z')
		|| f_token == '_')
		{
			identifier(f_token);
			return;
		}

		// anything else is returned as is (i.e. ',', '(', ')', etc.)
		return;
	}
}

void expression::unary(variable& result)
{
	variable value;

//std::cerr << "Unary with " << f_token << std::endl;

	switch(f_token)
	{
	case '!':
		next_token();
		prefix(value);
		result.logic_not(value);
		break;

	case '~':
		next_token();
		prefix(value);
		result.bitwise_not(value);
		break;

	case '+':
		next_token();
		prefix(value);
		result.pls(value);
		break;

	case '-':
		next_token();
		prefix(value);
		result.neg(value);
		break;

	case '(':
		//next_token(); -- comma() calls this function already
		comma(result);
		if(f_token != ')')
		{
			std::stringstream msg;
			if(f_token == TOK_IDENTIFIER)
			{
				msg << "identifier \"" << f_var.get_name() << "\"";
			}
			else
			{
				msg << "token number " << f_token;
			}
			throw syntax_error("expected ')' to close the parenthesis instead of " + msg.str());
		}
		next_token();
		break;

	case TOK_IDENTIFIER:
		{
			std::string name(f_var.get_name());
			result.set_name(name);
			next_token();
			if(f_token != '=' && f_token != '(' && !f_evaluator.get(name, result))
			{
				throw undefined_variable("undefined variable \"" + name + "\" (1)");
			}
		}
		break;

	case TOK_STRING:
		result = f_var;
		next_token();
		while(f_token == TOK_STRING)
		{
			// concatenate
			variable lhs(result);
			result.add(lhs, f_var);
			next_token();
		}
		break;

	case TOK_INTEGER:
	case TOK_FLOAT:
		result = f_var;
		next_token();
		break;

	default:
		// this includes ')', ';', EOF, etc.
		break;

	}
}


void expression::array_func(variable& result)
{
	unary(result);

	if(f_token == '(')
	{
		// skip the '('
		next_token();

		// function call
		expr_evaluator::arglist list;
		if(f_token != ')')
		{
			if(f_token == EOF)
			{
				throw syntax_error("unterminated list of parameters");
			}
			for(;;)
			{
				variable param;
				assignment(param);
				list.push_back(param);
				if(f_token == ')')
				{
					break;
				}
				if(f_token != ',')
				{
					throw syntax_error("expected a ',' or ')' in a function list of arguments");
				}
				// skip the ','
				next_token();
			}
		}

		// skip the ')'
		next_token();

		// call the function now
		std::string name(result.get_name());
		if(name.empty())
		{
			throw syntax_error("a function name must be an identifier");
		}
		f_evaluator.call_function(name, list, result);
	}
}


void expression::postfix(variable& result)
{
	array_func(result);

	long adjust;
	switch(f_token)
	{
	case TOK_INCREMENT:
		adjust = 1;
		break;

	case TOK_DECREMENT:
		adjust = -1;
		break;

	default:
		return;

	}

	next_token();
	std::string name(result.get_name());
	if(name.empty())
	{
		throw expected_a_variable("expected a variable to apply ++ or -- operator");
	}
	variable new_value, old_value, increment;
	if(!f_evaluator.get(name, old_value))
	{
		// this should not be reached by coverage tests since
		// TOK_IDENTIFIER already checked whether the variable existed
		throw undefined_variable("undefined variable \"" + name + "\" (2)"); // LCOV_EXCL_LINE
	}
	increment.set(adjust);
	new_value.add(old_value, increment);
	f_evaluator.set(name, new_value);
	// notice how result is not affected by the operation, only the variable
}


void expression::prefix(variable& result)
{
	long adjust;
	switch(f_token)
	{
	case TOK_INCREMENT:
		adjust = 1;
		next_token();
		break;;

	case TOK_DECREMENT:
		adjust = -1;
		next_token();
		break;

	default:
		adjust = 0;
		break;

	}

	postfix(result);

	if(adjust != 0)
	{
		std::string name(result.get_name());
		if(name.empty())
		{
			throw expected_a_variable("expected a variable to apply ++ or -- operator");
		}
		variable old_value, increment;
		if(!f_evaluator.get(name, old_value))
		{
			// this should not be reached by coverage tests since
			// TOK_IDENTIFIER already checked whether the variable existed
			throw undefined_variable("undefined variable \"" + name + "\" (3)"); // LCOV_EXCL_LINE
		}
		increment.set(adjust);
		result.add(old_value, increment);
		f_evaluator.set(name, result);
		// notice how in this case we tweak result instead of new_value
		// since the operation affects the result here
	}
}


void expression::multiplicative(variable& result)
{
	prefix(result);

	for(;;)
	{
		switch(f_token)
		{
		case '*':
			{
			variable lhs(result), rhs;
			next_token();
			prefix(rhs);
			result.mul(lhs, rhs);
			}
			continue;

		case '/':
			{
			variable lhs(result), rhs;
			next_token();
			prefix(rhs);
			result.div(lhs, rhs);
			}
			continue;

		case '%':
			{
			variable lhs(result), rhs;
			next_token();
			prefix(rhs);
			result.mod(lhs, rhs);
			}
			continue;

		default:;
			/*FALLTHROUGH*/
		}
		break;
	}
}


void expression::additive(variable& result)
{
	multiplicative(result);

	for(;;)
	{
		switch(f_token)
		{
		case '+':
			{
			variable lhs(result), rhs;
			next_token();
			multiplicative(rhs);
			result.add(lhs, rhs);
			}
			continue;

		case '-':
			{
			variable lhs(result), rhs;
			next_token();
			multiplicative(rhs);
			result.sub(lhs, rhs);
			}
			continue;

		default:;
			/*FALLTHROUGH*/
		}
		break;
	}
}


void expression::shift(variable& result)
{
	additive(result);

	for(;;)
	{
		switch(f_token)
		{
		case TOK_SHIFT_LEFT:	// <<
			{
			variable lhs(result), rhs;
			next_token();
			additive(rhs);
			result.shl(lhs, rhs);
			}
			continue;

		case TOK_SHIFT_RIGHT:	// >>
			{
			variable lhs(result), rhs;
			next_token();
			additive(rhs);
			result.shr(lhs, rhs);
			}
			continue;

		default:;
			/*FALLTHROUGH*/
		}
		break;
	}
}


void expression::relational(variable& result)
{
	shift(result);

	for(;;)
	{
		switch(f_token)
		{
		case '<':
			{
			variable lhs(result), rhs;
			next_token();
			shift(rhs);
			result.lt(lhs, rhs);
			}
			continue;

		case TOK_LESS_EQUAL:	// <=
			{
			variable lhs(result), rhs;
			next_token();
			shift(rhs);
			result.le(lhs, rhs);
			}
			continue;

		case TOK_GREATER_EQUAL:	// >=
			{
			variable lhs(result), rhs;
			next_token();
			shift(rhs);
			result.ge(lhs, rhs);
			}
			continue;

		case '>':
			{
			variable lhs(result), rhs;
			next_token();
			shift(rhs);
			result.gt(lhs, rhs);
			}
			continue;

		default:;
			/*FALLTHROUGH*/
		}
		break;
	}
}


void expression::compare(variable& result)
{
	relational(result);

	for(;;)
	{
		switch(f_token)
		{
		case TOK_EQUAL:	// ==
			{
			variable lhs(result), rhs;
			next_token();
			relational(rhs);
			result.eq(lhs, rhs);
			}
			continue;

		case TOK_NOT_EQUAL:	// !=
			{
			variable lhs(result), rhs;
			next_token();
			relational(rhs);
			result.ne(lhs, rhs);
			}
			continue;

		default:;
			/*FALLTHROUGH*/
		}
		break;
	}
}


void expression::bitwise_and(variable& result)
{
	compare(result);

	while(f_token == '&') {
		variable lhs(result), rhs;
		next_token();
		compare(rhs);

		result.bitwise_and(lhs, rhs);
	}
}


void expression::bitwise_xor(variable& result)
{
	bitwise_and(result);

	while(f_token == '^')
	{
		variable lhs(result), rhs;
		next_token();
		bitwise_and(rhs);

		result.bitwise_xor(lhs, rhs);
	}
}


void expression::bitwise_or(variable& result)
{
	bitwise_xor(result);

	while(f_token == '|')
	{
		variable lhs(result), rhs;
		next_token();
		bitwise_xor(rhs);

		result.bitwise_or(lhs, rhs);
	}
}


void expression::logical_and(variable& result)
{
	bitwise_or(result);

	while(f_token == TOK_LOGIC_AND)		// &&
	{
		variable lhs(result), rhs;
		next_token();
		bitwise_or(rhs);

		result.logic_and(lhs, rhs);
	}
}


void expression::logical_xor(variable& result)
{
	logical_and(result);

	while(f_token == TOK_LOGIC_XOR)		// ^^
	{
		variable lhs(result), rhs;
		next_token();
		logical_and(rhs);

		result.logic_xor(lhs, rhs);
	}
}


void expression::logical_or(variable& result)
{
	logical_xor(result);

	while(f_token == TOK_LOGIC_OR)		// ||
	{
		variable lhs(result), rhs;
		next_token();
		logical_xor(rhs);

		result.logic_or(lhs, rhs);
	}
}


void expression::conditional(variable& result)
{
	logical_or(result);

	if(f_token == '?')
	{
		variable if_true, if_false, test;
		long value;

		// TODO: we should avoid evaluating these expressions
		//	 depending on the value of result.
		//next_token(); -- comma calls this function already
		comma(if_true);
		if(f_token != ':')
		{
			throw syntax_error("expected ':' in conditional");
		}
		next_token();
		assignment(if_false);
		test.logic_not(result);
		test.get(value);
		if(value == 0) // WARNING: we use logic_not() so this is inverted
		{
			result = if_true;
		}
		else {
			result = if_false;
		}
	}
}


void expression::assignment(variable& result)
{
	variable value, old_value, new_value;
	std::string name;
	int assign;

	conditional(result);

	switch(f_token) {
	case '=':
		next_token();
		assignment(value);
		name = result.get_name();
		if(name.empty())
		{
			throw expected_a_variable("expected a variable to apply the assignment operator (1)");
		}
		f_evaluator.set(name, value);
		result = value;
		break;

	case TOK_ASSIGN_MUL:
	case TOK_ASSIGN_DIV:
	case TOK_ASSIGN_MOD:
	case TOK_ASSIGN_ADD:
	case TOK_ASSIGN_SUB:
	case TOK_ASSIGN_SHL:
	case TOK_ASSIGN_SHR:
	case TOK_ASSIGN_AND:
	case TOK_ASSIGN_XOR:
	case TOK_ASSIGN_OR:
		assign = f_token;
		next_token();
		assignment(value);
		name = result.get_name();
		if(name.empty())
		{
			throw expected_a_variable("expected a variable to apply the assignment operator (2)");
		}
		if(!f_evaluator.get(name, old_value))
		{
			// this should not be reached by coverage tests since
			// TOK_IDENTIFIER already checked whether the variable existed
			throw undefined_variable("undefined variable \"" + name + "\" (4)"); // LCOV_EXCL_LINE
		}
		switch(assign) {
		case TOK_ASSIGN_MUL:
			new_value.mul(old_value, value);
			break;

		case TOK_ASSIGN_DIV:
			new_value.div(old_value, value);
			break;

		case TOK_ASSIGN_MOD:
			new_value.mod(old_value, value);
			break;

		case TOK_ASSIGN_ADD:
			new_value.add(old_value, value);
			break;

		case TOK_ASSIGN_SUB:
			new_value.sub(old_value, value);
			break;

		case TOK_ASSIGN_SHL:
			new_value.shl(old_value, value);
			break;

		case TOK_ASSIGN_SHR:
			new_value.shr(old_value, value);
			break;

		case TOK_ASSIGN_AND:
			new_value.bitwise_and(old_value, value);
			break;

		case TOK_ASSIGN_XOR:
			new_value.bitwise_xor(old_value, value);
			break;

		case TOK_ASSIGN_OR:
			new_value.bitwise_or(old_value, value);
			break;

		}
		f_evaluator.set(name, new_value);
		result = new_value;
		break;

	}
}


void expression::comma(variable& result)
{
	do {
		next_token();
		result.reset();
		assignment(result);
	} while(f_token == ',');
}


void expression::eval(const std::string& expr, variable& result)
{
	//f_evaluator -- defined in constructor, do not reset between calls
	f_token = EOF;
	f_var.reset();
	f_unget = EOF;
	f_pos = 0;
	f_expr = expr;
	f_line = 1;
	f_character = 1;

	comma(result);

	// allow any number of ';' at the end of the expression
	bool has_semicolon(false);
	while(f_token == ';')
	{
		has_semicolon = true;
		next_token();
	}

	if(f_token != EOF)
	{
		if(!has_semicolon && f_token == ')')
		{
			throw syntax_error("missing '(', found ')' at the end of the expression");
		}
		throw syntax_error("expected the end of the expression, found another token instead");
	}
}



/** \brief Clean up an expression evaluator.
 *
 * This destructor cleans up the object as required.
 *
 * Note that it is here mainly because it is virtual so it is possible to
 * derive from this class and have all the destructors called as expected.
 */
expr_evaluator::~expr_evaluator()
{
}

void expr_evaluator::eval(const std::string& expr, variable& result)
{
	expression e(*this);
	e.eval(expr, result);
}


namespace {
void func_acos(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(acos(flt));
}

void func_acosh(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(acosh(flt));
}

void func_asin(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(asin(flt));
}

void func_asinh(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(asinh(flt));
}

void func_atan(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(atan(flt));
}

void func_atan2(expr_evaluator::arglist& list, variable& result)
{
	double x, y;
	list[0].get(x);
	list[1].get(y);
	result.set(atan2(x, y));
}

void func_atanh(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(atanh(flt));
}

void func_ceil(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(ceil(flt));
}

void func_cos(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(cos(flt));
}

void func_cosh(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(cosh(flt));
}

void func_ctime(expr_evaluator::arglist& list, variable& result)
{
	long t;
	list[0].get(t);
	time_t tt(t);
	std::string r(ctime(&tt));
	if(r.size() > 0 && *r.rbegin() == '\n')
	{
		r = r.substr(0, r.size() - 1);
	}
	if(r.size() > 0 && *r.rbegin() == '\r')
	{
		// IMPORTANT NOTE: this may not be accessible by the coverage
		// depending on whether the function generates a \\r and a \\n
		r = r.substr(0, r.size() - 1);
	}
	result.set(r);
}

void func_exp(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(exp(flt));
}

void func_fabs(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(fabs(flt));
}

void func_floor(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(floor(flt));
}

void func_fmod(expr_evaluator::arglist& list, variable& result)
{
	double nominator, denominator;
	list[0].get(nominator);
	list[1].get(denominator);
	result.set(fmod(nominator, denominator));
}

void func_log(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(log(flt));
}

void func_log10(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(log10(flt));
}

void func_lrint(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(lrint(flt));
}

void func_pow(expr_evaluator::arglist& list, variable& result)
{
	double value, power;
	list[0].get(value);
	list[1].get(power);
	result.set(pow(value, power));
}

void func_rint(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(rint(flt));
}

void func_shell(expr_evaluator::arglist& list, variable& result)
{
	std::string command;
	std::string mode("output");
	list[0].get(command);
	if(list.size() == 2)
	{
		list[1].get(mode);
	}
	if(mode == "output")
	{
#if defined(MO_WINDOWS)
#	define popen _popen
#	define pclose _pclose
#endif
		FILE *p = popen(command.c_str(), "r");
		if(p == NULL)
		{
			throw libexpr_runtime_error("command \"" + command + "\" could not be started with popen().");
		}
		std::string output;
		for(;;)
		{
			// TODO: use wchar_t under MS-Windows and convert to UTF-8
			char buf[4096];
			buf[sizeof(buf) - 1] = '\0';
			char *r(fgets(buf, sizeof(buf) - 1, p));
			if(r == 0)
			{
				break;
			}
			output += buf;
		}
		int e(pclose(p));
        // unfortunately it doesn't look too good to test this to know that the command failed?!
#if defined(MO_WINDOWS)
		if( e == -1 )
#else
#ifdef __GNUC__
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#	pragma GCC diagnostic push
#	pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#endif
		if(!WIFEXITED(e) || WEXITSTATUS(e) == 127)
#ifdef __GNUC__
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#	pragma GCC diagnostic pop
#endif
#endif
#endif
		{
			throw libexpr_runtime_error("command \"" + command + "\" could not be started with popen().");
		}
		result.set(output);
	}
	else if(mode == "exitcode")
	{
		long r(system(command.c_str()));
		result.set(r);
	}
	else
	{
		throw function_args("the second argument to shell() must be \"output\" or \"exitcode\", not \"" + mode + "\"");
	}
}

void func_sin(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(sin(flt));
}

void func_sinh(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(sinh(flt));
}

void func_sqrt(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(sqrt(flt));
}

void func_strlen(expr_evaluator::arglist& list, variable& result)
{
	std::string str;
	list[0].get(str);
	result.set(static_cast<long>(str.size()));
}

void func_tan(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(tan(flt));
}

void func_tanh(expr_evaluator::arglist& list, variable& result)
{
	double flt;
	list[0].get(flt);
	result.set(tanh(flt));
}

void func_time(expr_evaluator::arglist& /*list*/, variable& result)
{
	result.set(static_cast<long>(time(NULL)));
}
}	// namespace <noname>

void expr_evaluator::call_function(std::string& name, arglist& list, variable& result)
{
	struct func_t {
		const char *	f_name;
		size_t		f_min;
		size_t		f_max;
		void		(*f_func)(arglist& list, variable& result);
	};

	static const func_t functions[] = {
		{ "acos",		1, 1, func_acos },
		{ "acosh",		1, 1, func_acosh },
		{ "asin",		1, 1, func_asin },
		{ "asinh",		1, 1, func_asinh },
		{ "atan",		1, 1, func_atan },
		{ "atan2",		2, 2, func_atan2 },
		{ "atanh",		1, 1, func_atanh },
		{ "ceil",		1, 1, func_ceil },
		{ "cos",		1, 1, func_cos },
		{ "cosh",		1, 1, func_cosh },
		{ "ctime",		1, 1, func_ctime },
		{ "exp",		1, 1, func_exp },
		{ "fabs",		1, 1, func_fabs },
		{ "floor",		1, 1, func_floor },
		{ "fmod",		2, 2, func_fmod },
		{ "log",		1, 1, func_log },
		{ "log10",		1, 1, func_log10 },
		{ "lrint",		1, 1, func_lrint },
		{ "pow",		2, 2, func_pow },
		{ "rint",		1, 1, func_rint },
		{ "shell",		1, 2, func_shell },
		{ "sin",		1, 1, func_sin },
		{ "sinh",		1, 1, func_sinh },
		{ "sqrt",		1, 1, func_sqrt },
		{ "strlen",		1, 1, func_strlen },
		{ "tan",		1, 1, func_tan },
		{ "tanh",		1, 1, func_tanh },
		{ "time",		0, 0, func_time }
	};

	// find system functions to execute and call them with arglist

	int i, j, p, r;

	i = 0;
	j = sizeof(functions) / sizeof(functions[0]);
	while(i < j) {
		// get the center position of the current range
		p = i + (j - i) / 2;
		r = strcmp(name.c_str(), functions[p].f_name);
		if(r == 0) {
			// found it! verify the number of arguments
			if(list.size() < functions[p].f_min
			|| list.size() > functions[p].f_max) {
				throw function_args("the invalid number of arguments was specified");
			}
			(*functions[p].f_func)(list, result);
			return;
		}
		if(r > 0) {
			// move the range up (we already checked p so use p + 1)
			p++;
			i = p;
		}
		else {
			// move the range down (we never check an item at position j)
			j = p;
		}
	}

	std::stringstream msg;
	msg << "cannot call undefined function \"" << name << "\"" << std::endl;
	throw undefined_function(msg.str());
}



}		// namespace libexpr
// vim: ts=8 sw=8
