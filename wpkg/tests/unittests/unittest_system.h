/*    unittest_system.h
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
 *
 *    Authors
 *    Alexis Wilke   alexis@m2osw.com
 */
#ifndef UNIT_TEST_SYSTEM_H
#define UNIT_TEST_SYSTEM_H

#include <cppunit/extensions/HelperMacros.h>

//namespace wpkg_filename
//{
//class uri_filename;
//}

class SystemUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( SystemUnitTests );
        CPPUNIT_TEST( manual_builds );
        CPPUNIT_TEST( automated_builds );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();

protected:
    void manual_builds();
    void automated_builds();
};

#endif
// vim: ts=4 sw=4 et
