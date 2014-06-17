// WARNING: do not edit; this is an auto-generated
// WARNING: file; please, use the generator named
// WARNING: controlled_vars to re-generate
//
// File:	controlled_vars_limited_no_init.h
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
#ifndef CONTROLLED_VARS_LIMITED_NO_INIT_H
#define CONTROLLED_VARS_LIMITED_NO_INIT_H
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
#include "controlled_vars_exceptions.h"
#include "controlled_vars_static_assert.h"
namespace controlled_vars {
#ifdef CONTROLLED_VARS_DEBUG
/** \brief Documentation available online.
 * Please go to http://snapwebsites.org/project/controlled-vars
 */
template<class T, T min, T max> class limited_no_init {
public:
	typedef T primary_type_t;
	static const primary_type_t MIN_BOUND = min;
	static const primary_type_t MAX_BOUND = max;
	CONTROLLED_VARS_STATIC_ASSERT(min <= max);
	template<class L> T check(L v) {
#ifdef CONTROLLED_VARS_LIMITED
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wlogical-op"
#endif
		if(v < min || v > max) throw controlled_vars_error_out_of_bounds("value out of bounds");
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#endif
		return static_cast<primary_type_t>(v);
	}
	limited_no_init() { f_initialized = false; }
	limited_no_init(bool v) { f_initialized = true; f_value = check(v); }
	limited_no_init(char v) { f_initialized = true; f_value = check(v); }
	limited_no_init(signed char v) { f_initialized = true; f_value = check(v); }
	limited_no_init(unsigned char v) { f_initialized = true; f_value = check(v); }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	limited_no_init(wchar_t v) { f_initialized = true; f_value = check(v); }
#endif
	limited_no_init(int16_t v) { f_initialized = true; f_value = check(v); }
	limited_no_init(uint16_t v) { f_initialized = true; f_value = check(v); }
	limited_no_init(int32_t v) { f_initialized = true; f_value = check(v); }
	limited_no_init(uint32_t v) { f_initialized = true; f_value = check(v); }
#if UINT_MAX == ULONG_MAX
	limited_no_init(long v) { f_initialized = true; f_value = check(v); }
#endif
#if UINT_MAX == ULONG_MAX
	limited_no_init(unsigned long v) { f_initialized = true; f_value = check(v); }
#endif
	limited_no_init(int64_t v) { f_initialized = true; f_value = check(v); }
	limited_no_init(uint64_t v) { f_initialized = true; f_value = check(v); }
	limited_no_init(float v) { f_initialized = true; f_value = check(v); }
	limited_no_init(double v) { f_initialized = true; f_value = check(v); }
#ifndef __APPLE__
	limited_no_init(long double v) { f_initialized = true; f_value = check(v); }
#endif
#ifdef __APPLE__
	limited_no_init(size_t v) { f_initialized = true; f_value = check(v); }
#endif
#ifdef __APPLE__
	limited_no_init(time_t v) { f_initialized = true; f_value = check(v); }
#endif
	operator T () const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value; }
	operator T () { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value; }
	T value() const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value; }
	const T * ptr() const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return &f_value; }
	T * ptr() { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return &f_value; }
	bool operator ! () const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return !f_value; }
	T operator ~ () const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return ~f_value; }
	T operator + () const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return +f_value; }
	T operator - () const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return -f_value; }
	limited_no_init& operator ++ () { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(f_value + 1 > max) throw controlled_vars_error_out_of_bounds("++ would render value out of bounds"); ++f_value; return *this; }
	limited_no_init operator ++ (int) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); limited_no_init<T, min, max> result(*this); if(f_value + 1 > max) throw controlled_vars_error_out_of_bounds("++ would render value out of bounds"); ++f_value; return result; }
	limited_no_init& operator -- () { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(f_value - 1 < min) throw controlled_vars_error_out_of_bounds("-- would render value out of bounds"); --f_value; return *this; }
	limited_no_init operator -- (int) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); limited_no_init<T, min, max> result(*this); if(f_value - 1 < min) throw controlled_vars_error_out_of_bounds("-- would render value out of bounds"); --f_value; return result; }
	limited_no_init& operator = (const limited_no_init& n) { f_initialized = true; if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(n.f_value); return *this; }
	limited_no_init& operator = (bool v) { f_initialized = true; f_value = check(v); return *this; }
	limited_no_init& operator = (char v) { f_initialized = true; f_value = check(v); return *this; }
	limited_no_init& operator = (signed char v) { f_initialized = true; f_value = check(v); return *this; }
	limited_no_init& operator = (unsigned char v) { f_initialized = true; f_value = check(v); return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	limited_no_init& operator = (wchar_t v) { f_initialized = true; f_value = check(v); return *this; }
#endif
	limited_no_init& operator = (int16_t v) { f_initialized = true; f_value = check(v); return *this; }
	limited_no_init& operator = (uint16_t v) { f_initialized = true; f_value = check(v); return *this; }
	limited_no_init& operator = (int32_t v) { f_initialized = true; f_value = check(v); return *this; }
	limited_no_init& operator = (uint32_t v) { f_initialized = true; f_value = check(v); return *this; }
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator = (long v) { f_initialized = true; f_value = check(v); return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator = (unsigned long v) { f_initialized = true; f_value = check(v); return *this; }
#endif
	limited_no_init& operator = (int64_t v) { f_initialized = true; f_value = check(v); return *this; }
	limited_no_init& operator = (uint64_t v) { f_initialized = true; f_value = check(v); return *this; }
	limited_no_init& operator = (float v) { f_initialized = true; f_value = check(v); return *this; }
	limited_no_init& operator = (double v) { f_initialized = true; f_value = check(v); return *this; }
#ifndef __APPLE__
	limited_no_init& operator = (long double v) { f_initialized = true; f_value = check(v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator = (size_t v) { f_initialized = true; f_value = check(v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator = (time_t v) { f_initialized = true; f_value = check(v); return *this; }
#endif
	limited_no_init& operator *= (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * n.f_value); return *this; }
	limited_no_init& operator *= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
	limited_no_init& operator *= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
	limited_no_init& operator *= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
	limited_no_init& operator *= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	limited_no_init& operator *= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
#endif
	limited_no_init& operator *= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
	limited_no_init& operator *= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
	limited_no_init& operator *= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
	limited_no_init& operator *= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator *= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator *= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
#endif
	limited_no_init& operator *= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
	limited_no_init& operator *= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
	limited_no_init& operator *= (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
	limited_no_init& operator *= (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
#ifndef __APPLE__
	limited_no_init& operator *= (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator *= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator *= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value * v); return *this; }
#endif
	limited_no_init& operator /= (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / n.f_value); return *this; }
	limited_no_init& operator /= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
	limited_no_init& operator /= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
	limited_no_init& operator /= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
	limited_no_init& operator /= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	limited_no_init& operator /= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
#endif
	limited_no_init& operator /= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
	limited_no_init& operator /= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
	limited_no_init& operator /= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
	limited_no_init& operator /= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator /= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator /= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
#endif
	limited_no_init& operator /= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
	limited_no_init& operator /= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
	limited_no_init& operator /= (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
	limited_no_init& operator /= (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
#ifndef __APPLE__
	limited_no_init& operator /= (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator /= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator /= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value / v); return *this; }
#endif
	limited_no_init& operator %= (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % n.f_value); return *this; }
	limited_no_init& operator %= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
	limited_no_init& operator %= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
	limited_no_init& operator %= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
	limited_no_init& operator %= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	limited_no_init& operator %= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
#endif
	limited_no_init& operator %= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
	limited_no_init& operator %= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
	limited_no_init& operator %= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
	limited_no_init& operator %= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator %= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator %= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
#endif
	limited_no_init& operator %= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
	limited_no_init& operator %= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
#ifdef __APPLE__
	limited_no_init& operator %= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator %= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value % v); return *this; }
#endif
	limited_no_init& operator += (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + n.f_value); return *this; }
	limited_no_init& operator += (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
	limited_no_init& operator += (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
	limited_no_init& operator += (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
	limited_no_init& operator += (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	limited_no_init& operator += (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
#endif
	limited_no_init& operator += (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
	limited_no_init& operator += (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
	limited_no_init& operator += (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
	limited_no_init& operator += (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator += (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator += (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
#endif
	limited_no_init& operator += (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
	limited_no_init& operator += (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
	limited_no_init& operator += (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
	limited_no_init& operator += (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
#ifndef __APPLE__
	limited_no_init& operator += (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator += (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator += (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value + v); return *this; }
#endif
	limited_no_init& operator -= (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - n.f_value); return *this; }
	limited_no_init& operator -= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
	limited_no_init& operator -= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
	limited_no_init& operator -= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
	limited_no_init& operator -= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	limited_no_init& operator -= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
#endif
	limited_no_init& operator -= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
	limited_no_init& operator -= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
	limited_no_init& operator -= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
	limited_no_init& operator -= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator -= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator -= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
#endif
	limited_no_init& operator -= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
	limited_no_init& operator -= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
	limited_no_init& operator -= (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
	limited_no_init& operator -= (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
#ifndef __APPLE__
	limited_no_init& operator -= (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator -= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator -= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value - v); return *this; }
#endif
	limited_no_init& operator <<= (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << n.f_value); return *this; }
	limited_no_init& operator <<= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
	limited_no_init& operator <<= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
	limited_no_init& operator <<= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
	limited_no_init& operator <<= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	limited_no_init& operator <<= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
#endif
	limited_no_init& operator <<= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
	limited_no_init& operator <<= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
	limited_no_init& operator <<= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
	limited_no_init& operator <<= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator <<= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator <<= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
#endif
	limited_no_init& operator <<= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
	limited_no_init& operator <<= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
#ifdef __APPLE__
	limited_no_init& operator <<= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator <<= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value << v); return *this; }
#endif
	limited_no_init& operator >>= (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> n.f_value); return *this; }
	limited_no_init& operator >>= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
	limited_no_init& operator >>= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
	limited_no_init& operator >>= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
	limited_no_init& operator >>= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	limited_no_init& operator >>= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
#endif
	limited_no_init& operator >>= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
	limited_no_init& operator >>= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
	limited_no_init& operator >>= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
	limited_no_init& operator >>= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator >>= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator >>= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
#endif
	limited_no_init& operator >>= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
	limited_no_init& operator >>= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
#ifdef __APPLE__
	limited_no_init& operator >>= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator >>= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value >> v); return *this; }
#endif
	limited_no_init& operator &= (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & n.f_value); return *this; }
	limited_no_init& operator &= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
	limited_no_init& operator &= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
	limited_no_init& operator &= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
	limited_no_init& operator &= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	limited_no_init& operator &= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
#endif
	limited_no_init& operator &= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
	limited_no_init& operator &= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
	limited_no_init& operator &= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
	limited_no_init& operator &= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator &= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator &= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
#endif
	limited_no_init& operator &= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
	limited_no_init& operator &= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
#ifdef __APPLE__
	limited_no_init& operator &= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator &= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value & v); return *this; }
#endif
	limited_no_init& operator |= (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | n.f_value); return *this; }
	limited_no_init& operator |= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
	limited_no_init& operator |= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
	limited_no_init& operator |= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
	limited_no_init& operator |= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	limited_no_init& operator |= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
#endif
	limited_no_init& operator |= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
	limited_no_init& operator |= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
	limited_no_init& operator |= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
	limited_no_init& operator |= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator |= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator |= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
#endif
	limited_no_init& operator |= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
	limited_no_init& operator |= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
#ifdef __APPLE__
	limited_no_init& operator |= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator |= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value | v); return *this; }
#endif
	limited_no_init& operator ^= (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ n.f_value); return *this; }
	limited_no_init& operator ^= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
	limited_no_init& operator ^= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
	limited_no_init& operator ^= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
	limited_no_init& operator ^= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	limited_no_init& operator ^= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
#endif
	limited_no_init& operator ^= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
	limited_no_init& operator ^= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
	limited_no_init& operator ^= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
	limited_no_init& operator ^= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator ^= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	limited_no_init& operator ^= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
#endif
	limited_no_init& operator ^= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
	limited_no_init& operator ^= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
#ifdef __APPLE__
	limited_no_init& operator ^= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
#endif
#ifdef __APPLE__
	limited_no_init& operator ^= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = check(f_value ^ v); return *this; }
#endif
	T operator * (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * n.f_value; }
	T operator * (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
	T operator * (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
	T operator * (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
	T operator * (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator * (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
#endif
	T operator * (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
	T operator * (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
	T operator * (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
	T operator * (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
#if UINT_MAX == ULONG_MAX
	T operator * (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator * (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
#endif
	T operator * (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
	T operator * (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
	T operator * (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
	T operator * (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
#ifndef __APPLE__
	T operator * (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
#endif
#ifdef __APPLE__
	T operator * (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
#endif
#ifdef __APPLE__
	T operator * (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * v; }
#endif
	T operator / (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / n.f_value; }
	T operator / (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
	T operator / (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
	T operator / (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
	T operator / (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator / (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
#endif
	T operator / (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
	T operator / (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
	T operator / (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
	T operator / (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
#if UINT_MAX == ULONG_MAX
	T operator / (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator / (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
#endif
	T operator / (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
	T operator / (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
	T operator / (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
	T operator / (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
#ifndef __APPLE__
	T operator / (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
#endif
#ifdef __APPLE__
	T operator / (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
#endif
#ifdef __APPLE__
	T operator / (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / v; }
#endif
	T operator % (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % n.f_value; }
	T operator % (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
	T operator % (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
	T operator % (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
	T operator % (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator % (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
#endif
	T operator % (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
	T operator % (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
	T operator % (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
	T operator % (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
#if UINT_MAX == ULONG_MAX
	T operator % (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator % (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
#endif
	T operator % (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
	T operator % (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
#ifdef __APPLE__
	T operator % (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
#endif
#ifdef __APPLE__
	T operator % (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % v; }
#endif
	T operator + (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + n.f_value; }
	T operator + (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
	T operator + (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
	T operator + (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
	T operator + (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator + (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
#endif
	T operator + (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
	T operator + (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
	T operator + (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
	T operator + (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
#if UINT_MAX == ULONG_MAX
	T operator + (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator + (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
#endif
	T operator + (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
	T operator + (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
	T operator + (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
	T operator + (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
#ifndef __APPLE__
	T operator + (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
#endif
#ifdef __APPLE__
	T operator + (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
#endif
#ifdef __APPLE__
	T operator + (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + v; }
#endif
	T operator - (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - n.f_value; }
	T operator - (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
	T operator - (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
	T operator - (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
	T operator - (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator - (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
#endif
	T operator - (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
	T operator - (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
	T operator - (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
	T operator - (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
#if UINT_MAX == ULONG_MAX
	T operator - (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator - (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
#endif
	T operator - (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
	T operator - (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
	T operator - (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
	T operator - (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
#ifndef __APPLE__
	T operator - (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
#endif
#ifdef __APPLE__
	T operator - (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
#endif
#ifdef __APPLE__
	T operator - (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - v; }
#endif
	T operator << (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << n.f_value; }
	T operator << (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
	T operator << (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
	T operator << (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
	T operator << (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator << (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
#endif
	T operator << (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
	T operator << (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
	T operator << (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
	T operator << (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
#if UINT_MAX == ULONG_MAX
	T operator << (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator << (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
#endif
	T operator << (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
	T operator << (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
#ifdef __APPLE__
	T operator << (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
#endif
#ifdef __APPLE__
	T operator << (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << v; }
#endif
	T operator >> (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> n.f_value; }
	T operator >> (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
	T operator >> (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
	T operator >> (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
	T operator >> (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator >> (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
#endif
	T operator >> (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
	T operator >> (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
	T operator >> (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
	T operator >> (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
#if UINT_MAX == ULONG_MAX
	T operator >> (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator >> (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
#endif
	T operator >> (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
	T operator >> (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
#ifdef __APPLE__
	T operator >> (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
#endif
#ifdef __APPLE__
	T operator >> (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> v; }
#endif
	T operator & (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & n.f_value; }
	T operator & (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
	T operator & (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
	T operator & (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
	T operator & (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator & (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
#endif
	T operator & (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
	T operator & (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
	T operator & (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
	T operator & (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
#if UINT_MAX == ULONG_MAX
	T operator & (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator & (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
#endif
	T operator & (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
	T operator & (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
#ifdef __APPLE__
	T operator & (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
#endif
#ifdef __APPLE__
	T operator & (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & v; }
#endif
	T operator | (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | n.f_value; }
	T operator | (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
	T operator | (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
	T operator | (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
	T operator | (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator | (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
#endif
	T operator | (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
	T operator | (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
	T operator | (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
	T operator | (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
#if UINT_MAX == ULONG_MAX
	T operator | (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator | (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
#endif
	T operator | (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
	T operator | (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
#ifdef __APPLE__
	T operator | (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
#endif
#ifdef __APPLE__
	T operator | (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | v; }
#endif
	T operator ^ (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ n.f_value; }
	T operator ^ (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
	T operator ^ (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
	T operator ^ (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
	T operator ^ (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator ^ (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
#endif
	T operator ^ (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
	T operator ^ (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
	T operator ^ (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
	T operator ^ (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
#if UINT_MAX == ULONG_MAX
	T operator ^ (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator ^ (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
#endif
	T operator ^ (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
	T operator ^ (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
#ifdef __APPLE__
	T operator ^ (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
#endif
#ifdef __APPLE__
	T operator ^ (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ v; }
#endif
	bool operator == (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == n.f_value; }
	bool operator == (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
	bool operator == (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
	bool operator == (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
	bool operator == (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	bool operator == (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
#endif
	bool operator == (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
	bool operator == (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
	bool operator == (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
	bool operator == (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
#if UINT_MAX == ULONG_MAX
	bool operator == (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
#endif
#if UINT_MAX == ULONG_MAX
	bool operator == (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
#endif
	bool operator == (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
	bool operator == (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
	bool operator == (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
	bool operator == (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
#ifndef __APPLE__
	bool operator == (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
#endif
#ifdef __APPLE__
	bool operator == (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
#endif
#ifdef __APPLE__
	bool operator == (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == v; }
#endif
	bool operator != (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != n.f_value; }
	bool operator != (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
	bool operator != (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
	bool operator != (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
	bool operator != (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	bool operator != (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
#endif
	bool operator != (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
	bool operator != (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
	bool operator != (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
	bool operator != (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
#if UINT_MAX == ULONG_MAX
	bool operator != (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
#endif
#if UINT_MAX == ULONG_MAX
	bool operator != (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
#endif
	bool operator != (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
	bool operator != (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
	bool operator != (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
	bool operator != (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
#ifndef __APPLE__
	bool operator != (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
#endif
#ifdef __APPLE__
	bool operator != (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
#endif
#ifdef __APPLE__
	bool operator != (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != v; }
#endif
	bool operator < (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < n.f_value; }
	bool operator < (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
	bool operator < (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
	bool operator < (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
	bool operator < (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	bool operator < (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
#endif
	bool operator < (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
	bool operator < (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
	bool operator < (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
	bool operator < (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
#if UINT_MAX == ULONG_MAX
	bool operator < (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
#endif
#if UINT_MAX == ULONG_MAX
	bool operator < (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
#endif
	bool operator < (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
	bool operator < (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
	bool operator < (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
	bool operator < (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
#ifndef __APPLE__
	bool operator < (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
#endif
#ifdef __APPLE__
	bool operator < (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
#endif
#ifdef __APPLE__
	bool operator < (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < v; }
#endif
	bool operator <= (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= n.f_value; }
	bool operator <= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
	bool operator <= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
	bool operator <= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
	bool operator <= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	bool operator <= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
#endif
	bool operator <= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
	bool operator <= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
	bool operator <= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
	bool operator <= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
#if UINT_MAX == ULONG_MAX
	bool operator <= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
#endif
#if UINT_MAX == ULONG_MAX
	bool operator <= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
#endif
	bool operator <= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
	bool operator <= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
	bool operator <= (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
	bool operator <= (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
#ifndef __APPLE__
	bool operator <= (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
#endif
#ifdef __APPLE__
	bool operator <= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
#endif
#ifdef __APPLE__
	bool operator <= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= v; }
#endif
	bool operator > (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > n.f_value; }
	bool operator > (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
	bool operator > (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
	bool operator > (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
	bool operator > (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	bool operator > (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
#endif
	bool operator > (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
	bool operator > (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
	bool operator > (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
	bool operator > (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
#if UINT_MAX == ULONG_MAX
	bool operator > (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
#endif
#if UINT_MAX == ULONG_MAX
	bool operator > (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
#endif
	bool operator > (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
	bool operator > (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
	bool operator > (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
	bool operator > (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
#ifndef __APPLE__
	bool operator > (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
#endif
#ifdef __APPLE__
	bool operator > (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
#endif
#ifdef __APPLE__
	bool operator > (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > v; }
#endif
	bool operator >= (const limited_no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= n.f_value; }
	bool operator >= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
	bool operator >= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
	bool operator >= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
	bool operator >= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	bool operator >= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
#endif
	bool operator >= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
	bool operator >= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
	bool operator >= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
	bool operator >= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
#if UINT_MAX == ULONG_MAX
	bool operator >= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
#endif
#if UINT_MAX == ULONG_MAX
	bool operator >= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
#endif
	bool operator >= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
	bool operator >= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
	bool operator >= (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
	bool operator >= (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
#ifndef __APPLE__
	bool operator >= (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
#endif
#ifdef __APPLE__
	bool operator >= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
#endif
#ifdef __APPLE__
	bool operator >= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= v; }
#endif
	bool is_initialized() const { return f_initialized; }
private:
	bool f_initialized;
	T f_value;
};
typedef limited_no_init<bool, false, true> rlbool_t;
#else
typedef bool rlbool_t;
#endif
} // namespace controlled_vars
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
