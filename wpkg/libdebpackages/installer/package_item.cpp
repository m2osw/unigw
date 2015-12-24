/*    installer_package_item.cpp -- handle a deb package for installation
 *
 *    Copyright (C) 2006-2015  Made to Order Software Corporation
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
 * \brief The implementation of the wpkg --install command.
 *
 * The following includes all the necessary functions to implement the install
 * class which is used to install packages on a target system.
 *
 * The functions include support to the --install, --unpack, and --configure
 * functions.
 */
#include    "libdebpackages/installer_package_item.h"
#if defined(MO_CYGWIN)
#   include    <Windows.h>
#endif


namespace wpkgar
{


namespace installer
{


/** \class package_item_t
 *
 * \brief A package object for the installer.
 *
 * It is used internally to carry the current state of that package as
 * required by the installation processes (validation, configuration,
 * unpacking.)
 *
 * The item has the ability to handle packages that are neither install
 * nor even in existence. It is used to register the list of packages
 * the user wants to install (and thus these may not even exist,)
 * the packages found in the target, the packages found in repositories.
 *
 */


package_item_t::package_item_t( wpkgar_manager::pointer_t manager, const wpkg_filename::uri_filename& filename, package_type_t type)
    : f_manager(manager)
    , f_filename(filename)
    , f_type(type)
    //, f_ctrl(NULL) -- auto-init
    //, f_fields(NULL) -- auto-init
    //, f_loaded(false) -- auto-init
    //, f_depends_done(false) -- auto-init
    //, f_unpacked(false) -- auto-init
    //, f_name("") -- auto-init
    //, f_architecture("") -- auto-init
    //, f_version("") -- auto-init
    , f_upgrade(-1) // no upgrade
{
}

package_item_t::package_item_t( wpkgar_manager::pointer_t manager, const wpkg_filename::uri_filename& filename, package_type_t type, const memfile::memory_file& ctrl)
    : f_manager(manager)
    , f_filename(filename)
    , f_type(type)
    , f_ctrl(new memfile::memory_file)
    //, f_fields(NULL) -- auto-init
    //, f_loaded(false) -- auto-init
    //, f_depends_done(false) -- auto-init
    //, f_unpacked(false) -- auto-init
    //, f_name("") -- auto-init
    //, f_architecture("") -- auto-init
    //, f_version("") -- auto-init
    , f_upgrade(-1) // no upgrade
{
    ctrl.copy(*f_ctrl);
}

void package_item_t::load( bool ctrl )
{
    // if we are only interested in a control file and it is available, then
    // load that data from the accompanying control file
    if( ctrl && f_ctrl )
    {
        if( load_state_not_loaded == f_loaded )
        {
            f_fields.reset(new wpkg_control::binary_control_file(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::control_file_state_t)));
            f_fields->set_input_file(&*f_ctrl);
            f_fields->read();
            f_fields->set_input_file(NULL);
            f_name            = f_fields->get_field(wpkg_control::control_file::field_package_factory_t::canonicalized_name());
            f_architecture    = f_fields->get_field(wpkg_control::control_file::field_architecture_factory_t::canonicalized_name());
            f_version         = f_fields->get_field(wpkg_control::control_file::field_version_factory_t::canonicalized_name());
            f_original_status = wpkgar_manager::unknown; // temporary packages have an unknown status by default
            f_loaded          = load_state_control_file;
        }
        return;
    }

