/*    case_insensitive_string.h -- compare std::string's ignoring case
 *    Copyright (C) 2012-2014  Made to Order Software Corporation
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
#ifndef CASE_INSENSITIVE_STRING_H
#define CASE_INSENSITIVE_STRING_H

/** \file
 * \brief Definitions of the case insensitive string.
 *
 * The case insensitive string is a simple class derived from the std::string
 * which reimplements the comparison operators using case insensitive string
 * comparison operations.
 *
 * This is particularly useful when comparing filenames on file systems
 * that make ignore case.
 */

#include    "debian_export.h"

#include    <string>

namespace case_insensitive
{

// I find it weird that the std::string does not offer a way
// to compare two strings ignoring case...
// anyway, there is a remedy so we can have maps with any case
// but matching as expected
class DEBIAN_PACKAGE_EXPORT case_insensitive_string : public std::string
{
public:
    case_insensitive_string(const std::string& s);
    case_insensitive_string(const std::string& str, size_t pos, size_t n = npos);
    case_insensitive_string(const char *s, size_t n);
    case_insensitive_string(const char *s);
    case_insensitive_string(size_t n, char c);

    bool operator < (const case_insensitive_string& rhs) const;
    bool operator == (const case_insensitive_string& rhs) const;
    bool operator == (const char *rhs) const;
    bool operator != (const case_insensitive_string& rhs) const;
    bool operator != (const char *rhs) const;
};

}   // namespace case_insensitive
#endif
//#ifdef CASE_INSENSITIVE_STRING_H
// vim: ts=4 sw=4 et
