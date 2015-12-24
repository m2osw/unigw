/*    debian_version -- a set of functions to compare versions together
 *    Copyright (C) 2006-2015  Made to Order Software Corporation
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
#ifndef DEBIAN_VERSION_H
#define DEBIAN_VERSION_H 1

/** \file
 * \brief C functions used to parse and compare versions.
 *
 * The library supports ways to handle versions in C or C++. The C++ API
 * is not exposed. Outsiders are expected to make use of the C API.
 *
 * This file describes the necessary functions to parse a version and then
 * compare two versions together.
 */
#include    "debian_export.h"

#include    <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif


struct DEBIAN_PACKAGE_EXPORT debian_version_t;
typedef struct debian_version_t *debian_version_handle_t;

DEBIAN_PACKAGE_EXPORT int validate_debian_version(const char *string, char *error_string, size_t error_size);
DEBIAN_PACKAGE_EXPORT debian_version_handle_t string_to_debian_version(const char *string, char *error_string, size_t error_size);
DEBIAN_PACKAGE_EXPORT int debian_version_to_string(const debian_version_handle_t debian_version, char *string, size_t string_size);
DEBIAN_PACKAGE_EXPORT int debian_versions_compare(const debian_version_handle_t left, const debian_version_handle_t right);
DEBIAN_PACKAGE_EXPORT void delete_debian_version(debian_version_handle_t debian_version);



#ifdef __cplusplus
}
#endif

#endif
// vim: ts=4 sw=4 et