    if( load_state_full != f_loaded )
    {
        f_manager->load_package(f_filename);
        if(load_state_control_file != f_loaded)
        {
            f_name         = f_manager->get_field(f_filename, wpkg_control::control_file::field_package_factory_t::canonicalized_name());
            f_architecture = f_manager->get_field(f_filename, wpkg_control::control_file::field_architecture_factory_t::canonicalized_name());
            f_version      = f_manager->get_field(f_filename, wpkg_control::control_file::field_version_factory_t::canonicalized_name());
        }
        f_original_status = f_manager->package_status(f_filename);
        f_loaded          = load_state_full;
    }
}

const wpkg_filename::uri_filename& package_item_t::get_filename() const
{
    return f_filename;
}

void package_item_t::set_type(const package_type_t type)
{
    f_type = type;
}

package_item_t::package_type_t package_item_t::get_type() const
{
    return f_type;
}

const std::string& package_item_t::get_name() const
{
    const_cast<package_item_t *>(this)->load(true);
    return f_name;
}

const std::string& package_item_t::get_architecture() const
{
    const_cast<package_item_t *>(this)->load(true);
    return f_architecture;
}

const std::string& package_item_t::get_version() const
{
    const_cast<package_item_t *>(this)->load(true);
    return f_version;
}

wpkgar_manager::package_status_t package_item_t::get_original_status() const
{
    const_cast<package_item_t *>(this)->load(true);
    return f_original_status;
}

bool package_item_t::field_is_defined(const std::string& name) const
{
    const_cast<package_item_t *>(this)->load(true);
    if(load_state_full == f_loaded)
    {
        return f_manager->field_is_defined(f_filename, name);
    }
    if(f_fields)
    {
        return f_fields->field_is_defined(name);
    }
    return false;
}

std::string package_item_t::get_field(const std::string& name) const
{
    const_cast<package_item_t *>(this)->load(true);
    if(load_state_full == f_loaded)
    {
        return f_manager->get_field(f_filename, name);
    }
    return f_fields->get_field(name);
}

bool package_item_t::get_boolean_field(const std::string& name) const
{
    const_cast<package_item_t *>(this)->load(true);
    if(load_state_full == f_loaded)
    {
        return f_manager->get_field_boolean(f_filename, name);
    }
    // Coverage Note:
    //   Cannot be reached because at this point we test boolean fields
    //   only of installed packages.
    if(f_fields)
    {
        return f_fields->get_field_boolean(name);
    }
    return false;
}

bool package_item_t::validate_fields(const std::string& expression) const
{
    const_cast<package_item_t *>(this)->load(true);
    if(load_state_full == f_loaded)
    {
        return f_manager->validate_fields(f_filename, expression);
    }
    if(f_fields)
    {
        return f_fields->validate_fields(expression);
    }
    return false;
}

bool package_item_t::is_conffile(const std::string& path) const
{
    const_cast<package_item_t *>(this)->load(false);
    return const_cast<package_item_t *>(this)->f_manager->is_conffile(f_filename, path);
}

void package_item_t::set_upgrade(int32_t upgrade)
{
    f_upgrade = upgrade;
}

int32_t package_item_t::get_upgrade() const
{
    return f_upgrade;
}

void package_item_t::mark_unpacked()
{
    f_unpacked = true;
}

bool package_item_t::is_unpacked() const
{
    return f_unpacked;
}

/** \brief Check whether a package is marked for installation.
 *
 * This function returns true if the current package type is set to
 * a value that represents a package that is to be installed.
 *
 * \return true if this package it to be installed.
 */
bool package_item_t::is_marked_for_install() const
{
    switch(f_type)
    {
    case package_item_t::package_type_explicit:
    case package_item_t::package_type_implicit:
    case package_item_t::package_type_configure:
    case package_item_t::package_type_upgrade:
    case package_item_t::package_type_upgrade_implicit:
    case package_item_t::package_type_downgrade:
        return true;

    default:
        return false;

    }
}



/** \brief Copy the package from the temporary directory to the database.
 *
 * This function copies the data from a temporary package to its
 * database under the --instdir directory.
 *
 * If hooks are detected, then these get installed in the core/hooks/...
 * directory as expected.
 *
 * The function also updates the index.wpkgar file with all the new files
 * found in the final directory.
 *
 * The md5sums file has some special handling so we are still able to check
 * old md5sums after the copy.
 *
 * \note
 * The function fails if the database directory includes a file of the same
 * name as the package being copied.
 */
void package_item_t::copy_package_in_database()
{
    // create a copy of this package in the database
    // the package must be an explicit or implicit
    // package (i.e. not a package that's already
    // installed or non-existent)

    // make sure the package was loaded (frankly, if not by now, wow!)
    load(false);

    // first check whether it exists, if so make sure it is a directory
    wpkg_filename::uri_filename dir(f_manager->get_database_path().append_child(f_name));
    if(dir.exists())
    {
        // directory already exists!?
        // (this happens whenever we upgrade or re-installed)
        if(!dir.is_dir())
        {
            throw wpkgar_exception_locked("the package \"" + f_name + "\" cannot be created because a regular file with that name exists in the database.");
        }
        // this is an upgrade, replace the existing folder
        // (although we keep a copy of the old md5sums!)
    }
    else
    {
        dir.os_mkdir_p();
    }

    // now go through the files in the existing package folder
    // (the temporary folder) and copy them to the new folder
    memfile::memory_file temp;
    wpkg_filename::uri_filename temp_path(f_manager->get_package_path(f_filename));
    temp.dir_rewind(temp_path, false);
    bool has_old_md5sums(false);
    for(;;)
    {
        memfile::memory_file::file_info info;
        memfile::memory_file data;
        if(!temp.dir_next(info, &data))
        {
            break;
        }
        if(info.get_file_type() != memfile::memory_file::file_info::regular_file)
        {
            // we're only interested by regular files, anything
            // else we skip silently (that includes "." and "..")
            continue;
        }
#if defined(MO_WINDOWS)
        case_insensitive::case_insensitive_string basename(info.get_basename());
#else
        const std::string& basename(info.get_basename());
#endif
        const wpkg_filename::uri_filename destination(dir.append_child(basename));

        // we actually ignore those status files because otherwise upgrades
        // may smash out the main ("real") status file
        if(basename == "wpkg-status" && destination.exists())
        {
            continue;
        }

        if(basename == "md5sums")
        {
            // we want to keep a copy of the old md5sums in order
            // to determine whether the configuration files changed
            // or not; do an unlink() first because rename fails
            // otherwise, we ignore errors on unlink()
            wpkg_filename::uri_filename old(destination.append_path(".wpkg-old"));
            old.os_unlink();
            // if we're not upgrading the rename() fails too
            has_old_md5sums = destination.os_rename(old, false);
        }
        data.write_file(destination);
    }

    memfile::memory_file wpkgar_file_out;
    wpkgar_file_out.create(memfile::memory_file::file_format_wpkg);
    wpkgar_file_out.set_package_path(dir);
    memfile::memory_file wpkgar_file_in;
    wpkgar_file_in.read_file(dir.append_child("index.wpkgar"));
    wpkgar_file_in.dir_rewind();
    for(;;)
    {
        memfile::memory_file::file_info info;
        if(!wpkgar_file_in.dir_next(info, NULL))
        {
            break;
        }
        // avoid the md5sums.wpkg-old file
        if(info.get_basename() != "md5sums.wpkg-old")
        {
            memfile::memory_file data;
            wpkgar_file_out.append_file(info, data);
        }
    }
    if(has_old_md5sums)
    {
        // if it exists, save it in the index
        memfile::memory_file::file_info info;
        info.set_mode(0444);
        info.set_user("Administrator");
        info.set_group("Administrators");
        info.set_filename("md5sums.wpkg-old");
        const wpkg_filename::uri_filename destination(dir.append_child("md5sums.wpkg-old"));
        wpkg_filename::uri_filename::file_stat s;
        destination.os_stat(s);
        info.set_size(static_cast<int>(s.get_size()));
        memfile::memory_file data;
        wpkgar_file_out.append_file(info, data);
    }
    wpkgar_file_out.write_file(dir.append_child("index.wpkgar"));

    // we can now load this like an installed package!
    f_manager->load_package(f_name, true);

    // and just in case, install the global hooks if any
    f_manager->install_hooks(f_name);
}

}
// namespace installer

}
// namespace wpkgar
// vim: ts=4 sw=4 et
