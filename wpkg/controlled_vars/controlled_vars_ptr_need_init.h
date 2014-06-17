// WARNING: do not edit; this is an auto-generated
// WARNING: file; please, use the generator named
// WARNING: controlled_vars to re-generate
//
// File:	controlled_vars_ptr_need_init.h
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
#ifndef CONTROLLED_VARS_PTR_NEED_INIT_H
#define CONTROLLED_VARS_PTR_NEED_INIT_H
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
/** \brief Documentation available online.
 * Please go to http://snapwebsites.org/project/controlled-vars
 */
template<class T> class ptr_need_init {
public:
	typedef T *primary_type_t;
	static T *null() { return 0; }
	ptr_need_init(T *p) { f_ptr = p; }
	ptr_need_init(T& p) { f_ptr = &p; }
	ptr_need_init(const ptr_need_init *p) { f_ptr = p == 0 ? 0 : p->f_ptr; }
	ptr_need_init(const ptr_need_init& p) { f_ptr = &p == 0 ? 0 : p.f_ptr; }
	operator primary_type_t () const { return f_ptr; }
	operator primary_type_t () { return f_ptr; }
	primary_type_t value() const { return f_ptr; }
	T *get() const { return f_ptr; }
	primary_type_t *ptr() const { return &f_ptr; }
	T *operator -> () const { if(f_ptr == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr; }
	T& operator * () const { if(f_ptr == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return *f_ptr; }
	T operator [] (int index) const { if(f_ptr == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr[index]; }
	T *get() { return f_ptr; }
	primary_type_t *ptr() { return &f_ptr; }
	T *operator -> () { if(f_ptr == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr; }
	T& operator * () { if(f_ptr == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return *f_ptr; }
	T& operator [] (int index) { if(f_ptr == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr[index]; }
	void swap(ptr_need_init& p) { primary_type_t n(f_ptr); f_ptr = p.f_ptr; p.f_ptr = n; }
	operator bool () const { return f_ptr != 0; }
	bool operator ! () const { return f_ptr == 0; }
	ptr_need_init& operator ++ () { ++f_ptr; return *this; }
	ptr_need_init operator ++ (int) { ptr_need_init<T> result(*this); ++f_ptr; return result; }
	ptr_need_init& operator -- () { --f_ptr; return *this; }
	ptr_need_init operator -- (int) { ptr_need_init<T> result(*this); --f_ptr; return result; }
	void reset() { f_ptr = null(); }
	void reset(T& p) { f_ptr = &p; }
	void reset(primary_type_t p) { f_ptr = p; }
	void reset(const ptr_need_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); f_ptr = p.f_ptr; }
	void reset(const ptr_need_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); f_ptr = p->f_ptr; }
	ptr_need_init& operator = (T& p) { f_ptr = &p; return *this; }
	ptr_need_init& operator = (primary_type_t p) { f_ptr = p; return *this; }
	ptr_need_init& operator = (const ptr_need_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); f_ptr = p.f_ptr; return *this; }
	ptr_need_init& operator = (const ptr_need_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); f_ptr = p->f_ptr; return *this; }
	primary_type_t operator += (signed char v) { return f_ptr += v; }
	primary_type_t operator += (unsigned char v) { return f_ptr += v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	primary_type_t operator += (wchar_t v) { return f_ptr += v; }
#endif
	primary_type_t operator += (int16_t v) { return f_ptr += v; }
	primary_type_t operator += (uint16_t v) { return f_ptr += v; }
	primary_type_t operator += (int32_t v) { return f_ptr += v; }
	primary_type_t operator += (uint32_t v) { return f_ptr += v; }
#if UINT_MAX == ULONG_MAX
	primary_type_t operator += (long v) { return f_ptr += v; }
#endif
#if UINT_MAX == ULONG_MAX
	primary_type_t operator += (unsigned long v) { return f_ptr += v; }
#endif
	primary_type_t operator += (int64_t v) { return f_ptr += v; }
	primary_type_t operator += (uint64_t v) { return f_ptr += v; }
#ifdef __APPLE__
	primary_type_t operator += (size_t v) { return f_ptr += v; }
#endif
	primary_type_t operator -= (signed char v) { return f_ptr -= v; }
	primary_type_t operator -= (unsigned char v) { return f_ptr -= v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	primary_type_t operator -= (wchar_t v) { return f_ptr -= v; }
#endif
	primary_type_t operator -= (int16_t v) { return f_ptr -= v; }
	primary_type_t operator -= (uint16_t v) { return f_ptr -= v; }
	primary_type_t operator -= (int32_t v) { return f_ptr -= v; }
	primary_type_t operator -= (uint32_t v) { return f_ptr -= v; }
#if UINT_MAX == ULONG_MAX
	primary_type_t operator -= (long v) { return f_ptr -= v; }
#endif
#if UINT_MAX == ULONG_MAX
	primary_type_t operator -= (unsigned long v) { return f_ptr -= v; }
#endif
	primary_type_t operator -= (int64_t v) { return f_ptr -= v; }
	primary_type_t operator -= (uint64_t v) { return f_ptr -= v; }
#ifdef __APPLE__
	primary_type_t operator -= (size_t v) { return f_ptr -= v; }
#endif
	primary_type_t operator + (signed char v) { return f_ptr + v; }
	primary_type_t operator + (unsigned char v) { return f_ptr + v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	primary_type_t operator + (wchar_t v) { return f_ptr + v; }
#endif
	primary_type_t operator + (int16_t v) { return f_ptr + v; }
	primary_type_t operator + (uint16_t v) { return f_ptr + v; }
	primary_type_t operator + (int32_t v) { return f_ptr + v; }
	primary_type_t operator + (uint32_t v) { return f_ptr + v; }
#if UINT_MAX == ULONG_MAX
	primary_type_t operator + (long v) { return f_ptr + v; }
#endif
#if UINT_MAX == ULONG_MAX
	primary_type_t operator + (unsigned long v) { return f_ptr + v; }
#endif
	primary_type_t operator + (int64_t v) { return f_ptr + v; }
	primary_type_t operator + (uint64_t v) { return f_ptr + v; }
#ifdef __APPLE__
	primary_type_t operator + (size_t v) { return f_ptr + v; }
#endif
	primary_type_t operator - (signed char v) { return f_ptr - v; }
	primary_type_t operator - (unsigned char v) { return f_ptr - v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	primary_type_t operator - (wchar_t v) { return f_ptr - v; }
#endif
	primary_type_t operator - (int16_t v) { return f_ptr - v; }
	primary_type_t operator - (uint16_t v) { return f_ptr - v; }
	primary_type_t operator - (int32_t v) { return f_ptr - v; }
	primary_type_t operator - (uint32_t v) { return f_ptr - v; }
#if UINT_MAX == ULONG_MAX
	primary_type_t operator - (long v) { return f_ptr - v; }
#endif
#if UINT_MAX == ULONG_MAX
	primary_type_t operator - (unsigned long v) { return f_ptr - v; }
#endif
	primary_type_t operator - (int64_t v) { return f_ptr - v; }
	primary_type_t operator - (uint64_t v) { return f_ptr - v; }
#ifdef __APPLE__
	primary_type_t operator - (size_t v) { return f_ptr - v; }
#endif
	bool operator == (T& p) { return f_ptr == &p; }
	bool operator == (primary_type_t p) { return f_ptr == p; }
	bool operator == (const ptr_need_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr == p.f_ptr; }
	bool operator == (const ptr_need_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr == p->f_ptr; }
	bool operator != (T& p) { return f_ptr != &p; }
	bool operator != (primary_type_t p) { return f_ptr != p; }
	bool operator != (const ptr_need_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr != p.f_ptr; }
	bool operator != (const ptr_need_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr != p->f_ptr; }
	bool operator < (T& p) { return f_ptr < &p; }
	bool operator < (primary_type_t p) { return f_ptr < p; }
	bool operator < (const ptr_need_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr < p.f_ptr; }
	bool operator < (const ptr_need_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr < p->f_ptr; }
	bool operator <= (T& p) { return f_ptr <= &p; }
	bool operator <= (primary_type_t p) { return f_ptr <= p; }
	bool operator <= (const ptr_need_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr <= p.f_ptr; }
	bool operator <= (const ptr_need_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr <= p->f_ptr; }
	bool operator > (T& p) { return f_ptr > &p; }
	bool operator > (primary_type_t p) { return f_ptr > p; }
	bool operator > (const ptr_need_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr > p.f_ptr; }
	bool operator > (const ptr_need_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr > p->f_ptr; }
	bool operator >= (T& p) { return f_ptr >= &p; }
	bool operator >= (primary_type_t p) { return f_ptr >= p; }
	bool operator >= (const ptr_need_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr >= p.f_ptr; }
	bool operator >= (const ptr_need_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr >= p->f_ptr; }
#ifdef CONTROLLED_VARS_DEBUG
	bool is_initialized() const { return true; }
#endif
private:
	primary_type_t f_ptr;
};
typedef ptr_need_init<bool> mpbool_t;
typedef ptr_need_init<char> mpchar_t;
typedef ptr_need_init<signed char> mpschar_t;
typedef ptr_need_init<unsigned char> mpuchar_t;
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
typedef ptr_need_init<wchar_t> mpwchar_t;
#endif
typedef ptr_need_init<int16_t> mpint16_t;
typedef ptr_need_init<uint16_t> mpuint16_t;
typedef ptr_need_init<int32_t> mpint32_t;
typedef ptr_need_init<uint32_t> mpuint32_t;
#if UINT_MAX == ULONG_MAX
typedef ptr_need_init<long> mpplain_long_t;
#endif
#if UINT_MAX == ULONG_MAX
typedef ptr_need_init<unsigned long> mpplain_ulong_t;
#endif
typedef ptr_need_init<int64_t> mpint64_t;
typedef ptr_need_init<uint64_t> mpuint64_t;
typedef ptr_need_init<float> mpfloat_t;
typedef ptr_need_init<double> mpdouble_t;
#ifndef __APPLE__
typedef ptr_need_init<long double> mplongdouble_t;
#endif
#ifdef __APPLE__
typedef ptr_need_init<size_t> mpsize_t;
#endif
#ifdef __APPLE__
typedef ptr_need_init<time_t> mptime_t;
#endif
} // namespace controlled_vars
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
