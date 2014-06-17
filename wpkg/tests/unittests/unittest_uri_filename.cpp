/*    unittest_uri_filename.cpp
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

#include "unittest_uri_filename.h"
#include "unittest_main.h"
#include "libdebpackages/wpkg_filename.h"
#include "libdebpackages/wpkg_util.h"

#include <cppunit/config/SourcePrefix.h>
#include <string.h>
#include <time.h>


CPPUNIT_TEST_SUITE_REGISTRATION( URIFilenameUnitTests );

void URIFilenameUnitTests::setUp()
{
}

void URIFilenameUnitTests::check(const wpkg_filename::uri_filename& fn, const expected_result& er)
{
    std::string msg("URIFilenameUnitTests::check(): \"");
    msg += er.f_original_filename;
    msg += "\"";

    msg += " [";
    msg += std::string(er.f_full_path) + "] [";
    msg += fn.full_path() + "] [";
    msg += fn.get_decode() ? "true" : "false";
    msg += "]";

    if(er.f_fixed_original_filename)
    {
        CPPUNIT_ASSERT_MESSAGE(msg, fn.original_filename() == er.f_fixed_original_filename);
    }
    else
    {
        CPPUNIT_ASSERT_MESSAGE(msg, fn.original_filename() == er.f_original_filename);
    }
    CPPUNIT_ASSERT_MESSAGE(msg, fn.path_type() == er.f_path_type);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.path_scheme() == er.f_path_scheme);

    CPPUNIT_ASSERT_MESSAGE(msg, fn.path_only( /*true*/ ) == er.f_path_only);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.path_only(true) == er.f_path_only);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.path_only(false) == er.f_path_only_no_drive);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.full_path() == er.f_full_path);

    size_t i(0);
    for(; i < sizeof(er.f_segments) / sizeof(er.f_segments[0]); ++i)
    {
        if(er.f_segments[i] == NULL)
        {
            break;
        }
        std::string smsg = msg + " \"" + fn.segment(static_cast<int>(i)) + "\" [" + er.f_segments[i] + "]";
        CPPUNIT_ASSERT_MESSAGE(smsg, fn.segment(static_cast<int>(i)) == er.f_segments[i]);

        if(er.f_is_direct)
        {
            CPPUNIT_ASSERT_MESSAGE(smsg, wpkg_util::is_valid_windows_filename(fn.segment(static_cast<int>(i))));
        }
    }
    CPPUNIT_ASSERT_MESSAGE(msg, fn.segment_size() == static_cast<int>(i));

    CPPUNIT_ASSERT_MESSAGE(msg, fn.dirname( /*true*/ ) == er.f_dirname);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.dirname(true) == er.f_dirname);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.dirname(false) == er.f_dirname_no_drive);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.basename( /*false*/ ) == er.f_basename);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.basename(false) == er.f_basename);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.basename(true) == er.f_basename_last_only);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.extension() == er.f_extension);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.previous_extension() == er.f_previous_extension);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.msdos_drive() == er.f_msdos_drive);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.get_username() == er.f_username);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.get_password() == er.f_password);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.get_domain() == er.f_domain);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.get_port() == er.f_port);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.get_share() == er.f_share);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.get_decode() == er.f_decode);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.get_anchor() == er.f_anchor);

    // test the map knowing the exact variable names
    i = 0;
    for(; i < sizeof(er.f_query_variables) / sizeof(er.f_query_variables[0]) / 2; ++i)
    {
        if(er.f_query_variables[i * 2] == NULL)
        {
            break;
        }
        std::string smsg = msg + " \"" + fn.query_variable(er.f_query_variables[i * 2]) + "\" [" + er.f_query_variables[i * 2 + 1] + "]";
        CPPUNIT_ASSERT_MESSAGE(smsg, fn.query_variable(er.f_query_variables[i * 2]) == er.f_query_variables[i * 2 + 1]);
    }

    CPPUNIT_ASSERT_MESSAGE(msg, fn.query_variable("no-a-variable") == "");

    // now test the map itself
    wpkg_filename::uri_filename::query_variables_t vars(fn.all_query_variables());
    i = 0;
    for(wpkg_filename::uri_filename::query_variables_t::const_iterator v(vars.begin()); i < sizeof(er.f_query_variables) / sizeof(er.f_query_variables[0]) / 2; ++i, ++v)
    {
        if(er.f_query_variables[i * 2] == NULL)
        {
            break;
        }
        std::string smsg = msg + " \"" + fn.query_variable(er.f_query_variables[i * 2]) + "\" [" + er.f_query_variables[i * 2 + 1] + "]";
        CPPUNIT_ASSERT_MESSAGE(smsg, v != vars.end());
        CPPUNIT_ASSERT_MESSAGE(smsg, v->first == er.f_query_variables[i * 2]);
        CPPUNIT_ASSERT_MESSAGE(smsg, v->second == er.f_query_variables[i * 2 + 1]);
    }
    // check that the size of the vars is not larger than the f_query_variables
    CPPUNIT_ASSERT_MESSAGE(msg, vars.size() == i);

    // check a glob() call on each file, that allows us to make sure the glob()
    // function works as expected
    {
        std::string smsg = msg + " glob: \"" + er.f_glob + "\"";
        CPPUNIT_ASSERT_MESSAGE(smsg, fn.glob(er.f_glob));
    }

    CPPUNIT_ASSERT_MESSAGE(msg, fn.empty() == er.f_empty);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.is_deb() == er.f_is_deb);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.is_valid() == er.f_is_valid);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.is_direct() == er.f_is_direct);
    CPPUNIT_ASSERT_MESSAGE(msg, fn.is_absolute() == er.f_is_absolute);
}

