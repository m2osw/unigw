/*    unittest_package.h
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
#ifndef UNIT_TEST_PACKAGE_H
#define UNIT_TEST_PACKAGE_H

#include <cppunit/extensions/HelperMacros.h>

namespace wpkg_filename
{
class uri_filename;
}

class PackageUnitTests : public CPPUNIT_NS::TestFixture
{
    CPPUNIT_TEST_SUITE( PackageUnitTests );
        CPPUNIT_TEST( simple_package );
        CPPUNIT_TEST( simple_package_with_spaces );
        CPPUNIT_TEST( depends_with_simple_packages );
        CPPUNIT_TEST( depends_with_simple_packages_with_spaces );
        CPPUNIT_TEST( essential_package );
        CPPUNIT_TEST( essential_package_with_spaces );
        CPPUNIT_TEST( admindir_package );
        CPPUNIT_TEST( admindir_package_with_spaces );
        CPPUNIT_TEST( upgrade_package );
        CPPUNIT_TEST( upgrade_package_with_spaces );
        CPPUNIT_TEST( file_exists_in_admindir );
        CPPUNIT_TEST( file_exists_in_admindir_with_spaces );
        CPPUNIT_TEST( depends_distribution_packages );
        CPPUNIT_TEST( depends_distribution_packages_with_spaces );
        CPPUNIT_TEST( conflicting_packages );
        CPPUNIT_TEST( conflicting_packages_with_spaces );
        CPPUNIT_TEST( sorted_packages_auto_index );
        CPPUNIT_TEST( sorted_packages_auto_index_with_spaces );
        CPPUNIT_TEST( sorted_packages_ready_index );
        CPPUNIT_TEST( sorted_packages_ready_index_with_spaces );
        CPPUNIT_TEST( choices_packages );
        CPPUNIT_TEST( choices_packages_with_spaces );
        CPPUNIT_TEST( same_package_two_places_errors );
        CPPUNIT_TEST( same_package_two_places_errors_with_spaces );
        CPPUNIT_TEST( self_upgrade );
        CPPUNIT_TEST( self_upgrade_with_spaces );
        CPPUNIT_TEST( scripts_order );
        CPPUNIT_TEST( scripts_order_with_spaces );
        CPPUNIT_TEST( compare_versions );
        CPPUNIT_TEST( compare_versions_with_spaces );
        CPPUNIT_TEST( auto_upgrade );
        CPPUNIT_TEST( auto_upgrade_with_spaces );
        CPPUNIT_TEST( auto_downgrade );
        CPPUNIT_TEST( auto_downgrade_with_spaces );
        CPPUNIT_TEST( test_hold );
        CPPUNIT_TEST( test_hold_with_spaces );
        CPPUNIT_TEST( minimum_upgradable_version );
        CPPUNIT_TEST( minimum_upgradable_version_with_spaces );
        CPPUNIT_TEST( check_drive_subst );
        CPPUNIT_TEST( check_drive_subst_with_spaces );
        CPPUNIT_TEST( check_architecture_vendor );
        CPPUNIT_TEST( check_architecture_vendor_with_spaces );
        CPPUNIT_TEST( check_architecture_vendor2 );
        CPPUNIT_TEST( check_architecture_vendor2_with_spaces );
        CPPUNIT_TEST( install_hooks );
        CPPUNIT_TEST( install_hooks_with_spaces );
        CPPUNIT_TEST( auto_remove );
        CPPUNIT_TEST( auto_remove_with_spaces );
        CPPUNIT_TEST( scripts_selection );
        CPPUNIT_TEST( scripts_selection_with_spaces );
        CPPUNIT_TEST( complex_tree_in_repository );
        CPPUNIT_TEST( complex_tree_in_repository_with_spaces );
        CPPUNIT_TEST( unacceptable_filename );
    CPPUNIT_TEST_SUITE_END();

public:
    void setUp();

protected:
    void sorted_packages_run(int precreate_index);

    void simple_package();
    void simple_package_with_spaces();
    void depends_with_simple_packages();
    void depends_with_simple_packages_with_spaces();
    void essential_package();
    void essential_package_with_spaces();
    void admindir_package();
    void admindir_package_with_spaces();
    void upgrade_package();
    void upgrade_package_with_spaces();
    void file_exists_in_admindir();
    void file_exists_in_admindir_with_spaces();
    void depends_distribution_packages();
    void depends_distribution_packages_with_spaces();
    void conflicting_packages();
    void conflicting_packages_with_spaces();
    void sorted_packages_auto_index();
    void sorted_packages_auto_index_with_spaces();
    void sorted_packages_ready_index();
    void sorted_packages_ready_index_with_spaces();
    void choices_packages();
    void choices_packages_with_spaces();
    void same_package_two_places_errors();
    void same_package_two_places_errors_with_spaces();
    void self_upgrade();
    void self_upgrade_with_spaces();
    void scripts_order();
    void scripts_order_with_spaces();
    void compare_versions();
    void compare_versions_with_spaces();
    void auto_upgrade();
    void auto_upgrade_with_spaces();
    void auto_downgrade();
    void auto_downgrade_with_spaces();
    void test_hold();
    void test_hold_with_spaces();
    void minimum_upgradable_version();
    void minimum_upgradable_version_with_spaces();
    void check_drive_subst();
    void check_drive_subst_with_spaces();
    void check_architecture_vendor();
    void check_architecture_vendor_with_spaces();
    void check_architecture_vendor2();
    void check_architecture_vendor2_with_spaces();
    void install_hooks();
    void install_hooks_with_spaces();
    void auto_remove();
    void auto_remove_with_spaces();
    void scripts_selection();
    void scripts_selection_with_spaces();
    void complex_tree_in_repository();
    void complex_tree_in_repository_with_spaces();
    void unacceptable_filename();
};

#endif
// vim: ts=4 sw=4 et
