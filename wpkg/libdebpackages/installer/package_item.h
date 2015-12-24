/*    installer_package_item.h -- maintain a package for the installer.
 *
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
 *    Doug Barbieri  doug@m2osw.com
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
#include    "libdebpackages/wpkg_control.h"
#include    "libdebpackages/memfile.h"
#include    "controlled_vars/controlled_vars_auto_enum_init.h"

#include    <memory>

namespace wpkgar
{

namespace installer
{

class DEBIAN_PACKAGE_EXPORT package_item_t
{
public:
    typedef std::vector<package_item_t> vector_t;

    enum package_type_t
    {
        // command line defined
        package_type_explicit,          // requested by the administrator (command line)

        // repository defined
        package_type_implicit,          // necessary to satisfy dependencies
        package_type_available,         // not marked as necessary or invalid yet

        // installed status
        package_type_not_installed,     // package is not currently installed
        package_type_installed,         // package is installed
        package_type_unpacked,          // package is unpacked but not configured
        package_type_configure,         // package is going to be configured
        package_type_upgrade,           // package is going to be upgraded
        package_type_upgrade_implicit,  // package is implicitly upgraded to satisfy dependencies
        package_type_downgrade,         // package is going to be downgraded

        // different "invalid" states
        package_type_invalid,           // clearly determined as invalid (bad architecture, version, etc.)
        package_type_same,              // ignored because it is already installed
        package_type_older,             // removed because the version is smaller (package is older)
        package_type_directory          // this is a directory, read it once when check dependencies and then ignore
    };

    package_item_t( wpkgar_manager::pointer_t manager, const wpkg_filename::uri_filename& filename, package_type_t type = package_type_explicit );
    package_item_t( wpkgar_manager::pointer_t manager, const wpkg_filename::uri_filename& filename, package_type_t type, const memfile::memory_file& ctrl );

    const wpkg_filename::uri_filename& get_filename            () const;
    const std::string&                 get_name                () const;
    const std::string&                 get_architecture        () const;
    const std::string&                 get_version             () const;
    wpkgar_manager::package_status_t   get_original_status     () const;

    bool        field_is_defined  ( const std::string& name)       const;
    std::string get_field         ( const std::string& name)       const;
    bool        get_boolean_field ( const std::string& name)       const;
    bool        validate_fields   ( const std::string& expression) const;
    bool        is_conffile       ( const std::string& path)       const;

    void            set_type                ( const package_type_t type);
    package_type_t  get_type                () const;
    void            set_upgrade             ( int32_t upgrade );
    int32_t         get_upgrade             () const;
    void            mark_unpacked           ();
    bool            is_unpacked             () const;
    bool            is_marked_for_install   () const;
    void            copy_package_in_database();

    void            load( bool ctrl );

private:
    enum loaded_state_t
    {
        load_state_not_loaded,
        load_state_control_file,
        load_state_full
    };
    typedef controlled_vars::limited_auto_enum_init
        < loaded_state_t
        , load_state_not_loaded
        , load_state_control_file
        , load_state_not_loaded
        >
            safe_loaded_state_t;

    wpkgar_manager::pointer_t                   f_manager;
    wpkg_filename::uri_filename                 f_filename;
    package_type_t                              f_type;
    std::shared_ptr<memfile::memory_file>       f_ctrl;
    std::shared_ptr<wpkg_control::control_file> f_fields;
    safe_loaded_state_t                         f_loaded;
    controlled_vars::fbool_t                    f_depends_done;
    controlled_vars::fbool_t                    f_unpacked;
    std::string                                 f_name;
    std::string                                 f_architecture;
    std::string                                 f_version;
    wpkgar_manager::package_status_t            f_original_status;
    controlled_vars::mint32_t                   f_upgrade;
};

}   // namespace installer

}   // namespace wpkgar

// vim: ts=4 sw=4 et
