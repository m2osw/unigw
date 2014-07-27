/*    unittest_control.cpp
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

#include "unittest_control.h"
#include "unittest_main.h"
#include <stdexcept>
#include <cstring>

CPPUNIT_TEST_SUITE_REGISTRATION( ControlUnitTests );

void ControlUnitTests::setUp()
{
}



void ControlUnitTests::files_field_to_list()
{
    const wpkg_control::file_list_t files("Files");
    CPPUNIT_ASSERT(files.to_string().empty());
}



void ControlUnitTests::check_field(const std::string& field_name, const std::string& default_format, wpkg_control::file_item::format_t sources_format)
{
    {
        std::shared_ptr<wpkg_control::control_file> ctrl(new wpkg_control::binary_control_file(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::build_control_file_state_t)));

        // test with a simple list
        ctrl->set_field(field_name, "list\n"
            "/etc/t1.conf\n"
            "/usr/bin/t1\n"
            "/usr/share/doc/t1/copyright\n"
            "\"/usr/share/doc/t 1/copyright\"\n"
        );

        if(field_name != "Conf-Files")
        {
            try
            {
                const wpkg_control::file_list_t invalid_files(ctrl->get_files("Conf-Files"));
                CPPUNIT_ASSERT(!"get_files() with the wrong name succeeded");
            }
            catch(const wpkg_field::wpkg_field_exception_undefined&)
            {
                // success
            }
        }

        const wpkg_control::file_list_t files(ctrl->get_files(field_name));
        CPPUNIT_ASSERT(files.size() == 4);

        // file 0
        CPPUNIT_ASSERT(files[0].get_format() == wpkg_control::file_item::format_list);
        CPPUNIT_ASSERT(files[0].best_format(wpkg_control::file_item::format_unknown) == wpkg_control::file_item::format_unknown);
        CPPUNIT_ASSERT(files[0].best_format(wpkg_control::file_item::format_list) == wpkg_control::file_item::format_list);
        CPPUNIT_ASSERT(files[0].get_filename() == "/etc/t1.conf");
        CPPUNIT_ASSERT(files[0].get_mode() == 0);
        CPPUNIT_ASSERT(files[0].get_size() == 0);
        CPPUNIT_ASSERT(files[0].get_checksum() == "");

        // file 1
        CPPUNIT_ASSERT(files[1].get_format() == wpkg_control::file_item::format_list);
        CPPUNIT_ASSERT(files[1].best_format(wpkg_control::file_item::format_list) == wpkg_control::file_item::format_list);
        CPPUNIT_ASSERT(files[1].get_filename() == "/usr/bin/t1");
        CPPUNIT_ASSERT(files[1].get_mode() == 0);
        CPPUNIT_ASSERT(files[1].get_size() == 0);
        CPPUNIT_ASSERT(files[1].get_checksum() == "");

        // file 2
        CPPUNIT_ASSERT(files[2].get_format() == wpkg_control::file_item::format_list);
        CPPUNIT_ASSERT(files[2].best_format(wpkg_control::file_item::format_list) == wpkg_control::file_item::format_list);
        CPPUNIT_ASSERT(files[2].get_filename() == "/usr/share/doc/t1/copyright");
        CPPUNIT_ASSERT(files[2].get_mode() == 0);
        CPPUNIT_ASSERT(files[2].get_size() == 0);
        CPPUNIT_ASSERT(files[2].get_checksum() == "");

        // file 3
        CPPUNIT_ASSERT(files[3].get_format() == wpkg_control::file_item::format_list);
        CPPUNIT_ASSERT(files[3].best_format(wpkg_control::file_item::format_list) == wpkg_control::file_item::format_list);
        CPPUNIT_ASSERT(files[3].get_filename() == "/usr/share/doc/t 1/copyright");
        CPPUNIT_ASSERT(files[3].get_mode() == 0);
        CPPUNIT_ASSERT(files[3].get_size() == 0);
        CPPUNIT_ASSERT(files[3].get_checksum() == "");
    }

    {
        std::shared_ptr<wpkg_control::control_file> ctrl(new wpkg_control::binary_control_file(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::build_control_file_state_t)));

        // test with the conffiles format
        ctrl->set_field(field_name, (default_format == "conffiles" ? "" : "conffiles") + std::string("\n"
            "/etc/t2.conf 0123456789abcdef0123456789abcdef\n"
            "/usr/bin/t2 cdef0123456789abcdef0123456789ab\n"
            "/usr/share/doc/t2/copyright 021346578a9bcedf021346578a9bcedf\n"
            "\"/usr/share/doc/t 2/copyright\" 021346578a9bcedf021346578a9bcedf\n")
        );

        if(field_name != "Files")
        {
            try
            {
                const wpkg_control::file_list_t invalid_files(ctrl->get_files("Files"));
                CPPUNIT_ASSERT(!"get_files() with the wrong name succeeded");
            }
            catch(const wpkg_field::wpkg_field_exception_undefined&)
            {
                // success
            }
        }

        const wpkg_control::file_list_t files(ctrl->get_files(field_name));
        CPPUNIT_ASSERT(files.size() == 4);

        // file 0
        CPPUNIT_ASSERT(files[0].get_format() == wpkg_control::file_item::format_conffiles);
        CPPUNIT_ASSERT(files[0].best_format(wpkg_control::file_item::format_unknown) == wpkg_control::file_item::format_unknown);
        CPPUNIT_ASSERT(files[0].best_format(wpkg_control::file_item::format_list) == wpkg_control::file_item::format_conffiles);
        CPPUNIT_ASSERT(files[0].get_filename() == "/etc/t2.conf");
        CPPUNIT_ASSERT(files[0].get_mode() == 0);
        CPPUNIT_ASSERT(files[0].get_size() == 0);
        CPPUNIT_ASSERT(files[0].get_checksum() == "0123456789abcdef0123456789abcdef");

        // file 1
        CPPUNIT_ASSERT(files[1].get_format() == wpkg_control::file_item::format_conffiles);
        CPPUNIT_ASSERT(files[1].best_format(wpkg_control::file_item::format_list) == wpkg_control::file_item::format_conffiles);
        CPPUNIT_ASSERT(files[1].get_filename() == "/usr/bin/t2");
        CPPUNIT_ASSERT(files[1].get_mode() == 0);
        CPPUNIT_ASSERT(files[1].get_size() == 0);
        CPPUNIT_ASSERT(files[1].get_checksum() == "cdef0123456789abcdef0123456789ab");

        // file 2
        CPPUNIT_ASSERT(files[2].get_format() == wpkg_control::file_item::format_conffiles);
        CPPUNIT_ASSERT(files[2].best_format(wpkg_control::file_item::format_list) == wpkg_control::file_item::format_conffiles);
        CPPUNIT_ASSERT(files[2].get_filename() == "/usr/share/doc/t2/copyright");
        CPPUNIT_ASSERT(files[2].get_mode() == 0);
        CPPUNIT_ASSERT(files[2].get_size() == 0);
        CPPUNIT_ASSERT(files[2].get_checksum() == "021346578a9bcedf021346578a9bcedf");

        // file 3
        CPPUNIT_ASSERT(files[3].get_format() == wpkg_control::file_item::format_conffiles);
        CPPUNIT_ASSERT(files[3].best_format(wpkg_control::file_item::format_list) == wpkg_control::file_item::format_conffiles);
        CPPUNIT_ASSERT(files[3].get_filename() == "/usr/share/doc/t 2/copyright");
        CPPUNIT_ASSERT(files[3].get_mode() == 0);
        CPPUNIT_ASSERT(files[3].get_size() == 0);
        CPPUNIT_ASSERT(files[3].get_checksum() == "021346578a9bcedf021346578a9bcedf");
    }

    {
        std::shared_ptr<wpkg_control::control_file> ctrl(new wpkg_control::binary_control_file(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::build_control_file_state_t)));

        // test with the conffiles format
        std::string fmt;
        if(default_format != "sources" && default_format != "md5sum")
        {
            switch(rand() & 3)
            {
            case 0:
                fmt = "sources";
                sources_format = wpkg_control::file_item::format_md5sum;
                break;

            case 1:
                fmt = "md5sum";
                sources_format = wpkg_control::file_item::format_md5sum;
                break;

            case 2:
                fmt = "sha1";
                sources_format = wpkg_control::file_item::format_sha1;
                break;

            case 3:
                fmt = "sha256";
                sources_format = wpkg_control::file_item::format_sha256;
                break;

            }
        }
        ctrl->set_field(field_name, fmt + "\n"
            "0123456789abcdef0123456789abcdef 1234 /etc/t3.conf\n"
            "\"cdef0123456789abcdef0123456789ab\" 3455 /usr/bin/t3\n"
            "021346578a9bcedf021346578a9bcedf \"1122\" /usr/share/doc/t3/copyright\n"
            "021346578a900000021346578a9bcedf 333 \"/usr/share/doc/t 3/index.html\"\n"
        );

        if(field_name != "Checksums-Sha1")
        {
            try
            {
                const wpkg_control::file_list_t invalid_files(ctrl->get_files("Checksums-Sha1"));
                CPPUNIT_ASSERT(!"get_files() with the wrong name succeeded");
            }
            catch(const wpkg_field::wpkg_field_exception_undefined&)
            {
                // success
            }
        }

        const wpkg_control::file_list_t files(ctrl->get_files(field_name));
        CPPUNIT_ASSERT(files.size() == 4);

        // file 0
        CPPUNIT_ASSERT(files[0].get_format() == sources_format);
        CPPUNIT_ASSERT(files[0].best_format(wpkg_control::file_item::format_unknown) == wpkg_control::file_item::format_unknown);
        CPPUNIT_ASSERT(files[0].best_format(wpkg_control::file_item::format_list) == sources_format);
        CPPUNIT_ASSERT(files[0].get_filename() == "/etc/t3.conf");
        CPPUNIT_ASSERT(files[0].get_mode() == 0);
        CPPUNIT_ASSERT(files[0].get_size() == 1234);
        CPPUNIT_ASSERT(files[0].get_checksum() == "0123456789abcdef0123456789abcdef");

        // file 1
        CPPUNIT_ASSERT(files[1].get_format() == sources_format);
        CPPUNIT_ASSERT(files[1].best_format(wpkg_control::file_item::format_list) == sources_format);
        CPPUNIT_ASSERT(files[1].get_filename() == "/usr/bin/t3");
        CPPUNIT_ASSERT(files[1].get_mode() == 0);
        CPPUNIT_ASSERT(files[1].get_size() == 3455);
        CPPUNIT_ASSERT(files[1].get_checksum() == "cdef0123456789abcdef0123456789ab");

        // file 2
        CPPUNIT_ASSERT(files[2].get_format() == sources_format);
        CPPUNIT_ASSERT(files[2].best_format(wpkg_control::file_item::format_list) == sources_format);
        CPPUNIT_ASSERT(files[2].get_filename() == "/usr/share/doc/t3/copyright");
        CPPUNIT_ASSERT(files[2].get_mode() == 0);
        CPPUNIT_ASSERT(files[2].get_size() == 1122);
        CPPUNIT_ASSERT(files[2].get_checksum() == "021346578a9bcedf021346578a9bcedf");

        // file 3
        CPPUNIT_ASSERT(files[3].get_format() == sources_format);
        CPPUNIT_ASSERT(files[3].best_format(wpkg_control::file_item::format_list) == sources_format);
        CPPUNIT_ASSERT(files[3].get_filename() == "/usr/share/doc/t 3/index.html");
        CPPUNIT_ASSERT(files[3].get_mode() == 0);
        CPPUNIT_ASSERT(files[3].get_size() == 333);
        CPPUNIT_ASSERT(files[3].get_checksum() == "021346578a900000021346578a9bcedf");
    }

}

void ControlUnitTests::all_files_field()
{
    check_field("Checksums-Sha1", "sources", wpkg_control::file_item::format_sha1);
    check_field("Checksums-Sha256", "sources", wpkg_control::file_item::format_sha256);
    check_field("Conf-Files", "conffiles", wpkg_control::file_item::format_md5sum);
    check_field("Files", "sources", wpkg_control::file_item::format_md5sum);
}




// vim: ts=4 sw=4 et