URIFilenameUnitTests::expected_result empty =
{
    /* f_original_filename;         */ "",
    /* f_fixed_original_filename;   */ NULL,
    /* f_path_type;                 */ wpkg_filename::uri_filename::uri_type_undefined,
    /* f_path_scheme;               */ "",
    /* f_path_only;                 */ "",
    /* f_path_only_no_drive;        */ "",
    /* f_full_path;                 */ "",
    /* f_segments[32];              */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* f_dirname;                   */ "",
    /* f_dirname_no_drive;          */ "",
    /* f_basename;                  */ "",
    /* f_basename_last_only;        */ "",
    /* f_extension;                 */ "",
    /* f_previous_extension;        */ "",
    /* f_msdos_drive;               */ wpkg_filename::uri_filename::uri_no_msdos_drive,
    /* f_username;                  */ "",
    /* f_password;                  */ "",
    /* f_domain;                    */ "",
    /* f_port;                      */ "",
    /* f_share;                     */ "",
    /* f_decode;                    */ false,
    /* f_anchor;                    */ "",
    /* f_query_variables[32];       */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
    /* f_glob;                      */ "*",
    /* f_empty;                     */ true,
    /* f_is_deb;                    */ false,
    /* f_is_valid;                  */ false,
    /* f_is_direct;                 */ false,
    /* f_is_absolute;               */ false
};

