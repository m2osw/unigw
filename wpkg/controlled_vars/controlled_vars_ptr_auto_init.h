// WARNING: do not edit; this is an auto-generated
// WARNING: file; please, use the generator named
// WARNING: controlled_vars to re-generate
//
// File:	controlled_vars_ptr_auto_init.h
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
#ifndef CONTROLLED_VARS_PTR_AUTO_INIT_H
#define CONTROLLED_VARS_PTR_AUTO_INIT_H
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
template<class T> class trait_ptr_auto_null { public: static T *DEFAULT_VALUE() { return 0; } };
template<class T, typename init_value = trait_ptr_auto_null<T> > class ptr_auto_init {
public:
	typedef T *primary_type_t;
	static T *DEFAULT_VALUE() { return init_value::DEFAULT_VALUE(); }
	static T *null() { return 0; }
	ptr_auto_init() { f_ptr = DEFAULT_VALUE(); }
	ptr_auto_init(T *p) { f_ptr = p; }
	ptr_auto_init(T& p) { f_ptr = &p; }
	ptr_auto_init(const ptr_auto_init *p) { f_ptr = p == 0 ? 0 : p->f_ptr; }
	ptr_auto_init(const ptr_auto_init& p) { f_ptr = &p == 0 ? 0 : p.f_ptr; }
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
	void swap(ptr_auto_init& p) { primary_type_t n(f_ptr); f_ptr = p.f_ptr; p.f_ptr = n; }
	operator bool () const { return f_ptr != 0; }
	bool operator ! () const { return f_ptr == 0; }
	ptr_auto_init& operator ++ () { ++f_ptr; return *this; }
	ptr_auto_init operator ++ (int) { ptr_auto_init<T> result(*this); ++f_ptr; return result; }
	ptr_auto_init& operator -- () { --f_ptr; return *this; }
	ptr_auto_init operator -- (int) { ptr_auto_init<T> result(*this); --f_ptr; return result; }
	void reset() { f_ptr = init_value::DEFAULT_VALUE(); }
	void reset(T& p) { f_ptr = &p; }
	void reset(primary_type_t p) { f_ptr = p; }
	void reset(const ptr_auto_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); f_ptr = p.f_ptr; }
	void reset(const ptr_auto_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); f_ptr = p->f_ptr; }
	ptr_auto_init& operator = (T& p) { f_ptr = &p; return *this; }
	ptr_auto_init& operator = (primary_type_t p) { f_ptr = p; return *this; }
	ptr_auto_init& operator = (const ptr_auto_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); f_ptr = p.f_ptr; return *this; }
	ptr_auto_init& operator = (const ptr_auto_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); f_ptr = p->f_ptr; return *this; }
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
//#ifdef __APPLE__
//	primary_type_t operator += (size_t v) { return f_ptr += v; }
//#endif
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
//#ifdef __APPLE__
//	primary_type_t operator -= (size_t v) { return f_ptr -= v; }
//#endif
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
//#ifdef __APPLE__
//	primary_type_t operator + (size_t v) { return f_ptr + v; }
//#endif
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
//#ifdef __APPLE__
//	primary_type_t operator - (size_t v) { return f_ptr - v; }
//#endif
	bool operator == (T& p) { return f_ptr == &p; }
	bool operator == (primary_type_t p) { return f_ptr == p; }
	bool operator == (const ptr_auto_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr == p.f_ptr; }
	bool operator == (const ptr_auto_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr == p->f_ptr; }
	bool operator != (T& p) { return f_ptr != &p; }
	bool operator != (primary_type_t p) { return f_ptr != p; }
	bool operator != (const ptr_auto_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr != p.f_ptr; }
	bool operator != (const ptr_auto_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr != p->f_ptr; }
	bool operator < (T& p) { return f_ptr < &p; }
	bool operator < (primary_type_t p) { return f_ptr < p; }
	bool operator < (const ptr_auto_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr < p.f_ptr; }
	bool operator < (const ptr_auto_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr < p->f_ptr; }
	bool operator <= (T& p) { return f_ptr <= &p; }
	bool operator <= (primary_type_t p) { return f_ptr <= p; }
	bool operator <= (const ptr_auto_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr <= p.f_ptr; }
	bool operator <= (const ptr_auto_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr <= p->f_ptr; }
	bool operator > (T& p) { return f_ptr > &p; }
	bool operator > (primary_type_t p) { return f_ptr > p; }
	bool operator > (const ptr_auto_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr > p.f_ptr; }
	bool operator > (const ptr_auto_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr > p->f_ptr; }
	bool operator >= (T& p) { return f_ptr >= &p; }
	bool operator >= (primary_type_t p) { return f_ptr >= p; }
	bool operator >= (const ptr_auto_init& p) { if(&p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr >= p.f_ptr; }
	bool operator >= (const ptr_auto_init *p) { if(p == 0) throw controlled_vars_error_null_pointer("dereferencing a null pointer"); return f_ptr >= p->f_ptr; }
#ifdef CONTROLLED_VARS_DEBUG
	bool is_initialized() const { return true; }
#endif
private:
	primary_type_t f_ptr;
};
typedef ptr_auto_init<bool> zpbool_t;
typedef ptr_auto_init<char> zpchar_t;
typedef ptr_auto_init<signed char> zpschar_t;
typedef ptr_auto_init<unsigned char> zpuchar_t;
#if !defined(_MSC_VER) || (defined(_WCHAR_T_DEFINED) && defined(_NATIVE_WCHAR_T_DEFINED))
typedef ptr_auto_init<wchar_t> zpwchar_t;
#endif
typedef ptr_auto_init<int16_t> zpint16_t;
typedef ptr_auto_init<uint16_t> zpuint16_t;
typedef ptr_auto_init<int32_t> zpint32_t;
typedef ptr_auto_init<uint32_t> zpuint32_t;
#if UINT_MAX == ULONG_MAX
typedef ptr_auto_init<long> zpplain_long_t;
#endif
#if UINT_MAX == ULONG_MAX
typedef ptr_auto_init<unsigned long> zpplain_ulong_t;
#endif
typedef ptr_auto_init<int64_t> zpint64_t;
typedef ptr_auto_init<uint64_t> zpuint64_t;
//#ifdef __APPLE__
//typedef ptr_auto_init<size_t> zpsize_t;
//#endif
//#ifdef __APPLE__
//typedef ptr_auto_init<time_t> zptime_t;
//#endif
} // namespace controlled_vars
#ifdef _MSC_VER
#pragma warning(pop)
#endif
#endif
