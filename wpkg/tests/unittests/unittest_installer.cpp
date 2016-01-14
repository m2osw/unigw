/*    unittest_installer.cpp
 *    Copyright (C) 2016  Made to Order Software Corporation
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
 *    Doug Barbieri		doug@m2osw.com
 */

#include "unittest_main.h"

#include "libdebpackages/wpkgar_install.h"

#include <catch.hpp>


class InstallerUnitTests : public test_common::wpkg_tools
{
public:
    InstallerUnitTests();

    void a_test();
};


InstallerUnitTests::InstallerUnitTests() : test_common::wpkg_tools()
{
}


void InstallerUnitTests::a_test()
{
}


// vim: ts=4 sw=4 et
