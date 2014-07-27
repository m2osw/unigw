/*    libutf8.h -- convert between wchar_t and UTF-8 encodings
 *    Copyright (C) 2000-2014  Made to Order Software Corporation
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
 * \brief The declarations of the UTF-8 library.
 *
 * This file is the declarations of the UTF-8 library which are just a few
 * functions used to convert a string from one format to another.
 */
#ifndef LIBUTF8_H
#define LIBUTF8_H
#include "libutf8/libutf8_export.h"

#include <stdint.h>
#include <string>
#include <stdexcept>

namespace libutf8
{

class libutf8_exception : public std::runtime_error
{
public:
    libutf8_exception(const std::string& msg) : runtime_error(msg) {}
};

class libutf8_exception_decoding : public libutf8_exception
{
public:
    libutf8_exception_decoding(const std::string& msg) : libutf8_exception(msg) {}
};


LIBUTF8_EXPORT std::string wcstombs(const std::wstring& str);
LIBUTF8_EXPORT std::wstring mbstowcs(const std::string& str);
LIBUTF8_EXPORT size_t mbslen(const std::string& str);
LIBUTF8_EXPORT int mbscasecmp(const std::string& lhs, const std::string& rhs);

} // libutf8 namespace
#endif
//#ifndef LIBUTF8_H
// vim: ts=4 sw=4 et
