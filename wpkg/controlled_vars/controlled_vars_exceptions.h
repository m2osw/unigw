// WARNING: do not edit; this is an auto-generated
// WARNING: file; please, use the generator named
// WARNING: controlled_vars to re-generate
//
// File:	controlled_vars_exceptions.h
// Object:	Help you by constraining basic types like classes.
//
// Copyright:	Copyright (c) 2005-2012 Made to Order Software Corp.
//		All Rights Reserved.
//
// http://snapwebsites.org/
// contact@m2osw.com
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//
#ifndef CONTROLLED_VARS_EXCEPTIONS_H
#define CONTROLLED_VARS_EXCEPTIONS_H
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4005 4018 4244 4800)
#if _MSC_VER > 1000
#pragma once
#endif
#elif defined(__GNUC__)
#if (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) || (__GNUC__ >= 4)
#pragma once
#endif
#endif
#include <limits.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdexcept>
namespace controlled_vars {
class controlled_vars_error : public std::logic_error {
public:
	explicit controlled_vars_error(const std::string& what_msg) : logic_error(what_msg) {}
};
class controlled_vars_error_not_initialized : public controlled_vars_error {
public:
	explicit controlled_vars_error_not_initialized(const std::string& what_msg) : controlled_vars_error(what_msg) {}
};
class controlled_vars_error_out_of_bounds : public controlled_vars_error {
public:
	explicit controlled_vars_error_out_of_bounds(const std::string& what_msg) : controlled_vars_error(what_msg) {}
};
class controlled_vars_error_null_pointer : public controlled_vars_error {
public:
	explicit controlled_vars_error_null_pointer(const std::string& what_msg) : controlled_vars_error(what_msg) {}
};
} // namespace controlled_vars
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
