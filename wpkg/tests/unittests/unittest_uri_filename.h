/*    unittest_uri_filename.h
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
#ifndef UNIT_TEST_URI_FILENAME_H
#define UNIT_TEST_URI_FILENAME_H

#include <cppunit/extensions/HelperMacros.h>

namespace wpkg_filename
{
class uri_filename;
}

class URIFilenameUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( URIFilenameUnitTests );
        CPPUNIT_TEST( path );
        CPPUNIT_TEST( invalid_ms_paths );
        CPPUNIT_TEST( invalid_uri );
        CPPUNIT_TEST( common_segments );
        CPPUNIT_TEST( long_filename );
    CPPUNIT_TEST_SUITE_END();

public:
    struct expected_result
    {
        const char *    f_original_filename;
        const char *    f_fixed_original_filename;
        const char *    f_path_type;
        const char *    f_path_scheme;
        const char *    f_path_only;
        const char *    f_path_only_no_drive;
        const char *    f_full_path;
        const char *    f_segments[32];
        const char *    f_dirname;
        const char *    f_dirname_no_drive;
        const char *    f_basename;
        const char *    f_basename_last_only;
        const char *    f_extension;
        const char *    f_previous_extension;
        const char      f_msdos_drive;
        const char *    f_username;
        const char *    f_password;
        const char *    f_domain;
        const char *    f_port;
        const char *    f_share;
        bool            f_decode;
        const char *    f_anchor;
        const char *    f_query_variables[32];
        const char *    f_glob;

        bool            f_empty;
        bool            f_is_deb;
        bool            f_is_valid;
        bool            f_is_direct;
        bool            f_is_absolute;
    };

    void setUp();

protected:
    void check(const wpkg_filename::uri_filename& fn, const expected_result& er);

    void path();
    void invalid_ms_paths();
    void invalid_uri();
    void common_segments();
    void long_filename();
};

#endif
// vim: ts=4 sw=4 et
