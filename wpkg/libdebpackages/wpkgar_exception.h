/*    wpkgar.h -- declaration of the wpkg archive
 *    Copyright (C) 2012-2015  Made to Order Software Corporation
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
 * \brief wpkg archive manager.
 *
 * This file declares the wpkg archive manager which is used to build,
 * unpack, install, configure, upgrade, deconfigure, remove, purge
 * packages.
 *
 * The class is very handy to handle any number of packages in a fairly
 * transparent manner as it will give you direct access to control files
 * and their fields, package data, repositories, etc.
 */
#pragma once

#include <exception>

namespace wpkgar
{

class wpkgar_exception : public std::runtime_error
{
public:
    wpkgar_exception(const std::string& what_msg) : runtime_error(what_msg) {}
};

class wpkgar_exception_parameter : public wpkgar_exception
{
public:
    wpkgar_exception_parameter(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_invalid : public wpkgar_exception
{
public:
    wpkgar_exception_invalid(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_invalid_emptydir : public wpkgar_exception_invalid
{
public:
    wpkgar_exception_invalid_emptydir(const std::string& what_msg) : wpkgar_exception_invalid(what_msg) {}
};

class wpkgar_exception_compatibility : public wpkgar_exception
{
public:
    wpkgar_exception_compatibility(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_undefined : public wpkgar_exception
{
public:
    wpkgar_exception_undefined(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_io : public wpkgar_exception
{
public:
    wpkgar_exception_io(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_defined_twice : public wpkgar_exception
{
public:
    wpkgar_exception_defined_twice(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_locked : public wpkgar_exception
{
public:
    wpkgar_exception_locked(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_stop : public wpkgar_exception
{
public:
    wpkgar_exception_stop(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

}
// namespace wpkgar

// vim: ts=4 sw=4 et
