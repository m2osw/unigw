/*    expr_export.h -- definitions about import/export for DLLs
 *    Copyright (C) 2013  Made to Order Software Corporation
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
 */

/** \file
 * \brief Export definitions for MS-Windows DLLs.
 *
 * This file declares the import/export information as defined by MS-Windows
 * for DLLs.
 */
#ifndef LIBEXPR_EXPORT_H
#define LIBEXPR_EXPORT_H
// Before including this file, in your tool .h or .cpp file do:
//    #define EXPR_DLL
// to make use of the DLL under Windows. You may defining it in
// your DLLs too, although that is not required.
#if ( defined(EXPR_DLL) || defined(_WINDLL) ) && ( defined(_WINDOWS) ||  defined(WINDOWS) )
#ifdef expr_EXPORTS
#define EXPR_EXPORT __declspec(dllexport)
#else
#define EXPR_EXPORT __declspec(dllimport)
#endif
#else
#define EXPR_EXPORT
#endif
#endif
// #ifndef LIBEXPR_EXPORT_H
// vim: ts=4 sw=4 et
