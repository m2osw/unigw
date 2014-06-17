/*    case_insensitive_string.cpp -- implementation of the case insensitive string
 *    Copyright (C) 2012-2013  Made to Order Software Corporation
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
 * \brief A standard string that is case insensitive.
 *
 * This implementation of std::string compares strings using the string case
 * insensitive compare functions. It is otherwise identical to the std::string
 * class except for constructors that are not all implemented.
 *
 * The main use for these type of strings is for filenames on operating
 * systems that do not distinguish upper and lower case characters in their
 * file systems (i.e. Mac OS/X and MS-Windows.)
 */
#include    "libdebpackages/case_insensitive_string.h"
#include    "libdebpackages/compatibility.h"
#include    "libutf8/libutf8.h"
#include    <string.h>


/** \brief The case insensitive namespace.
 *
 * This namespace encompasses all the case insensitive functions used by
 * the case_insensitive_string declarations and implementation.
 */
namespace case_insensitive
{


/** \class case_insensitive_string
 * \brief An extension of std::string to support case insensitive strings.
 *
 * This class is an overload of the std::string class which allows for
 * comparison operators to compare in a case insensitive manner.
 * This is quite practical instead of having to use the strcasecmp()
 * function because the same variable can either be defined as an std::string
 * or a case_insensitive::case_insensitive_string and then used the same
 * way for the rest of your function knowing that each == or < operator
 * will function in a case insensitive manner for the latter type.
 *
 * Note that this overload does not duplicate all the constructors so
 * it does not work 1 to 1 like an std::string. However, in most cases it
 * is possible to make it all work the same way.
 */


/** \brief Initialize the case insensitive string from a standard string.
 *
 * This constructor creates a case insensitive string from the specified
 * parameter, \p str.
 *
 * \param[in] str  The case insensitive string is a copy of this string.
 */
case_insensitive_string::case_insensitive_string(const std::string& str)
    : std::string(str)
{
}


/** \brief Initialize the case insensitive string from a standard string.
 *
 * This constructor creates a case insensitive string from the specified
 * parameter, \p str, starting at position \p pos and with at most \p n
 * characters.
 *
 * \param[in] str  The string to copy.
 * \param[in] pos  Start the copy from this position.
 * \param[in] n  The number of characters to copy from str.
 */
case_insensitive_string::case_insensitive_string(const std::string& str, size_t pos, size_t n)
    : std::string(str, pos, n)
{
}


/** \brief Initialize the case insensitive string from a null terminated C-string.
 *
 * This function copies the specified C-like string to the case insensitive
 * string. The number of characters to copy is limited by \p n.
 *
 * \note
 * The string does not need to end with a NUL character ('\0') although if
 * \p n may be set to a value larger than the number of characters in the
 * input string, then the copy will have undetermined results.
 *
 * \param[in] s  A pointer to the null terminated C-string to copy.
 * \param[in] n  The maximum number of characters to copy from \p s.
 */
case_insensitive_string::case_insensitive_string(const char *s, size_t n)
    : std::string(s, n)
{
}


/** \brief Initialize the case insensitive string from a null terminated C-string.
 *
 * Copy the specified C-string in the case insensitive string. The C-string
 * must be NUL ('\0') terminated or the copy ends with undetermined results.
 *
 * \param[in] s  The pointer to a null terminated C-string to copy.
 */
case_insensitive_string::case_insensitive_string(const char *s)
    : std::string(s)
{
}


/** \brief Compare two strings case insensitively.
 *
 * If this string is considered smaller than \p rhs, not taking character
 * case in account, then this function returns true.
 *
 * \param[in] rhs  The other string to compare against.
 *
 * \return true if this string is considered smaller than \p rhs.
 */
bool case_insensitive_string::operator < (const case_insensitive_string& rhs) const
{
    return libutf8::mbscasecmp(*this, rhs) < 0;
}


/** \brief Compare two strings case insensitively.
 *
 * If this string is considered equal to \p rhs, not taking character
 * case in account, then this function returns true.
 *
 * \param[in] rhs  The other string to compare against.
 *
 * \return true if this string is equal to \p rhs.
 */
bool case_insensitive_string::operator == (const case_insensitive_string& rhs) const
{
    return libutf8::mbscasecmp(*this, rhs) == 0;
}


/** \brief Compare two strings case insensitively.
 *
 * If this string is considered equal to \p rhs, not taking character
 * case in account, then this function returns true.
 *
 * \param[in] rhs  The other string to compare against.
 *
 * \return true if this string is equal to \p rhs.
 */
bool case_insensitive_string::operator == (const char *rhs) const
{
    return libutf8::mbscasecmp(*this, rhs) == 0;
}


/** \brief Compare two strings case insensitively.
 *
 * If this string is considered inequal to \p rhs, not taking character
 * case in account, then this function returns true.
 *
 * \param[in] rhs  The other string to compare against.
 *
 * \return true if this string is not equal to \p rhs.
 */
bool case_insensitive_string::operator != (const case_insensitive_string& rhs) const
{
    return libutf8::mbscasecmp(*this, rhs) != 0;
}


/** \brief Compare two strings case insensitively.
 *
 * If this string is considered inequal to \p rhs, not taking character
 * case in account, then this function returns true.
 *
 * \param[in] rhs  The other string to compare against.
 *
 * \return true if this string is not equal to \p rhs.
 */
bool case_insensitive_string::operator != (const char *rhs) const
{
    return libutf8::mbscasecmp(*this, rhs) != 0;
}


}  // namespace case_insensitive
// vim: ts=4 sw=4 et
