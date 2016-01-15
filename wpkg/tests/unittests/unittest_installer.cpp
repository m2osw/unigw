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
#include "libdebpackages/wpkg_util.h"

#include <catch.hpp>

using namespace test_common;
using namespace wpkgar;

class InstallerUnitTests : public wpkg_tools
{
public:
    InstallerUnitTests();

    void install_package();
};


InstallerUnitTests::InstallerUnitTests() : wpkg_tools()
{
}


void InstallerUnitTests::install_package()
{
    std::shared_ptr<wpkg_control::control_file> ctrl(get_new_control_file(__FUNCTION__));
    ctrl->set_field("Files", "conffiles\n"
                             "/etc/t1.conf 0123456789abcdef0123456789abcdef\n"
                             "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
                             "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
                    );
    create_package( "t1", ctrl, 0 );

    init_database();

    wpkgar_install::pointer_t manager( new wpkgar_manager );
    manager->set_root_path  ( get_root()       );
    manager->add_repository ( get_repository() );

    wpkgar_install::pointer_t installer( new wpkgar_install(manager) );
    auto package_list( installer->get_package_list() );
    wpkg_filename::uri_filename package_name( get_package_file_name( "t1", ctrl ) );
    installer->add_package( package_name.full_path() );

    installer->set_installing();
    CATCH_REQUIRE( installer->validate() );

    installer::install_info_list_t install_list( installer->get_install_list() );
    CATCH_REQUIRE( !install_list.empty() );

    int explicit_count = 0;
    for( const auto& info : install_list )
    {
        if( info.get_install_type() == installer::install_info_t::install_type_implicit )
        {
            explicit_count++;
        }
    }
    CATCH_REQUIRE( explicit_count == 1 );

    CATCH_REQUIRE( installer->pre_configure() );
    for(;;)
    {
        manager->check_interrupt();
        const int i( installer->unpack() );
        if( wpkgar_install::WPKGAR_EOP == i )
        {
            break;
        }
        CATCH_REQUIRE( i != wpkgar_install::WPKGAR_ERROR );
        CATCH_REQUIRE( installer->configure(i) );
    }
}


CATCH_TEST_CASE( "InstallerUnitTests::install_package", "InstallerUnitTests" )
{
    InstallerUnitTests instut;
    instut.install_package();
}


// vim: ts=4 sw=4 et
