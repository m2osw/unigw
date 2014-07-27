/*    unittest_expr.h
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
#ifndef UNIT_TEST_EXPR_H
#define UNIT_TEST_EXPR_H

#include <cppunit/extensions/HelperMacros.h>

namespace wpkg_filename
{
class uri_filename;
}

class ExprUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( ExprUnitTests );
        CPPUNIT_TEST( bad_literals );
        CPPUNIT_TEST( bad_variables );
        CPPUNIT_TEST( bad_expressions );
        CPPUNIT_TEST( additions );
        CPPUNIT_TEST( shifts );
        CPPUNIT_TEST( increments );
        CPPUNIT_TEST( multiplications );
        CPPUNIT_TEST( bitwise );
        CPPUNIT_TEST( comparisons );
        CPPUNIT_TEST( assignments );
        CPPUNIT_TEST( functions );
        CPPUNIT_TEST( misc );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();

protected:
    void bad_literals();
    void bad_variables();
    void bad_expressions();
    void additions();
    void shifts();
    void increments();
    void multiplications();
    void bitwise();
    void comparisons();
    void assignments();
    void functions();
    void misc();
};

#endif
// vim: ts=4 sw=4 et
