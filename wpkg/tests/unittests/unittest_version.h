/*    unittest_control.h
 *    Copyright (C) 2013-2014  Made to Order Software Corporation
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
#ifndef UNIT_TEST_VERSION_H
#define UNIT_TEST_VERSION_H

#include <cppunit/extensions/HelperMacros.h>
#include "libdebpackages/wpkg_control.h"

namespace wpkg_filename
{
class uri_filename;
}

class VersionUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( VersionUnitTests );
        CPPUNIT_TEST( valid_versions );
        CPPUNIT_TEST( invalid_versions );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();

protected:
    void valid_versions();
    void invalid_versions();
};

#endif
// vim: ts=4 sw=4 et
