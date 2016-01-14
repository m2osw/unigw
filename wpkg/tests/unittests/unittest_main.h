/*    unittest_main.h
 *    Copyright (C) 2013-2015  Made to Order Software Corporation
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
#pragma once

#include "wpkg_tools.h"

#include <string>
#include <cstring>
#include <cstdlib>

#ifdef _MSC_VER
// The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: ... See online help for details.
// This is because of the putenv() and strdup() used in the class below
#	pragma warning(disable: 4996)
#endif

// vim: ts=4 sw=4 et
