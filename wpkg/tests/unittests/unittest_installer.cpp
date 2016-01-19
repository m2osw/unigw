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

#include "libdebpackages/installer/details/disk.h"
#include "libdebpackages/installer/flags.h"
#include "libdebpackages/installer/package_list.h"

#include <iostream>

#include <catch.hpp>

using namespace test_common;
using namespace wpkgar;
using namespace installer;


namespace my_output
{
    void log_message( const wpkg_output::message_t& msg )
    {
        std::string message(msg.get_full_message(false));
        std::cout << message;
        if(message.length() > 0 && message[message.length() - 1] != '\n')
        {
            std::cout << std::endl;
        }
    }

    void output_message( const wpkg_output::message_t& msg )
    {
        if( (msg.get_debug_flags() & wpkg_output::debug_flags::debug_progress) != 0)
        {
            std::cerr << msg.get_full_message(true) << std::endl;
        }
    }
}

class InstallerUnitTests : public wpkg_tools
{
public:
    InstallerUnitTests();
    ~InstallerUnitTests();

    void install_simple_package();
    void test_disk_t();
    void test_disk_list_t();


private:
    wpkgar_manager::pointer_t  f_manager;

    typedef std::shared_ptr<wpkg_control::control_file> control_file_pointer_t;
    void compute_size_and_verify_overwrite
        ( installer::details::disk_list_t& disk_list
        , const std::string&        name
        , control_file_pointer_t    ctrl
        , int                       factor
        , memfile::memory_file*     upgrade = 0
        );
};


InstallerUnitTests::InstallerUnitTests()
    : wpkg_tools()
    , f_manager( new wpkgar_manager )
{
    // Set up the output so it goes to cout/cerr respectively
    //
    auto output( wpkg_output::get_output() );
    output->register_raw_log_listener(
            [&]( const wpkg_output::message_t& msg ) { my_output::log_message( msg ); }
            );
    output->register_user_log_listener(
            [&]( const wpkg_output::message_t& msg ) { my_output::output_message( msg ); }
            );
    // TODO: exercise the progress listener functionality

    // Setup the manager
    //
    init_database();
    f_manager->set_root_path     ( get_target_path()   );
    f_manager->set_database_path ( get_database_path() );
    f_manager->add_repository    ( get_repository() );
}

InstallerUnitTests::~InstallerUnitTests()
{
    wpkg_output::get_output()->clear_listeners();
}


void InstallerUnitTests::install_simple_package()
{
    // Create the installer
    wpkgar_install::pointer_t installer( new wpkgar_install(f_manager) );
    installer->set_installing();

    // Create the package, then initialize the database.
    //
    control_file_pointer_t ctrl(get_new_control_file(__FUNCTION__));
    ctrl->set_field("Files", "conffiles\n"
                             "/etc/t1.conf 0123456789abcdef0123456789abcdef\n"
                             "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
                             "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
                    );
    create_package( "t1", ctrl, 0 );

    // Add the package to be installed.
    //
    auto package_list( installer->get_package_list() );
    wpkg_filename::uri_filename package_name( get_package_file_name( "t1", ctrl ) );
    package_list->add_package( package_name.full_path() );

    // This should throw because the database is not locked:
    //
    CATCH_REQUIRE_THROWS( installer->validate() );

    // Create the lock file
    //
    wpkgar::wpkgar_lock the_lock( f_manager, "Installing unit test package..." );

    // Now validate for real. This should not fail:
    //
    const bool validated = installer->validate();
    CATCH_REQUIRE( validated );
    if( !validated )
    {
        std::cerr << "There was a problem--package did not validate!!!!" << std::endl;
        return;
    }

    // Make sure the install list is not empty
    //
    install_info_list_t install_list( installer->get_install_list() );
    CATCH_REQUIRE( !install_list.empty() );

    // There should be exactly one explicit package:
    int explicit_count = 0;
    for( const auto& info : install_list )
    {
        if( info.get_install_type() == install_info_t::install_type_explicit )
        {
            explicit_count++;
        }
    }
    CATCH_REQUIRE( explicit_count == 1 );

    // Now pre-configure:
    //
    CATCH_REQUIRE( installer->pre_configure() );

    // And unpack and configure the package:
    //
    for(;;)
    {
        f_manager->check_interrupt();
        const int i( installer->unpack() );
        if( wpkgar_install::WPKGAR_EOP == i )
        {
            break;
        }

        CATCH_REQUIRE( wpkgar_install::WPKGAR_ERROR != i );
        if( i < 0 )
        {
            break;
        }

        // Configure the package
        //
        const bool configured = installer->configure(i);
        CATCH_REQUIRE( configured );
        if( !configured )
        {
            break;
        }
    }
}


