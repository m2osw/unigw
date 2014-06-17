// WARNING: do not edit; this is an auto-generated
// WARNING: file; please, use the generator named
// WARNING: controlled_vars to re-generate
//
// File:	controlled_vars_no_init.h
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
#ifndef CONTROLLED_VARS_NO_INIT_H
#define CONTROLLED_VARS_NO_INIT_H
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
namespace controlled_vars {
#ifdef CONTROLLED_VARS_DEBUG
/** \brief Documentation available online.
 * Please go to http://snapwebsites.org/project/controlled-vars
 */
template<class T> class no_init {
public:
	typedef T primary_type_t;
	no_init() { f_initialized = false; }
	no_init(bool v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
	no_init(char v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
	no_init(signed char v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
	no_init(unsigned char v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	no_init(wchar_t v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
#endif
	no_init(int16_t v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
	no_init(uint16_t v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
	no_init(int32_t v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
	no_init(uint32_t v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
#if UINT_MAX == ULONG_MAX
	no_init(long v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
#endif
#if UINT_MAX == ULONG_MAX
	no_init(unsigned long v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
#endif
	no_init(int64_t v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
	no_init(uint64_t v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
	no_init(float v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
	no_init(double v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
#ifndef __APPLE__
	no_init(long double v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
#endif
#ifdef __APPLE__
	no_init(size_t v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
#endif
#ifdef __APPLE__
	no_init(time_t v) { f_initialized = true; f_value = static_cast<primary_type_t>(v); }
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
	no_init& operator ++ () { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); ++f_value; return *this; }
	no_init operator ++ (int) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); no_init<T> result(*this); ++f_value; return result; }
	no_init& operator -- () { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); --f_value; return *this; }
	no_init operator -- (int) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); no_init<T> result(*this); --f_value; return result; }
	no_init& operator = (const no_init& n) { f_initialized = true; if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value = n.f_value; return *this; }
	no_init& operator = (bool v) { f_initialized = true; f_value = v; return *this; }
	no_init& operator = (char v) { f_initialized = true; f_value = v; return *this; }
	no_init& operator = (signed char v) { f_initialized = true; f_value = v; return *this; }
	no_init& operator = (unsigned char v) { f_initialized = true; f_value = v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	no_init& operator = (wchar_t v) { f_initialized = true; f_value = v; return *this; }
#endif
	no_init& operator = (int16_t v) { f_initialized = true; f_value = v; return *this; }
	no_init& operator = (uint16_t v) { f_initialized = true; f_value = v; return *this; }
	no_init& operator = (int32_t v) { f_initialized = true; f_value = v; return *this; }
	no_init& operator = (uint32_t v) { f_initialized = true; f_value = v; return *this; }
#if UINT_MAX == ULONG_MAX
	no_init& operator = (long v) { f_initialized = true; f_value = v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	no_init& operator = (unsigned long v) { f_initialized = true; f_value = v; return *this; }
#endif
	no_init& operator = (int64_t v) { f_initialized = true; f_value = v; return *this; }
	no_init& operator = (uint64_t v) { f_initialized = true; f_value = v; return *this; }
	no_init& operator = (float v) { f_initialized = true; f_value = v; return *this; }
	no_init& operator = (double v) { f_initialized = true; f_value = v; return *this; }
#ifndef __APPLE__
	no_init& operator = (long double v) { f_initialized = true; f_value = v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator = (size_t v) { f_initialized = true; f_value = v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator = (time_t v) { f_initialized = true; f_value = v; return *this; }
#endif
	no_init& operator *= (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= n.f_value; return *this; }
	no_init& operator *= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
	no_init& operator *= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
	no_init& operator *= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
	no_init& operator *= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	no_init& operator *= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
#endif
	no_init& operator *= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
	no_init& operator *= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
	no_init& operator *= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
	no_init& operator *= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
#if UINT_MAX == ULONG_MAX
	no_init& operator *= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	no_init& operator *= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
#endif
	no_init& operator *= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
	no_init& operator *= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
	no_init& operator *= (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
	no_init& operator *= (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
#ifndef __APPLE__
	no_init& operator *= (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator *= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator *= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value *= v; return *this; }
#endif
	no_init& operator /= (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= n.f_value; return *this; }
	no_init& operator /= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
	no_init& operator /= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
	no_init& operator /= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
	no_init& operator /= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	no_init& operator /= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
#endif
	no_init& operator /= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
	no_init& operator /= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
	no_init& operator /= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
	no_init& operator /= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
#if UINT_MAX == ULONG_MAX
	no_init& operator /= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	no_init& operator /= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
#endif
	no_init& operator /= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
	no_init& operator /= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
	no_init& operator /= (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
	no_init& operator /= (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
#ifndef __APPLE__
	no_init& operator /= (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator /= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator /= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value /= v; return *this; }
#endif
	no_init& operator %= (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= n.f_value; return *this; }
	no_init& operator %= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
	no_init& operator %= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
	no_init& operator %= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
	no_init& operator %= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	no_init& operator %= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
#endif
	no_init& operator %= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
	no_init& operator %= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
	no_init& operator %= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
	no_init& operator %= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
#if UINT_MAX == ULONG_MAX
	no_init& operator %= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	no_init& operator %= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
#endif
	no_init& operator %= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
	no_init& operator %= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
#ifdef __APPLE__
	no_init& operator %= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator %= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value %= v; return *this; }
#endif
	no_init& operator += (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += n.f_value; return *this; }
	no_init& operator += (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
	no_init& operator += (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
	no_init& operator += (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
	no_init& operator += (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	no_init& operator += (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
#endif
	no_init& operator += (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
	no_init& operator += (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
	no_init& operator += (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
	no_init& operator += (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
#if UINT_MAX == ULONG_MAX
	no_init& operator += (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	no_init& operator += (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
#endif
	no_init& operator += (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
	no_init& operator += (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
	no_init& operator += (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
	no_init& operator += (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
#ifndef __APPLE__
	no_init& operator += (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator += (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator += (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value += v; return *this; }
#endif
	no_init& operator -= (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= n.f_value; return *this; }
	no_init& operator -= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
	no_init& operator -= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
	no_init& operator -= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
	no_init& operator -= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	no_init& operator -= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
#endif
	no_init& operator -= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
	no_init& operator -= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
	no_init& operator -= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
	no_init& operator -= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
#if UINT_MAX == ULONG_MAX
	no_init& operator -= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	no_init& operator -= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
#endif
	no_init& operator -= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
	no_init& operator -= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
	no_init& operator -= (float v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
	no_init& operator -= (double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
#ifndef __APPLE__
	no_init& operator -= (long double v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator -= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator -= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value -= v; return *this; }
#endif
	no_init& operator <<= (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= n.f_value; return *this; }
	no_init& operator <<= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
	no_init& operator <<= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
	no_init& operator <<= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
	no_init& operator <<= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	no_init& operator <<= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
#endif
	no_init& operator <<= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
	no_init& operator <<= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
	no_init& operator <<= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
	no_init& operator <<= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
#if UINT_MAX == ULONG_MAX
	no_init& operator <<= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	no_init& operator <<= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
#endif
	no_init& operator <<= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
	no_init& operator <<= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
#ifdef __APPLE__
	no_init& operator <<= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator <<= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value <<= v; return *this; }
#endif
	no_init& operator >>= (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= n.f_value; return *this; }
	no_init& operator >>= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
	no_init& operator >>= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
	no_init& operator >>= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
	no_init& operator >>= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	no_init& operator >>= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
#endif
	no_init& operator >>= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
	no_init& operator >>= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
	no_init& operator >>= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
	no_init& operator >>= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
#if UINT_MAX == ULONG_MAX
	no_init& operator >>= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	no_init& operator >>= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
#endif
	no_init& operator >>= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
	no_init& operator >>= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
#ifdef __APPLE__
	no_init& operator >>= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator >>= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value >>= v; return *this; }
#endif
	no_init& operator &= (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= n.f_value; return *this; }
	no_init& operator &= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
	no_init& operator &= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
	no_init& operator &= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
	no_init& operator &= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	no_init& operator &= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
#endif
	no_init& operator &= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
	no_init& operator &= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
	no_init& operator &= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
	no_init& operator &= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
#if UINT_MAX == ULONG_MAX
	no_init& operator &= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	no_init& operator &= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
#endif
	no_init& operator &= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
	no_init& operator &= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
#ifdef __APPLE__
	no_init& operator &= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator &= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value &= v; return *this; }
#endif
	no_init& operator |= (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= n.f_value; return *this; }
	no_init& operator |= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
	no_init& operator |= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
	no_init& operator |= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
	no_init& operator |= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	no_init& operator |= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
#endif
	no_init& operator |= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
	no_init& operator |= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
	no_init& operator |= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
	no_init& operator |= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
#if UINT_MAX == ULONG_MAX
	no_init& operator |= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	no_init& operator |= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
#endif
	no_init& operator |= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
	no_init& operator |= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
#ifdef __APPLE__
	no_init& operator |= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator |= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value |= v; return *this; }
#endif
	no_init& operator ^= (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= n.f_value; return *this; }
	no_init& operator ^= (bool v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
	no_init& operator ^= (char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
	no_init& operator ^= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
	no_init& operator ^= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	no_init& operator ^= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
#endif
	no_init& operator ^= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
	no_init& operator ^= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
	no_init& operator ^= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
	no_init& operator ^= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
#if UINT_MAX == ULONG_MAX
	no_init& operator ^= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	no_init& operator ^= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
#endif
	no_init& operator ^= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
	no_init& operator ^= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
#ifdef __APPLE__
	no_init& operator ^= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
#endif
#ifdef __APPLE__
	no_init& operator ^= (time_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); f_value ^= v; return *this; }
#endif
	T operator * (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value * n.f_value; }
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
	T operator / (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value / n.f_value; }
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
	T operator % (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value % n.f_value; }
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
	T operator + (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value + n.f_value; }
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
	T operator - (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value - n.f_value; }
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
	T operator << (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value << n.f_value; }
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
	T operator >> (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >> n.f_value; }
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
	T operator & (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value & n.f_value; }
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
	T operator | (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value | n.f_value; }
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
	T operator ^ (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value ^ n.f_value; }
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
	bool operator == (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value == n.f_value; }
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
	bool operator != (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value != n.f_value; }
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
	bool operator < (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value < n.f_value; }
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
	bool operator <= (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value <= n.f_value; }
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
	bool operator > (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value > n.f_value; }
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
	bool operator >= (const no_init& n) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!n.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_value >= n.f_value; }
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
typedef no_init<bool> rbool_t;
typedef no_init<char> rchar_t;
typedef no_init<signed char> rschar_t;
typedef no_init<unsigned char> ruchar_t;
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
typedef no_init<wchar_t> rwchar_t;
#endif
typedef no_init<int16_t> rint16_t;
typedef no_init<uint16_t> ruint16_t;
typedef no_init<int32_t> rint32_t;
typedef no_init<uint32_t> ruint32_t;
#if UINT_MAX == ULONG_MAX
typedef no_init<long> rplain_long_t;
#endif
#if UINT_MAX == ULONG_MAX
typedef no_init<unsigned long> rplain_ulong_t;
#endif
typedef no_init<int64_t> rint64_t;
typedef no_init<uint64_t> ruint64_t;
typedef no_init<float> rfloat_t;
typedef no_init<double> rdouble_t;
#ifndef __APPLE__
typedef no_init<long double> rlongdouble_t;
#endif
#ifdef __APPLE__
typedef no_init<size_t> rsize_t;
#endif
#ifdef __APPLE__
typedef no_init<time_t> rtime_t;
#endif
#else
typedef bool rbool_t;
typedef char rchar_t;
typedef signed char rschar_t;
typedef unsigned char ruchar_t;
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
typedef wchar_t rwchar_t;
#endif
typedef int16_t rint16_t;
typedef uint16_t ruint16_t;
typedef int32_t rint32_t;
typedef uint32_t ruint32_t;
#if UINT_MAX == ULONG_MAX
typedef long rplain_long_t;
#endif
#if UINT_MAX == ULONG_MAX
typedef unsigned long rplain_ulong_t;
#endif
typedef int64_t rint64_t;
typedef uint64_t ruint64_t;
typedef float rfloat_t;
typedef double rdouble_t;
#ifndef __APPLE__
typedef long double rlongdouble_t;
#endif
#ifdef __APPLE__
typedef size_t rsize_t;
#endif
#ifdef __APPLE__
typedef time_t rtime_t;
#endif
#endif
} // namespace controlled_vars
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
