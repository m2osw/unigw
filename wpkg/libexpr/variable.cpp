/*    variable -- a variable implementation
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
 * \brief Variable handling in the expression library.
 *
 * This file is the variable implementation of the expression library. It
 * manages variables as a map of name and value pairs.
 *
 * Variables can be set to the different types supported by the library:
 *
 * \li Undefined -- variable content is not yet defined
 * \li Integer (INT) -- the variable is an integer value
 * \li Float (FLT) -- the variable is a floating point value
 * \li String (STR) -- the variable is a string
 *
 * The library understands casting so integers, floating points, and strings
 * can be converted to another type on the fly as required (integers are
 * often converted to floating points, strings are often converted to a
 * number, and numbers are at times converted to strings automatically.)
 */
#include	"libexpr/variable.h"
#include	"libutf8/libutf8.h"
#include	<cmath>
#include	<sstream>
#include	<iostream>

#ifndef M_PI
#define M_PI		3.14159265358979323846
#endif
#ifndef M_E
#define M_E		2.7182818284590452354
#endif

namespace libexpr
{


/** \class variable
 * \brief Handle an expression variable.
 *
 * This class allows for the handling of expression variables. This means
 * a variable that can change type (integer, floating point, or string)
 * and that knows how to add, multiply, compare, etc. two variables against
 * each others.
 *
 * Note that strings are taken as UTF-8 strings. The class supports wide
 * strings and immediately converts them to UTF-8.
 */


/** \class variable_list
 * \brief List of variables.
 *
 * This class handles a list of variables. Variables are sorted by name
 * and can be retrieved by name.
 *
 * This is used as a pool of variables in another object such as your
 * expression evaluator implementation for reference to any variable.
 *
 * This list is not used as a list of arguments to a function since this
 * list is sorted by name. A list of arguments is in the order they are
 * supplied and thus uses a vector (see expr_evaluator::arglist)
 */


/** \brief Initializes a variable as undefined.
 *
 * This function initializes a variable and sets it to undefined.
 *
 * Use one of the set functions to change the value of a variable.
 * The variable::reset() functions resets the variable to undefined.
 */
variable::variable()
	: f_type(TYPE_UNDEFINED),
	  f_int(int()),
	  f_flt(double()),
	  f_str(std::string())
{
}

/** \brief Get the variable name.
 *
 * Variables that are given a name (i.e. an identifier) are given a
 * name using the set_name() function and the different expressions
 * that can handle a variable (++a, a = 5, etc.) can make use of
 * this name as appropriate.
 *
 * The main value is the content of the variable so in an expression
 * such as: a = b + c; the b and c variable values are used and their
 * names are ignored. The a variable is used and set as expected.
 *
 * \param[in] name  The name of this variable.
 *
 * \sa set_name()
 */
std::string variable::get_name() const
{
	return f_name;
}

/** \brief Set the variable name.
 *
 * This entry is used when encountering an identifier referencing
 * a variable. It's name is saved with the set_name() and can be
 * retrieved with the get_name() function.
 *
 * Values (direct integers, strings, floating point values) do not
 * receive a name.
 *
 * \return The name of this variable, may be empty.
 *
 * \sa get_name()
 */
void variable::set_name(const std::string& name)
{
	f_name = name;
}

/** \brief Get the variable type.
 *
 * This function is used to determine the type of a variable.
 *
 * \return one of the variable::TYPE_... enumerations
 */
variable::type_t variable::type() const
{
	return f_type;
}

/** \brief Reset the variable to undefined.
 *
 * This function resets the variable type to variable::TYPE_UNDEFINED.
 */
void variable::reset()
{
	f_type = TYPE_UNDEFINED;
}

/** \brief Set the variable to an integer value from a boolean value.
 *
 * This function changes the value of the variable and sets it to
 * a long value.
 *
 * This function as the side effect of changing the type of the
 * variable to variable::TYPE_INT.
 *
 * \param[in] value  The new boolean value of the variable.
 */
void variable::set(bool value)
{
	f_type = TYPE_INT;
	f_int = value ? 1L : 0L;
}

/** \brief Set the variable to an integer value.
 *
 * This function changes the value of the variable and sets it to
 * a long value.
 *
 * This function as the side effect of changing the type of the
 * variable to variable::TYPE_INT.
 *
 * \param[in] value  The new integer value of the variable.
 */
void variable::set(long value)
{
	f_type = TYPE_INT;
	f_int = value;
}

/** \brief Set the variable to a floating point value.
 *
 * This function changes the value of the variable and sets it to
 * a floating point value.
 *
 * This function as the side effect of changing the type of the
 * variable to variable::TYPE_FLT.
 *
 * \param[in] value  The new floating point value of the variable.
 */
void variable::set(double value)
{
	f_type = TYPE_FLT;
	f_flt = value;
}

/** \brief Set the variable to a string value.
 *
 * This function changes the value of the variable and sets it to
 * a string value.
 *
 * This function as the side effect of changing the type of the
 * variable to variable::TYPE_STR.
 *
 * \param[in] str  The new string value of the variable.
 */
void variable::set(const std::string& str)
{
	f_type = TYPE_STR;
	f_str = str;
}

/** \brief Set the variable to a string value.
 *
 * This function changes the value of the variable and sets it to
 * a string value.
 *
 * This function as the side effect of changing the type of the
 * variable to variable::TYPE_STR.
 *
 * \param[in] str  The new string value of the variable.
 */
void variable::set(const std::wstring& str)
{
	f_type = TYPE_STR;
	f_str = libutf8::wcstombs(str);
}

/** \brief Set the variable to a string value.
 *
 * This function changes the value of the variable and sets it to
 * a null terminated string value.
 *
 * This function as the side effect of changing the type of the
 * variable to variable::TYPE_STR.
 *
 * \param[in] str  The new string value of the variable.
 */
void variable::set(const char *str)
{
	f_type = TYPE_STR;
	f_str = str;
}

/** \brief Set the variable to a string value.
 *
 * This function changes the value of the variable and sets it to
 * a null terminated string value.
 *
 * This function as the side effect of changing the type of the
 * variable to variable::TYPE_STR.
 *
 * \param[in] str  The new string value of the variable.
 */
void variable::set(const wchar_t *str)
{
	f_type = TYPE_STR;
	f_str = libutf8::wcstombs(str);
}

/** \brief Get the integer value of the variable.
 *
 * This function is used to retrieve the integer value of the
 * variable.
 *
 * \bug
 * This function throws an error of type libexpr::invalid_type
 * if the variable is not current an integer.
 *
 * \param[out] value  The current value of the integer variable.
 */
void variable::get(long& value) const
{
	validate_type(TYPE_INT);
	value = f_int;
}

/** \brief Get the floating point value of the variable.
 *
 * This function is used to retrieve the floating point value of
 * the variable.
 *
 * \bug
 * This function throws an error of type libexpr::invalid_type
 * if the variable is not current a floating point.
 *
 * \param[out] value  The current value of the floating point variable.
 */
void variable::get(double& value) const
{
	validate_type(TYPE_FLT);
	value = f_flt;
}

/** \brief Get the string of the variable.
 *
 * This function is used to retrieve the string of
 * the variable.
 *
 * \bug
 * This function throws an error of type libexpr::invalid_type
 * if the variable is not current a string.
 *
 * \param[out] str  The current string of the variable.
 */
void variable::get(std::string& str) const
{
	validate_type(TYPE_STR);
	str = f_str;
}

/** \brief Get the content of the variable as a string.
 *
 * This function transforms the current content of the
 * variable into a string and returns that string.
 *
 * An undefined variable returns the word "undefined".
 * (which is indeed indistinguishable from the string
 * "undefined".)
 *
 * \param[out] str  The string representing the content of the variable.
 */
void variable::to_string(std::string& str) const
{
	std::stringstream out;

	switch(f_type) {
	case TYPE_UNDEFINED:
		str = "undefined";
		break;

	case TYPE_INT:
		out << f_int;
		str = out.str();
		break;

	case TYPE_FLT:
		out << f_flt;
		str = out.str();
		break;

	case TYPE_STR:
		str = f_str;
		break;

	}
}

/** \brief Internal function used to validate a typed request.
 *
 * This function checks that the type equals its parameter type.
 * If not, it throws a libexpr::invalid_type exception.
 *
 * \param[in] t        The type the variable needs to be defined as
 */
void variable::validate_type(type_t t) const
{
	if(f_type != t) {
		throw invalid_type("type mismatch");
	}
}

/** \brief Negate the integer or floating point.
 *
 * This function negates the input integer or floating point
 * and saves the result in this variable.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The value to negate
 */
void variable::neg(const variable& v1)
{
	switch(v1.type()) {
	case TYPE_INT:
		set(-v1.f_int);
		break;

	case TYPE_FLT:
		set(-v1.f_flt);
		break;

	default:
		throw incompatible_type("type not supported by unary - operator");

	}
}

/** \brief Apply the unary + operator on an integer or floating point.
 *
 * This function "pluses" the input integer or floating point
 * and saves the result in this variable.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The value to "pluses"
 */
void variable::pls(const variable& v1)
{
	switch(v1.f_type) {
	case TYPE_INT:
		set(+v1.f_int);
		break;

	case TYPE_FLT:
		set(+v1.f_flt);
		break;

	default:
		throw incompatible_type("type not supported by unary + operator");

	}
}

/** \brief Multiply v1 by v2.
 *
 * This function multiplies the input integer or floating point
 * values and saves the result in this variable.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::mul(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int * v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_int * v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by * operator");

		}
		break;

	case TYPE_FLT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_flt * v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_flt * v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by * operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by * operator");

	}
}

/** \brief Divide v1 by v2.
 *
 * This function divides the input integer or floating point
 * values and saves the result in this variable.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::div(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int / v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_int / v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by / operator");

		}
		break;

	case TYPE_FLT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_flt / v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_flt / v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by / operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by / operator");

	}
}

/** \brief Computes v1 modulo v2.
 *
 * This function computes the module between the input integer or
 * floating point values and saves the result in this variable.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::mod(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int % v2.f_int);
			break;

		default:
			throw incompatible_type("type not supported by %% operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by %% operator");

	}
}

/** \brief Add v1 to v2.
 *
 * This function adds the input integer or floating point values
 * and saves the result in this variable.
 *
 * This function concatenates the input if at least one of them
 * is a string then the other is converted to a string.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::add(const variable& v1, const variable& v2)
{
	std::string str;

	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int + v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_int + v2.f_flt);
			break;

		case TYPE_STR:
			v1.to_string(str);
			set(str + v2.f_str);
			break;

		default:
			throw incompatible_type("type not supported by the + operator");

		}
		break;

	case TYPE_FLT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_flt + v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_flt + v2.f_flt);
			break;

		case TYPE_STR:
			v1.to_string(str);
			set(str + v2.f_str);
			break;

		default:
			throw incompatible_type("type not supported by the + operator");

		}
		break;

	case TYPE_STR:
		switch(v2.f_type) {
		case TYPE_INT:
		case TYPE_FLT:
			v2.to_string(str);
			set(v1.f_str + str);
			break;

		case TYPE_STR:
			set(v1.f_str + v2.f_str);
			break;

		default:
			throw incompatible_type("type not supported by the + operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by + operator");

	}
}

/** \brief Subtract v2 from v1.
 *
 * This function subtract the input integer or floating point values
 * and saves the result in this variable.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::sub(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int - v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_int - v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the - operator");

		}
		break;

	case TYPE_FLT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_flt - v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_flt - v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the - operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by - operator");

	}
}

/** \brief Bitwise AND between v1 and v2.
 *
 * This function computes the bitwise AND of the input integer values
 * and saves the result in this variable.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::bitwise_and(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int & v2.f_int);
			break;

		default:
			throw incompatible_type("type not supported by the & operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by & operator");

	}
}

/** \brief Bitwise OR between v1 and v2.
 *
 * This function computes the bitwise OR of the input integer values
 * and saves the result in this variable.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::bitwise_or(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int | v2.f_int);
			break;

		default:
			throw incompatible_type("type not supported by the | operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by | operator");

	}
}

/** \brief Bitwise XOR between v1 and v2.
 *
 * This function computes the bitwise XOR of the input integer values
 * and saves the result in this variable.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::bitwise_xor(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int ^ v2.f_int);
			break;

		default:
			throw incompatible_type("type not supported by the ^ operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by ^ operator");

	}
}

/** \brief Bitwise NOT of v1.
 *
 * This function computes the bitwise XOR of the input integer values
 * and saves the result in this variable.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::bitwise_not(const variable& v1)
{
	switch(v1.f_type) {
	case TYPE_INT:
		set(~v1.f_int);
		break;

	default:
		throw incompatible_type("type not supported by ~ operator");

	}
}

/** \brief Left shift of v1 by v2.
 *
 * This function computes the left shift of the input integer values
 * and saves the result in this variable.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::shl(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int << v2.f_int);
			break;

		default:
			throw incompatible_type("type not supported by the << operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by << operator");

	}
}

/** \brief Signed right shift of v1 by v2.
 *
 * This function computes the right shift of the input integer values
 * and saves the result in this variable.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::shr(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int >> v2.f_int);
			break;

		default:
			throw incompatible_type("type not supported by the >> operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by >> operator");

	}
}

/** \brief Compare whether v1 is smaller than v2.
 *
 * This function compares the input values and saves true in
 * this variable if v1 is smaller than v2.
 *
 * Integers and floating points can be compared between each
 * others. Strings can be compared between each others.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::lt(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int < v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_int < v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the < operator");

		}
		break;

	case TYPE_FLT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_flt < v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_flt < v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the < operator");

		}
		break;

	case TYPE_STR:
		switch(v2.f_type) {
		case TYPE_STR:
			set(v1.f_str < v2.f_str);
			break;

		default:
			throw incompatible_type("type not supported by the < operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by < operator");

	}
}

/** \brief Compare whether v1 is smaller or equal to v2.
 *
 * This function compares the input values and saves true in
 * this variable if v1 is smaller or equal to v2.
 *
 * Integers and floating points can be compared between each
 * others. Strings can be compared between each others.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::le(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int <= v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_int <= v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the <= operator");

		}
		break;

	case TYPE_FLT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_flt <= v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_flt <= v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the <= operator");

		}
		break;

	case TYPE_STR:
		switch(v2.f_type) {
		case TYPE_STR:
			set(v1.f_str <= v2.f_str);
			break;

		default:
			throw incompatible_type("type not supported by the <= operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by <= operator");

	}
}

/** \brief Compare whether v1 is equal to v2.
 *
 * This function compares the input values and saves true in
 * this variable if v1 equals v2.
 *
 * Integers and floating points can be compared between each
 * others. Strings can be compared between each others.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::eq(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int == v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_int == v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the == operator");

		}
		break;

	case TYPE_FLT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_flt == v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_flt == v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the == operator");

		}
		break;

	case TYPE_STR:
		switch(v2.f_type) {
		case TYPE_STR:
			set(v1.f_str == v2.f_str);
			break;

		default:
			throw incompatible_type("type not supported by the == operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by == operator");

	}
}

/** \brief Compare whether v1 is not equal to v2.
 *
 * This function compares the input values and saves true in
 * this variable if v1 is not equal to v2.
 *
 * Integers and floating points can be compared between each
 * others. Strings can be compared between each others.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::ne(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int != v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_int != v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the != operator");

		}
		break;

	case TYPE_FLT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_flt != v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_flt != v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the != operator");

		}
		break;

	case TYPE_STR:
		switch(v2.f_type) {
		case TYPE_STR:
			set(v1.f_str != v2.f_str);
			break;

		default:
			throw incompatible_type("type not supported by the != operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by != operator");

	}
}

/** \brief Compare whether v1 is greater or equal to v2.
 *
 * This function compares the input values and saves true in
 * this variable if v1 is greater or equal to v2.
 *
 * Integers and floating points can be compared between each
 * others. Strings can be compared between each others.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::ge(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int >= v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_int >= v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the >= operator");

		}
		break;

	case TYPE_FLT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_flt >= v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_flt >= v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the >= operator");

		}
		break;

	case TYPE_STR:
		switch(v2.f_type) {
		case TYPE_STR:
			set(v1.f_str >= v2.f_str);
			break;

		default:
			throw incompatible_type("type not supported by the >= operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by >= operator");

	}
}

/** \brief Compare whether v1 is greater than v2.
 *
 * This function compares the input values and saves true in
 * this variable if v1 is greater than v2.
 *
 * Integers and floating points can be compared between each
 * others. Strings can be compared between each others.
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::gt(const variable& v1, const variable& v2)
{
	switch(v1.f_type) {
	case TYPE_INT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_int > v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_int > v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the > operator");

		}
		break;

	case TYPE_FLT:
		switch(v2.f_type) {
		case TYPE_INT:
			set(v1.f_flt > v2.f_int);
			break;

		case TYPE_FLT:
			set(v1.f_flt > v2.f_flt);
			break;

		default:
			throw incompatible_type("type not supported by the > operator");

		}
		break;

	case TYPE_STR:
		switch(v2.f_type) {
		case TYPE_STR:
			set(v1.f_str > v2.f_str);
			break;

		default:
			throw incompatible_type("type not supported by the > operator");

		}
		break;

	default:
		throw incompatible_type("type not supported by > operator");

	}
}

/** \brief Apply the logical AND between v1 and v2.
 *
 * This function applies the logical AND between v1 and v2.
 *
 * Integers and floating points are considered true when not
 * zero. Strings are considered true when not empty.
 *
 * The function returns an integer set to 0 (false) or 1 (true).
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::logic_and(const variable& v1, const variable& v2)
{
	bool l1, l2;

	switch(v1.f_type) {
	case TYPE_INT:
		l1 = v1.f_int != 0;
		break;

	case TYPE_FLT:
		l1 = v1.f_flt != 0;
		break;

	case TYPE_STR:
		l1 = !v1.f_str.empty();
		break;

	default:
		throw incompatible_type("type not supported by the && operator");

	}
	switch(v2.f_type) {
	case TYPE_INT:
		l2 = v2.f_int != 0;
		break;

	case TYPE_FLT:
		l2 = v2.f_flt != 0;
		break;

	case TYPE_STR:
		l2 = !v2.f_str.empty();
		break;

	default:
		throw incompatible_type("type not supported by the && operator");

	}

	set(l1 && l2 ? 1L : 0L);
}

/** \brief Apply the logical OR between v1 and v2.
 *
 * This function applies the logical OR between v1 and v2.
 *
 * Integers and floating points are considered true when not
 * zero. Strings are considered true when not empty.
 *
 * The function returns an integer set to 0 (false) or 1 (true).
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::logic_or(const variable& v1, const variable& v2)
{
	bool l1, l2;

	switch(v1.f_type) {
	case TYPE_INT:
		l1 = v1.f_int != 0;
		break;

	case TYPE_FLT:
		l1 = v1.f_flt != 0;
		break;

	case TYPE_STR:
		l1 = !v1.f_str.empty();
		break;

	default:
		throw incompatible_type("type not supported by the || operator");

	}
	switch(v2.f_type) {
	case TYPE_INT:
		l2 = v2.f_int != 0;
		break;

	case TYPE_FLT:
		l2 = v2.f_flt != 0;
		break;

	case TYPE_STR:
		l2 = !v2.f_str.empty();
		break;

	default:
		throw incompatible_type("type not supported by the || operator");

	}

	set(l1 || l2 ? 1L : 0L);
}

/** \brief Apply the logical XOR between v1 and v2.
 *
 * This function applies the logical XOR between v1 and v2.
 *
 * Integers and floating points are considered true when not
 * zero. Strings are considered true when not empty.
 *
 * The function returns an integer set to 0 (false) or 1 (true).
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::logic_xor(const variable& v1, const variable& v2)
{
	bool l1, l2;

	switch(v1.f_type) {
	case TYPE_INT:
		l1 = v1.f_int != 0;
		break;

	case TYPE_FLT:
		l1 = v1.f_flt != 0;
		break;

	case TYPE_STR:
		l1 = !v1.f_str.empty();
		break;

	default:
		throw incompatible_type("type not supported by the ^^ operator");

	}
	switch(v2.f_type) {
	case TYPE_INT:
		l2 = v2.f_int != 0;
		break;

	case TYPE_FLT:
		l2 = v2.f_flt != 0;
		break;

	case TYPE_STR:
		l2 = !v2.f_str.empty();
		break;

	default:
		throw incompatible_type("type not supported by the ^^ operator");

	}

	set(l1 ^ l2 ? 1L : 0L);
}

/** \brief Apply the logical NOT to v1.
 *
 * This function applies the logical NOT to v1.
 *
 * Integers and floating points are considered true when not
 * zero. Strings are considered true when not empty.
 *
 * The function returns an integer set to 0 (false) or 1 (true).
 *
 * \bug
 * This function throws an incompatible_type exception
 * whenever this function is called with a value of
 * incompatible type.
 *
 * \param[in] v1        The left hand side
 * \param[in] v2        The right hand side
 */
void variable::logic_not(const variable& v1)
{
	switch(v1.f_type) {
	case TYPE_INT:
		set(v1.f_int == 0 ? 1L : 0L);
		break;

	case TYPE_FLT:
		set(v1.f_flt == 0 ? 1L : 0L);
		break;

	case TYPE_STR:
		set(v1.f_str.empty() ? 1L : 0L);
		break;

	default:
		throw incompatible_type("type not supported by the ^^ operator");

	}
}



/** \brief Get a copy of the named variable
 *
 * This function searches for the named variable and if found
 * it copies the current value to the user supplied variable.
 * If not found, the function returns false and the user
 * supplied variable is variable::reset().
 *
 * This function knows of the "e" and "pi" variables internally.
 * You do not have to define those two variables, they will
 * always be defined.
 *
 * \param[in]  name       The name of the variable to search
 * \param[out] var        The user variable to set with the value of the variable found in the list
 *
 * \return true if the variable is found; false otherwise
 */
bool variable_list::get(const std::string& name, variable& var) const
{
	list_t::const_iterator v = f_list.find(name);
	if(v == f_list.end())
	{
		if(name == "e")
		{
			var.set(M_E);
		}
		else if(name == "pi")
		{
			var.set(M_PI);
		}
		else
		{
			var.reset();
			return false;
		}
	}
	else
	{
		var = v->second;
	}

	return true;
}

/** \brief Set a variable in the variable list.
 *
 * This function sets the named variable to the value of
 * the specified variable.
 *
 * If the variable does not exist yet, it is created.
 *
 * If the variable already exists, its current value is
 * overwritten. If you want to simulate constants, try
 * first to get() the variable. If you succeed, then
 * do not set.
 *
 * \param[in] name        The name of the variable
 * \param[in] var         The variable value
 */
void variable_list::set(const std::string& name, const variable& var)
{
	f_list[name] = var;
}




}		// namespace expr