void URIFilenameUnitTests::path()
{
    {
        wpkg_filename::uri_filename filename;
        check(filename, empty);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "/simple/path/.with./.double.extensions.tar.gz",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "/simple/path/.with./.double.extensions.tar.gz",
            /* f_path_only_no_drive;  */ "/simple/path/.with./.double.extensions.tar.gz",
            /* f_full_path;           */ "/simple/path/.with./.double.extensions.tar.gz",
            /* f_segments[32];        */ { "simple", "path", ".with.", ".double.extensions.tar.gz", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/simple/path/.with.",
            /* f_dirname_no_drive;    */ "/simple/path/.with.",
            /* f_basename;            */ ".double.extensions",
            /* f_basename_last_only;  */ ".double.extensions.tar",
            /* f_extension;           */ "gz",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ false,
            /* f_anchor;              */ "",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/simple/path/.[a-z]ith./.double.extensions.t?r.*",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "simple/relative/path/with/one-extension.tar",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "simple/relative/path/with/one-extension.tar",
            /* f_path_only_no_drive;  */ "simple/relative/path/with/one-extension.tar",
            /* f_full_path;           */ "simple/relative/path/with/one-extension.tar",
            /* f_segments[32];        */ { "simple", "relative", "path", "with", "one-extension.tar", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "simple/relative/path/with",
            /* f_dirname_no_drive;    */ "simple/relative/path/with",
            /* f_basename;            */ "one-extension",
            /* f_basename_last_only;  */ "one-extension",
            /* f_extension;           */ "tar",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ false,
            /* f_anchor;              */ "",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "simple/*/path/with/one-extension.tar",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ false
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "~/simple/relative/path/with/one-extension.tar",
            /* f_fixed_original_fi... */ "/home/wpkg/simple/relative/path/with/one-extension.tar",
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "/home/wpkg/simple/relative/path/with/one-extension.tar",
            /* f_path_only_no_drive;  */ "/home/wpkg/simple/relative/path/with/one-extension.tar",
            /* f_full_path;           */ "/home/wpkg/simple/relative/path/with/one-extension.tar",
            /* f_segments[32];        */ { "home", "wpkg", "simple", "relative", "path", "with", "one-extension.tar", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, },
            /* f_dirname;             */ "/home/wpkg/simple/relative/path/with",
            /* f_dirname_no_drive;    */ "/home/wpkg/simple/relative/path/with",
            /* f_basename;            */ "one-extension",
            /* f_basename_last_only;  */ "one-extension",
            /* f_extension;           */ "tar",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ false,
            /* f_anchor;              */ "",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/home/wpkg/simple/relative/[p][a][t][h]/with/one-*?.tar",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        putenv(const_cast<char *>("HOME=/home/wpkg"));
        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "~name/simple/relative/path/with/one-extension.tar",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "simple/relative/path/with/one-extension.tar",
            /* f_path_only_no_drive;  */ "simple/relative/path/with/one-extension.tar",
            /* f_full_path;           */ "simple/relative/path/with/one-extension.tar",
            /* f_segments[32];        */ { "simple", "relative", "path", "with", "one-extension.tar", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "simple/relative/path/with",
            /* f_dirname_no_drive;    */ "simple/relative/path/with",
            /* f_basename;            */ "one-extension",
            /* f_basename_last_only;  */ "one-extension",
            /* f_extension;           */ "tar",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ false,
            /* f_anchor;              */ "",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "~name/simple/relative/p?t?/wi?h?one-extension.tar",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ false
        };

        try
        {
            wpkg_filename::uri_filename filename(result.f_original_filename);
            CPPUNIT_ASSERT(!"~name/... did not generate an exception");
        }
        catch(const wpkg_filename::wpkg_filename_exception_parameter&)
        {
        }
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "~/simple/relative/path/with/one-extension.tar",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "simple/relative/path/with/one-extension.tar",
            /* f_path_only_no_drive;  */ "simple/relative/path/with/one-extension.tar",
            /* f_full_path;           */ "simple/relative/path/with/one-extension.tar",
            /* f_segments[32];        */ { "simple", "relative", "path", "with", "one-extension.tar", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "simple/relative/path/with",
            /* f_dirname_no_drive;    */ "simple/relative/path/with",
            /* f_basename;            */ "one-extension",
            /* f_basename_last_only;  */ "one-extension",
            /* f_extension;           */ "tar",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ false,
            /* f_anchor;              */ "",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "simple/rel*ative/path/with/one-extension.tar",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ false
        };

        try
        {
            putenv(const_cast<char *>("HOME=~/test"));
            wpkg_filename::uri_filename filename(result.f_original_filename);
            CPPUNIT_ASSERT(!"HOME=~/test did not generate an exception");
        }
        catch(const wpkg_filename::wpkg_filename_exception_parameter&)
        {
        }
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "~/simple/relative/path/with/one-extension.tar",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "simple/relative/path/with/one-extension.tar",
            /* f_path_only_no_drive;  */ "simple/relative/path/with/one-extension.tar",
            /* f_full_path;           */ "simple/relative/path/with/one-extension.tar",
            /* f_segments[32];        */ { "simple", "relative", "path", "with", "one-extension.tar", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "simple/relative/path/with",
            /* f_dirname_no_drive;    */ "simple/relative/path/with",
            /* f_basename;            */ "one-extension",
            /* f_basename_last_only;  */ "one-extension",
            /* f_extension;           */ "tar",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ false,
            /* f_anchor;              */ "",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "simple/re?*?ve/path/with/one-extension.tar",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ false
        };

        try
        {
            putenv(const_cast<char *>("HOME=not/absolute"));
            wpkg_filename::uri_filename filename(result.f_original_filename);
            CPPUNIT_ASSERT(!"HOME=not/absolute did not generate an exception");
        }
        catch(const wpkg_filename::wpkg_filename_exception_parameter&)
        {
        }
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "File://127.0.0.1/simple/full/path/with/one-extension.tar#test",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "/simple/full/path/with/one-extension.tar",
            /* f_path_only_no_drive;  */ "/simple/full/path/with/one-extension.tar",
            /* f_full_path;           */ "/simple/full/path/with/one-extension.tar",
            /* f_segments[32];        */ { "simple", "full", "path", "with", "one-extension.tar", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/simple/full/path/with",
            /* f_dirname_no_drive;    */ "/simple/full/path/with",
            /* f_basename;            */ "one-extension",
            /* f_basename_last_only;  */ "one-extension",
            /* f_extension;           */ "tar",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/simple/full/*.tar",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "File:///k:/simple/full/path/with/one-extension.tar#test",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "K:/simple/full/path/with/one-extension.tar",
            /* f_path_only_no_drive;  */ "/simple/full/path/with/one-extension.tar",
            /* f_full_path;           */ "K:/simple/full/path/with/one-extension.tar",
            /* f_segments[32];        */ { "simple", "full", "path", "with", "one-extension.tar", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "K:/simple/full/path/with",
            /* f_dirname_no_drive;    */ "/simple/full/path/with",
            /* f_basename;            */ "one-extension",
            /* f_basename_last_only;  */ "one-extension",
            /* f_extension;           */ "tar",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ 'K',
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/simple/full/*.tar",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "File:///k%3a/simple/full/path/with/one-extension.tar#test",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "K:/simple/full/path/with/one-extension.tar",
            /* f_path_only_no_drive;  */ "/simple/full/path/with/one-extension.tar",
            /* f_full_path;           */ "K:/simple/full/path/with/one-extension.tar",
            /* f_segments[32];        */ { "simple", "full", "path", "with", "one-extension.tar", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "K:/simple/full/path/with",
            /* f_dirname_no_drive;    */ "/simple/full/path/with",
            /* f_basename;            */ "one-extension",
            /* f_basename_last_only;  */ "one-extension",
            /* f_extension;           */ "tar",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ 'K',
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/simple/full/*.tar",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "File://localhost/c|/simple/full/path/with/one-extension.tar#test",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "C:/simple/full/path/with/one-extension.tar",
            /* f_path_only_no_drive;  */ "/simple/full/path/with/one-extension.tar",
            /* f_full_path;           */ "C:/simple/full/path/with/one-extension.tar",
            /* f_segments[32];        */ { "simple", "full", "path", "with", "one-extension.tar", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "C:/simple/full/path/with",
            /* f_dirname_no_drive;    */ "/simple/full/path/with",
            /* f_basename;            */ "one-extension",
            /* f_basename_last_only;  */ "one-extension",
            /* f_extension;           */ "tar",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ 'C',
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/simple/full/*.tar",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "File://localhost/%43%7C/simple/full/path/with/one-extension.tar#test",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "C:/simple/full/path/with/one-extension.tar",
            /* f_path_only_no_drive;  */ "/simple/full/path/with/one-extension.tar",
            /* f_full_path;           */ "C:/simple/full/path/with/one-extension.tar",
            /* f_segments[32];        */ { "simple", "full", "path", "with", "one-extension.tar", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "C:/simple/full/path/with",
            /* f_dirname_no_drive;    */ "/simple/full/path/with",
            /* f_basename;            */ "one-extension",
            /* f_basename_last_only;  */ "one-extension",
            /* f_extension;           */ "tar",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ 'C',
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/simple/full/*.tar",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "File://127.000.000.001/%43%7C/simple+full+path/with/one%2dextension.tar%23test?special=encoding+of+hash+with+%23",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "C:/simple full path/with/one-extension.tar#test",
            /* f_path_only_no_drive;  */ "/simple full path/with/one-extension.tar#test",
            /* f_full_path;           */ "C:/simple full path/with/one-extension.tar#test",
            /* f_segments[32];        */ { "simple full path", "with", "one-extension.tar#test", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "C:/simple full path/with",
            /* f_dirname_no_drive;    */ "/simple full path/with",
            /* f_basename;            */ "one-extension",
            /* f_basename_last_only;  */ "one-extension",
            /* f_extension;           */ "tar#test",
            /* f_previous_extension;  */ "tar#test",
            /* f_msdos_drive;         */ 'C',
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ true,
            /* f_anchor;              */ "",
            /* f_query_variables[32]; */ { "special", "encoding+of+hash+with+%23", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/simple full *.tar#test",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "File://www.m2osw.com/simple/full/path/with/one-extension.tar#test",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "/simple/full/path/with/one-extension.tar",
            /* f_path_only_no_drive;  */ "/simple/full/path/with/one-extension.tar",
            /* f_full_path;           */ "/simple/full/path/with/one-extension.tar",
            /* f_segments[32];        */ { "simple", "full", "path", "with", "one-extension.tar", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/simple/full/path/with",
            /* f_dirname_no_drive;    */ "/simple/full/path/with",
            /* f_basename;            */ "one-extension",
            /* f_basename_last_only;  */ "one-extension",
            /* f_extension;           */ "tar",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "www.m2osw.com",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/simple/full/*.tar",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "File://alexis:secret@www.m2osw.com:123/simple/full/path/with/one-extension.tar?position=line&EOF=^Z#test",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "/simple/full/path/with/one-extension.tar",
            /* f_path_only_no_drive;  */ "/simple/full/path/with/one-extension.tar",
            /* f_full_path;           */ "/simple/full/path/with/one-extension.tar",
            /* f_segments[32];        */ { "simple", "full", "path", "with", "one-extension.tar", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/simple/full/path/with",
            /* f_dirname_no_drive;    */ "/simple/full/path/with",
            /* f_basename;            */ "one-extension",
            /* f_basename_last_only;  */ "one-extension",
            /* f_extension;           */ "tar",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "alexis",
            /* f_password;            */ "secret",
            /* f_domain;              */ "www.m2osw.com",
            /* f_port;                */ "123",
            /* f_share;               */ "",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { "EOF", "^Z", "position", "line", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/simple/full/*.tar",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "HTTP://alexis:secret@www.m2osw.com:888/some/path//to/filename.tar.zip?#here",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "http",
            /* f_path_only;           */ "/some/path/to/filename.tar.zip",
            /* f_path_only_no_drive;  */ "/some/path/to/filename.tar.zip",
            /* f_full_path;           */ "http://alexis:secret@www.m2osw.com:888/some/path/to/filename.tar.zip#here",
            /* f_segments[32];        */ { "some", "path", "to", "filename.tar.zip", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/some/path/to",
            /* f_dirname_no_drive;    */ "/some/path/to",
            /* f_basename;            */ "filename.tar",
            /* f_basename_last_only;  */ "filename.tar",
            /* f_extension;           */ "zip",
            /* f_previous_extension;  */ "zip", // zip is not expected to be used with .tar!
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "alexis",
            /* f_password;            */ "secret",
            /* f_domain;              */ "www.m2osw.com",
            /* f_port;                */ "888",
            /* f_share;               */ "",
            /* f_decode;              */ true,
            /* f_anchor;              */ "here",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/some/path/to/*",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ false,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "HTTP://alexis:top%2Dsecret@www.m2osw.com:848/some+path//to/filename.tar.zip?#here",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "http",
            /* f_path_only;           */ "/some path/to/filename.tar.zip",
            /* f_path_only_no_drive;  */ "/some path/to/filename.tar.zip",
            /* f_full_path;           */ "http://alexis:top%2Dsecret@www.m2osw.com:848/some%20path/to/filename.tar.zip#here",
            /* f_segments[32];        */ { "some path", "to", "filename.tar.zip", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/some path/to",
            /* f_dirname_no_drive;    */ "/some path/to",
            /* f_basename;            */ "filename.tar",
            /* f_basename_last_only;  */ "filename.tar",
            /* f_extension;           */ "zip",
            /* f_previous_extension;  */ "zip", // zip is not expected to be used with .tar!
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "alexis",
            /* f_password;            */ "top%2Dsecret",
            /* f_domain;              */ "www.m2osw.com",
            /* f_port;                */ "848",
            /* f_share;               */ "",
            /* f_decode;              */ true,
            /* f_anchor;              */ "here",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/some path/to/*",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ false,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "HTTP://alexis:top%2Dsecret@www.m2osw.com:878/some+path%3F//to/filename.tar.zip%3F%23here",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "http",
            /* f_path_only;           */ "/some path?/to/filename.tar.zip?#here",
            /* f_path_only_no_drive;  */ "/some path?/to/filename.tar.zip?#here",
            /* f_full_path;           */ "http://alexis:top%2Dsecret@www.m2osw.com:878/some%20path%3F/to/filename.tar.zip%3F%23here",
            /* f_segments[32];        */ { "some path?", "to", "filename.tar.zip?#here", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/some path?/to",
            /* f_dirname_no_drive;    */ "/some path?/to",
            /* f_basename;            */ "filename.tar",
            /* f_basename_last_only;  */ "filename.tar",
            /* f_extension;           */ "zip?#here",
            /* f_previous_extension;  */ "zip?#here",
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "alexis",
            /* f_password;            */ "top%2Dsecret",
            /* f_domain;              */ "www.m2osw.com",
            /* f_port;                */ "878",
            /* f_share;               */ "",
            /* f_decode;              */ true,
            /* f_anchor;              */ "",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/some path?/to/*",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ false,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "smb://alexis:secret@simple:123/share/full/path/filename.tar.zip?var=555#test",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "smb",
            /* f_path_only;           */ "/full/path/filename.tar.zip",
            /* f_path_only_no_drive;  */ "/full/path/filename.tar.zip",
            /* f_full_path;           */ "smb://alexis:secret@simple:123/share/full/path/filename.tar.zip?var=555#test",
            /* f_segments[32];        */ { "full", "path", "filename.tar.zip", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/full/path",
            /* f_dirname_no_drive;    */ "/full/path",
            /* f_basename;            */ "filename.tar",
            /* f_basename_last_only;  */ "filename.tar",
            /* f_extension;           */ "zip",
            /* f_previous_extension;  */ "zip", // zip is not expected to be used with .tar!
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "alexis",
            /* f_password;            */ "secret",
            /* f_domain;              */ "simple",
            /* f_port;                */ "123",
            /* f_share;               */ "share",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { "var", "555", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/full/path/filename.tar.zip",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "netbios://alexis:secret@simple:123/share/full/path/filename.tar.zip?var=555&home=/#test",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "smb",
            /* f_path_only;           */ "/full/path/filename.tar.zip",
            /* f_path_only_no_drive;  */ "/full/path/filename.tar.zip",
            /* f_full_path;           */ "smb://alexis:secret@simple:123/share/full/path/filename.tar.zip?home=/&var=555#test",
            /* f_segments[32];        */ { "full", "path", "filename.tar.zip", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/full/path",
            /* f_dirname_no_drive;    */ "/full/path",
            /* f_basename;            */ "filename.tar",
            /* f_basename_last_only;  */ "filename.tar",
            /* f_extension;           */ "zip",
            /* f_previous_extension;  */ "zip", // zip is not expected to be used with .tar!
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "alexis",
            /* f_password;            */ "secret",
            /* f_domain;              */ "simple",
            /* f_port;                */ "123",
            /* f_share;               */ "share",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { "home", "/", "var", "555", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/f?ll/p?th/f?lename.tar.zip",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "nbs://alexis@simple:123/share/full/path/filename.tar.zip?var=555#test",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "smbs",
            /* f_path_only;           */ "/full/path/filename.tar.zip",
            /* f_path_only_no_drive;  */ "/full/path/filename.tar.zip",
            /* f_full_path;           */ "smbs://alexis@simple:123/share/full/path/filename.tar.zip?var=555#test",
            /* f_segments[32];        */ { "full", "path", "filename.tar.zip", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/full/path",
            /* f_dirname_no_drive;    */ "/full/path",
            /* f_basename;            */ "filename.tar",
            /* f_basename_last_only;  */ "filename.tar",
            /* f_extension;           */ "zip",
            /* f_previous_extension;  */ "zip", // zip is not expected to be used with .tar!
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "alexis",
            /* f_password;            */ "secret",
            /* f_domain;              */ "simple",
            /* f_port;                */ "123",
            /* f_share;               */ "share",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { "var", "555", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "*/filename.tar.zip*",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        try
        {
            wpkg_filename::uri_filename filename(result.f_original_filename);
            CPPUNIT_ASSERT(!"netbios with the username or password missing");
        }
        catch(const wpkg_filename::wpkg_filename_exception_parameter&)
        {
        }
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "nbs://:password@simple:123/share/full/path\\filename.tar.zip?var=555#test",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "smbs",
            /* f_path_only;           */ "/full/path/filename.tar.zip",
            /* f_path_only_no_drive;  */ "/full/path/filename.tar.zip",
            /* f_full_path;           */ "smbs://alexis@simple:123/share/full/path/filename.tar.zip?var=555#test",
            /* f_segments[32];        */ { "full", "path", "filename.tar.zip", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/full/path",
            /* f_dirname_no_drive;    */ "/full/path",
            /* f_basename;            */ "filename.tar",
            /* f_basename_last_only;  */ "filename.tar",
            /* f_extension;           */ "zip",
            /* f_previous_extension;  */ "zip", // zip is not expected to be used with .tar!
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "alexis",
            /* f_password;            */ "secret",
            /* f_domain;              */ "simple",
            /* f_port;                */ "123",
            /* f_share;               */ "share",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { "var", "555", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "*/share/full/path\\filename.tar.zip",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        try
        {
            wpkg_filename::uri_filename filename(result.f_original_filename);
            CPPUNIT_ASSERT(!"netbios with the username or password missing");
        }
        catch(const wpkg_filename::wpkg_filename_exception_parameter&)
        {
        }
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "nbs://simple/share/full/path/filename.tar.zip?v r=555#test",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "smbs",
            /* f_path_only;           */ "/full/path/filename.tar.zip",
            /* f_path_only_no_drive;  */ "/full/path/filename.tar.zip",
            /* f_full_path;           */ "smbs://alexis@simple:123/share/full/path/filename.tar.zip?var=555#test",
            /* f_segments[32];        */ { "full", "path", "filename.tar.zip", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/full/path",
            /* f_dirname_no_drive;    */ "/full/path",
            /* f_basename;            */ "filename.tar",
            /* f_basename_last_only;  */ "filename.tar",
            /* f_extension;           */ "zip",
            /* f_previous_extension;  */ "zip", // zip is not expected to be used with .tar!
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "alexis",
            /* f_password;            */ "secret",
            /* f_domain;              */ "simple",
            /* f_port;                */ "123",
            /* f_share;               */ "share",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { "var", "555", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "*path*",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        try
        {
            wpkg_filename::uri_filename filename(result.f_original_filename);
            CPPUNIT_ASSERT(!"space in variable name");
        }
        catch(const wpkg_filename::wpkg_filename_exception_parameter&)
        {
        }
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "nbs://alexis:@simple:123/share/full/path/filename.tar.zip?var=555#test",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "smbs",
            /* f_path_only;           */ "/full/path/filename.tar.zip",
            /* f_path_only_no_drive;  */ "/full/path/filename.tar.zip",
            /* f_full_path;           */ "smbs://alexis@simple:123/share/full/path/filename.tar.zip?var=555#test",
            /* f_segments[32];        */ { "full", "path", "filename.tar.zip", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/full/path",
            /* f_dirname_no_drive;    */ "/full/path",
            /* f_basename;            */ "filename.tar",
            /* f_basename_last_only;  */ "filename.tar",
            /* f_extension;           */ "zip",
            /* f_previous_extension;  */ "zip", // zip is not expected to be used with .tar!
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "alexis",
            /* f_password;            */ "secret",
            /* f_domain;              */ "simple",
            /* f_port;                */ "123",
            /* f_share;               */ "share",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { "var", "555", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "nbs://\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?\?/share/full/path/filename.tar.zip?var=555#test",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        try
        {
            wpkg_filename::uri_filename filename(result.f_original_filename);
            CPPUNIT_ASSERT(!"netbios with the username or password missing");
        }
        catch(const wpkg_filename::wpkg_filename_exception_parameter&)
        {
        }
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "nbs://simple/",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "smbs",
            /* f_path_only;           */ "/full/path/filename.tar.zip",
            /* f_path_only_no_drive;  */ "/full/path/filename.tar.zip",
            /* f_full_path;           */ "smbs://alexis:secret@simple:123/share/full/path/filename.tar.zip?var=555#test",
            /* f_segments[32];        */ { "full", "path", "filename.tar.zip", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/full/path",
            /* f_dirname_no_drive;    */ "/full/path",
            /* f_basename;            */ "filename.tar",
            /* f_basename_last_only;  */ "filename.tar",
            /* f_extension;           */ "zip",
            /* f_previous_extension;  */ "zip", // zip is not expected to be used with .tar!
            /* f_msdos_drive;         */ wpkg_filename::uri_filename::uri_no_msdos_drive,
            /* f_username;            */ "alexis",
            /* f_password;            */ "secret",
            /* f_domain;              */ "simple",
            /* f_port;                */ "123",
            /* f_share;               */ "share",
            /* f_decode;              */ true,
            /* f_anchor;              */ "test",
            /* f_query_variables[32]; */ { "var", "555", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "[A-Za-z]bs://simple/",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        try
        {
            wpkg_filename::uri_filename filename(result.f_original_filename);
            CPPUNIT_ASSERT(!"netbios path without shared name");
        }
        catch(const wpkg_filename::wpkg_filename_exception_parameter&)
        {
        }
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "c:\\simple\\full\\path\\with\\bzip2-extension.tar.bz2",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "C:/simple/full/path/with/bzip2-extension.tar.bz2",
            /* f_path_only_no_drive;  */ "/simple/full/path/with/bzip2-extension.tar.bz2",
            /* f_full_path;           */ "C:/simple/full/path/with/bzip2-extension.tar.bz2",
            /* f_segments[32];        */ { "simple", "full", "path", "with", "bzip2-extension.tar.bz2", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "C:/simple/full/path/with",
            /* f_dirname_no_drive;    */ "/simple/full/path/with",
            /* f_basename;            */ "bzip2-extension",
            /* f_basename_last_only;  */ "bzip2-extension.tar",
            /* f_extension;           */ "bz2",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ 'C',
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ false,
            /* f_anchor;              */ "",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "/*.bz2",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "J:\\windows\\\\path\\and///Unix/too.tar.bz2",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "J:/windows/path/and/Unix/too.tar.bz2",
            /* f_path_only_no_drive;  */ "/windows/path/and/Unix/too.tar.bz2",
            /* f_full_path;           */ "J:/windows/path/and/Unix/too.tar.bz2",
            /* f_segments[32];        */ { "windows", "path", "and", "Unix", "too.tar.bz2", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "J:/windows/path/and/Unix",
            /* f_dirname_no_drive;    */ "/windows/path/and/Unix",
            /* f_basename;            */ "too",
            /* f_basename_last_only;  */ "too.tar",
            /* f_extension;           */ "bz2",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ 'J',
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ false,
            /* f_anchor;              */ "",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "\\windows\\*\\and/Unix/too.tar.bz2",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ true
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "k:non-absolute\\windows\\\\path\\and///UNIX/too.tar.bz2",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "K:non-absolute/windows/path/and/UNIX/too.tar.bz2",
            /* f_path_only_no_drive;  */ "non-absolute/windows/path/and/UNIX/too.tar.bz2",
            /* f_full_path;           */ "K:non-absolute/windows/path/and/UNIX/too.tar.bz2",
            /* f_segments[32];        */ { "non-absolute", "windows", "path", "and", "UNIX", "too.tar.bz2", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "K:non-absolute/windows/path/and/UNIX",
            /* f_dirname_no_drive;    */ "non-absolute/windows/path/and/UNIX",
            /* f_basename;            */ "too",
            /* f_basename_last_only;  */ "too.tar",
            /* f_extension;           */ "bz2",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ 'K',
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ false,
            /* f_anchor;              */ "",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "non-*\\windows/path\\and/*\\too.tar.bz2",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ false
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "z:non-absolute\\wind:ows\\\\path\\and///UNIX/*.tar.bz2",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "Z:non-absolute/wind:ows/path/and/UNIX/*.tar.bz2",
            /* f_path_only_no_drive;  */ "non-absolute/wind:ows/path/and/UNIX/*.tar.bz2",
            /* f_full_path;           */ "Z:non-absolute/wind:ows/path/and/UNIX/*.tar.bz2",
            /* f_segments[32];        */ { "non-absolute", "wind:ows", "path", "and", "UNIX", "*.tar.bz2", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "Z:non-absolute/wind:ows/path/and/UNIX",
            /* f_dirname_no_drive;    */ "non-absolute/wind:ows/path/and/UNIX",
            /* f_basename;            */ "*",
            /* f_basename_last_only;  */ "*.tar",
            /* f_extension;           */ "bz2",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ 'Z',
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ false,
            /* f_anchor;              */ "",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "non?absolute\\wind?ows\\path[ -z]and/U?IX/*.tar.bz2",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ false
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

    {
        expected_result result =
        {
            /* f_original_filename;   */ "f:non-absolute\\wind:ows\\\\path\\and///UNIX/*.tar.bz2",
            /* f_fixed_original_fi... */ NULL,
            /* f_path_type;           */ wpkg_filename::uri_filename::uri_type_direct,
            /* f_path_scheme;         */ "file",
            /* f_path_only;           */ "/opt/wpkg/m2osw/packages/non-absolute/wind:ows/path/and/UNIX/*.tar.bz2",
            /* f_path_only_no_drive;  */ "non-absolute/wind:ows/path/and/UNIX/*.tar.bz2",
            /* f_full_path;           */ "/opt/wpkg/m2osw/packages/non-absolute/wind:ows/path/and/UNIX/*.tar.bz2",
            /* f_segments[32];        */ { "non-absolute", "wind:ows", "path", "and", "UNIX", "*.tar.bz2", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_dirname;             */ "/opt/wpkg/m2osw/packages/non-absolute/wind:ows/path/and/UNIX",
            /* f_dirname_no_drive;    */ "non-absolute/wind:ows/path/and/UNIX",
            /* f_basename;            */ "*",
            /* f_basename_last_only;  */ "*.tar",
            /* f_extension;           */ "bz2",
            /* f_previous_extension;  */ "tar",
            /* f_msdos_drive;         */ 'F',
            /* f_username;            */ "",
            /* f_password;            */ "",
            /* f_domain;              */ "",
            /* f_port;                */ "",
            /* f_share;               */ "",
            /* f_decode;              */ false,
            /* f_anchor;              */ "",
            /* f_query_variables[32]; */ { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
            /* f_glob;                */ "non?absolute\\wind?ows\\path[ -z]and/U?IX/*.tar.bz2",
            /* f_empty;               */ false,
            /* f_is_deb;              */ false,
            /* f_is_valid;            */ true,
            /* f_is_direct;           */ true,
            /* f_is_absolute;         */ false
        };

        wpkg_filename::uri_filename filename(result.f_original_filename);
        check(filename, result);
    }

}


namespace
{
    const char *bad_filenames[] =
    {
        "/invalid/COM1/filename",
        "/invalid/com2/filename",
        "/invalid/Com3/filename",
        "/invalid/cOm4/filename",
        "/invalid/cOM5/filename",
        "/invalid/COm6/filename",
        "/invalid/CoM7/filename",
        "/invalid/coM8/filename",
        "/invalid/COM9/filename",
        "/invalid/LPT1/filename",
        "/invalid/lpt2/filename",
        "/invalid/Lpt3/filename",
        "/invalid/lPt4/filename",
        "/invalid/lPT5/filename",
        "/invalid/LPt6/filename",
        "/invalid/LpT7/filename",
        "/invalid/lpT8/filename",
        "/invalid/LPT9/filename",
        "/Aux/filename",
        "cOn",
        "/bad/NUL",
        "bad/NUL",
        "NUL/test",
        "/prn/"
    };

    // Note: The : * and ? characters are accepted as versions need :
    //       on unices and patterns use * and ? here and there
    const char *bad_characters[] =
    {
        "/really/invalid/file\"name",
        "/really/invalid/file<name",
        "/really/invalid/file>name",
        "/really/invalid/file|name",

        "/in-valid/file\"name",
        "/in-valid/file<name",
        "/in-valid/file>name",
        "/in-valid/file|name",

        "/invalid_/file\"name/here",
        "/invalid_/file<name/here/too",
        "/invalid_/file>name/",
        "/invalid_/file|name/here",

        "file\"name",
        "file<name",
        "file>name",
        "file|name",

        " filename",
        "filename ",
        " filename ",

        "/file\"name",
        "/file<name",
        "/file>name",
        "/file|name",

        "file\"name/file*name",
        "file<name/plus",
        "file>name/minus",
        "file|name/more/path"
    };
}

void URIFilenameUnitTests::invalid_ms_paths()
{
    for(size_t i(0); i < sizeof(bad_filenames) / sizeof(bad_filenames[0]); ++i)
    {
        try
        {
            wpkg_filename::uri_filename filename(bad_filenames[i]);
            CPPUNIT_ASSERT_MESSAGE(bad_filenames[i], !"invalid MS-Windows filename accepted");
        }
        catch(const wpkg_filename::wpkg_filename_exception_parameter&)
        {
        }
    }

    for(size_t i(0); i < sizeof(bad_characters) / sizeof(bad_characters[0]); ++i)
    {
        try
        {
            wpkg_filename::uri_filename filename(bad_characters[i]);
            CPPUNIT_ASSERT_MESSAGE(bad_characters[i], !"invalid MS-Windows character accepted");
        }
        catch(const wpkg_filename::wpkg_filename_exception_parameter&)
        {
        }
    }
}



namespace
{
    const char *bad_uri[][2] =
    {
        { "http://www.m2osw.com/bad/var?=555", "a URI query string variable name cannot be empty in" },
        { "http://www.m2osw.com/bad/var?this one=555", "a URI query string variable name cannot include a space in" },
        { "~username/not-legal", "tilde + username is not supported; '~/' was expected at the start of your filename" },
        { "smb://domain.only.is.not.enough", "smb paths require at least the share name not found in" },
        { "smb://domain.only.is.not.enough/", "smb paths require at least the share name not found in" },
        { "http://alexis:@www.m2osw.com/", "when specifying a username and password, both must be valid (not empty)" },
        { "http://:topsecret@www.m2osw.com/", "when specifying a username and password, both must be valid (not empty)" },
        { "http://www.m2osw.com:123x/bad/port", "a port in a URI must exclusively be composed of digits. \"123x\" is not valid!" }
    };
}


void URIFilenameUnitTests::invalid_uri()
{
    const char *home = getenv("HOME");
    putenv(const_cast<char *>("HOME=/home/wpkg"));
    for(size_t i(0); i < sizeof(bad_uri) / sizeof(bad_uri[0]); ++i)
    {
        try
        {
            wpkg_filename::uri_filename filename(bad_uri[i][0]);
            CPPUNIT_ASSERT_MESSAGE(bad_uri[i][0], !"invalid URI filename accepted");
        }
        catch(const wpkg_filename::wpkg_filename_exception_parameter& e)
        {
            CPPUNIT_ASSERT_MESSAGE(std::string("got \"") + e.what() + "\", expected \"" + bad_uri[i][1] + "\"", std::string(e.what()).substr(0, strlen(bad_uri[i][1])) == bad_uri[i][1]);
        }
    }
    putenv(const_cast<char *>("HOME=~/bad/home"));
    try
    {
        wpkg_filename::uri_filename filename("~/fail/because/of/home");
        CPPUNIT_ASSERT_MESSAGE("~/fail/because/of/home", !"invalid URI filename accepted");
    }
    catch(const wpkg_filename::wpkg_filename_exception_parameter& e)
    {
        CPPUNIT_ASSERT_MESSAGE(std::string("got \"") + e.what() + "\", expected \"$HOME path cannot itself start with a tilde (~).\"", std::string(e.what()) == "$HOME path cannot itself start with a tilde (~).");
    }
    putenv(const_cast<char *>("HOME=bad/home"));
    try
    {
        wpkg_filename::uri_filename filename("~/fail/because/of/home");
        CPPUNIT_ASSERT_MESSAGE("~/fail/because/of/home", !"invalid URI filename accepted");
    }
    catch(const wpkg_filename::wpkg_filename_exception_parameter& e)
    {
        CPPUNIT_ASSERT_MESSAGE(std::string("got \"") + e.what() + "\", expected \"$HOME path is not absolute; we cannot safely replace the ~ character.\"", std::string(e.what()) == "$HOME path is not absolute; we cannot safely replace the ~ character.");
    }
    putenv(const_cast<char *>(home));
}


namespace
{
    const char *common_segment_samples[][4] =
    {
        {
            "test/common/segments/removal",
            "test/common/segment/removal",
            "segments/removal",
            "segment/removal"
        },
        {
            "c:test/common/segments/removal",
            "c:test/common/segment/removal",
            "segments/removal",
            "segment/removal"
        },
        {
            "c:test/common/segments/removal",
            "d:test/common/segment/removal",
            "C:test/common/segments/removal",
            "D:test/common/segment/removal"
        },
        {
            "/test/common/segments/removal",
            "/test/common/segment/removal",
            "segments/removal",
            "segment/removal"
        },
        {
            "c:/test/common/segments/removal",
            "c:/test/common/segment/removal",
            "segments/removal",
            "segment/removal"
        },
        {
            "c:/test/common/segments/removal",
            "d:/test/common/segment/removal",
            "C:/test/common/segments/removal",
            "D:/test/common/segment/removal"
        },
        {
            "c:test/common/segments/removal",
            "d:test/common/segment/removal",
            "C:test/common/segments/removal",
            "D:test/common/segment/removal"
        },
        {
            "c:/test/common/segments/removal",
            "c:/against/common/segment/removal",
            "/test/common/segments/removal",
            "/against/common/segment/removal"
        },
        {
            "http://www.m2osw.com:80/test/common/segments/removal",
            "http://www.m2osw.com/test/common/segment/removal",
            "segments/removal",
            "segment/removal"
        },
        {
            "http://www.m2osw.com:8800/test/common/segments/removal",
            "http://www.m2osw.com:8800/test/common/segment/removal",
            "segments/removal",
            "segment/removal"
        },
        {
            "http://alexis:secret@www.m2osw.com/test/common/segments/removal",
            "http://alexis:secret@www.m2osw.com:80/test/common/segment/removal",
            "segments/removal",
            "segment/removal"
        },
        {
            "http://alexis:secret@www.m2osw.com:8080/test/common/segments/removal",
            "http://alexis:secret@www.m2osw.com:8080/test/common/segment/removal",
            "segments/removal",
            "segment/removal"
        },
        {
            "http://www.m2osw.com/test/common/segments/removal",
            "https://www.m2osw.com/test/common/segment/removal",
            "http://www.m2osw.com/test/common/segments/removal",
            "https://www.m2osw.com/test/common/segment/removal"
        },
        {
            "http://www.m2osw.com/test/common/segments/removal",
            "http://ww2.m2osw.com:80/test/common/segment/removal",
            "http://www.m2osw.com/test/common/segments/removal",
            "http://ww2.m2osw.com/test/common/segment/removal"
        },
        {
            "http://www.m2osw.com:80/test/common/segments/removal",
            "http://www.m2osw.com/test/common/segment/removal",
            "segments/removal",
            "segment/removal"
        },
        {
            "http://alexis:secret@www.m2osw.com/test/common/segments/removal",
            "http://alexis:secretz@www.m2osw.com/test/common/segment/removal",
            "http://alexis:secret@www.m2osw.com/test/common/segments/removal",
            "http://alexis:secretz@www.m2osw.com/test/common/segment/removal"
        },
        {
            "http://alexis:secret@www.m2osw.com/test/common/segments/removal",
            "http://alexif:secret@www.m2osw.com:80/test/common/segment/removal",
            "http://alexis:secret@www.m2osw.com/test/common/segments/removal",
            "http://alexif:secret@www.m2osw.com/test/common/segment/removal"
        }
    };
}

void URIFilenameUnitTests::common_segments()
{
    for(size_t i(0); i < sizeof(common_segment_samples) / sizeof(common_segment_samples[0]); ++i)
    {
        wpkg_filename::uri_filename a(common_segment_samples[i][0]);
        wpkg_filename::uri_filename b(common_segment_samples[i][1]);
        wpkg_filename::uri_filename c(a.remove_common_segments(b));
        CPPUNIT_ASSERT_MESSAGE("got: \"" + c.full_path() + "\", expected: \"" + common_segment_samples[i][2] + "\" from \"" + common_segment_samples[i][0] + "\" and \"" + common_segment_samples[i][1] + "\"", c.full_path() == common_segment_samples[i][2]);
        wpkg_filename::uri_filename d(b.remove_common_segments(a));
        CPPUNIT_ASSERT_MESSAGE("got: \"" + d.full_path() + "\", expected: \"" + common_segment_samples[i][3] + "\" from \"" + common_segment_samples[i][0] + "\" and \"" + common_segment_samples[i][1] + "\"", d.full_path() == common_segment_samples[i][3]);
    }
}


/** \brief Generate a random filename.
 *
 * This function generates a long random filename composed of digits
 * and ASCII letters. The result is expected to be 100% compatible
 * with all operating systems (MS-Windows has a few special cases but
 * these are very short names.)
 *
 * The result of the function can immediately be used as a filename
 * although it is expected to be used in a sub-directory (i.e. the
 * function does not generate a sub-directory path.)
 *
 * The maximum \p limit is 136 because 135 + 120 = 255 which is the
 * maximum filename on ext[234] and NTFS. This will definitively
 * fail on a direct FAT32 file system, although with MS-Windows it
 * should still work.
 *
 * \param[in] limit  The variable limit (we add 120 to that value).
 *
 * \return The randomly generated long filename.
 */
std::string generate_uri_filename(int limit)
{
    std::string filename;
    const int filename_length(rand() % limit + 1);
    for(int i(0); i < filename_length; ++i)
    {
        // we're not testing special characters or anything like that
        // so just digits and ASCII letters are used
        char c(rand() % 63);
        if(c < 10)
        {
            c += '0';
        }
        else if(c < 36)
        {
            c += 'A' - 10;
        }
        else if(c < 62)
        {
            c += 'a' - 36;
        }
        else
        {
            c = '_';
        }
        filename += c;
    }

//fprintf(stderr, "ln %3ld [%s]\n", filename.length(), filename.c_str());
    return filename;
}

void URIFilenameUnitTests::long_filename()
{
    // define a long filename
    //
    // note that we're not trying to reach the exact limit, just close to
    // it so that way we can test a file in the last directory to prove
    // that works too
#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
    const int max_size(32700); // The docs say about 32Kb...
#else
    const int max_size(4000);
#endif

    // make sure that the temporary directory is not empty, may be relative
    if(unittest::tmp_dir.empty())
    {
        fprintf(stderr, "\nerror:unittest_uri_filename: a temporary directory is required to run the long_filename() unit test.\n");
        throw std::runtime_error("--tmp <directory> missing");
    }

    wpkg_filename::uri_filename real_tmpdir(unittest::tmp_dir);
    real_tmpdir.os_unlink_rf(); // clean up the existing mess before we start our test
    real_tmpdir.os_mkdir_p();
    real_tmpdir = real_tmpdir.os_real_path();
    const int offset(static_cast<int>(real_tmpdir.full_path().length() - unittest::tmp_dir.length() + 10));

    const int count(5);
    for(int i(0); i < count; ++i)
    {
        wpkg_filename::uri_filename filename(unittest::tmp_dir);
        for(;;)
        {
            const int size(static_cast<int>(filename.full_path().length()));
            int limit(max_size - size - offset);
            if(limit > 254)
            {
                limit = 254;
            }
            else if(limit < 10)
            {
                break;
            }
            const std::string name(generate_uri_filename(limit));
            filename = filename.append_child(name);
            filename.os_mkdir_p();

            // test a few things as we're at it
            wpkg_filename::uri_filename real(filename.os_real_path());
            // I'm not too sure we can just compare "real" and "filename"...
        }

        // once at the end, also create a file in that last directory
        // (we could create a file in each and every directory but that would
        // be a bit much)
        {
            size_t size(rand() & 0x3FFFF);
            memfile::memory_file file;
            file.create(memfile::memory_file::file_format_other);
            for(size_t j(0); j < size; ++j)
            {
                char c(rand() & 255);
                file.write(&c, static_cast<int>(j), static_cast<int>(sizeof(c)));
            }
            file.write_file(filename.append_child("test.txt"));
        }
    }
}


// vim: ts=4 sw=4 et
