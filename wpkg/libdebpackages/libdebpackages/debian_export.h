/*    debian_export.h -- definitions about import/export for DLLs
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
#ifndef DEBIAN_PACKAGE_EXPORT_H
#define DEBIAN_PACKAGE_EXPORT_H 1

/** \file
 * \brief Support of public classes and functions under MS-Windows.
 *
 * Unfortunately it is required, with the MS-Windows compiler (namely, cl)
 * to specify a list of public items via mechanism which we can control at
 * compile time using macros.
 *
 * This file offers a definition, DEBIAN_PACKAGE_EXPORT, used to mark
 * all the classes and public functions so they are accessible from the
 * outside.
 */

// Before including this file, in your tool .h or .cpp file do:
//    #define DEBIAN_PACKAGE_DLL
// to make use of the DLL under Windows. You may defining it in
// your DLLs too, although that is not required.
#if ( defined(DEBIAN_PACKAGE_DLL) || defined(_WINDLL) ) && ( defined(_WINDOWS) ||  defined(WINDOWS) )
#ifdef debpackages_EXPORTS
#define DEBIAN_PACKAGE_EXPORT __declspec(dllexport)
#else
#define DEBIAN_PACKAGE_EXPORT __declspec(dllimport)
#endif
#else
#define DEBIAN_PACKAGE_EXPORT
#endif
#endif
// #ifndef DEBIAN_PACKAGE_EXPORT_H
// vim: ts=4 sw=4 et
