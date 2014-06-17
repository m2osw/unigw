/*    wpkg_util.h -- useful functions available to wpkg users
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
 * \brief Utilities declarations.
 *
 * This includes a set of utility functions used by the library and made
 * public as these can be useful from the outside as well.
 */
#ifndef WPKG_UTIL_H
#define WPKG_UTIL_H
#include    "libdebpackages/memfile.h"


namespace wpkg_util
{

// generic wpkg util exception
class wpkg_util_exception : public std::runtime_error
{
public:
    wpkg_util_exception(const std::string& msg) : runtime_error(msg) {}
};

// problem with data
class wpkg_util_exception_invalid : public wpkg_util_exception
{
public:
    wpkg_util_exception_invalid(const std::string& msg) : wpkg_util_exception(msg) {}
};

typedef std::map<std::string, std::string>  md5sums_map_t;


DEBIAN_PACKAGE_EXPORT bool is_special_windows_filename(const std::string& path_part);
DEBIAN_PACKAGE_EXPORT bool is_valid_windows_filename(const std::string& filename);
DEBIAN_PACKAGE_EXPORT bool is_package_name(const std::string& name);
DEBIAN_PACKAGE_EXPORT void parse_md5sums(md5sums_map_t& sums, memfile::memory_file& md5file);
DEBIAN_PACKAGE_EXPORT std::string rfc2822_date(time_t t = 0);
DEBIAN_PACKAGE_EXPORT bool is_valid_uri(const std::string& uri, std::string protocols = "");
DEBIAN_PACKAGE_EXPORT std::string make_safe_console_string(const std::string& str);
DEBIAN_PACKAGE_EXPORT int versioncmp(const std::string& a, const std::string& b);
DEBIAN_PACKAGE_EXPORT std::string canonicalize_version_for_filename(const std::string& version);
DEBIAN_PACKAGE_EXPORT std::string canonicalize_version(const std::string& version);
DEBIAN_PACKAGE_EXPORT std::string utf8_getenv(const std::string& names, const std::string& default_value);


}    // namespace wpkg_util

#endif
//#ifndef WPKG_UTIL_H
// vim: ts=4 sw=4 et
