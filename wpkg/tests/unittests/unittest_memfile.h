/*    unittest_memfile.h
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
#ifndef UNIT_TEST_MEMFILE_H
#define UNIT_TEST_MEMFILE_H

#include <cppunit/extensions/HelperMacros.h>

class MemfileUnitTests : public CPPUNIT_NS::TestFixture
{
        CPPUNIT_TEST_SUITE( MemfileUnitTests );
        CPPUNIT_TEST( compression1 );
        CPPUNIT_TEST( compression2 );
        CPPUNIT_TEST( compression3 );
        CPPUNIT_TEST( compression4 );
        CPPUNIT_TEST( compression5 );
        CPPUNIT_TEST( compression6 );
        CPPUNIT_TEST( compression7 );
        CPPUNIT_TEST( compression8 );
        CPPUNIT_TEST( compression9 );
        CPPUNIT_TEST_SUITE_END();

public:
        void setUp();

protected:
        void compression(int level);

        void compression1();
        void compression2();
        void compression3();
        void compression4();
        void compression5();
        void compression6();
        void compression7();
        void compression8();
        void compression9();
};

#endif
