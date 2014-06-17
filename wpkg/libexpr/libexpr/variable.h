/*    variable -- the libexpr variable implementation
 *    Copyright (C) 2007-2013  Made to Order Software Corporation
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
 * \brief Declarations of the variable support of the expression library.
 *
 * This header defines the variable class and the variable list class
 * which are used by the expression class while parsing and computing
 * an expression. The variable class knows how to process any one
 * operator against two different variables.
 *
 * The list of variable sorts the variables by name. It cannot be used
 * to create a list of parameters to a function call.
 */
#ifndef LIBEXPR_VARIABLE_H
#define LIBEXPR_VARIABLE_H

#include	"libexpr/exceptions.h"
#include	<map>

namespace libexpr
{


class EXPR_EXPORT variable
{
public:
	enum type_t
	{
		TYPE_UNDEFINED,
		TYPE_INT,
		TYPE_FLT,
		TYPE_STR
	};

	variable();

	std::string get_name() const;
	void set_name(const std::string& name);

	type_t type() const;
	void reset();
	void get(long& value) const;
	void get(double& value) const;
	void get(std::string& str) const;
	void set(bool value);
	void set(long value);
	void set(double value);
	void set(const std::string& str);
	void set(const std::wstring& str);
	void set(const char *str);
	void set(const wchar_t *str);
	// XXX add char16 and char32 for tr1/C++11
	void to_string(std::string& str) const;

	// arithmetic
	void neg(const variable& v1);
	void pls(const variable& v1);
	void mul(const variable& v1, const variable& v2);
	void div(const variable& v1, const variable& v2);
	void mod(const variable& v1, const variable& v2);
	void add(const variable& v1, const variable& v2);
	void sub(const variable& v1, const variable& v2);
	void bitwise_and(const variable& v1, const variable& v2);
	void bitwise_or(const variable& v1, const variable& v2);
	void bitwise_xor(const variable& v1, const variable& v2);
	void bitwise_not(const variable& v1);

	// shift
	void shl(const variable& v1, const variable& v2);
	void shr(const variable& v1, const variable& v2);

	// comparisons
	void lt(const variable& v1, const variable& v2);
	void le(const variable& v1, const variable& v2);
	void eq(const variable& v1, const variable& v2);
	void ne(const variable& v1, const variable& v2);
	void ge(const variable& v1, const variable& v2);
	void gt(const variable& v1, const variable& v2);

	// logic
	void logic_and(const variable& v1, const variable& v2);
	void logic_or(const variable& v1, const variable& v2);
	void logic_xor(const variable& v1, const variable& v2);
	void logic_not(const variable& v1);

private:
	void validate_type(type_t t) const;

	type_t			f_type;
	std::string		f_name;
	long			f_int;
	double			f_flt;
	std::string		f_str;
};




class EXPR_EXPORT variable_list
{
public:
	bool get(const std::string& name, variable& var) const;
	void set(const std::string& name, const variable& var);

private:
	typedef std::map<const std::string, variable>	list_t;
	typedef std::pair<const std::string, variable>	pair_t;
	list_t			f_list;
};





}
#endif
// LIBEXPR_EXCEPTIONS_H
