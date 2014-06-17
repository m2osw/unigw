/*    unittest_control.h
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
#ifndef UNIT_TEST_CONTROL_H
#define UNIT_TEST_CONTROL_H

#include <cppunit/extensions/HelperMacros.h>
#include "libdebpackages/wpkg_control.h"

namespace wpkg_filename
{
class uri_filename;
}

class ControlUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( ControlUnitTests );
        CPPUNIT_TEST( files_field_to_list );
        CPPUNIT_TEST( all_files_field );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();

protected:
    void files_field_to_list();
    void all_files_field();

private:
    // helper functions
    void check_field(const std::string& field_name, const std::string& default_format, wpkg_control::file_item::format_t format);
};

#endif
// vim: ts=4 sw=4 et
