// WARNING: do not edit; this is an auto-generated
// WARNING: file; please, use the generator named
// WARNING: controlled_vars to re-generate
//
// File:	controlled_vars_static_assert.h
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
#ifndef CONTROLLED_VARS_STATIC_ASSERT_H
#define CONTROLLED_VARS_STATIC_ASSERT_H
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
namespace controlled_vars {
// The following is 100% coming from boost/static_assert.hpp
// At this time we only support MSC and GNUC
#if defined(_MSC_VER)||defined(__GNUC__)
#define CONTROLLED_VARS_JOIN(X,Y) CONTROLLED_VARS_DO_JOIN(X,Y)
#define CONTROLLED_VARS_DO_JOIN(X,Y) CONTROLLED_VARS_DO_JOIN2(X,Y)
#define CONTROLLED_VARS_DO_JOIN2(X,Y) X##Y
template<bool x> struct STATIC_ASSERTION_FAILURE;
template<> struct STATIC_ASSERTION_FAILURE<true>{enum{value=1};};
template<int x> struct static_assert_test{};
#if defined(__GNUC__)&&((__GNUC__>3)||((__GNUC__==3)&&(__GNUC_MINOR__>=4)))
#define CONTROLLED_VARS_STATIC_ASSERT_BOOL_CAST(x) ((x)==0?false:true)
#else
#define CONTROLLED_VARS_STATIC_ASSERT_BOOL_CAST(x) (bool)(x)
#endif
#ifdef _MSC_VER
#define CONTROLLED_VARS_STATIC_ASSERT(B) typedef ::controlled_vars::static_assert_test<sizeof(::controlled_vars::STATIC_ASSERTION_FAILURE<CONTROLLED_VARS_STATIC_ASSERT_BOOL_CAST(B)>)>CONTROLLED_VARS_JOIN(controlled_vars_static_assert_typedef_,__COUNTER__)
#else
#define CONTROLLED_VARS_STATIC_ASSERT(B) typedef ::controlled_vars::static_assert_test<sizeof(::controlled_vars::STATIC_ASSERTION_FAILURE<CONTROLLED_VARS_STATIC_ASSERT_BOOL_CAST(B)>)>CONTROLLED_VARS_JOIN(controlled_vars_static_assert_typedef_,__LINE__)
#endif
#else
#define CONTROLLED_VARS_STATIC_ASSERT(B)
#endif
} // namespace controlled_vars
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