void InstallerUnitTests::test_disk_t()
{
    installer::details::disk_t d( "/" );

    // Test defaults
    //
    CATCH_REQUIRE( d.get_block_size() == 0 );
    CATCH_REQUIRE( d.get_free_space() == 0 );
    CATCH_REQUIRE( d.get_block_size() == 0 );
    CATCH_REQUIRE( !d.is_readonly()        );
    CATCH_REQUIRE( d.is_valid()            );
    CATCH_REQUIRE( d.get_path() == "/"     );

    // Test setters
    //
    d.set_block_size( 10 );
    CATCH_REQUIRE( d.get_block_size() == 10 );
    //
    d.add_size( 10 );
    //
    // the current size has this added to it:
    // f_size += (size + f_block_size - 1) / f_block_size;
    //
    int new_size = (10 + d.get_block_size() - 1 ) / d.get_block_size();
    //
    // The return value is the block size * size:
    //
    CATCH_REQUIRE( d.get_size() == new_size * d.get_block_size() );
    //
    d.add_size( 10 );
    new_size += (10 + d.get_block_size() - 1) / d.get_block_size();
    CATCH_REQUIRE( d.get_size() == new_size * d.get_block_size() );
    //
    CATCH_REQUIRE( !d.is_valid() );
    //
    d.set_free_space( 100 );
    //
    CATCH_REQUIRE( d.is_valid() );

    // Make sure operations work
    //
    CATCH_REQUIRE( d.match("/") );
}


void InstallerUnitTests::compute_size_and_verify_overwrite
    ( installer::details::disk_list_t& disk_list
    , const std::string&        name
    , control_file_pointer_t    ctrl
    , int                       factor
    , memfile::memory_file*     upgrade
    )
{
    const wpkg_filename::uri_filename   root( f_manager->get_inst_path() );
    wpkg_filename::uri_filename         pkg_filename( get_package_file_name( name, ctrl ) );
    package_item_t                      pkg_item( f_manager, pkg_filename );
    //
    memfile::memory_file* data = 0;
    f_manager->load_package( pkg_filename );
    f_manager->get_wpkgar_file( pkg_filename, data );

    disk_list.compute_size_and_verify_overwrite
            ( 0
            , pkg_item
            , root
            , data
            , upgrade
            , factor
            );
}


void InstallerUnitTests::test_disk_list_t()
{
    wpkgar::wpkgar_lock				the_lock ( f_manager, "disk_list_t unit test" );
    package_list::pointer_t		    pkg_list ( new package_list( f_manager ) );
    flags::pointer_t				the_flags( new flags() );
    installer::details::disk_list_t	disk_list( pkg_list, the_flags );

#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
    // TODO: some more testing of the default disk would be nice,
    // but for now, just test its existence.
    CATCH_REQUIRE( disk_list.get_default_disk() );
#endif

    CATCH_REQUIRE( disk_list.are_valid() );

    // t1
    //
    control_file_pointer_t ctrl(get_new_control_file(__FUNCTION__));
    ctrl->set_field("Files", "conffiles\n"
                             "/etc/t1.conf 0123456789abcdef0123456789abcdef\n"
                             "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
                             "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
                    );
    ctrl->set_field("Version", "1.0");
    create_package( "t1", ctrl, 0 );
    //
    compute_size_and_verify_overwrite( disk_list, "t1", ctrl, 1 /*factor*/ );
    CATCH_REQUIRE( wpkg_output::get_output_error_count() == 0 );

    // t2
    //
    control_file_pointer_t ctrl_t2(get_new_control_file(__FUNCTION__ + std::string(" t2")));
    ctrl_t2->set_field("Files", "conffiles\n"
            "/usr/bin/t2 0123456789abcdef0123456789abcdef\n"
            "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
            "/usr/share/doc/t2/copyright 0123456789abcdef0123456789abcdef\n"
            );
    ctrl_t2->set_field("Version", "1.0");
    ctrl_t2->set_field("Depends", "t1");
    create_package("t2", ctrl_t2);
    init_database(); // this updates the index in the repository.
    //
    compute_size_and_verify_overwrite( disk_list, "t2", ctrl_t2, 1 /*factor*/ );
    CATCH_REQUIRE( wpkg_output::get_output_error_count() > 0 );
    wpkg_output::get_output()->reset_error_count();
    CATCH_REQUIRE( wpkg_output::get_output_error_count() == 0 );

    // t1 v1.1
    //
    control_file_pointer_t ctrl_t1_v11(get_new_control_file(__FUNCTION__ + std::string(" t1")));
    ctrl_t1_v11->set_field("Files", "conffiles\n"
                 "/etc/t1.conf 0123456789abcdef0123456789abcdef\n"
                 "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
                 "/usr/bin/libt1.a 0123456789abcdef0123456789abcdef\n"
                 "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
            );
    ctrl_t1_v11->set_field("Version", "1.1");
    create_package("t1", ctrl_t1_v11);
    //
    init_database(); // this updates the index in the repository.
    //
    compute_size_and_verify_overwrite( disk_list, "t1", ctrl_t1_v11, 1 /*factor*/ );
    CATCH_REQUIRE( wpkg_output::get_output_error_count() > 0 );
    wpkg_output::get_output()->reset_error_count();
}


CATCH_TEST_CASE( "InstallerUnitTests::install_package", "InstallerUnitTests" )
{
    InstallerUnitTests instut;
    instut.install_simple_package();
}


CATCH_TEST_CASE( "InstallerUnitTests::test_disk_t", "InstallerUnitTests" )
{
    InstallerUnitTests instut;
    instut.test_disk_t();
}


CATCH_TEST_CASE( "InstallerUnitTests::test_disk_list_t", "InstallerUnitTests" )
{
    InstallerUnitTests instut;
    instut.test_disk_list_t();
}


// vim: ts=4 sw=4 et
