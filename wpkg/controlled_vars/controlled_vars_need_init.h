// WARNING: do not edit; this is an auto-generated
// WARNING: file; please, use the generator named
// WARNING: controlled_vars to re-generate
//
// File:	controlled_vars_need_init.h
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
#ifndef CONTROLLED_VARS_NEED_INIT_H
#define CONTROLLED_VARS_NEED_INIT_H
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
/** \brief Documentation available online.
 * Please go to http://snapwebsites.org/project/controlled-vars
 */
template<class T> class need_init {
public:
	typedef T primary_type_t;
	need_init(bool v) { f_value = static_cast<primary_type_t>(v); }
	need_init(char v) { f_value = static_cast<primary_type_t>(v); }
	need_init(signed char v) { f_value = static_cast<primary_type_t>(v); }
	need_init(unsigned char v) { f_value = static_cast<primary_type_t>(v); }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	need_init(wchar_t v) { f_value = static_cast<primary_type_t>(v); }
#endif
	need_init(int16_t v) { f_value = static_cast<primary_type_t>(v); }
	need_init(uint16_t v) { f_value = static_cast<primary_type_t>(v); }
	need_init(int32_t v) { f_value = static_cast<primary_type_t>(v); }
	need_init(uint32_t v) { f_value = static_cast<primary_type_t>(v); }
#if UINT_MAX == ULONG_MAX
	need_init(long v) { f_value = static_cast<primary_type_t>(v); }
#endif
#if UINT_MAX == ULONG_MAX
	need_init(unsigned long v) { f_value = static_cast<primary_type_t>(v); }
#endif
	need_init(int64_t v) { f_value = static_cast<primary_type_t>(v); }
	need_init(uint64_t v) { f_value = static_cast<primary_type_t>(v); }
	need_init(float v) { f_value = static_cast<primary_type_t>(v); }
	need_init(double v) { f_value = static_cast<primary_type_t>(v); }
#ifndef __APPLE__
	need_init(long double v) { f_value = static_cast<primary_type_t>(v); }
#endif
#if 0 //def __APPLE__
	need_init(size_t v) { f_value = static_cast<primary_type_t>(v); }
#endif
#if 0 //def __APPLE__
	need_init(time_t v) { f_value = static_cast<primary_type_t>(v); }
#endif
	operator T () const { return f_value; }
	operator T () { return f_value; }
	T value() const { return f_value; }
	const T * ptr() const { return &f_value; }
	T * ptr() { return &f_value; }
	bool operator ! () const { return !f_value; }
	T operator ~ () const { return ~f_value; }
	T operator + () const { return +f_value; }
	T operator - () const { return -f_value; }
	need_init& operator ++ () { ++f_value; return *this; }
	need_init operator ++ (int) { need_init<T> result(*this); ++f_value; return result; }
	need_init& operator -- () { --f_value; return *this; }
	need_init operator -- (int) { need_init<T> result(*this); --f_value; return result; }
	need_init& operator = (const need_init& n) { f_value = n.f_value; return *this; }
	need_init& operator = (bool v) { f_value = v; return *this; }
	need_init& operator = (char v) { f_value = v; return *this; }
	need_init& operator = (signed char v) { f_value = v; return *this; }
	need_init& operator = (unsigned char v) { f_value = v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	need_init& operator = (wchar_t v) { f_value = v; return *this; }
#endif
	need_init& operator = (int16_t v) { f_value = v; return *this; }
	need_init& operator = (uint16_t v) { f_value = v; return *this; }
	need_init& operator = (int32_t v) { f_value = v; return *this; }
	need_init& operator = (uint32_t v) { f_value = v; return *this; }
#if UINT_MAX == ULONG_MAX
	need_init& operator = (long v) { f_value = v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	need_init& operator = (unsigned long v) { f_value = v; return *this; }
#endif
	need_init& operator = (int64_t v) { f_value = v; return *this; }
	need_init& operator = (uint64_t v) { f_value = v; return *this; }
	need_init& operator = (float v) { f_value = v; return *this; }
	need_init& operator = (double v) { f_value = v; return *this; }
#ifndef __APPLE__
	need_init& operator = (long double v) { f_value = v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator = (size_t v) { f_value = v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator = (time_t v) { f_value = v; return *this; }
#endif
	need_init& operator *= (const need_init& n) { f_value *= n.f_value; return *this; }
	need_init& operator *= (bool v) { f_value *= v; return *this; }
	need_init& operator *= (char v) { f_value *= v; return *this; }
	need_init& operator *= (signed char v) { f_value *= v; return *this; }
	need_init& operator *= (unsigned char v) { f_value *= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	need_init& operator *= (wchar_t v) { f_value *= v; return *this; }
#endif
	need_init& operator *= (int16_t v) { f_value *= v; return *this; }
	need_init& operator *= (uint16_t v) { f_value *= v; return *this; }
	need_init& operator *= (int32_t v) { f_value *= v; return *this; }
	need_init& operator *= (uint32_t v) { f_value *= v; return *this; }
#if UINT_MAX == ULONG_MAX
	need_init& operator *= (long v) { f_value *= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	need_init& operator *= (unsigned long v) { f_value *= v; return *this; }
#endif
	need_init& operator *= (int64_t v) { f_value *= v; return *this; }
	need_init& operator *= (uint64_t v) { f_value *= v; return *this; }
	need_init& operator *= (float v) { f_value *= v; return *this; }
	need_init& operator *= (double v) { f_value *= v; return *this; }
#ifndef __APPLE__
	need_init& operator *= (long double v) { f_value *= v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator *= (size_t v) { f_value *= v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator *= (time_t v) { f_value *= v; return *this; }
#endif
	need_init& operator /= (const need_init& n) { f_value /= n.f_value; return *this; }
	need_init& operator /= (bool v) { f_value /= v; return *this; }
	need_init& operator /= (char v) { f_value /= v; return *this; }
	need_init& operator /= (signed char v) { f_value /= v; return *this; }
	need_init& operator /= (unsigned char v) { f_value /= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	need_init& operator /= (wchar_t v) { f_value /= v; return *this; }
#endif
	need_init& operator /= (int16_t v) { f_value /= v; return *this; }
	need_init& operator /= (uint16_t v) { f_value /= v; return *this; }
	need_init& operator /= (int32_t v) { f_value /= v; return *this; }
	need_init& operator /= (uint32_t v) { f_value /= v; return *this; }
#if UINT_MAX == ULONG_MAX
	need_init& operator /= (long v) { f_value /= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	need_init& operator /= (unsigned long v) { f_value /= v; return *this; }
#endif
	need_init& operator /= (int64_t v) { f_value /= v; return *this; }
	need_init& operator /= (uint64_t v) { f_value /= v; return *this; }
	need_init& operator /= (float v) { f_value /= v; return *this; }
	need_init& operator /= (double v) { f_value /= v; return *this; }
#ifndef __APPLE__
	need_init& operator /= (long double v) { f_value /= v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator /= (size_t v) { f_value /= v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator /= (time_t v) { f_value /= v; return *this; }
#endif
	need_init& operator %= (const need_init& n) { f_value %= n.f_value; return *this; }
	need_init& operator %= (bool v) { f_value %= v; return *this; }
	need_init& operator %= (char v) { f_value %= v; return *this; }
	need_init& operator %= (signed char v) { f_value %= v; return *this; }
	need_init& operator %= (unsigned char v) { f_value %= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	need_init& operator %= (wchar_t v) { f_value %= v; return *this; }
#endif
	need_init& operator %= (int16_t v) { f_value %= v; return *this; }
	need_init& operator %= (uint16_t v) { f_value %= v; return *this; }
	need_init& operator %= (int32_t v) { f_value %= v; return *this; }
	need_init& operator %= (uint32_t v) { f_value %= v; return *this; }
#if UINT_MAX == ULONG_MAX
	need_init& operator %= (long v) { f_value %= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	need_init& operator %= (unsigned long v) { f_value %= v; return *this; }
#endif
	need_init& operator %= (int64_t v) { f_value %= v; return *this; }
	need_init& operator %= (uint64_t v) { f_value %= v; return *this; }
#if 0 //def __APPLE__
	need_init& operator %= (size_t v) { f_value %= v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator %= (time_t v) { f_value %= v; return *this; }
#endif
	need_init& operator += (const need_init& n) { f_value += n.f_value; return *this; }
	need_init& operator += (bool v) { f_value += v; return *this; }
	need_init& operator += (char v) { f_value += v; return *this; }
	need_init& operator += (signed char v) { f_value += v; return *this; }
	need_init& operator += (unsigned char v) { f_value += v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	need_init& operator += (wchar_t v) { f_value += v; return *this; }
#endif
	need_init& operator += (int16_t v) { f_value += v; return *this; }
	need_init& operator += (uint16_t v) { f_value += v; return *this; }
	need_init& operator += (int32_t v) { f_value += v; return *this; }
	need_init& operator += (uint32_t v) { f_value += v; return *this; }
#if UINT_MAX == ULONG_MAX
	need_init& operator += (long v) { f_value += v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	need_init& operator += (unsigned long v) { f_value += v; return *this; }
#endif
	need_init& operator += (int64_t v) { f_value += v; return *this; }
	need_init& operator += (uint64_t v) { f_value += v; return *this; }
	need_init& operator += (float v) { f_value += v; return *this; }
	need_init& operator += (double v) { f_value += v; return *this; }
#ifndef __APPLE__
	need_init& operator += (long double v) { f_value += v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator += (size_t v) { f_value += v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator += (time_t v) { f_value += v; return *this; }
#endif
	need_init& operator -= (const need_init& n) { f_value -= n.f_value; return *this; }
	need_init& operator -= (bool v) { f_value -= v; return *this; }
	need_init& operator -= (char v) { f_value -= v; return *this; }
	need_init& operator -= (signed char v) { f_value -= v; return *this; }
	need_init& operator -= (unsigned char v) { f_value -= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	need_init& operator -= (wchar_t v) { f_value -= v; return *this; }
#endif
	need_init& operator -= (int16_t v) { f_value -= v; return *this; }
	need_init& operator -= (uint16_t v) { f_value -= v; return *this; }
	need_init& operator -= (int32_t v) { f_value -= v; return *this; }
	need_init& operator -= (uint32_t v) { f_value -= v; return *this; }
#if UINT_MAX == ULONG_MAX
	need_init& operator -= (long v) { f_value -= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	need_init& operator -= (unsigned long v) { f_value -= v; return *this; }
#endif
	need_init& operator -= (int64_t v) { f_value -= v; return *this; }
	need_init& operator -= (uint64_t v) { f_value -= v; return *this; }
	need_init& operator -= (float v) { f_value -= v; return *this; }
	need_init& operator -= (double v) { f_value -= v; return *this; }
#ifndef __APPLE__
	need_init& operator -= (long double v) { f_value -= v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator -= (size_t v) { f_value -= v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator -= (time_t v) { f_value -= v; return *this; }
#endif
	need_init& operator <<= (const need_init& n) { f_value <<= n.f_value; return *this; }
	need_init& operator <<= (bool v) { f_value <<= v; return *this; }
	need_init& operator <<= (char v) { f_value <<= v; return *this; }
	need_init& operator <<= (signed char v) { f_value <<= v; return *this; }
	need_init& operator <<= (unsigned char v) { f_value <<= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	need_init& operator <<= (wchar_t v) { f_value <<= v; return *this; }
#endif
	need_init& operator <<= (int16_t v) { f_value <<= v; return *this; }
	need_init& operator <<= (uint16_t v) { f_value <<= v; return *this; }
	need_init& operator <<= (int32_t v) { f_value <<= v; return *this; }
	need_init& operator <<= (uint32_t v) { f_value <<= v; return *this; }
#if UINT_MAX == ULONG_MAX
	need_init& operator <<= (long v) { f_value <<= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	need_init& operator <<= (unsigned long v) { f_value <<= v; return *this; }
#endif
	need_init& operator <<= (int64_t v) { f_value <<= v; return *this; }
	need_init& operator <<= (uint64_t v) { f_value <<= v; return *this; }
#if 0 //def __APPLE__
	need_init& operator <<= (size_t v) { f_value <<= v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator <<= (time_t v) { f_value <<= v; return *this; }
#endif
	need_init& operator >>= (const need_init& n) { f_value >>= n.f_value; return *this; }
	need_init& operator >>= (bool v) { f_value >>= v; return *this; }
	need_init& operator >>= (char v) { f_value >>= v; return *this; }
	need_init& operator >>= (signed char v) { f_value >>= v; return *this; }
	need_init& operator >>= (unsigned char v) { f_value >>= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	need_init& operator >>= (wchar_t v) { f_value >>= v; return *this; }
#endif
	need_init& operator >>= (int16_t v) { f_value >>= v; return *this; }
	need_init& operator >>= (uint16_t v) { f_value >>= v; return *this; }
	need_init& operator >>= (int32_t v) { f_value >>= v; return *this; }
	need_init& operator >>= (uint32_t v) { f_value >>= v; return *this; }
#if UINT_MAX == ULONG_MAX
	need_init& operator >>= (long v) { f_value >>= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	need_init& operator >>= (unsigned long v) { f_value >>= v; return *this; }
#endif
	need_init& operator >>= (int64_t v) { f_value >>= v; return *this; }
	need_init& operator >>= (uint64_t v) { f_value >>= v; return *this; }
#if 0 //def __APPLE__
	need_init& operator >>= (size_t v) { f_value >>= v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator >>= (time_t v) { f_value >>= v; return *this; }
#endif
	need_init& operator &= (const need_init& n) { f_value &= n.f_value; return *this; }
	need_init& operator &= (bool v) { f_value &= v; return *this; }
	need_init& operator &= (char v) { f_value &= v; return *this; }
	need_init& operator &= (signed char v) { f_value &= v; return *this; }
	need_init& operator &= (unsigned char v) { f_value &= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	need_init& operator &= (wchar_t v) { f_value &= v; return *this; }
#endif
	need_init& operator &= (int16_t v) { f_value &= v; return *this; }
	need_init& operator &= (uint16_t v) { f_value &= v; return *this; }
	need_init& operator &= (int32_t v) { f_value &= v; return *this; }
	need_init& operator &= (uint32_t v) { f_value &= v; return *this; }
#if UINT_MAX == ULONG_MAX
	need_init& operator &= (long v) { f_value &= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	need_init& operator &= (unsigned long v) { f_value &= v; return *this; }
#endif
	need_init& operator &= (int64_t v) { f_value &= v; return *this; }
	need_init& operator &= (uint64_t v) { f_value &= v; return *this; }
#if 0 //def __APPLE__
	need_init& operator &= (size_t v) { f_value &= v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator &= (time_t v) { f_value &= v; return *this; }
#endif
	need_init& operator |= (const need_init& n) { f_value |= n.f_value; return *this; }
	need_init& operator |= (bool v) { f_value |= v; return *this; }
	need_init& operator |= (char v) { f_value |= v; return *this; }
	need_init& operator |= (signed char v) { f_value |= v; return *this; }
	need_init& operator |= (unsigned char v) { f_value |= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	need_init& operator |= (wchar_t v) { f_value |= v; return *this; }
#endif
	need_init& operator |= (int16_t v) { f_value |= v; return *this; }
	need_init& operator |= (uint16_t v) { f_value |= v; return *this; }
	need_init& operator |= (int32_t v) { f_value |= v; return *this; }
	need_init& operator |= (uint32_t v) { f_value |= v; return *this; }
#if UINT_MAX == ULONG_MAX
	need_init& operator |= (long v) { f_value |= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	need_init& operator |= (unsigned long v) { f_value |= v; return *this; }
#endif
	need_init& operator |= (int64_t v) { f_value |= v; return *this; }
	need_init& operator |= (uint64_t v) { f_value |= v; return *this; }
#if 0 //def __APPLE__
	need_init& operator |= (size_t v) { f_value |= v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator |= (time_t v) { f_value |= v; return *this; }
#endif
	need_init& operator ^= (const need_init& n) { f_value ^= n.f_value; return *this; }
	need_init& operator ^= (bool v) { f_value ^= v; return *this; }
	need_init& operator ^= (char v) { f_value ^= v; return *this; }
	need_init& operator ^= (signed char v) { f_value ^= v; return *this; }
	need_init& operator ^= (unsigned char v) { f_value ^= v; return *this; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	need_init& operator ^= (wchar_t v) { f_value ^= v; return *this; }
#endif
	need_init& operator ^= (int16_t v) { f_value ^= v; return *this; }
	need_init& operator ^= (uint16_t v) { f_value ^= v; return *this; }
	need_init& operator ^= (int32_t v) { f_value ^= v; return *this; }
	need_init& operator ^= (uint32_t v) { f_value ^= v; return *this; }
#if UINT_MAX == ULONG_MAX
	need_init& operator ^= (long v) { f_value ^= v; return *this; }
#endif
#if UINT_MAX == ULONG_MAX
	need_init& operator ^= (unsigned long v) { f_value ^= v; return *this; }
#endif
	need_init& operator ^= (int64_t v) { f_value ^= v; return *this; }
	need_init& operator ^= (uint64_t v) { f_value ^= v; return *this; }
#if 0 //def __APPLE__
	need_init& operator ^= (size_t v) { f_value ^= v; return *this; }
#endif
#if 0 //def __APPLE__
	need_init& operator ^= (time_t v) { f_value ^= v; return *this; }
#endif
	T operator * (const need_init& n) { return f_value * n.f_value; }
	T operator * (bool v) { return f_value * v; }
	T operator * (char v) { return f_value * v; }
	T operator * (signed char v) { return f_value * v; }
	T operator * (unsigned char v) { return f_value * v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator * (wchar_t v) { return f_value * v; }
#endif
	T operator * (int16_t v) { return f_value * v; }
	T operator * (uint16_t v) { return f_value * v; }
	T operator * (int32_t v) { return f_value * v; }
	T operator * (uint32_t v) { return f_value * v; }
#if UINT_MAX == ULONG_MAX
	T operator * (long v) { return f_value * v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator * (unsigned long v) { return f_value * v; }
#endif
	T operator * (int64_t v) { return f_value * v; }
	T operator * (uint64_t v) { return f_value * v; }
	T operator * (float v) { return f_value * v; }
	T operator * (double v) { return f_value * v; }
#ifndef __APPLE__
	T operator * (long double v) { return f_value * v; }
#endif
#if 0 //def __APPLE__
	T operator * (size_t v) { return f_value * v; }
#endif
#if 0 //def __APPLE__
	T operator * (time_t v) { return f_value * v; }
#endif
	T operator / (const need_init& n) { return f_value / n.f_value; }
	T operator / (bool v) { return f_value / v; }
	T operator / (char v) { return f_value / v; }
	T operator / (signed char v) { return f_value / v; }
	T operator / (unsigned char v) { return f_value / v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator / (wchar_t v) { return f_value / v; }
#endif
	T operator / (int16_t v) { return f_value / v; }
	T operator / (uint16_t v) { return f_value / v; }
	T operator / (int32_t v) { return f_value / v; }
	T operator / (uint32_t v) { return f_value / v; }
#if UINT_MAX == ULONG_MAX
	T operator / (long v) { return f_value / v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator / (unsigned long v) { return f_value / v; }
#endif
	T operator / (int64_t v) { return f_value / v; }
	T operator / (uint64_t v) { return f_value / v; }
	T operator / (float v) { return f_value / v; }
	T operator / (double v) { return f_value / v; }
#ifndef __APPLE__
	T operator / (long double v) { return f_value / v; }
#endif
#if 0 //def __APPLE__
	T operator / (size_t v) { return f_value / v; }
#endif
#if 0 //def __APPLE__
	T operator / (time_t v) { return f_value / v; }
#endif
	T operator % (const need_init& n) { return f_value % n.f_value; }
	T operator % (bool v) { return f_value % v; }
	T operator % (char v) { return f_value % v; }
	T operator % (signed char v) { return f_value % v; }
	T operator % (unsigned char v) { return f_value % v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator % (wchar_t v) { return f_value % v; }
#endif
	T operator % (int16_t v) { return f_value % v; }
	T operator % (uint16_t v) { return f_value % v; }
	T operator % (int32_t v) { return f_value % v; }
	T operator % (uint32_t v) { return f_value % v; }
#if UINT_MAX == ULONG_MAX
	T operator % (long v) { return f_value % v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator % (unsigned long v) { return f_value % v; }
#endif
	T operator % (int64_t v) { return f_value % v; }
	T operator % (uint64_t v) { return f_value % v; }
#if 0 //def __APPLE__
	T operator % (size_t v) { return f_value % v; }
#endif
#if 0 //def __APPLE__
	T operator % (time_t v) { return f_value % v; }
#endif
	T operator + (const need_init& n) { return f_value + n.f_value; }
	T operator + (bool v) { return f_value + v; }
	T operator + (char v) { return f_value + v; }
	T operator + (signed char v) { return f_value + v; }
	T operator + (unsigned char v) { return f_value + v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator + (wchar_t v) { return f_value + v; }
#endif
	T operator + (int16_t v) { return f_value + v; }
	T operator + (uint16_t v) { return f_value + v; }
	T operator + (int32_t v) { return f_value + v; }
	T operator + (uint32_t v) { return f_value + v; }
#if UINT_MAX == ULONG_MAX
	T operator + (long v) { return f_value + v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator + (unsigned long v) { return f_value + v; }
#endif
	T operator + (int64_t v) { return f_value + v; }
	T operator + (uint64_t v) { return f_value + v; }
	T operator + (float v) { return f_value + v; }
	T operator + (double v) { return f_value + v; }
#ifndef __APPLE__
	T operator + (long double v) { return f_value + v; }
#endif
#if 0 //def __APPLE__
	T operator + (size_t v) { return f_value + v; }
#endif
#if 0 //def __APPLE__
	T operator + (time_t v) { return f_value + v; }
#endif
	T operator - (const need_init& n) { return f_value - n.f_value; }
	T operator - (bool v) { return f_value - v; }
	T operator - (char v) { return f_value - v; }
	T operator - (signed char v) { return f_value - v; }
	T operator - (unsigned char v) { return f_value - v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator - (wchar_t v) { return f_value - v; }
#endif
	T operator - (int16_t v) { return f_value - v; }
	T operator - (uint16_t v) { return f_value - v; }
	T operator - (int32_t v) { return f_value - v; }
	T operator - (uint32_t v) { return f_value - v; }
#if UINT_MAX == ULONG_MAX
	T operator - (long v) { return f_value - v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator - (unsigned long v) { return f_value - v; }
#endif
	T operator - (int64_t v) { return f_value - v; }
	T operator - (uint64_t v) { return f_value - v; }
	T operator - (float v) { return f_value - v; }
	T operator - (double v) { return f_value - v; }
#ifndef __APPLE__
	T operator - (long double v) { return f_value - v; }
#endif
#if 0 //def __APPLE__
	T operator - (size_t v) { return f_value - v; }
#endif
#if 0 //def __APPLE__
	T operator - (time_t v) { return f_value - v; }
#endif
	T operator << (const need_init& n) { return f_value << n.f_value; }
	T operator << (bool v) { return f_value << v; }
	T operator << (char v) { return f_value << v; }
	T operator << (signed char v) { return f_value << v; }
	T operator << (unsigned char v) { return f_value << v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator << (wchar_t v) { return f_value << v; }
#endif
	T operator << (int16_t v) { return f_value << v; }
	T operator << (uint16_t v) { return f_value << v; }
	T operator << (int32_t v) { return f_value << v; }
	T operator << (uint32_t v) { return f_value << v; }
#if UINT_MAX == ULONG_MAX
	T operator << (long v) { return f_value << v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator << (unsigned long v) { return f_value << v; }
#endif
	T operator << (int64_t v) { return f_value << v; }
	T operator << (uint64_t v) { return f_value << v; }
#if 0 //def __APPLE__
	T operator << (size_t v) { return f_value << v; }
#endif
#if 0 //def __APPLE__
	T operator << (time_t v) { return f_value << v; }
#endif
	T operator >> (const need_init& n) { return f_value >> n.f_value; }
	T operator >> (bool v) { return f_value >> v; }
	T operator >> (char v) { return f_value >> v; }
	T operator >> (signed char v) { return f_value >> v; }
	T operator >> (unsigned char v) { return f_value >> v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator >> (wchar_t v) { return f_value >> v; }
#endif
	T operator >> (int16_t v) { return f_value >> v; }
	T operator >> (uint16_t v) { return f_value >> v; }
	T operator >> (int32_t v) { return f_value >> v; }
	T operator >> (uint32_t v) { return f_value >> v; }
#if UINT_MAX == ULONG_MAX
	T operator >> (long v) { return f_value >> v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator >> (unsigned long v) { return f_value >> v; }
#endif
	T operator >> (int64_t v) { return f_value >> v; }
	T operator >> (uint64_t v) { return f_value >> v; }
#if 0 //def __APPLE__
	T operator >> (size_t v) { return f_value >> v; }
#endif
#if 0 //def __APPLE__
	T operator >> (time_t v) { return f_value >> v; }
#endif
	T operator & (const need_init& n) { return f_value & n.f_value; }
	T operator & (bool v) { return f_value & v; }
	T operator & (char v) { return f_value & v; }
	T operator & (signed char v) { return f_value & v; }
	T operator & (unsigned char v) { return f_value & v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator & (wchar_t v) { return f_value & v; }
#endif
	T operator & (int16_t v) { return f_value & v; }
	T operator & (uint16_t v) { return f_value & v; }
	T operator & (int32_t v) { return f_value & v; }
	T operator & (uint32_t v) { return f_value & v; }
#if UINT_MAX == ULONG_MAX
	T operator & (long v) { return f_value & v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator & (unsigned long v) { return f_value & v; }
#endif
	T operator & (int64_t v) { return f_value & v; }
	T operator & (uint64_t v) { return f_value & v; }
#if 0 //def __APPLE__
	T operator & (size_t v) { return f_value & v; }
#endif
#if 0 //def __APPLE__
	T operator & (time_t v) { return f_value & v; }
#endif
	T operator | (const need_init& n) { return f_value | n.f_value; }
	T operator | (bool v) { return f_value | v; }
	T operator | (char v) { return f_value | v; }
	T operator | (signed char v) { return f_value | v; }
	T operator | (unsigned char v) { return f_value | v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator | (wchar_t v) { return f_value | v; }
#endif
	T operator | (int16_t v) { return f_value | v; }
	T operator | (uint16_t v) { return f_value | v; }
	T operator | (int32_t v) { return f_value | v; }
	T operator | (uint32_t v) { return f_value | v; }
#if UINT_MAX == ULONG_MAX
	T operator | (long v) { return f_value | v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator | (unsigned long v) { return f_value | v; }
#endif
	T operator | (int64_t v) { return f_value | v; }
	T operator | (uint64_t v) { return f_value | v; }
#if 0 //def __APPLE__
	T operator | (size_t v) { return f_value | v; }
#endif
#if 0 //def __APPLE__
	T operator | (time_t v) { return f_value | v; }
#endif
	T operator ^ (const need_init& n) { return f_value ^ n.f_value; }
	T operator ^ (bool v) { return f_value ^ v; }
	T operator ^ (char v) { return f_value ^ v; }
	T operator ^ (signed char v) { return f_value ^ v; }
	T operator ^ (unsigned char v) { return f_value ^ v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	T operator ^ (wchar_t v) { return f_value ^ v; }
#endif
	T operator ^ (int16_t v) { return f_value ^ v; }
	T operator ^ (uint16_t v) { return f_value ^ v; }
	T operator ^ (int32_t v) { return f_value ^ v; }
	T operator ^ (uint32_t v) { return f_value ^ v; }
#if UINT_MAX == ULONG_MAX
	T operator ^ (long v) { return f_value ^ v; }
#endif
#if UINT_MAX == ULONG_MAX
	T operator ^ (unsigned long v) { return f_value ^ v; }
#endif
	T operator ^ (int64_t v) { return f_value ^ v; }
	T operator ^ (uint64_t v) { return f_value ^ v; }
#if 0 //def __APPLE__
	T operator ^ (size_t v) { return f_value ^ v; }
#endif
#if 0 //def __APPLE__
	T operator ^ (time_t v) { return f_value ^ v; }
#endif
	bool operator == (const need_init& n) { return f_value == n.f_value; }
	bool operator == (bool v) { return f_value == v; }
	bool operator == (char v) { return f_value == v; }
	bool operator == (signed char v) { return f_value == v; }
	bool operator == (unsigned char v) { return f_value == v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	bool operator == (wchar_t v) { return f_value == v; }
#endif
	bool operator == (int16_t v) { return f_value == v; }
	bool operator == (uint16_t v) { return f_value == v; }
	bool operator == (int32_t v) { return f_value == v; }
	bool operator == (uint32_t v) { return f_value == v; }
#if UINT_MAX == ULONG_MAX
	bool operator == (long v) { return f_value == v; }
#endif
#if UINT_MAX == ULONG_MAX
	bool operator == (unsigned long v) { return f_value == v; }
#endif
	bool operator == (int64_t v) { return f_value == v; }
	bool operator == (uint64_t v) { return f_value == v; }
	bool operator == (float v) { return f_value == v; }
	bool operator == (double v) { return f_value == v; }
#ifndef __APPLE__
	bool operator == (long double v) { return f_value == v; }
#endif
#if 0 //def __APPLE__
	bool operator == (size_t v) { return f_value == v; }
#endif
#if 0 //def __APPLE__
	bool operator == (time_t v) { return f_value == v; }
#endif
	bool operator != (const need_init& n) { return f_value != n.f_value; }
	bool operator != (bool v) { return f_value != v; }
	bool operator != (char v) { return f_value != v; }
	bool operator != (signed char v) { return f_value != v; }
	bool operator != (unsigned char v) { return f_value != v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	bool operator != (wchar_t v) { return f_value != v; }
#endif
	bool operator != (int16_t v) { return f_value != v; }
	bool operator != (uint16_t v) { return f_value != v; }
	bool operator != (int32_t v) { return f_value != v; }
	bool operator != (uint32_t v) { return f_value != v; }
#if UINT_MAX == ULONG_MAX
	bool operator != (long v) { return f_value != v; }
#endif
#if UINT_MAX == ULONG_MAX
	bool operator != (unsigned long v) { return f_value != v; }
#endif
	bool operator != (int64_t v) { return f_value != v; }
	bool operator != (uint64_t v) { return f_value != v; }
	bool operator != (float v) { return f_value != v; }
	bool operator != (double v) { return f_value != v; }
#ifndef __APPLE__
	bool operator != (long double v) { return f_value != v; }
#endif
#if 0 //def __APPLE__
	bool operator != (size_t v) { return f_value != v; }
#endif
#if 0 //def __APPLE__
	bool operator != (time_t v) { return f_value != v; }
#endif
	bool operator < (const need_init& n) { return f_value < n.f_value; }
	bool operator < (bool v) { return f_value < v; }
	bool operator < (char v) { return f_value < v; }
	bool operator < (signed char v) { return f_value < v; }
	bool operator < (unsigned char v) { return f_value < v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	bool operator < (wchar_t v) { return f_value < v; }
#endif
	bool operator < (int16_t v) { return f_value < v; }
	bool operator < (uint16_t v) { return f_value < v; }
	bool operator < (int32_t v) { return f_value < v; }
	bool operator < (uint32_t v) { return f_value < v; }
#if UINT_MAX == ULONG_MAX
	bool operator < (long v) { return f_value < v; }
#endif
#if UINT_MAX == ULONG_MAX
	bool operator < (unsigned long v) { return f_value < v; }
#endif
	bool operator < (int64_t v) { return f_value < v; }
	bool operator < (uint64_t v) { return f_value < v; }
	bool operator < (float v) { return f_value < v; }
	bool operator < (double v) { return f_value < v; }
#ifndef __APPLE__
	bool operator < (long double v) { return f_value < v; }
#endif
#if 0 //def __APPLE__
	bool operator < (size_t v) { return f_value < v; }
#endif
#if 0 //def __APPLE__
	bool operator < (time_t v) { return f_value < v; }
#endif
	bool operator <= (const need_init& n) { return f_value <= n.f_value; }
	bool operator <= (bool v) { return f_value <= v; }
	bool operator <= (char v) { return f_value <= v; }
	bool operator <= (signed char v) { return f_value <= v; }
	bool operator <= (unsigned char v) { return f_value <= v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	bool operator <= (wchar_t v) { return f_value <= v; }
#endif
	bool operator <= (int16_t v) { return f_value <= v; }
	bool operator <= (uint16_t v) { return f_value <= v; }
	bool operator <= (int32_t v) { return f_value <= v; }
	bool operator <= (uint32_t v) { return f_value <= v; }
#if UINT_MAX == ULONG_MAX
	bool operator <= (long v) { return f_value <= v; }
#endif
#if UINT_MAX == ULONG_MAX
	bool operator <= (unsigned long v) { return f_value <= v; }
#endif
	bool operator <= (int64_t v) { return f_value <= v; }
	bool operator <= (uint64_t v) { return f_value <= v; }
	bool operator <= (float v) { return f_value <= v; }
	bool operator <= (double v) { return f_value <= v; }
#ifndef __APPLE__
	bool operator <= (long double v) { return f_value <= v; }
#endif
#if 0 //def __APPLE__
	bool operator <= (size_t v) { return f_value <= v; }
#endif
#if 0 //def __APPLE__
	bool operator <= (time_t v) { return f_value <= v; }
#endif
	bool operator > (const need_init& n) { return f_value > n.f_value; }
	bool operator > (bool v) { return f_value > v; }
	bool operator > (char v) { return f_value > v; }
	bool operator > (signed char v) { return f_value > v; }
	bool operator > (unsigned char v) { return f_value > v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	bool operator > (wchar_t v) { return f_value > v; }
#endif
	bool operator > (int16_t v) { return f_value > v; }
	bool operator > (uint16_t v) { return f_value > v; }
	bool operator > (int32_t v) { return f_value > v; }
	bool operator > (uint32_t v) { return f_value > v; }
#if UINT_MAX == ULONG_MAX
	bool operator > (long v) { return f_value > v; }
#endif
#if UINT_MAX == ULONG_MAX
	bool operator > (unsigned long v) { return f_value > v; }
#endif
	bool operator > (int64_t v) { return f_value > v; }
	bool operator > (uint64_t v) { return f_value > v; }
	bool operator > (float v) { return f_value > v; }
	bool operator > (double v) { return f_value > v; }
#ifndef __APPLE__
	bool operator > (long double v) { return f_value > v; }
#endif
#if 0 //def __APPLE__
	bool operator > (size_t v) { return f_value > v; }
#endif
#if 0 //def __APPLE__
	bool operator > (time_t v) { return f_value > v; }
#endif
	bool operator >= (const need_init& n) { return f_value >= n.f_value; }
	bool operator >= (bool v) { return f_value >= v; }
	bool operator >= (char v) { return f_value >= v; }
	bool operator >= (signed char v) { return f_value >= v; }
	bool operator >= (unsigned char v) { return f_value >= v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	bool operator >= (wchar_t v) { return f_value >= v; }
#endif
	bool operator >= (int16_t v) { return f_value >= v; }
	bool operator >= (uint16_t v) { return f_value >= v; }
	bool operator >= (int32_t v) { return f_value >= v; }
	bool operator >= (uint32_t v) { return f_value >= v; }
#if UINT_MAX == ULONG_MAX
	bool operator >= (long v) { return f_value >= v; }
#endif
#if UINT_MAX == ULONG_MAX
	bool operator >= (unsigned long v) { return f_value >= v; }
#endif
	bool operator >= (int64_t v) { return f_value >= v; }
	bool operator >= (uint64_t v) { return f_value >= v; }
	bool operator >= (float v) { return f_value >= v; }
	bool operator >= (double v) { return f_value >= v; }
#ifndef __APPLE__
	bool operator >= (long double v) { return f_value >= v; }
#endif
#if 0 //def __APPLE__
	bool operator >= (size_t v) { return f_value >= v; }
#endif
#if 0 //def __APPLE__
	bool operator >= (time_t v) { return f_value >= v; }
#endif
#ifdef CONTROLLED_VARS_DEBUG
	bool is_initialized() const { return true; }
#endif
private:
	T f_value;
};
typedef need_init<bool> mbool_t;
typedef need_init<char> mchar_t;
typedef need_init<signed char> mschar_t;
typedef need_init<unsigned char> muchar_t;
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
typedef need_init<wchar_t> mwchar_t;
#endif
typedef need_init<int16_t> mint16_t;
typedef need_init<uint16_t> muint16_t;
typedef need_init<int32_t> mint32_t;
typedef need_init<uint32_t> muint32_t;
#if UINT_MAX == ULONG_MAX
typedef need_init<long> mplain_long_t;
#endif
#if UINT_MAX == ULONG_MAX
typedef need_init<unsigned long> mplain_ulong_t;
#endif
typedef need_init<int64_t> mint64_t;
typedef need_init<uint64_t> muint64_t;
typedef need_init<float> mfloat_t;
typedef need_init<double> mdouble_t;
#ifndef __APPLE__
typedef need_init<long double> mlongdouble_t;
#endif
#if 0 //def __APPLE__
typedef need_init<size_t> msize_t;
#endif
#if 0 //def __APPLE__
typedef need_init<time_t> mtime_t;
#endif
} // namespace controlled_vars
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
