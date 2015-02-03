/*    unittest_advgetopt.h
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
#ifndef UNIT_TEST_ADVGETOPT_H
#define UNIT_TEST_ADVGETOPT_H

#include <cppunit/extensions/HelperMacros.h>

namespace wpkg_filename
{
class uri_filename;
}

class AdvGetOptUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( AdvGetOptUnitTests );
        CPPUNIT_TEST( invalid_parameters );
        CPPUNIT_TEST( valid_config_files );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();

protected:
    void invalid_parameters();
    void valid_config_files();
};

#endif
// vim: ts=4 sw=4 et
