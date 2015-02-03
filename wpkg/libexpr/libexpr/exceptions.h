/*    exceptions -- the libexpr exceptions implementation
 *    Copyright (C) 2007-2015  Made to Order Software Corporation
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
 * \brief Exceptions that the expression library may emit.
 *
 * This file declares the exceptions that the library generates as errors
 * are detected.
 *
 * Note that the library may also raise basic exceptions such as
 * std::bad_alloc.
 */
#ifndef LIBEXPR_EXCEPTIONS_H
#define LIBEXPR_EXCEPTIONS_H

#include	"libexpr/expr_export.h"

#include	<stdexcept>
#include	<string>

namespace libexpr
{

/// All the libexpr exceptions
class libexpr_exception : public std::runtime_error
{
public:
	libexpr_exception(const std::string& msg) : runtime_error(msg) {}
};

/// Run-time errors in the libexpr (syntax errors, overflows, etc.)
class libexpr_runtime_error : public libexpr_exception
{
public:
	libexpr_runtime_error(const std::string& msg) : libexpr_exception(msg) {}
};

/// Some internal error (i.e. invalid value for a variable)
class libexpr_internal_error : public libexpr_exception
{
public:
	libexpr_internal_error(const std::string& msg) : libexpr_exception(msg) {}
};

/// Type is not valid for the required operation
class invalid_type : public libexpr_runtime_error
{
public:
	invalid_type(const std::string& msg) : libexpr_runtime_error(msg) {}
};

/// Type is valid, but not compatible for the operation
class incompatible_type : public libexpr_runtime_error
{
public:
	incompatible_type(const std::string& msg) : libexpr_runtime_error(msg) {}
};

/// Syntax error (unexpected or missing token)
class syntax_error : public libexpr_runtime_error
{
public:
	syntax_error(const std::string& msg) : libexpr_runtime_error(msg) {}
};

/// Expected a variable instead of a value (3 = 5, "3" incorrect, or 7++)
class expected_a_variable : public libexpr_runtime_error
{
public:
	expected_a_variable(const std::string& msg) : libexpr_runtime_error(msg) {}
};

/// Trying to read a variable that is not set yet
class undefined_variable : public libexpr_runtime_error
{
public:
	undefined_variable(const std::string& msg) : libexpr_runtime_error(msg) {}
};

/// Trying to call a function that is not defined
class undefined_function : public libexpr_runtime_error
{
public:
	undefined_function(const std::string& msg) : libexpr_runtime_error(msg) {}
};

/// Trying to call a function with an invalid argument
class function_args : public libexpr_runtime_error
{
public:
	function_args(const std::string& msg) : libexpr_runtime_error(msg) {}
};


}		// namespace libexpr


#endif // LIBEXPR_EXCEPTIONS_H
