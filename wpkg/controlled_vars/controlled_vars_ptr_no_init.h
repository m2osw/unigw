// WARNING: do not edit; this is an auto-generated
// WARNING: file; please, use the generator named
// WARNING: controlled_vars to re-generate
//
// File:	controlled_vars_ptr_no_init.h
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
#ifndef CONTROLLED_VARS_PTR_NO_INIT_H
#define CONTROLLED_VARS_PTR_NO_INIT_H
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
template<class T> class ptr_no_init {
public:
	typedef T *primary_type_t;
	static T *null() { return 0; }
	ptr_no_init() { f_initialized = false; }
	ptr_no_init(T *p) { f_initialized = true; f_ptr = p; }
	ptr_no_init(T& p) { f_initialized = true; f_ptr = &p; }
	ptr_no_init(const ptr_no_init *p) { f_initialized = true; f_ptr = p == 0 ? 0 : p->f_ptr; }
	ptr_no_init(const ptr_no_init& p) { f_initialized = true; f_ptr = &p == 0 ? 0 : p.f_ptr; }
	operator primary_type_t () const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr; }
	operator primary_type_t () { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr; }
	primary_type_t value() const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr; }
	T *get() const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr; }
	primary_type_t *ptr() const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return &f_ptr; }
	T *operator -> () const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(f_ptr == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr; }
	T& operator * () const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(f_ptr == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return *f_ptr; }
	T operator [] (int index) const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(f_ptr == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr[index]; }
	T *get() { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr; }
	primary_type_t *ptr() { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return &f_ptr; }
	T *operator -> () { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(f_ptr == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr; }
	T& operator * () { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(f_ptr == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return *f_ptr; }
	T& operator [] (int index) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(f_ptr == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr[index]; }
	void swap(ptr_no_init& p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); primary_type_t n(f_ptr); f_ptr = p.f_ptr; p.f_ptr = n; }
	operator bool () const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr != 0; }
	bool operator ! () const { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr == 0; }
	ptr_no_init& operator ++ () { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); ++f_ptr; return *this; }
	ptr_no_init operator ++ (int) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); ptr_no_init<T> result(*this); ++f_ptr; return result; }
	ptr_no_init& operator -- () { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); --f_ptr; return *this; }
	ptr_no_init operator -- (int) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); ptr_no_init<T> result(*this); --f_ptr; return result; }
	void reset() { f_initialized = true; f_ptr = null(); }
	void reset(T& p) { f_initialized = true; f_ptr = &p; }
	void reset(primary_type_t p) { f_initialized = true; f_ptr = p; }
	void reset(const ptr_no_init& p) { if(!p.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); f_initialized = true; f_ptr = p.f_ptr; }
	void reset(const ptr_no_init *p) { if(!p->f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); f_initialized = true; f_ptr = p->f_ptr; }
	ptr_no_init& operator = (T& p) { f_initialized = true; f_ptr = &p; return *this; }
	ptr_no_init& operator = (primary_type_t p) { f_initialized = true; f_ptr = p; return *this; }
	ptr_no_init& operator = (const ptr_no_init& p) { if(!p.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); f_initialized = true; f_ptr = p.f_ptr; return *this; }
	ptr_no_init& operator = (const ptr_no_init *p) { if(!p->f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); f_initialized = true; f_ptr = p->f_ptr; return *this; }
	primary_type_t operator += (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr += v; }
	primary_type_t operator += (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr += v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	primary_type_t operator += (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr += v; }
#endif
	primary_type_t operator += (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr += v; }
	primary_type_t operator += (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr += v; }
	primary_type_t operator += (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr += v; }
	primary_type_t operator += (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr += v; }
#if UINT_MAX == ULONG_MAX
	primary_type_t operator += (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr += v; }
#endif
#if UINT_MAX == ULONG_MAX
	primary_type_t operator += (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr += v; }
#endif
	primary_type_t operator += (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr += v; }
	primary_type_t operator += (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr += v; }
#ifdef __APPLE__
	primary_type_t operator += (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr += v; }
#endif
	primary_type_t operator -= (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr -= v; }
	primary_type_t operator -= (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr -= v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	primary_type_t operator -= (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr -= v; }
#endif
	primary_type_t operator -= (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr -= v; }
	primary_type_t operator -= (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr -= v; }
	primary_type_t operator -= (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr -= v; }
	primary_type_t operator -= (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr -= v; }
#if UINT_MAX == ULONG_MAX
	primary_type_t operator -= (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr -= v; }
#endif
#if UINT_MAX == ULONG_MAX
	primary_type_t operator -= (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr -= v; }
#endif
	primary_type_t operator -= (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr -= v; }
	primary_type_t operator -= (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr -= v; }
#ifdef __APPLE__
	primary_type_t operator -= (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr -= v; }
#endif
	primary_type_t operator + (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr + v; }
	primary_type_t operator + (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr + v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	primary_type_t operator + (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr + v; }
#endif
	primary_type_t operator + (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr + v; }
	primary_type_t operator + (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr + v; }
	primary_type_t operator + (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr + v; }
	primary_type_t operator + (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr + v; }
#if UINT_MAX == ULONG_MAX
	primary_type_t operator + (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr + v; }
#endif
#if UINT_MAX == ULONG_MAX
	primary_type_t operator + (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr + v; }
#endif
	primary_type_t operator + (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr + v; }
	primary_type_t operator + (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr + v; }
#ifdef __APPLE__
	primary_type_t operator + (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr + v; }
#endif
	primary_type_t operator - (signed char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr - v; }
	primary_type_t operator - (unsigned char v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr - v; }
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
	primary_type_t operator - (wchar_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr - v; }
#endif
	primary_type_t operator - (int16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr - v; }
	primary_type_t operator - (uint16_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr - v; }
	primary_type_t operator - (int32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr - v; }
	primary_type_t operator - (uint32_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr - v; }
#if UINT_MAX == ULONG_MAX
	primary_type_t operator - (long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr - v; }
#endif
#if UINT_MAX == ULONG_MAX
	primary_type_t operator - (unsigned long v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr - v; }
#endif
	primary_type_t operator - (int64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr - v; }
	primary_type_t operator - (uint64_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr - v; }
#ifdef __APPLE__
	primary_type_t operator - (size_t v) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr - v; }
#endif
	bool operator == (T& p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr == &p; }
	bool operator == (primary_type_t p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr == p; }
	bool operator == (const ptr_no_init& p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!p.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr == p.f_ptr; }
	bool operator == (const ptr_no_init *p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!p->f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr == p->f_ptr; }
	bool operator != (T& p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr != &p; }
	bool operator != (primary_type_t p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr != p; }
	bool operator != (const ptr_no_init& p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!p.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr != p.f_ptr; }
	bool operator != (const ptr_no_init *p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!p->f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr != p->f_ptr; }
	bool operator < (T& p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr < &p; }
	bool operator < (primary_type_t p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr < p; }
	bool operator < (const ptr_no_init& p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!p.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr < p.f_ptr; }
	bool operator < (const ptr_no_init *p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!p->f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr < p->f_ptr; }
	bool operator <= (T& p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr <= &p; }
	bool operator <= (primary_type_t p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr <= p; }
	bool operator <= (const ptr_no_init& p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!p.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr <= p.f_ptr; }
	bool operator <= (const ptr_no_init *p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!p->f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr <= p->f_ptr; }
	bool operator > (T& p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr > &p; }
	bool operator > (primary_type_t p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr > p; }
	bool operator > (const ptr_no_init& p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!p.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr > p.f_ptr; }
	bool operator > (const ptr_no_init *p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!p->f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr > p->f_ptr; }
	bool operator >= (T& p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr >= &p; }
	bool operator >= (primary_type_t p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); return f_ptr >= p; }
	bool operator >= (const ptr_no_init& p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!p.f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr >= p.f_ptr; }
	bool operator >= (const ptr_no_init *p) { if(!f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(!p->f_initialized) throw controlled_vars_error_not_initialized("uninitialized variable"); if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr >= p->f_ptr; }
	bool is_initialized() const { return f_initialized; }
private:
	bool f_initialized;
	primary_type_t f_ptr;
};
typedef ptr_no_init<bool> rpbool_t;
typedef ptr_no_init<char> rpchar_t;
typedef ptr_no_init<signed char> rpschar_t;
typedef ptr_no_init<unsigned char> rpuchar_t;
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
typedef ptr_no_init<wchar_t> rpwchar_t;
#endif
typedef ptr_no_init<int16_t> rpint16_t;
typedef ptr_no_init<uint16_t> rpuint16_t;
typedef ptr_no_init<int32_t> rpint32_t;
typedef ptr_no_init<uint32_t> rpuint32_t;
#if UINT_MAX == ULONG_MAX
typedef ptr_no_init<long> rpplain_long_t;
#endif
#if UINT_MAX == ULONG_MAX
typedef ptr_no_init<unsigned long> rpplain_ulong_t;
#endif
typedef ptr_no_init<int64_t> rpint64_t;
typedef ptr_no_init<uint64_t> rpuint64_t;
typedef ptr_no_init<float> rpfloat_t;
typedef ptr_no_init<double> rpdouble_t;
#ifndef __APPLE__
typedef ptr_no_init<long double> rplongdouble_t;
#endif
#ifdef __APPLE__
typedef ptr_no_init<size_t> rpsize_t;
#endif
#ifdef __APPLE__
typedef ptr_no_init<time_t> rptime_t;
#endif
} // namespace controlled_vars
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
