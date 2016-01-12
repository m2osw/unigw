/*    dependencies.h -- determine dependencies for installation of explicit packages
 *    Copyright (C) 2012-2015  Made to Order Software Corporation
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
 *    Doug Barbieri  doug@dooglio.net
 */

/** \file
 * \brief Declaration of the install function of the library.
 *
 * Packages can be installed in a target. These declarations are used to
 * define a class used to install packages on a target. Internally, this is
 * very complex as you can see by the large number of validation functions.
 * The validation functions actually all have "side effects" in that they
 * actually compute the final tree of packages to be installed or find out
 * that the specified packages cannot be installed.
 */
#pragma once

#include    "libdebpackages/wpkgar.h"
#include    "libdebpackages/wpkg_dependencies.h"
#include    "libdebpackages/installer/flags.h"
#include    "libdebpackages/installer/install_info.h"
#include    "libdebpackages/installer/package_item.h"
#include    "libdebpackages/installer/package_list.h"
#include    "libdebpackages/installer/tree_generator.h"
#include    "controlled_vars/controlled_vars_auto_enum_init.h"

namespace wpkgar
{

namespace installer
{

class DEBIAN_PACKAGE_EXPORT dependency_error
    : public std::runtime_error
{
    public:
        dependency_error(const std::string& what_msg);
};

class DEBIAN_PACKAGE_EXPORT dependencies
    : public std::enable_shared_from_this<dependencies>
{
public:
    enum validation_return_t
    {
        validation_return_success,
        validation_return_error,
        validation_return_missing,
        validation_return_held,
        validation_return_unpacked
    };

    typedef std::shared_ptr<dependencies>                              pointer_t;
    typedef std::vector<package_item_t*>                               package_ptrs_t;
    typedef std::vector<wpkg_dependencies::dependencies::dependency_t> dependency_list_t;
    typedef std::vector<std::string>                                   string_list_t;

    dependencies
        ( wpkgar_manager::pointer_t manager
        , package_list::pointer_t   list
        , flags::pointer_t          flags
        );

    bool get_install_includes_choices() const;  // f_install_includes_choices

    // validation sub-functions
    int                 compare_trees(const package_list::list_t& left, const package_list::list_t& right) const;
    void                find_dependencies( package_list::list_t& tree, const package_list::list_t::size_type idx, dependency_list_t& missing, dependency_list_t& held );
    validation_return_t find_explicit_dependency(package_list::list_t::size_type index, const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d, const std::string& field_name);
    validation_return_t find_installed_dependency(package_list::list_t::size_type index, const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d, const std::string& field_name);
    bool                find_installed_predependency_package( package_item_t& pkg, const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d);
    void                find_installed_predependency(const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d);
    int                 match_dependency_version(const wpkg_dependencies::dependencies::dependency_t& d, const package_item_t& name);
    void                output_tree(int count, const package_list::list_t& tree, const std::string& sub_title);
    void                read_repositories();
    bool                read_repository_index( const wpkg_filename::uri_filename& repo_filename, memfile::memory_file& index_file );
    bool                trees_are_practically_identical(const package_list::list_t& left, const package_list::list_t& right) const;
    void                trim_conflicts
                            ( const bool check_available
                            , const bool only_explicit
                            , wpkg_filename::uri_filename filename
                            , package_item_t::package_type_t idx_type
                            , package_item_t& parent_package
                            , package_item_t& depends_package
                            , const wpkg_dependencies::dependencies::dependency_t& dependency
                            );
    void                trim_breaks
                            ( const bool check_available
                            , const bool only_explicit
                            , wpkg_filename::uri_filename filename
                            , package_item_t::package_type_t idx_type
                            , package_item_t& parent_package
                            , package_item_t& depends_package
                            , const wpkg_dependencies::dependencies::dependency_t& dependency
                            );
    void                trim_conflicts( package_list::list_t& tree, package_list::list_t::size_type idx, bool only_explicit );
    bool                trim_dependency
                            ( package_item_t& item
                            , package_ptrs_t& parents
                            , const wpkg_dependencies::dependencies::dependency_t& dependency
                            , const std::string& field_name
                            );
    void                trim_available(package_item_t& item, package_ptrs_t& parents);
    void                trim_available_packages();
    validation_return_t validate_installed_depends_field(const package_list::list_t::size_type idx, const std::string& field_name);
    validation_return_t validate_installed_dependencies();
    bool                verify_tree( package_list::list_t& tree, dependency_list_t& missing, dependency_list_t& held );

    // Main rountines you can call, but the others above are exposed for unit testing.
    //
    void                validate_predependencies();
    void                validate_dependencies();

private:
    wpkgar_manager::pointer_t  f_manager;
    package_list::pointer_t    f_package_list;
    flags::pointer_t           f_flags;
    controlled_vars::fbool_t   f_repository_packages_loaded;
    controlled_vars::fbool_t   f_install_includes_choices;
    controlled_vars::zuint32_t f_tree_max_depth;
    string_list_t              f_field_names;
};

}   // namespace installer

}   // namespace wpkgar

// vim: ts=4 sw=4 et
