/*    wpkgar_install.cpp -- install a debian package
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
#include    "libdebpackages/wpkgar_install.h"
#include    "libdebpackages/wpkgar_repository.h"
#include    "libdebpackages/debian_version.h"
#include    "libdebpackages/wpkg_backup.h"
#include    "libdebpackages/wpkg_util.h"
#include    "libdebpackages/debian_packages.h"
#include    <algorithm>
#include    <set>
#include    <fstream>
#include    <iostream>
#include    <sstream>
#include    <stdarg.h>
#include    <errno.h>
#include    <time.h>
#include    <cstdlib>
#if !defined(MO_WINDOWS)
#	if defined(MO_LINUX)
#		include    <mntent.h>
#	endif
#	include    <sys/statvfs.h>
#	include    <unistd.h>
#endif
#if defined(MO_CYGWIN)
#include    <Windows.h>
#endif


namespace wpkgar
{


/** \class wpkgar_install
 * \brief The package install manager.
 *
 * This class defines the functions necessary to install a package.
 * Note that before any installation can occur, the system needs to
 * calculate what needs to be done. This is done through the validation
 * process.
 *
 * In most cases, you want to create a wpkgar_install object, then add
 * one or more packages to be installed, then run validate() to make sure
 * that it will install properly. If validate() returns true, then you
 * can run pre_configure() and finally, unpack(), and configure() for
 * each package to be installed.
 *
 * Note that it is possible to add a package that will not get installed
 * because that very version is already installed and the
 * --skip-same-version was used.
 */


/** \class wpkgar_install::install_info_t
 * \brief One item in the list of packages to be installed.
 *
 * After running the validation process, the list of packages that is to be
 * installed is known by the wpkgar_install object. This list can be
 * retrieved with the use of the wpkgar_install::get_install_list() function.
 * That function returns a vector of install_info_t objects which give you
 * the name and version of the package, whether it is installed explicitly or
 * implicitly, and if the installation is a new installation or an
 * upgrade (i.e. see the is_upgrade() function.)
 *
 * Note that the function used to retrieve this list does not return the
 * other packages (i.e. packages that are available in a repository, already
 * installed, etc. are not returned in this list.)
 */


/** \class wpkgar_install::package_item_t
 * \brief A package object in the wpkgar_install object.
 *
 * The package object defined in the wpkgar_install object is private. It
 * is used internally to carry the current state of that package as
 * required by the installation processes (validation, configuration,
 * unpacking.)
 *
 * The item has the ability to handle packages that are neither install
 * nor even in existence. It is used to register the list of packages
 * the user wants to install (and thus these may not even exist,)
 * the packages found in the target, the packages found in repositories.
 */


/** \class wpkgar_install::tree_generator
 * \brief Generate all possible permutations of the package tree.
 *
 * Lazily generates all possible permutations of the package tree, such
 * that only one version of any named package is installable. The resulting
 * permutations are not guaranteed to be valid, checking the validity is
 * done afterward.
 *
 * Uses the cartesian product algorithm described here:
 *   http://phrogz.net/lazy-cartesian-product
 *
 * \todo
 * Hack out the wpkgar_package_list_t-specific lazy cartesian product
 * generator and turn it into a generic one.
 */






wpkgar_install::package_item_t::package_item_t(wpkgar_manager *manager, const wpkg_filename::uri_filename& filename, package_type_t type)
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

wpkgar_install::package_item_t::package_item_t(wpkgar_manager *manager, const wpkg_filename::uri_filename& filename, package_type_t type, const memfile::memory_file& ctrl)
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

void wpkgar_install::package_item_t::load(bool ctrl)
{
    // if we are only interested in a control file and it is available, then
    // load that data from the accompanying control file
    if(ctrl && f_ctrl)
    {
        if(load_state_not_loaded == f_loaded)
        {
            f_fields.reset(new wpkg_control::binary_control_file(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::control_file_state_t)));
            f_fields->set_input_file(&*f_ctrl);
            f_fields->read();
            f_fields->set_input_file(NULL);
            f_name = f_fields->get_field(wpkg_control::control_file::field_package_factory_t::canonicalized_name());
            f_architecture = f_fields->get_field(wpkg_control::control_file::field_architecture_factory_t::canonicalized_name());
            f_version = f_fields->get_field(wpkg_control::control_file::field_version_factory_t::canonicalized_name());
            f_original_status = wpkgar_manager::unknown; // temporary packages have an unknown status by default
            f_loaded = load_state_control_file;
        }
        return;
    }

    if(load_state_full != f_loaded)
    {
        f_manager->load_package(f_filename);
        if(load_state_control_file != f_loaded)
        {
            f_name = f_manager->get_field(f_filename, wpkg_control::control_file::field_package_factory_t::canonicalized_name());
            f_architecture = f_manager->get_field(f_filename, wpkg_control::control_file::field_architecture_factory_t::canonicalized_name());
            f_version = f_manager->get_field(f_filename, wpkg_control::control_file::field_version_factory_t::canonicalized_name());
        }
        f_original_status = f_manager->package_status(f_filename);
        f_loaded = load_state_full;
    }
}

const wpkg_filename::uri_filename& wpkgar_install::package_item_t::get_filename() const
{
    return f_filename;
}

void wpkgar_install::package_item_t::set_type(const package_type_t type)
{
    f_type = type;
}

wpkgar_install::package_item_t::package_type_t wpkgar_install::package_item_t::get_type() const
{
    return f_type;
}

const std::string& wpkgar_install::package_item_t::get_name() const
{
    const_cast<package_item_t *>(this)->load(true);
    return f_name;
}

const std::string& wpkgar_install::package_item_t::get_architecture() const
{
    const_cast<package_item_t *>(this)->load(true);
    return f_architecture;
}

const std::string& wpkgar_install::package_item_t::get_version() const
{
    const_cast<package_item_t *>(this)->load(true);
    return f_version;
}

wpkgar_manager::package_status_t wpkgar_install::package_item_t::get_original_status() const
{
    const_cast<package_item_t *>(this)->load(true);
    return f_original_status;
}

bool wpkgar_install::package_item_t::field_is_defined(const std::string& name) const
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

std::string wpkgar_install::package_item_t::get_field(const std::string& name) const
{
    const_cast<package_item_t *>(this)->load(true);
    if(load_state_full == f_loaded)
    {
        return f_manager->get_field(f_filename, name);
    }
    return f_fields->get_field(name);
}

bool wpkgar_install::package_item_t::get_boolean_field(const std::string& name) const
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

bool wpkgar_install::package_item_t::validate_fields(const std::string& expression) const
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

bool wpkgar_install::package_item_t::is_conffile(const std::string& path) const
{
    const_cast<package_item_t *>(this)->load(false);
    return const_cast<package_item_t *>(this)->f_manager->is_conffile(f_filename, path);
}

void wpkgar_install::package_item_t::set_upgrade(int32_t upgrade)
{
    f_upgrade = upgrade;
}

int32_t wpkgar_install::package_item_t::get_upgrade() const
{
    return f_upgrade;
}

void wpkgar_install::package_item_t::mark_unpacked()
{
    f_unpacked = true;
}

bool wpkgar_install::package_item_t::is_unpacked() const
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
bool wpkgar_install::package_item_t::is_marked_for_install() const
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
void wpkgar_install::package_item_t::copy_package_in_database()
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






// ----------------------------------------------------------------------------
// tree_generator implementation
// ----------------------------------------------------------------------------


/** \brief Initialize a tree generator object.
 *
 * This function pre-computes indices to make generating the
 * cartesian product of the package options a bit easier
 * later on.
 *
 * \attention
 * The behaviour is undefined if the order of the packages in the
 * master tree is changed while the tree_generator exists.
 *
 * \param[in] root_tree An immutable reference to the master package tree that
 *                      will serve as the source of the tree permutations.
 */
wpkgar_install::tree_generator::tree_generator(const wpkgar_package_list_t& root_tree)
    : f_master_tree(root_tree)
    //, f_pkg_alternatives() -- auto-init
    //, f_divisor() -- auto-init
    //, f_n(0) -- auto-init
    //, f_end(0) -- auto-init
{
    std::set<std::string> visited_packages;

    // Pre-compute the alternatives lists that we can then simply walk over
    // to generate the various permutations of the tree later on.
    for(wpkgar_package_index_t item_idx(0); item_idx < f_master_tree.size(); ++item_idx)
    {
        const package_item_t& pkg(f_master_tree[item_idx]);
        const std::string pkg_name(pkg.get_name());

        // have we already dealt with all the packages by this name?
        if(!visited_packages.insert(pkg_name).second)
        {
            continue;
        }

        pkg_alternatives_t alternatives;
        if(pkg.get_type() == package_item_t::package_type_available)
        {
            alternatives.push_back(item_idx);
        }

        for(wpkgar_package_index_t candidate_idx(0); candidate_idx < f_master_tree.size(); ++candidate_idx)
        {
            if(item_idx == candidate_idx)
            {
                // ignore self
                continue;
            }

            const package_item_t& candidate(f_master_tree[candidate_idx]);
            if(candidate.get_type() != package_item_t::package_type_available)
            {
                continue;
            }

            const std::string candidate_name(candidate.get_name());
            if(pkg_name == candidate_name)
            {
                alternatives.push_back(candidate_idx);
            }
        }

        if(!alternatives.empty())
        {
            f_pkg_alternatives.push_back(alternatives);
        }
    }

    // Pre-compute the divisors that we need to walk the list-of-alternatives-
    // lists such that we end up with the cartesian product of all the
    // alternatives.
    wpkgar_package_index_t factor(1);
    f_divisor.resize(f_pkg_alternatives.size());
    pkg_alternatives_list_t::size_type i(f_pkg_alternatives.size());
    while(i > 0)
    {
        --i;
        f_divisor[static_cast<wpkgar_package_index_t>(i)] = factor;
        factor = f_pkg_alternatives[static_cast<wpkgar_package_index_t>(i)].size() * factor;
    }
    f_end = factor;
}


/** \brief Computer the next permutation.
 *
 * This function computes and returns the next permutation of the master
 * package tree using the data generated in the constructor.
 *
 * \returns Returns a wpkgar_package_list_t where exactly one version of any
 *          given package is enabled. Returns an empty wpkgar_package_list_t
 *          when all possibilities have been exhausted.
 */
wpkgar_install::wpkgar_package_list_t wpkgar_install::tree_generator::next()
{
    wpkgar_package_list_t result;

    if(f_n < f_end)
    {
        result = f_master_tree;

        // for each group of version-specific alternatives ...
        for(wpkgar_package_index_t pkg_set(0); pkg_set < f_pkg_alternatives.size(); ++pkg_set)
        {
            // select one alternative from the list ...
            const pkg_alternatives_t& options(f_pkg_alternatives[pkg_set]);
            const wpkgar_package_index_t target_idx((f_n / f_divisor[pkg_set]) % options.size());

            // ... and mark the rest as invalid.
            for(wpkgar_package_index_t option_idx(0); option_idx < options.size(); ++option_idx)
            {
                if(option_idx != target_idx)
                {
                    // all except self
                   result[options[option_idx]].set_type(package_item_t::package_type_invalid);
                }
            }
        }

        f_n = f_n + 1;
    }

    return result;
}


/** \brief Return the current tree number.
 *
 * This function returns the number of the last tree returned by the
 * next() function. If the next() function was never called, then
 * the function returns zero. It can also return zero if no tree can
 * be generated.
 *
 * \warning
 * This means the tree number is 1 based which in C++ is uncommon!
 *
 * \return The current tree.
 */
uint64_t wpkgar_install::tree_generator::tree_number() const
{
    return f_n;
}






wpkgar_install::wpkgar_install(wpkgar_manager *manager)
    : f_manager(manager)
    //, f_list_installed_packages() -- auto-init
    //, f_flags() -- auto-init
    //, f_architecture("") -- auto-init
    //, f_original_status() -- auto-init
    //, f_packages() -- auto-init
    //, f_sorted_packages() -- auto-init
    //, f_repository() -- auto-init
    //, f_installing_packages(true) -- auto-init
    //, f_unpacking_packages(false) -- auto-init
    //, f_reconfiguring_packages(false) -- auto-init
    //, f_repository_packages_loaded(false) -- auto-init
    //, f_install_includes_choices(false) -- auto-init
    //, f_tree_max_depth(0) -- auto-init
    //, f_essential_files() -- auto-init
    //, f_field_validations() -- auto-init
    //, f_field_names() -- auto-init
    //, f_read_essentials(false) -- auto-init
    //, f_install_source(false) -- auto-init
{
}


void wpkgar_install::set_parameter(parameter_t flag, int value)
{
    f_flags[flag] = value;
}


int wpkgar_install::get_parameter(parameter_t flag, int default_value) const
{
    wpkgar_flags_t::const_iterator it(f_flags.find(flag));
    if(it == f_flags.end())
    {
        // This line is not currently used from wpkg because all the
        // parameters are always all defined from command line arguments
        return default_value; // LCOV_EXCL_LINE
    }
    return it->second;
}


void wpkgar_install::set_installing()
{
    f_installing_packages = true;
    f_unpacking_packages = false;
    f_reconfiguring_packages = false;
}


void wpkgar_install::set_configuring()
{
    f_installing_packages = false;
    f_unpacking_packages = false;
    f_reconfiguring_packages = false;
}


void wpkgar_install::set_reconfiguring()
{
    f_installing_packages = false;
    f_unpacking_packages = false;
    f_reconfiguring_packages = true;
}


void wpkgar_install::set_unpacking()
{
    f_installing_packages = true;
    f_unpacking_packages = true;
    f_reconfiguring_packages = false;
}


wpkgar_install::wpkgar_package_list_t::const_iterator wpkgar_install::find_package_item(const wpkg_filename::uri_filename& filename) const
{
    for(wpkgar_package_list_t::size_type i(0); i < f_packages.size(); ++i)
    {
        if(f_packages[i].get_filename().full_path() == filename.full_path())
        {
            return f_packages.begin() + i;
        }
    }
    return f_packages.end();
}


wpkgar_install::wpkgar_package_list_t::iterator wpkgar_install::find_package_item_by_name(const std::string& name)
{
    for(wpkgar_package_list_t::size_type i(0); i < f_packages.size(); ++i)
    {
        if(f_packages[i].get_name() == name)
        {
            return f_packages.begin() + i;
        }
    }
    return f_packages.end();
}




/** \brief Add one expression to run against all the packages to be installed.
 *
 * This function accepts one C-like expression that will be run against
 * all the packages that are about to be installed either explicitly
 * or implicitly.
 *
 * The expressions are not run against already installed packages.
 *
 * At this time there is no function offered to clear this list.
 *
 * \param[in] expression  One expression to add to the installer.
 */
void wpkgar_install::add_field_validation(const std::string& expression)
{
    f_field_validations.push_back(expression);
}


void wpkgar_install::add_package( const std::string& package, const bool force_reinstall )
{
    const wpkg_filename::uri_filename pck(package);
    wpkgar_package_list_t::const_iterator item(find_package_item(pck));
    if(item != f_packages.end())
    {
        // the user is trying to install the same package twice
        // (ignore if that's exactly the same one)
        if(item->get_type() != package_item_t::package_type_explicit)
        {
            wpkg_output::log("package %1 defined twice on your command line using two different paths.")
                    .quoted_arg(package)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(package)
                .action("install-validation");
        }
    }
    else
    {
        if( pck.extension() == "deb")
        {
            // this is an explicit package
            package_item_t package_item( f_manager, pck );
            f_packages.push_back(package_item);
        }
        else
        {
            wpkgar_repository repository( f_manager );

            bool found_package = false;
            for( const auto& entry : repository.upgrade_list() )
            {
                if( entry.get_name() == package )
                {
                    found_package = true;
                    if( entry.get_status() == wpkgar_repository::package_item_t::invalid )
                    {
                        std::stringstream ss;
                        ss << "Cannot install package '" << package << "' since it is invalid!";
                        throw std::runtime_error(ss.str());
                    }

                    bool install_it = false;
                    if( force_reinstall )
                    {
                        install_it = true;
                    }
                    else if( entry.get_status() == wpkgar_repository::package_item_t::not_installed )
                    {
                        install_it = true;
                    }

                    if( install_it )
                    {
                        const std::string full_path( entry.get_info().get_uri().full_path() );
                        package_item_t package_item( f_manager, full_path );
                        f_packages.push_back(package_item);
                    }
                }
            }

            if( !found_package )
            {
                std::stringstream ss;
                ss << "Cannot install package '" << package << "' because it doesn't exist in the repository!";
                throw std::runtime_error(ss.str());
            }
        }
    }
}


const std::string& wpkgar_install::get_package_name(const int idx) const
{
    return f_packages[idx].get_name();
}


int wpkgar_install::count() const
{
    return static_cast<int>(f_packages.size());
}


// transform the directories in a list of .deb packages
bool wpkgar_install::validate_directories()
{
    // if not installing (--configure, --reconfigure) then there is nothing to test here
    if(!f_installing_packages)
    {
        // in this case all the package names must match installed packages
        return true;
    }

    // now go through all of those and add dependencies as expected
    for(wpkgar_package_list_t::size_type i(0); i < f_packages.size(); ++i)
    {
        f_manager->check_interrupt();

        // if we cannot access that file, it's probably a direct package
        // name in which case we're done here (another error should occur
        // for those since it's illegal)
        const wpkg_filename::uri_filename filename(f_packages[i].get_filename());
        if(filename.is_dir())
        {
            // this is a directory, so mark it as such
            f_packages[i].set_type(package_item_t::package_type_directory);

            // read the directory *.deb files
            memfile::memory_file r;
            r.dir_rewind(filename, get_parameter(wpkgar_install_recursive, false) != 0);
            for(;;)
            {
                f_manager->check_interrupt();

                memfile::memory_file::file_info info;
                if(!r.dir_next(info, NULL))
                {
                    break;
                }
                if(info.get_file_type() != memfile::memory_file::file_info::regular_file)
                {
                    // we are only interested by regular files, anything
                    // else we skip silently
                    continue;
                }
                const std::string& package_filename(info.get_filename());
                std::string::size_type p(package_filename.find_last_of('.'));
                if(p == std::string::npos || package_filename.substr(p + 1) != "deb")
                {
                    // if there is no extension or the extension is not .deb
                    // then forget it
                    continue;
                }
                if(package_filename.find_first_of("_/") == std::string::npos)
                {
                    wpkg_output::log("file %1 does not have a valid package name.")
                            .quoted_arg(package_filename)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(package_filename)
                        .action("install-validation");
                    continue;
                }
                add_package(package_filename);
            }
        }
    }

    // this can happen if the user specify an empty directory as input
    int size(0);
    for(wpkgar_package_list_t::size_type i(0); i < f_packages.size(); ++i)
    {
        if(f_packages[i].get_type() == package_item_t::package_type_explicit
        || f_packages[i].get_type() == package_item_t::package_type_implicit)
        {
            // we don't need to know how many total, just that there is at
            // least one so we break immediately
            ++size;
            break;
        }
    }
    if(size == 0)
    {
        wpkg_output::log("the directories you specified do not include any valid *.deb files, did you forget --recursive?")
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_validate_installation)
            .action("install-validation");
        return false;
    }

    return true;
}


// configuring: only already installed package names
// installing, unpacking, checking an install: only new package names
void wpkgar_install::validate_package_names()
{
    for(wpkgar_package_list_t::iterator it(f_packages.begin());
                it != f_packages.end(); ++it)
    {
        f_manager->check_interrupt();

        if(!it->get_filename().is_deb())
        {
            // this is a full package name (a .deb file)
            if(!f_installing_packages)
            {
                wpkg_output::log("package %1 cannot be used with --configure or --reconfigure.")
                        .quoted_arg(it->get_filename())
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(it->get_filename())
                    .action("install-validation");
            }
        }
        else
        {
            // this is an install name
            if(f_installing_packages)
            {
                wpkg_output::log("package %1 cannot be used with --install, --unpack, or --check-install.")
                        .quoted_arg(it->get_filename())
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(it->get_filename())
                    .action("install-validation");
            }
            else if(f_reconfiguring_packages)
            {
                it->load(false);
                switch(it->get_original_status())
                {
                case wpkgar_manager::not_installed:
                    wpkg_output::log("package %1 cannot be reconfigured since it is not currently installed.")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::config_files:
                    wpkg_output::log("package %1 was removed. Its configuration files are still available but the package cannot be reconfigured.")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::installed:
                    // perfect -- the type remains explicit
                    {
                        wpkg_control::control_file::field_xselection_t::selection_t selection( wpkgar_install::get_xselection( it->get_filename() ) );
                        if(selection == wpkg_control::control_file::field_xselection_t::selection_hold)
                        {
                            if(get_parameter(wpkgar_install_force_hold, false))
                            {
                                wpkg_output::log("package %1 is on hold, yet it will be reconfigured.")
                                        .quoted_arg(it->get_filename())
                                    .level(wpkg_output::level_warning)
                                    .module(wpkg_output::module_validate_installation)
                                    .package(it->get_filename())
                                    .action("install-validation");
                            }
                            else
                            {
                                wpkg_output::log("package %1 is on hold, it cannot be reconfigured.")
                                        .quoted_arg(it->get_filename())
                                    .level(wpkg_output::level_error)
                                    .module(wpkg_output::module_validate_installation)
                                    .package(it->get_filename())
                                    .action("install-validation");
                            }
                        }
                    }
                    break;

                case wpkgar_manager::unpacked:
                    wpkg_output::log("package %1 is not configured yet, it cannot be reconfigured.")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::no_package:
                    wpkg_output::log("package %1 cannot be configured in its current state.")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::unknown:
                    wpkg_output::log("package %1 has an unexpected status of \"unknown\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::half_installed:
                    wpkg_output::log("package %1 has an unexpected status of \"half-installed\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::installing:
                    wpkg_output::log("package %1 has an unexpected status of \"installing\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::upgrading:
                    wpkg_output::log("package %1 has an unexpected status of \"upgrading\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::half_configured:
                    wpkg_output::log("package %1 has an unexpected status of \"half-configured\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::removing:
                    wpkg_output::log("package %1 has an unexpected status of \"removing\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::purging:
                    wpkg_output::log("package %1 has an unexpected status of \"purging\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::listing:
                    wpkg_output::log("package %1 has an unexpected status of \"listing\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::verifying:
                    wpkg_output::log("package %1 has an unexpected status of \"verifying\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::ready:
                    wpkg_output::log("package %1 has an unexpected status of \"ready\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                }
            }
            else
            {
                it->load(false);
                switch(it->get_original_status())
                {
                case wpkgar_manager::not_installed:
                    wpkg_output::log("package %1 cannot be configured since it is not currently unpacked.")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::config_files:
                    wpkg_output::log("package %1 was removed. Its configuration files are still available but the package cannot be configured.")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::installed:
                    // accepted although there is nothing to do against already installed packages
                    wpkg_output::log("package %1 is already installed --configure will have no effect.")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_warning)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    it->set_type(package_item_t::package_type_same);
                    break;

                case wpkgar_manager::unpacked:
                    {
                        wpkg_control::control_file::field_xselection_t::selection_t selection( wpkgar_install::get_xselection( it->get_filename() ) );
                        if(selection == wpkg_control::control_file::field_xselection_t::selection_hold)
                        {
                            if(get_parameter(wpkgar_install_force_hold, false))
                            {
                                wpkg_output::log("package %1 is on hold, yet it will be configured.")
                                        .quoted_arg(it->get_filename())
                                    .level(wpkg_output::level_warning)
                                    .module(wpkg_output::module_validate_installation)
                                    .package(it->get_filename())
                                    .action("install-validation");
                            }
                            else
                            {
                                wpkg_output::log("package %1 is on hold, it cannot be configured.")
                                        .quoted_arg(it->get_filename())
                                    .level(wpkg_output::level_error)
                                    .module(wpkg_output::module_validate_installation)
                                    .package(it->get_filename())
                                    .action("install-validation");
                            }
                        }
                    }
                    it->set_type(package_item_t::package_type_unpacked);
                    break;

                case wpkgar_manager::no_package:
                    wpkg_output::log("package %1 cannot be configured in its current state.")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::unknown:
                    wpkg_output::log("package %1 has an unexpected status of \"unknown\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::half_installed:
                    wpkg_output::log("package %1 has an unexpected status of \"half-installed\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::installing:
                    wpkg_output::log("package %1 has an unexpected status of \"installing\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::upgrading:
                    wpkg_output::log("package %1 has an unexpected status of \"upgrading\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::half_configured:
                    wpkg_output::log("package %1 has an unexpected status of \"half-configured\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::removing:
                    wpkg_output::log("package %1 has an unexpected status of \"removing\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::purging:
                    wpkg_output::log("package %1 has an unexpected status of \"purging\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::listing:
                    wpkg_output::log("package %1 has an unexpected status of \"listing\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::verifying:
                    wpkg_output::log("package %1 has an unexpected status of \"verifying\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                case wpkgar_manager::ready:
                    wpkg_output::log("package %1 has an unexpected status of \"ready\".")
                            .quoted_arg(it->get_filename())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(it->get_filename())
                        .action("install-validation");
                    break;

                }
            }
        }
    }
}


void wpkgar_install::installing_source()
{
    f_install_source = false;

    // if not installing (--configure, --reconfigure) then there is nothing to test here
    if(f_installing_packages)
    {
        for(wpkgar_package_list_t::const_iterator it(f_packages.begin());
                                                  it != f_packages.end();
                                                  ++it)
        {
            f_manager->check_interrupt();

            const std::string architecture(it->get_architecture());
            if(architecture == "src" || architecture == "source")
            {
                f_install_source = true;
                break;
            }
        }
    }
}


void wpkgar_install::validate_installed_packages()
{
    // read the names of all the installed packages
    f_manager->list_installed_packages(f_list_installed_packages);
    for(wpkgar_manager::package_list_t::const_iterator it(f_list_installed_packages.begin());
                                                       it != f_list_installed_packages.end();
                                                       ++it)
    {
        f_manager->check_interrupt();

        try
        {
            // this package is an installed package so we cannot
            // just load a control file from an index file; plus
            // at this point we do not know whether it will end
            // up in the f_packages vector
            f_manager->load_package(*it);
            package_item_t::package_type_t type(package_item_t::package_type_invalid);
            switch(f_manager->package_status(*it))
            {
            case wpkgar_manager::not_installed:
            case wpkgar_manager::config_files:
                // if not installed or just configuration files are available
                // then it is considered as uninstalled (for the installation
                // process cannot rely on such a package as a dependency!)
                type = package_item_t::package_type_not_installed;
                break;

            case wpkgar_manager::installed:
                // accepted as valid, be silent about all of those
                type = package_item_t::package_type_installed;
                break;

            case wpkgar_manager::unpacked:
                // fails later if it is a dependency as configuration is
                // required then, unless we have --force-configure-any
                type = package_item_t::package_type_unpacked;
                break;

            case wpkgar_manager::no_package:
                wpkg_output::log("somehow a folder named %1 found in your database does not represent an existing package.")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(*it)
                    .action("install-validation");
                break;

            case wpkgar_manager::unknown:
                wpkg_output::log("package %1 has an unexpected status of \"unknown\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(*it)
                    .action("install-validation");
                break;

            case wpkgar_manager::half_installed:
                wpkg_output::log("package %1 has an unexpected status of \"half-installed\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(*it)
                    .action("install-validation");
                break;

            case wpkgar_manager::installing:
                wpkg_output::log("package %1 has an unexpected status of \"installing\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(*it)
                    .action("install-validation");
                break;

            case wpkgar_manager::upgrading:
                wpkg_output::log("package %1 has an unexpected status of \"upgrading\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(*it)
                    .action("install-validation");
                break;

            case wpkgar_manager::half_configured:
                wpkg_output::log("package %1 has an unexpected status of \"half-configured\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(*it)
                    .action("install-validation");
                break;

            case wpkgar_manager::removing:
                wpkg_output::log("package %1 has an unexpected status of \"removing\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(*it)
                    .action("install-validation");
                break;

            case wpkgar_manager::purging:
                wpkg_output::log("package %1 has an unexpected status of \"purging\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(*it)
                    .action("install-validation");
                break;

            case wpkgar_manager::listing:
                wpkg_output::log("package %1 has an unexpected status of \"listing\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(*it)
                    .action("install-validation");
                break;

            case wpkgar_manager::verifying:
                wpkg_output::log("package %1 has an unexpected status of \"verifying\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(*it)
                    .action("install-validation");
                break;

            case wpkgar_manager::ready:
                wpkg_output::log("package %1 has an unexpected status of \"ready\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(*it)
                    .action("install-validation");
                break;

            }

            // note: f_installing_packages is true if you are installing or
            //       unpacking
            if(f_installing_packages)
            {
                if(type == package_item_t::package_type_not_installed)
                {
                    // user may be attempting to install this package, make
                    // sure it is not marked as a "Reject" (X-Selection)
                    if(f_manager->field_is_defined(*it, wpkg_control::control_file::field_xselection_factory_t::canonicalized_name()))
                    {
                        wpkg_control::control_file::field_xselection_t::selection_t selection( wpkgar_install::get_xselection( *it ) );
                        wpkgar_package_list_t::iterator item(find_package_item_by_name(*it));
                        if(item != f_packages.end()
                        && item->get_type() == package_item_t::package_type_explicit
                        && selection == wpkg_control::control_file::field_xselection_t::selection_reject)
                        {
                            wpkg_output::log("package %1 is marked as rejected; use --set-selection to change its status first.")
                                    .quoted_arg(*it)
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_validate_installation)
                                .package(*it)
                                .action("install-validation");
                        }
                    }
                }
                else
                {
                    // with --unpack we can do nearly everything:
                    //   1. from Not Installed to Unpacked
                    //   2. from Unpacked to Unpacked
                    //   3. from Installed to Unpacked
                    //   4. from Conf-Files to Unpacked
                    //
                    // with --install we can do many things too:
                    //   1. from Not Installed to Installed
                    //   2. from Unpacked to Installed -- this we actually prevent and force a --configure instead (correct?)
                    //   3. from Installed to Installed -- i.e. overwrite (same version), upgrade, or downgrade
                    //   4. from Conf-Files to Installed (re-unpack and re-configure)

                    // IMPORTANT: note that *it is a name (Package field), not a path, in this case
                    wpkgar_package_list_t::iterator item(find_package_item_by_name(*it));
                    if(item != f_packages.end())
                    {
                        // the user is doing an update, an overwrite, or a downgrade
                        // it must be from an explicit package; note that implicit
                        // packages are not yet defined here
                        if(item->get_type() != package_item_t::package_type_explicit)
                        {
                            // at this point the existing items MUST be explicit or
                            // something is really wrong
                            wpkg_output::log("package %1 found twice in the existing installation.")
                                    .quoted_arg(*it)
                                .level(wpkg_output::level_fatal)
                                .module(wpkg_output::module_validate_installation)
                                .package(*it)
                                .action("install-validation");
                        }
                        if(!f_unpacking_packages)
                        {
                            // with --install we cannot upgrade a package that was just unpacked.
                            if(type == package_item_t::package_type_unpacked)
                            {
                                // you cannot update/upgrade an unpacked package with --install, it needs configuration
                                if(get_parameter(wpkgar_install_force_configure_any, false))
                                {
                                    wpkg_output::log("package %1 is unpacked, it will be configured before getting upgraded.")
                                            .quoted_arg(*it)
                                        .level(wpkg_output::level_warning)
                                        .module(wpkg_output::module_validate_installation)
                                        .package(*it)
                                        .action("install-validation");
                                    item->set_type(package_item_t::package_type_configure);
                                    // we do not change the package 'type' on purpose
                                    // it will be checked again in the if() below
                                }
                                else
                                {
                                    wpkg_output::log("package %1 is unpacked, it cannot be updated with --install. Try --configure first, or use --unpack.")
                                            .quoted_arg(*it)
                                        .level(wpkg_output::level_error)
                                        .module(wpkg_output::module_validate_installation)
                                        .package(*it)
                                        .action("install-validation");
                                }
                            }
                        }
                        if(item->get_type() == package_item_t::package_type_explicit)
                        {
                            // Note: using f_manager directly since the package is not
                            //       yet in the f_packages vector
                            wpkg_control::control_file::field_xselection_t::selection_t selection( wpkgar_install::get_xselection( *it ) );
                            std::string vi(f_manager->get_field(*it, wpkg_control::control_file::field_version_factory_t::canonicalized_name()));
                            std::string vo(item->get_version());
                            const int c(wpkg_util::versioncmp(vi, vo));
                            if(c == 0)
                            {
                                if(get_parameter(wpkgar_install_skip_same_version, false))
                                {
                                    // package is already installed, user asked to skip it
                                    item->set_type(package_item_t::package_type_same);
                                }
                                else
                                {
                                    // allow normal unpack (i.e. overwrite)
                                    type = package_item_t::package_type_upgrade;
                                }
                            }
                            else if(c < 0)
                            {
                                if(selection == wpkg_control::control_file::field_xselection_t::selection_hold)
                                {
                                    if(get_parameter(wpkgar_install_force_hold, false))
                                    {
                                        wpkg_output::log("package %1 is being upgraded even though it is on hold.")
                                                .quoted_arg(*it)
                                            .level(wpkg_output::level_warning)
                                            .module(wpkg_output::module_validate_installation)
                                            .package(*it)
                                            .action("install-validation");
                                    }
                                    else
                                    {
                                        wpkg_output::log("package %1 is not getting upgraded because it is on hold.")
                                                .quoted_arg(*it)
                                            .level(wpkg_output::level_error)
                                            .module(wpkg_output::module_validate_installation)
                                            .package(*it)
                                            .action("install-validation");
                                    }
                                }

                                if(item->field_is_defined(wpkg_control::control_file::field_minimumupgradableversion_factory_t::canonicalized_name()))
                                {
                                    const std::string minimum_version(item->get_field(wpkg_control::control_file::field_minimumupgradableversion_factory_t::canonicalized_name()));
                                    const int m(wpkg_util::versioncmp(vi, minimum_version));
                                    if(m < 0)
                                    {
                                        if(get_parameter(wpkgar_install_force_upgrade_any_version, false))
                                        {
                                            wpkg_output::log("package %1 version %2 is being upgraded even though the Minimum-Upgradable-Version says it won't work right since it was not upgraded to at least version %3 first.")
                                                    .quoted_arg(*it)
                                                    .arg(vi)
                                                    .arg(minimum_version)
                                                .level(wpkg_output::level_warning)
                                                .module(wpkg_output::module_validate_installation)
                                                .package(*it)
                                                .action("install-validation");
                                        }
                                        else
                                        {
                                            wpkg_output::log("package %1 version %2 is not getting upgraded because the Minimum-Upgradable-Version says it won't work right without first upgrading it to at least version %3.")
                                                    .quoted_arg(*it)
                                                    .arg(vi)
                                                    .arg(minimum_version)
                                                .level(wpkg_output::level_error)
                                                .module(wpkg_output::module_validate_installation)
                                                .package(*it)
                                                .action("install-validation");
                                        }
                                    }
                                }

                                // normal upgrade
                                type = package_item_t::package_type_upgrade;
                            }
                            else
                            {
                                // user is trying to downgrade
                                if(get_parameter(wpkgar_install_force_downgrade, false))
                                {
                                    if(selection == wpkg_control::control_file::field_xselection_t::selection_hold)
                                    {
                                        if(get_parameter(wpkgar_install_force_hold, false))
                                        {
                                            wpkg_output::log("package %1 is being downgraded even though it is on hold.")
                                                    .quoted_arg(*it)
                                                .level(wpkg_output::level_warning)
                                                .module(wpkg_output::module_validate_installation)
                                                .package(*it)
                                                .action("install-validation");
                                        }
                                        else
                                        {
                                            wpkg_output::log("package %1 is not getting downgraded because it is on hold.")
                                                    .quoted_arg(*it)
                                                .level(wpkg_output::level_error)
                                                .module(wpkg_output::module_validate_installation)
                                                .package(*it)
                                                .action("install-validation");
                                        }
                                    }

                                    // at this time it's just a warning but a dependency
                                    // version may break because of this
                                    wpkg_output::log("package %1 is being downgraded which may cause some dependency issues.")
                                            .quoted_arg(*it)
                                        .level(wpkg_output::level_warning)
                                        .module(wpkg_output::module_validate_installation)
                                        .package(*it)
                                        .action("install-validation");
                                    // unexpected downgrade
                                    type = package_item_t::package_type_downgrade;
                                }
                                else
                                {
                                    wpkg_output::log("package %1 cannot be downgraded.")
                                            .quoted_arg(*it)
                                        .level(wpkg_output::level_error)
                                        .module(wpkg_output::module_validate_installation)
                                        .package(*it)
                                        .action("install-validation");
                                }
                            }
                        }
                    }
                    // add the result, but only if installing or unpacking
                    // (i.e. in most cases this indicates an installed package)
                    package_item_t package_item(f_manager, *it, type);
                    f_packages.push_back(package_item);
                }
            }
        }
        catch(const std::exception& e)
        {
            wpkg_output::log("installed package %1 could not be loaded (%2).")
                    .quoted_arg(*it)
                    .arg(e.what())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(*it)
                .action("install-validation");
        }
    }
}


/** \brief Validate the distribution field.
 *
 * This function checks whether a distribution field is defined in the
 * core package (i.e. the database control settings.) If so, then the
 * name of that field has to match all the packages that are about
 * to be installed implicitly or explicitly.
 */
void wpkgar_install::validate_distribution()
{
    // if the Distribution field is not defined for that target
    // then we're done here
    if(!f_manager->field_is_defined("core", "Distribution"))
    {
        return;
    }

    const std::string distribution(f_manager->get_field("core", "Distribution"));
    for( const auto& package : f_packages )
    {
        f_manager->check_interrupt();

        switch(package.get_type())
        {
        case package_item_t::package_type_explicit:
        case package_item_t::package_type_implicit:
            // we only check the explicit and implicit packages; a package
            // that is already installed (upgrade/downgrade) may have a
            // foreign distribution and that's okay because the administrator
            // may have used --force-distribution for those packages
            {
                // note that the Distribution field restriction does not
                // apply to source packages
                const std::string architecture(package.get_architecture());
                if(architecture != "source" && architecture != "src")
                {
                    const wpkg_filename::uri_filename filename(package.get_filename());

                    // is the Distribution field defined?
                    if(!package.field_is_defined("Distribution"))
                    {
                        if(get_parameter(wpkgar_install_force_distribution, false))
                        {
                            wpkg_output::log("package %1 is missing the Distribution field.")
                                    .quoted_arg(filename)
                                .level(wpkg_output::level_warning)
                                .module(wpkg_output::module_validate_installation)
                                .package(filename)
                                .action("install-validation");
                        }
                        else
                        {
                            wpkg_output::log("package %1 is missing the required Distribution field.")
                                    .quoted_arg(filename)
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_validate_installation)
                                .package(filename)
                                .action("install-validation");
                        }
                        break;
                    }

                    // match the distribution
                    const std::string d(package.get_field("Distribution"));
                    if(d != distribution)
                    {
                        if(get_parameter(wpkgar_install_force_distribution, false))
                        {
                            wpkg_output::log("package %1 may not be compatible with your installation target, it is for a different distribution: %2 instead of %3.")
                                    .quoted_arg(filename)
                                    .quoted_arg(d)
                                    .quoted_arg(distribution)
                                .level(wpkg_output::level_warning)
                                .module(wpkg_output::module_validate_installation)
                                .package(filename)
                                .action("install-validation");
                        }
                        else
                        {
                            wpkg_output::log("package %1 is not compatible with your installation target, it is for a different distribution: %2 instead of %3.")
                                    .quoted_arg(filename)
                                    .quoted_arg(d)
                                    .quoted_arg(distribution)
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_validate_installation)
                                .package(filename)
                                .action("install-validation");
                        }
                    }
                }
            }
            break;

        default:
            // ignore other packages as they are not going to be installed
            break;

        }
    }
}


void wpkgar_install::validate_architecture()
{
    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < f_packages.size();
                                         ++idx)
    {
        f_manager->check_interrupt();

        if(f_packages[idx].get_type() == package_item_t::package_type_explicit)
        {
            // match the architecture
            const std::string arch(f_packages[idx].get_architecture());
            // all and source architectures can always be installed
            if(arch != "all" && arch != "src" && arch != "source"
            && !wpkg_dependencies::dependencies::match_architectures(arch, f_architecture, get_parameter(wpkgar_install_force_vendor, false) != 0))
            {
                const wpkg_filename::uri_filename filename(f_packages[idx].get_filename());
                if(!get_parameter(wpkgar_install_force_architecture, false))
                {
                    wpkg_output::log("file %1 has an incompatible architecture (%2) for the current target (%3).")
                            .quoted_arg(filename)
                            .arg(arch)
                            .arg(f_architecture)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(filename)
                        .action("install-validation");
                }
                else
                {
                    wpkg_output::log("file %1 has an incompatible architecture (%2) for the current target (%3), but since --force-architecture was used it will anyway be installed.")
                            .quoted_arg(filename)
                            .arg(arch)
                            .arg(f_architecture)
                        .level(wpkg_output::level_warning)
                        .module(wpkg_output::module_validate_installation)
                        .package(filename)
                        .action("install-validation");
                }
            }
        }
    }
}


// if invalid, return -1
// if valid but not in range, return 0
// if valid and in range, return 1
int wpkgar_install::match_dependency_version(const wpkg_dependencies::dependencies::dependency_t& d, const package_item_t& item)
{
    // check the version if necessary
    if(!d.f_version.empty()
    && d.f_operator != wpkg_dependencies::dependencies::operator_any)
    {
        std::string version(item.get_field(wpkg_control::control_file::field_version_factory_t::canonicalized_name()));

        const int c(wpkg_util::versioncmp(version, d.f_version));

        bool r(false);
        switch(d.f_operator)
        {
        case wpkg_dependencies::dependencies::operator_any:
            throw std::logic_error("the operator_any cannot happen in match_dependency_version() unless the if() checking this value earlier is invalid");

        case wpkg_dependencies::dependencies::operator_lt:
            r = c < 0;
            break;

        case wpkg_dependencies::dependencies::operator_le:
            r = c <= 0;
            break;

        case wpkg_dependencies::dependencies::operator_eq:
            r = c == 0;
            break;

        case wpkg_dependencies::dependencies::operator_ne:
            throw std::runtime_error("the != operator is not legal in a control file.");

        case wpkg_dependencies::dependencies::operator_ge:
            r = c >= 0;
            break;

        case wpkg_dependencies::dependencies::operator_gt:
            r = c > 0;
            break;

        }
        return r ? 1 : 0;
    }

    return 1;
}


bool wpkgar_install::find_installed_predependency(const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d)
{
    // search for package d.f_name in the list of installed packages
    for(wpkgar_package_list_t::size_type idx(0); idx < f_packages.size(); ++idx)
    {
        if(d.f_name == f_packages[idx].get_name())
        {
            const wpkg_filename::uri_filename filename(f_packages[idx].get_filename());
            switch(f_packages[idx].get_type())
            {
            case package_item_t::package_type_installed:
            case package_item_t::package_type_unpacked:
                // the version check is required for both installed
                // and unpacked packages
                if(match_dependency_version(d, f_packages[idx]) != 1)
                {
                    if(!get_parameter(wpkgar_install_force_depends_version, false))
                    {
                        // should we mark the package as invalid (instead of explicit?)
                        // since we had an error it's probably not necessary?
                        wpkg_output::log("file %1 has an incompatible version for pre-dependency %2.")
                                .quoted_arg(filename)
                                .quoted_arg(d.to_string())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(package_name)
                            .action("install-validation");
                        return false;
                    }
                    // there is a version problem but the user is okay with it
                    // just generate a warning
                    wpkg_output::log("use file %1 even though it has an incompatible version for pre-dependency %2.")
                            .quoted_arg(filename)
                            .quoted_arg(d.to_string())
                        .level(wpkg_output::level_warning)
                        .module(wpkg_output::module_validate_installation)
                        .package(package_name)
                        .action("install-validation");
                }
                else
                {
                    // we got it in our list of installed packages, we're all good
                    wpkg_output::log("use file %1 to satisfy pre-dependency %2.")
                            .quoted_arg(filename)
                            .quoted_arg(d.to_string())
                        .debug(wpkg_output::debug_flags::debug_detail_config)
                        .module(wpkg_output::module_validate_installation)
                        .package(package_name);
                }
                if(f_packages[idx].get_type() == package_item_t::package_type_installed)
                {
                    return true;
                }

                // handle the package_item_t::package_type_unpacked case which
                // requires some additional tests
                if(get_parameter(wpkgar_install_force_configure_any, false))
                {
                    // user accepts auto-configurations so mark this package
                    // as requiring a pre-configuration
                    // (this will happen whatever tree will be selected later)
                    wpkg_output::log("file %1 has pre-dependency %2 which is not yet configured, wpkg will auto-configure it before the rest of the installation proceeds.")
                            .quoted_arg(filename)
                            .quoted_arg(d.to_string())
                        .level(wpkg_output::level_warning)
                        .module(wpkg_output::module_validate_installation)
                        .package(package_name)
                        .action("install-validation");
                    f_packages[idx].set_type(package_item_t::package_type_configure);
                    return true;
                }
                if(get_parameter(wpkgar_install_force_depends, false))
                {
                    // user accepts broken dependencies...
                    wpkg_output::log("file %1 has pre-dependency %2 but it is not yet configured and still accepted because you used --force-depends.")
                            .quoted_arg(filename)
                            .quoted_arg(d.to_string())
                        .level(wpkg_output::level_warning)
                        .module(wpkg_output::module_validate_installation)
                        .package(package_name)
                        .action("install-validation");
                    return true;
                }
                // dependency is broken, fail with an error
                wpkg_output::log("file %1 has pre-dependency %2 which still needs to be configured.")
                        .quoted_arg(filename)
                        .quoted_arg(d.to_string())
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(package_name)
                    .action("install-validation");
                return false;

            default:
                wpkg_output::log("file %1 has a pre-dependency (%2) which is not in a valid state to continue our installation (it was removed or purged?)")
                        .quoted_arg(filename)
                        .quoted_arg(d.f_name)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(filename)
                    .action("install-validation");
                return false;

            }
        }
    }

    // the file doesn't exist (is missing) but user may not care
    if(get_parameter(wpkgar_install_force_depends, false))
    {
        // user accepts broken dependencies...
        wpkg_output::log("package %1 has pre-dependency %2 which is not installed.")
                .quoted_arg(package_name)
                .quoted_arg(d.to_string())
            .level(wpkg_output::level_warning)
            .module(wpkg_output::module_validate_installation)
            .package(package_name)
            .action("install-validation");
        return true;
    }

    // auto-unpacking and configuring of a pre-dependency would make
    // things quite a bit more complicated so we just generate an error
    // (i.e. that package may be available in the repository...)
    // the problem here is that we'd need multiple lists of packages to
    // install, each list with its own set of pre-dependencies, etc.
    wpkg_output::log("package %1 has pre-dependency %2 which is not installed.")
            .quoted_arg(package_name)
            .quoted_arg(d.to_string())
        .level(wpkg_output::level_error)
        .module(wpkg_output::module_validate_installation)
        .package(package_name)
        .action("install-validation");
    return false;
}


void wpkgar_install::validate_predependencies()
{
    // note: at this point we have not read repositories yet

    // already installed packages must have already been loaded for this
    // validation function to work
    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < f_packages.size();
                                         ++idx)
    {
        f_manager->check_interrupt();

        if(f_packages[idx].get_type() == package_item_t::package_type_explicit)
        {
            // full path to package
            const wpkg_filename::uri_filename filename(f_packages[idx].get_filename());

            // get list of pre-dependencies if any
            if(f_packages[idx].field_is_defined(wpkg_control::control_file::field_predepends_factory_t::canonicalized_name()))
            {
                wpkg_dependencies::dependencies pre_depends(f_packages[idx].get_field(wpkg_control::control_file::field_predepends_factory_t::canonicalized_name()));
                for(int i(0); i < pre_depends.size(); ++i)
                {
                    const wpkg_dependencies::dependencies::dependency_t& d(pre_depends.get_dependency(i));
                    find_installed_predependency(filename, d);
                }
            }
        }
    }
}


void wpkgar_install::read_repositories()
{
    // load the files once
    if(!f_repository_packages_loaded)
    {
        f_repository_packages_loaded = true;
        const wpkg_filename::filename_list_t& repositories(f_manager->get_repositories());
        for(wpkg_filename::filename_list_t::const_iterator it(repositories.begin()); it != repositories.end(); ++it)
        {
            f_manager->check_interrupt();

            // repository must include an index, if not and the repository
            // is a direct filename then we attempt to create the index now
            wpkg_filename::uri_filename index_filename(it->append_child("index.tar.gz"));
            memfile::memory_file index_file;
            memfile::memory_file compressed;
            if(index_filename.is_direct())
            {
                if(!index_filename.exists())
                {
                    wpkg_output::log("Creating index file, since it does not exist in repository '%1'.")
                            .quoted_arg(*it)
                        .debug(wpkg_output::debug_flags::debug_detail_config)
                        .module(wpkg_output::module_validate_installation)
                        .package(index_filename);

                    // that's a direct filename but the index is missing,
                    // create it on the spot
                    wpkgar_repository repository(f_manager);
                    // If the user wants a recursive repository index he will have to do it manually because --recursive is already
                    // used for another purpose along the --install and I do not think that it is wise to do this here anyway
                    //repository.set_parameter(wpkgar::wpkgar_repository::wpkgar_repository_recursive, get_parameter(wpkgar_install_recursive, false));
                    repository.create_index(index_file);
                    index_file.compress(compressed, memfile::memory_file::file_format_gz);
                    compressed.write_file(index_filename);
                }
                else
                {
                    wpkg_output::log("Reading index file from repository '%1'.")
                            .quoted_arg(*it)
                        .debug(wpkg_output::debug_flags::debug_detail_config)
                        .module(wpkg_output::module_validate_installation)
                        .package(index_filename);

                    // index exists, read it
                    compressed.read_file(index_filename);
                    compressed.decompress(index_file);
                }
            }
            else
            {
                // from remote URIs we cannot really expect the exists() call
                // to work so we instead try to load the file directly; if it
                // fails we just ignore that entry
                try
                {
                    wpkg_output::log("Reading index file from remote repository '%1'.")
                            .quoted_arg(*it)
                        .debug(wpkg_output::debug_flags::debug_detail_config)
                        .module(wpkg_output::module_validate_installation)
                        .package(index_filename);

                    compressed.read_file(index_filename);
                    compressed.decompress(index_file);
                }
                catch(const memfile::memfile_exception&)
                {
                    wpkg_output::log("skip remote repository %1 as it does not seem to include an index.tar.gz file.")
                            .quoted_arg(*it)
                        .debug(wpkg_output::debug_flags::debug_detail_config)
                        .module(wpkg_output::module_validate_installation)
                        .package(index_filename);
                    continue;
                }
            }

            // we keep a complete list of all the packages that have a valid filename
            index_file.dir_rewind();
            for(;;)
            {
                f_manager->check_interrupt();

                memfile::memory_file::file_info info;
                memfile::memory_file ctrl;
                if(!index_file.dir_next(info, &ctrl))
                {
                    break;
                }
                std::string filename(info.get_filename());
                // the filename in a repository index ends with .ctrl, we want to
                // change that extension with .deb
                if(filename.size() > 5 && filename.substr(filename.size() - 5) == ".ctrl")
                {
                    filename = filename.substr(0, filename.size() - 4) + "deb";
                }
                package_item_t package(f_manager, it->append_child(filename), package_item_t::package_type_available, ctrl);

                // verify package architecture
                const std::string arch(package.get_architecture());
                if(arch != "all" && !wpkg_dependencies::dependencies::match_architectures(arch, f_architecture, get_parameter(wpkgar_install_force_vendor, false) != 0))
                {
                    // this is not an error, although in the end we may not
                    // find any package that satisfy this dependency...
                    wpkg_output::log("implicit package in file %1 does not have a valid architecture (%2) for this target machine (%3).")
                            .quoted_arg(filename)
                            .arg(arch)
                            .arg(f_architecture)
                        .debug(wpkg_output::debug_flags::debug_config)
                        .module(wpkg_output::module_validate_installation)
                        .package(filename);
                    continue;
                }

                f_packages.push_back(package);
            }
        }
    }
}


/** \brief Check whether a package is in conflict with another.
 *
 * This function checks whether the specified package (tree[idx]) is in
 * conflict with any others.
 *
 * The specified tree maybe f_packages or a copy that we're working on.
 *
 * The only-explicit flag is used to know whether we're only checking
 * explicit packages as conflict destinations. This is useful to trim
 * the f_packages tree before building all the trees.
 *
 * The function checks the Conflicts field and then the Breaks field.
 * The Breaks fields are ignored if the packager is just unpacking
 * packages specified on the command line.
 *
 * \param[in,out] tree  The tree to check for conflicts.
 * \param[in] idx  The index of the item being checked.
 * \param[in] only_explicit  Whether only explicit packages are checked.
 */
void wpkgar_install::trim_conflicts(wpkgar_package_list_t& tree, const wpkgar_package_list_t::size_type idx, const bool only_explicit)
{
    wpkg_filename::uri_filename filename(tree[idx].get_filename());
    package_item_t::package_type_t idx_type(tree[idx].get_type());
    bool check_available(false);
    switch(idx_type)
    {
    case package_item_t::package_type_explicit:
    case package_item_t::package_type_installed:
    case package_item_t::package_type_configure:
    case package_item_t::package_type_upgrade:
    case package_item_t::package_type_downgrade:
    case package_item_t::package_type_unpacked:
        check_available = true;
        break;

    default:
        // not explicit or installed
        break;

    }

    // got a Conflicts field?
    if(tree[idx].field_is_defined(wpkg_control::control_file::field_conflicts_factory_t::canonicalized_name()))
    {
        wpkg_dependencies::dependencies depends(tree[idx].get_field(wpkg_control::control_file::field_conflicts_factory_t::canonicalized_name()));
        for(int i(0); i < depends.size(); ++i)
        {
            const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(i));
            for(wpkgar_package_list_t::size_type j(0); j < tree.size(); ++j)
            {
                f_manager->check_interrupt();

                if(j != idx
                && (!only_explicit || tree[j].get_type() == package_item_t::package_type_explicit))
                {
                    switch(tree[j].get_type())
                    {
                    case package_item_t::package_type_available:
                        if(!check_available)
                        {
                            // available not checked against implicit
                            // and other available packages
                            break;
                        }
                    case package_item_t::package_type_explicit:
                    case package_item_t::package_type_installed:
                    case package_item_t::package_type_configure:
                    case package_item_t::package_type_implicit:
                    case package_item_t::package_type_upgrade:
                    case package_item_t::package_type_upgrade_implicit:
                    case package_item_t::package_type_downgrade:
                    case package_item_t::package_type_unpacked:
                        if(d.f_name == tree[j].get_name()
                        && match_dependency_version(d, tree[j]) == 1)
                        {
                            // ouch! found a match, mark that package as invalid
                            int err(2);
                            switch(tree[j].get_type())
                            {
                            case package_item_t::package_type_explicit:
                            case package_item_t::package_type_installed:
                            case package_item_t::package_type_configure:
                            case package_item_t::package_type_upgrade:
                            case package_item_t::package_type_downgrade:
                            case package_item_t::package_type_unpacked:
                                break;

                            case package_item_t::package_type_implicit:
                            case package_item_t::package_type_upgrade_implicit:
                            case package_item_t::package_type_available:
                                err = 1;
                                tree[j].set_type(package_item_t::package_type_invalid);
                                break;

                            default:
                                // logic broken between previous switch()
                                // and this switch()
                                throw std::logic_error("invalid packages type in trim_conflicts() [Conflicts]");

                            }
                            switch(idx_type)
                            {
                            case package_item_t::package_type_explicit:
                            case package_item_t::package_type_installed:
                            case package_item_t::package_type_configure:
                            case package_item_t::package_type_upgrade:
                            case package_item_t::package_type_downgrade:
                            case package_item_t::package_type_unpacked:
                                break;

                            case package_item_t::package_type_implicit:
                            case package_item_t::package_type_upgrade_implicit:
                            case package_item_t::package_type_available:
                                err = 1;
                                tree[idx].set_type(package_item_t::package_type_invalid);
                                break;

                            default:
                                // caller is at fault
                                throw std::logic_error("trim_conflicts() called with an unexpected package type [Conflicts]");

                            }
                            if(err == 2)
                            {
                                // we do not mark explicit/installed packages
                                // as invalid; output an error instead
                                if(get_parameter(wpkgar_install_force_conflicts, false))
                                {
                                    // user accepts conflicts, use a warning
                                    err = 0;
                                }
                                wpkg_output::log("package %1 is in conflict with %2.")
                                        .quoted_arg(filename)
                                        .quoted_arg(tree[j].get_filename())
                                    .level(err == 0 ? wpkg_output::level_warning : wpkg_output::level_error)
                                    .module(wpkg_output::module_validate_installation)
                                    .package(filename)
                                    .action("install-validation");
                            }
                        }
                        break;

                    case package_item_t::package_type_not_installed:
                    case package_item_t::package_type_invalid:
                    case package_item_t::package_type_same: // should not happen here
                    case package_item_t::package_type_older:
                    case package_item_t::package_type_directory:
                        // uninstalled or invalid are ignored
                        break;

                    }
                }
            }
        }
    }

    // breaks don't apply if we're just unpacking
    if(f_unpacking_packages)
    {
        return;
    }

    // got a Breaks field?
    if(tree[idx].field_is_defined(wpkg_control::control_file::field_breaks_factory_t::canonicalized_name()))
    {
        wpkg_dependencies::dependencies depends(tree[idx].get_field(wpkg_control::control_file::field_breaks_factory_t::canonicalized_name()));
        for(int i(0); i < depends.size(); ++i)
        {
            const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(i));
            for(wpkgar_package_list_t::size_type j(0); j < tree.size(); ++j)
            {
                if(j != idx
                && (!only_explicit || tree[j].get_type() == package_item_t::package_type_explicit))
                {
                    switch(tree[j].get_type())
                    {
                    case package_item_t::package_type_available:
                        if(!check_available)
                        {
                            // available not checked against implicit
                            // and other available packages
                            break;
                        }
                    case package_item_t::package_type_explicit:
                    case package_item_t::package_type_implicit:
                    case package_item_t::package_type_installed:
                    case package_item_t::package_type_configure:
                    case package_item_t::package_type_upgrade:
                    case package_item_t::package_type_upgrade_implicit:
                    case package_item_t::package_type_downgrade:
                        if(d.f_name == tree[j].get_name()
                        && match_dependency_version(d, tree[j]) == 1)
                        {
                            // ouch! found a match, mark that package as invalid
                            int err(2);
                            switch(tree[j].get_type())
                            {
                            case package_item_t::package_type_explicit:
                            case package_item_t::package_type_installed:
                            case package_item_t::package_type_configure:
                            case package_item_t::package_type_upgrade:
                            case package_item_t::package_type_downgrade:
                            case package_item_t::package_type_unpacked:
                                break;

                            case package_item_t::package_type_implicit:
                            case package_item_t::package_type_upgrade_implicit:
                            case package_item_t::package_type_available:
                                err = 1;
                                tree[j].set_type(package_item_t::package_type_invalid);
                                break;

                            default:
                                // logic broken between previous switch()
                                // and this switch()
                                throw std::logic_error("invalid packages type in trim_conflicts() [Breaks]");

                            }
                            switch(idx_type)
                            {
                            case package_item_t::package_type_explicit:
                            case package_item_t::package_type_installed:
                            case package_item_t::package_type_configure:
                            case package_item_t::package_type_upgrade:
                            case package_item_t::package_type_downgrade:
                            case package_item_t::package_type_unpacked:
                                break;

                            case package_item_t::package_type_implicit:
                            case package_item_t::package_type_upgrade_implicit:
                            case package_item_t::package_type_available:
                                err = 1;
                                tree[idx].set_type(package_item_t::package_type_invalid);
                                break;

                            default:
                                // caller is at fault
                                throw std::logic_error("trim_conflicts() called with an unexpected package type [Breaks]");

                            }
                            if(err == 2)
                            {
                                // we do not mark explicit/installed packages
                                // as invalid; generate an error instead
                                if(get_parameter(wpkgar_install_force_breaks, false))
                                {
                                    // user accepts Breaks, use a warning
                                    err = 0;
                                }
                                wpkg_output::log("package %1 breaks %2.")
                                        .quoted_arg(filename)
                                        .quoted_arg(tree[j].get_filename())
                                    .level(err == 0 ? wpkg_output::level_warning : wpkg_output::level_error)
                                    .module(wpkg_output::module_validate_installation)
                                    .package(filename)
                                    .action("install-validation");
                            }
                        }
                        break;

                    case package_item_t::package_type_unpacked:       // accepted because it's not configured
                    case package_item_t::package_type_not_installed:  // ignored anyway
                    case package_item_t::package_type_invalid:        // ignored anyway
                    case package_item_t::package_type_same:           // ignored anyway
                    case package_item_t::package_type_older:          // ignored anyway
                    case package_item_t::package_type_directory:      // ignored anyway
                        break;

                    }
                }
            }
        }
    }
}


bool wpkgar_install::trim_dependency
    ( package_item_t& item
    , wpkgar_package_ptrs_t& parents
    , const wpkg_dependencies::dependencies::dependency_t& dependency
    , const std::string& field_name
    )
{
    const auto filename( item.get_filename() );

    // if an explicit package has a dependency satisfied by another
    // explicit package then we mark all implicit packages of the same
    // name as invalid because they for sure won't get used
    bool found_package( false );
    for( auto& pkg : f_packages )
    {
        f_manager->check_interrupt();

        if(pkg.get_type() == package_item_t::package_type_explicit
                && dependency.f_name == pkg.get_name())
        {
            // note that explicit to explicit dependencies already had their
            // version checked but implicit to explicit, not yet; if
            // explicit to explicit we just check it again, that's quite
            // fast anyway
            if(match_dependency_version(dependency, pkg) == 1)
            {
                // recursive call to check circular definitions, just
                // in case we had such
                parents.push_back(&item);
                trim_available( pkg, parents );
                parents.pop_back();
            }
            else
            {
                if(!get_parameter(wpkgar_install_force_depends_version, false))
                {
                    // since we cannot replace an explicit dependency, we
                    // generate an error in this case
                    wpkg_output::log("package %1 depends on %2 with an incompatible version constraint (%3).")
                        .quoted_arg(filename)
                        .quoted_arg(pkg.get_filename())
                        .arg(dependency.to_string())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(filename)
                        .action("install-validation");
                }
                else
                {
                    // there is a version problem but the user is okay with it
                    // just generate a wPaying taxes that help those less fortunate than we are is not stealing.arning
                    wpkg_output::log("use package %1 which has an incompatible version for dependency %2 found in field %3.")
                        .quoted_arg(filename)
                        .quoted_arg(dependency.to_string())
                        .arg(field_name)
                        .level(wpkg_output::level_warning)
                        .module(wpkg_output::module_validate_installation)
                        .package(filename)
                        .action("install-validation");
                }
            }
            // we found the package we're done with this test
            found_package = true;
            break;
        }
    }

    // if available among explicit packages, mark all implicit packages
    // with the same name as invalid (they cannot "legally" get used!)
    if( found_package )
    {
        for( auto& pkg : f_packages )
        {
            f_manager->check_interrupt();

            if(pkg.get_type() == package_item_t::package_type_available
                    && dependency.f_name == pkg.get_name())
            {
                // completely ignore those
                pkg.set_type(package_item_t::package_type_invalid);
            }
        }
        return false;
    }

    // we use the auto-update flag to know when an implicit package is used
    // to automatically update an installed package (opposed to installing
    // a new intermediate package) although at this point we do NOT mark
    // the installed packages as being upgraded since it will depend on
    // whether this specific case is used in the end or not
    bool auto_upgrade(false);

    // not found as an explicit package,
    // try as an already installed package
    bool found(false);
    for( const auto& pkg : f_packages )
    {
        bool quit( false );
        f_manager->check_interrupt();

        if(dependency.f_name == pkg.get_name())
        {
            switch(pkg.get_type())
            {
                case package_item_t::package_type_unpacked:
                    wpkg_output::log("unpacked version of %1 checked for dependency %2, if selected later, it will fail.")
                        .quoted_arg(pkg.get_name())
                        .quoted_arg(dependency.to_string())
                        .debug(wpkg_output::debug_flags::debug_detail_config)
                        .module(wpkg_output::module_validate_installation)
                        .package(filename);
                    /*FLOWTHROUGH*/
                case package_item_t::package_type_installed:
                case package_item_t::package_type_configure:
                case package_item_t::package_type_upgrade:
                case package_item_t::package_type_downgrade:
                    // note that we cannot err about the unpacked package
                    // right now as we cannot be certain it will be
                    // included in the tree we're going to select in the
                    // end (and thus it may not be a problem.)

                    // TODO: support --force-depends-version
                    // if we're checking an implicit package, the version
                    // must match or that implicit package cannot be
                    // installed unless we can auto-update
                    if(match_dependency_version(dependency, pkg) != 1)
                    {
                        // When this fails, we could still have an implicit
                        // package that could be used to upgrade this
                        // package... try that first
                        auto_upgrade = true;

                        // simulate the end of the list so we don't waste
                        // our time and enter the next loop
                        quit = true;
                    }
                    else
                    {
                        // recursive call? -- not necessary for installed
                        // packages since we expect the existing installation
                        // to already be in a working state and thus to have
                        // all the dependencies satisfied; this being said we
                        // may end up auto-upgrading packages to satisfy some
                        // dependencies... right?
                        //trim_available(pkg, parents);

                        // we found it, stop here
                        found = true;
                    }
                    break;

                default:
                    // other types do not represent an installed package
                    break;

            }
        }

        if( found || quit )
        {
            break;
        }
    }

    if( found )
    {
        // in this case (i.e. package was found in the list of installed
        // packages) we keep the implicit packages because even if
        // already installed is satisfactory in this case, we may hit a
        // case where we end up having to update these files and for
        // that purpose we need to have access to the implicit packages!
        return false;
    }

    // not found as an explicit or installed package,
    // try as an implicit package
    uint32_t match_count(0);
    bool match_installed(false);
    package_item_t* last_package( 0 );
    for( auto& pkg : f_packages )
    {
        last_package = &pkg;
        f_manager->check_interrupt();

        if(dependency.f_name == pkg.get_name())
        {
            switch(pkg.get_type())
            {
                case package_item_t::package_type_installed:
                case package_item_t::package_type_upgrade:
                    // if already installed, we're all good
                    match_installed = true;
                    break;

                case package_item_t::package_type_available:
                    //
                    // WARNING: We cannot check a version from an implicit
                    //          package to an implicit package at this point
                    //          because we're not creating a valid tree, only
                    //          trimming what is for sure invalid/incompatible
                    //
                    // TODO:    I remarked off the test for not-explicit, because
                    //          coupled with the logical OR operator, any non-explicit
                    //          package in the queue which happened to match names
                    //          would be considered a match, because the second test
                    //          would never be executed. Now, since I need to confer
                    //          with the original author (Alexis) to discover his
                    //          intent, I've marked this TODO. But for now, this
                    //          fixes the upgrade bug that was preventing package
                    //          upgrade when multiple different versions of a
                    //          dependency existed in the repository.
                    //
                    if(//item.get_type() != package_item_t::package_type_explicit
                            match_dependency_version(dependency, pkg) == 1)
                    {
                        // found at least one
                        ++match_count;

                        // recursive call to handle the Depends of this entry
                        // since in this case we need it
                        parents.push_back(&item);
                        trim_available(pkg, parents);
                        parents.pop_back();
                    }
                    else
                    {
                        // if the version doesn't match from then this
                        // implicit package cannot be used at all because
                        // an explicit package directly depends on it
                        //
                        // here it is not an error because we may have
                        // other satisfactory implicit packages (see
                        // 'match_count == 0' below)
                        wpkg_output::log("file %1 does not satisfy dependency %2 because of its version.")
                            .quoted_arg(filename)
                            .quoted_arg(dependency.to_string())
                            .debug(wpkg_output::debug_flags::debug_detail_config)
                            .module(wpkg_output::module_validate_installation)
                            .package(filename);
                        pkg.set_type(package_item_t::package_type_invalid);
                    }
                    break;

                default:
                    // other types are ignored here
                    break;

            }
        }

        if( match_installed )
        {
            break;
        }
    }
    //
    if( match_count == 0 )
    {
        if(!match_installed)
        {
            if(auto_upgrade)
            {
                if( last_package == 0 )
                {
                    wpkg_output::log("Error with last_package! Should never be null!")
                        .level(wpkg_output::level_warning)
                        .module(wpkg_output::module_validate_installation)
                        .package(filename)
                        .action("install-validation");
                }
                else
                {
                    // in this case we tell the user that the existing
                    // installation is not compatible rather than that there
                    // is no package that satisfies the dependency
                    //
                    // XXX add a --force-auto-upgrade flag?
                    if(!get_parameter(wpkgar_install_force_depends, false))
                    {
                        wpkg_output::log("package %1 depends on %2 which is an installed package with an incompatible version constraint (%3).")
                                .quoted_arg(filename)
                                .quoted_arg(last_package->get_filename())
                                .arg(dependency.to_string())
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_validate_installation)
                                .package(filename)
                                .action("install-validation");
                    }
                    else
                    {
                        // TBD: is that warning in the way?
                        //
                        // TODO:
                        // Mark the tree (somehow) as "less good" since it
                        // has warnings (i.e. count warnings for each tree)
                        wpkg_output::log("package %1 depends on %2 which is an installed package with an incompatible version constraint (%3); it may still get installed using this tree.")
                                .quoted_arg(filename)
                                .quoted_arg(last_package->get_filename())
                                .arg(dependency.to_string())
                                .level(wpkg_output::level_warning)
                                .module(wpkg_output::module_validate_installation)
                                .package(filename)
                                .action("install-validation");
                    }
                }
            }
            else
            {
                if(!get_parameter(wpkgar_install_force_depends, false))
                {
                    wpkg_output::log("no explicit or implicit package satisfy dependency %1 of package %2.")
                        .quoted_arg(dependency.to_string())
                        .quoted_arg(item.get_name())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(filename)
                        .action("install-validation");
                }
                else
                {
                    // TBD: is that warning in the way?
                    //
                    // TODO:
                    // Mark the tree (somehow) as "less good" since it
                    // has warnings (i.e. count warnings for each tree)
                    wpkg_output::log("no explicit or implicit package satisfy dependency %1 of package %2; it may still get installed using this tree.")
                        .quoted_arg(dependency.to_string())
                        .quoted_arg(item.get_name())
                        .level(wpkg_output::level_warning)
                        .module(wpkg_output::module_validate_installation)
                        .package(filename)
                        .action("install-validation");
                }
            }
        }
        //else if(match_installed) ... we're updating that package
    }
    else if(match_count > 1)
    {
        f_install_includes_choices = true;
    }

    return true;
}


void wpkgar_install::trim_available( package_item_t& item, wpkgar_package_ptrs_t& parents )
{
    const wpkg_filename::uri_filename filename(item.get_filename());

    if(parents.size() > f_tree_max_depth)
    {
        f_tree_max_depth = parents.size();
        if(1000 == f_tree_max_depth)
        {
            wpkg_output::log("tree depth is now 1,000, since we use the processor stack for validation purposes, it may end up crashing.")
                .level(wpkg_output::level_warning)
                .module(wpkg_output::module_validate_installation)
                .package(filename)
                .action("install-validation");
        }
    }

    // verify loops (i.e. A -> B -> A)
    for(wpkgar_package_ptrs_t::size_type q(0); q < parents.size(); ++q)
    {
        if(parents[q] == &item)
        {
            wpkg_output::log("package %1 depends on itself (circular dependency).")
                    .quoted_arg(filename)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(filename)
                .action("install-validation");
            return;
        }
    }

    // got a Depends field?
    for( const auto& field_name : f_field_names )
    {
        if(!item.field_is_defined( field_name ))
        {
            continue;
        }

        // satisfy all dependencies
        wpkg_dependencies::dependencies depends( item.get_field( field_name ) );
        for( int i(0); i < depends.size(); ++i )
        {
            trim_dependency( item, parents, depends.get_dependency(i), field_name );
        }
    }
}


void wpkgar_install::trim_available_packages()
{
    // start by removing all the available packages that are in conflict with
    // the explicit packages because we'll never be able to use them
    for(wpkgar_package_list_t::size_type idx(0); idx < f_packages.size(); ++idx)
    {
        // start from the top level (i.e. only check explicit dependencies)
        switch(f_packages[idx].get_type())
        {
        case package_item_t::package_type_explicit:
            if(!f_reconfiguring_packages)
            {
                trim_conflicts(f_packages, idx, false);
            }
            break;

        case package_item_t::package_type_installed:
        case package_item_t::package_type_configure:
        case package_item_t::package_type_implicit:
        case package_item_t::package_type_available:
        case package_item_t::package_type_upgrade:
        case package_item_t::package_type_upgrade_implicit:
        case package_item_t::package_type_downgrade:
        case package_item_t::package_type_unpacked:
            trim_conflicts(f_packages, idx, true);
            break;

        case package_item_t::package_type_not_installed:
        case package_item_t::package_type_invalid:
        case package_item_t::package_type_same:
        case package_item_t::package_type_older:
        case package_item_t::package_type_directory:
            // these are already ignored
            break;

        }
    }

    if(!f_reconfiguring_packages)
    {
        wpkgar_package_ptrs_t parents;
        for(wpkgar_package_list_t::size_type idx(0); idx < f_packages.size(); ++idx)
        {
            // start from the top level (i.e. only check explicit dependencies)
            if(f_packages[idx].get_type() == package_item_t::package_type_explicit)
            {
                trim_available(f_packages[idx], parents);
                if(!parents.empty())
                {
                    // with the recursivity, it is not possible to get this error
                    throw std::logic_error("the parents vector is not empty after returning from trim_available()");
                }
            }
        }
    }
}


/** \brief Search for dependencies in the list of explicit packages.
 *
 * Before attempting to find a package in the list of already installed
 * packages, we search for it in the list of explicit packages.
 *
 * This is an important distinction because checks imposed on explicit
 * packages are slightly different than those imposed on installed
 * package.
 *
 * \param[in] index  The index of the explicit package being checked.
 * \param[in] package_name  The name of the package being checked.
 * \param[in] d  The dependency structure representing one specific
 *               dependency of the package at \p index.
 * \param[in] field_name  Name of the field being checked in case an error
 *                        occur (so we can tell the user which one failed.)
 *
 * \return Return whether all the dependencies could be found or not.
 */
wpkgar_install::validation_return_t wpkgar_install::find_explicit_dependency(wpkgar_package_list_t::size_type index, const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d, const std::string& field_name)
{
    // check whether it is part of the list of packages the user
    // specified on the command line (explicit)
    wpkgar_package_list_t::size_type found(static_cast<wpkgar_package_list_t::size_type>(-1));
    for(wpkgar_package_list_t::size_type idx(0); idx < f_packages.size(); ++idx)
    {
        if(index != idx // skip myself
        && f_packages[idx].get_type() == package_item_t::package_type_explicit
        && d.f_name == f_packages[idx].get_name())
        {
            if(found != static_cast<wpkgar_package_list_t::size_type>(-1))
            {
                // found more than one!
                if(!same_file(package_name.os_filename().get_utf8(), f_packages[idx].get_filename().os_filename().get_utf8()))
                {
                    // and they both come from two different files so that's an error!
                    wpkg_output::log("files %1 and %2 define the same package (their Package field match) but are distinct files.")
                            .quoted_arg(f_packages[idx].get_filename())
                            .quoted_arg(package_name)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(package_name)
                        .action("install-validation");
                }
                else
                {
                    // this package is as valid as the other since they both are
                    // the exact same, but we don't want to do the work twice so
                    // to ignore it set as invalid
                    f_packages[idx].set_type(package_item_t::package_type_invalid);
                }
            }
            else
            {
                const std::string& architecture(f_packages[idx].get_architecture());
                if((architecture == "src" || architecture == "source")
                && field_name != wpkg_control::control_file::field_builtusing_factory_t::canonicalized_name())
                {
                    // the only place were a source package can depend on
                    // another source package is in the Built-Using field;
                    // anywhere else and it is an error because nothing shall
                    // otherwise depend on a source package
                    wpkg_output::log("package %1 is a source package and it cannot be part of the list of dependencies defined in %2.")
                            .quoted_arg(f_packages[idx].get_filename())
                            .arg(field_name)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(package_name)
                        .action("install-validation");
                    return validation_return_error;
                }

                // keep the first we find
                found = idx;
            }
        }
    }

    if(found != static_cast<wpkgar_package_list_t::size_type>(-1))
    {
        const wpkg_filename::uri_filename filename(f_packages[found].get_filename());
        if(match_dependency_version(d, f_packages[found]) == 1)
        {
            // we got it in our list of explicit packages, we're all good
            wpkg_output::log("use file %1 to satisfy dependency %2 as it was specified on the command line.")
                    .quoted_arg(filename)
                    .quoted_arg(d.to_string())
                .debug(wpkg_output::debug_flags::debug_detail_config)
                .module(wpkg_output::module_validate_installation)
                .package(package_name);
            return validation_return_success;
        }
        // should we mark the package as invalid (instead of explicit?)
        // since we had an error it's probably not necessary?
        wpkg_output::log("file %1 has an incompatible version for dependency %2.")
                .quoted_arg(filename)
                .quoted_arg(d.to_string())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_validate_installation)
            .package(package_name)
            .action("install-validation");
        return validation_return_error;
    }

    return validation_return_missing;
}


/** \brief Search for dependencies in the list of installed packages.
 *
 * If an explicit package was not found in the list of explicit packages,
 * then we can try whether it exists in the list of already installed
 * packages, including the correct version.
 *
 * \param[in] index  The index of the explicit package being checked.
 * \param[in] package_name  The name of the package being checked.
 * \param[in] d  The dependency structure representing one specific
 *               dependency of the package at \p index.
 * \param[in] field_name  Name of the field being checked in case an error
 *                        occur (so we can tell the user which one failed.)
 *
 * \return Return whether all the dependencies could be found or not.
 */
wpkgar_install::validation_return_t wpkgar_install::find_installed_dependency(wpkgar_package_list_t::size_type index, const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d, const std::string& field_name)
{
    // check whether it is part of the list of packages the user
    // specified on the command line (explicit)
    wpkgar_package_list_t::size_type found(static_cast<wpkgar_package_list_t::size_type>(-1));
    for(wpkgar_package_list_t::size_type idx(0); idx < f_packages.size(); ++idx)
    {
        if(index != idx // skip myself
        && f_packages[idx].get_type() == package_item_t::package_type_installed
        && d.f_name == f_packages[idx].get_name())
        {
            if(found != static_cast<wpkgar_package_list_t::size_type>(-1))
            {
                // found more than one!?
                // this should never happen since you cannot install two
                // distinct packages on a target with the exact same name!
                wpkg_output::log("found two distinct installed packages, %1 and %2, with the same name?!")
                        .quoted_arg(f_packages[idx].get_filename())
                        .quoted_arg(f_packages[found].get_filename())
                        .quoted_arg(package_name)
                    .level(wpkg_output::level_fatal)
                    .module(wpkg_output::module_validate_installation)
                    .package(package_name)
                    .action("install-validation");
                return validation_return_error;
            }

            const std::string& architecture(f_packages[idx].get_architecture());
            if((architecture == "src" || architecture == "source")
            && field_name != wpkg_control::control_file::field_builtusing_factory_t::canonicalized_name())
            {
                // the only place were a source package can depend on
                // another source package is in the Built-Using field;
                // anywhere else and it is an error because nothing shall
                // otherwise depend on a source package
                wpkg_output::log("package %1 is a source package and it cannot be part of the list of dependencies defined in %2.")
                        .quoted_arg(f_packages[idx].get_filename())
                        .arg(field_name)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(package_name)
                    .action("install-validation");
                return validation_return_error;
            }

            // keep the first we find
            found = idx;
        }
    }

    if(found != static_cast<wpkgar_package_list_t::size_type>(-1))
    {
        const wpkg_filename::uri_filename name(f_packages[found].get_name());
        if(match_dependency_version(d, f_packages[found]) == 1)
        {
            // we got it in our list of installed packages, we're all good
            wpkg_output::log("use installed package %1 to satisfy dependency %2.")
                    .quoted_arg(name)
                    .quoted_arg(d.to_string())
                .debug(wpkg_output::debug_flags::debug_detail_config)
                .module(wpkg_output::module_validate_installation)
                .package(package_name);
            return validation_return_success;
        }
        // in this case we say that the dependency is missing which allows
        // the system to check some more using the more complex dependency
        // search mechanism (i.e. maybe this installed package will
        // automatically get upgraded to satisfy the version requirements)
    }

    return validation_return_missing;
}


wpkgar_install::validation_return_t wpkgar_install::validate_installed_depends_field(const wpkgar_package_list_t::size_type idx, const std::string& field_name)
{
    // full path to package
    const wpkg_filename::uri_filename filename(f_packages[idx].get_filename());
    validation_return_t result(validation_return_success);

    // we already checked that the field existed in the previous function
    wpkg_dependencies::dependencies depends(f_packages[idx].get_field(field_name));
    for(int i(0); i < depends.size(); ++i)
    {
        f_manager->check_interrupt();

        const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(i));
        validation_return_t r(find_explicit_dependency(idx, filename, d, field_name));
        if(r == validation_return_error)
        {
            return r;
        }
        if(r == validation_return_missing)
        {
            // not found as an explicit package, try with installed packages
            r = find_installed_dependency(idx, filename, d, field_name);
        }
        if(r == validation_return_missing && result == validation_return_success)
        {
            // at least one dependency is missing...
            result = validation_return_missing;
        }
    }

    return result;
}


wpkgar_install::validation_return_t wpkgar_install::validate_installed_dependencies()
{
    // first check whether all the dependencies are self contained (i.e. the
    // package being installed only needs already installed packages or has
    // no dependencies in the first place.)
    //
    // if so we avoid the whole algorithm trying to auto-install missing
    // dependencies using packages defined in repositories

    // result is success by default
    validation_return_t result(validation_return_success);

    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < f_packages.size();
                                         ++idx)
    {
        if(f_packages[idx].get_type() == package_item_t::package_type_explicit)
        {
            // full path to package
            const wpkg_filename::uri_filename filename(f_packages[idx].get_filename());
            const std::string& architecture(f_packages[idx].get_architecture());
            const bool is_source(architecture == "src" || architecture == "source");

            // get list of dependencies if any
            for(wpkgar_list_of_strings_t::const_iterator f(f_field_names.begin());
                                                         f != f_field_names.end();
                                                         ++f)
            {
                if(f_packages[idx].field_is_defined(*f))
                {
                    // kind of a hackish test here... if not Depends field
                    // and it is a binary package, that's an error
                    if(!is_source && f != f_field_names.begin())
                    {
                        wpkg_output::log("%1 is a binary package and yet it includes build dependencies.")
                                .quoted_arg(filename)
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(f_packages[idx].get_name())
                            .action("install-validation");
                        return validation_return_error;
                    }

                    validation_return_t r(validate_installed_depends_field(idx, *f));
                    if(r == validation_return_error)
                    {
                        return r;
                    }
                    if(r == validation_return_missing && result == validation_return_success)
                    {
                        // at least one dependency is missing...
                        result = validation_return_missing;
                    }
                }
            }
        }
    }

    // if everything is self-contained, no need for auto-installations!
    return result;
}


bool wpkgar_install::check_implicit_for_upgrade(wpkgar_package_list_t& tree, const wpkgar_package_list_t::size_type idx)
{
    // check whether this implicit package is upgrading an existing package
    // because if so we have to mark the already installed package as being
    // upgraded and we have to link both packages together; also we do not
    // allow implicit downgrade, we prevent auto-upgrade of package which
    // selection is "Hold", and a few other things too...

    // TBD: if not installing I do not think we should end up here...
    //      but just in case I test
    if(!f_installing_packages)
    {
        return true;
    }

    // no problem if the package is not already installed
    // (we first test whether it's listed because that's really fast)
    const std::string name(tree[idx].get_name());
    if(std::find(f_list_installed_packages.begin(), f_list_installed_packages.end(), name) == f_list_installed_packages.end())
    {
        return true;
    }

    // IMPORTANT: remember that we're building a tree here so this function
    //            cannot generate errors otherwise it could prevent any
    //            tree from being selected
    package_item_t::package_type_t type(package_item_t::package_type_invalid);
    switch(f_manager->package_status(name))
    {   // <- TBD -- should we have a try/catch around this one?
    case wpkgar_manager::not_installed:
    case wpkgar_manager::config_files:
        // it's not currently installed so we can go ahead
        // and auto-install this dependency
        return true;

    case wpkgar_manager::installed:
        // we can upgrade those
        type = package_item_t::package_type_installed;
        break;

    case wpkgar_manager::unpacked:
        // with --install we cannot upgrade a package that was just unpacked.
        // (it needs an explicit --configure first)
        if(!f_unpacking_packages)
        {
            // we do not allow auto-configure of implicit targets
            return false;
        }
        // we're just unpacking so we're fine
        type = package_item_t::package_type_unpacked;
        break;

    case wpkgar_manager::no_package:
    case wpkgar_manager::unknown:
    case wpkgar_manager::half_installed:
    case wpkgar_manager::installing:
    case wpkgar_manager::upgrading:
    case wpkgar_manager::half_configured:
    case wpkgar_manager::removing:
    case wpkgar_manager::purging:
    case wpkgar_manager::listing:
    case wpkgar_manager::verifying:
    case wpkgar_manager::ready:
        // definitively invalid, cannot use this implicit target
        return false;
    }

    // Note: using f_manager directly since the package is there
    //       already anyway
    std::string vi(f_manager->get_field(name, wpkg_control::control_file::field_version_factory_t::canonicalized_name()));
    std::string vo(tree[idx].get_version());
    int c(wpkg_util::versioncmp(vi, vo));
    if(c == 0)
    {
        // this is a bug because we do not need an implicit dependency if
        // the version we want to implicitly install is already installed
        throw std::logic_error("an implicit target with the same version as the installed target was going to be added to the list of packages to installed; this is an internal error and the code needs to be fixed if it ever happens"); // LCOV_EXCL_LINE
    }

    if(c > 0)
    {
        // this is a downgrade, we refuse any implicit downgrading
        return false;
    }

    if(f_manager->field_is_defined(name, wpkg_control::control_file::field_xselection_factory_t::canonicalized_name())
    && wpkg_control::control_file::field_xselection_t::validate_selection(f_manager->get_field(name, wpkg_control::control_file::field_xselection_factory_t::canonicalized_name())) == wpkg_control::control_file::field_xselection_t::selection_hold)
    {
        // we cannot auto-upgrade if the installed package of an implicit
        // target is on hold; even with --force-hold
        return false;
    }

    // acceptable upgrade for an implicit package; mark the corresponding
    // installed package as an upgrade
    for(wpkgar_package_list_t::size_type i(0); i < tree.size(); ++i)
    {
        if(tree[i].get_type() == type && tree[i].get_name() == name)
        {
            tree[i].set_type(package_item_t::package_type_upgrade);
            return true;
        }
    }

    // we've got an error here; the installed package must already exists
    // since it was loaded when validating said installed packages
    throw std::logic_error("an implicit target cannot upgrade an existing package if that package does not exist in the f_packages vector; this is an internal error and the code needs to be fixed if it ever happens"); // LCOV_EXCL_LINE
}


/** \brief Return XSelection if defined in the package status.
 *
 * This method checks if XSelection is defined in the package status. If so, then it returns the selection.
 *
 * \return selection, or "selection_normal" if not defined.
 *
 */
wpkg_control::control_file::field_xselection_t::selection_t wpkgar_install::get_xselection( const wpkg_filename::uri_filename& filename ) const
{
    return get_xselection( filename.os_filename().get_utf8() );
}

wpkg_control::control_file::field_xselection_t::selection_t wpkgar_install::get_xselection( const std::string& filename ) const
{
    wpkg_control::control_file::field_xselection_t::selection_t
        selection( wpkg_control::control_file::field_xselection_t::selection_normal );

    if( f_manager->field_is_defined( filename,
                wpkg_control::control_file::field_xselection_factory_t::canonicalized_name()) )
    {
        selection = wpkg_control::control_file::field_xselection_t::validate_selection(
                f_manager->get_field( filename,
                    wpkg_control::control_file::field_xselection_factory_t::canonicalized_name())
                );
    }

    return selection;
}


/** \brief Find all dependencies of all the packages in the tree.
 *
 * This function recursively finds the dependencies for a given package.
 * If necessary and the user specified a repository, it promotes packages
 * that are available to implicit status when found.
 */
void wpkgar_install::find_dependencies( wpkgar_package_list_t& tree, const wpkgar_package_list_t::size_type idx, wpkgar_dependency_list_t& missing, wpkgar_dependency_list_t& held )
{
    const wpkg_filename::uri_filename filename(tree[idx].get_filename());

    trim_conflicts(tree, idx, false);

    const std::string& architecture(tree[idx].get_architecture());
    const bool is_source(architecture == "src" || architecture == "source");

    for(wpkgar_list_of_strings_t::const_iterator f(f_field_names.begin());
                                                 f != f_field_names.end();
                                                 ++f)
    {
        // any dependencies in this entry?
        if(!tree[idx].field_is_defined(*f))
        {
            // no dependencies means "satisfied"
            continue;
        }

        // check the dependencies
        wpkg_dependencies::dependencies depends(tree[idx].get_field(*f));
        for(int i(0); i < depends.size(); ++i)
        {
            const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(i));

            wpkgar_package_list_t::size_type unpacked_idx(0);
            validation_return_t found(validation_return_missing);
            for(wpkgar_package_list_t::size_type tree_idx(0);
                (found != validation_return_success)
                    && (found != validation_return_held)
                    && (tree_idx < tree.size());
                ++tree_idx
                )
            {
                f_manager->check_interrupt();

                auto& tree_item( tree[tree_idx] );

                switch(tree_item.get_type())
                {
                case package_item_t::package_type_explicit:
                case package_item_t::package_type_implicit:
                case package_item_t::package_type_available:
                case package_item_t::package_type_installed:
                case package_item_t::package_type_configure:
                case package_item_t::package_type_upgrade:
                case package_item_t::package_type_upgrade_implicit:
                case package_item_t::package_type_downgrade:
                    if(d.f_name == tree_item.get_name())
                    {
                        // this is a match, use it if possible!
                        switch(tree_item.get_type())
                        {
                        case package_item_t::package_type_available:
                            if(match_dependency_version(d, tree_item) == 1
                            && check_implicit_for_upgrade(tree, tree_idx))
                            {
                                // this one becomes implicit!
                                found = validation_return_success;

                                tree_item.set_type(package_item_t::package_type_implicit);
                                find_dependencies(tree, tree_idx, missing, held);
                            }
                            break;

                        case package_item_t::package_type_explicit:
                        case package_item_t::package_type_implicit:
                        case package_item_t::package_type_installed:
                        case package_item_t::package_type_configure:
                        case package_item_t::package_type_upgrade:
                        case package_item_t::package_type_upgrade_implicit:
                        case package_item_t::package_type_downgrade:
                            if(match_dependency_version(d, tree_item) == 1)
                            {
                                auto the_file( tree_item.get_filename() );
                                if( the_file.is_deb() )
                                {
                                    wpkg_control::control_file::field_xselection_t::selection_t
                                            selection( wpkgar_install::get_xselection( tree_item.get_filename() ) );

                                    if( selection == wpkg_control::control_file::field_xselection_t::selection_hold )
                                    {
                                        found = validation_return_held;
                                    }
                                    else
                                    {
                                        found = validation_return_success;
                                    }
                                }
                                else
                                {
                                    found = validation_return_success;
                                }
                            }
                            break;

                        default:
                            throw std::logic_error("code must have changed because all types that are accepted were handled!");
                        }
                    }
                    break;

                case package_item_t::package_type_unpacked:
                    if(d.f_name == tree_item.get_name()
                    && match_dependency_version(d, tree_item) == 1)
                    {
                        found = validation_return_unpacked;
                        unpacked_idx = tree_idx;
                    }
                    break;

                default:
                    // all other status are packages that are not available
                    break;

                }
            }
            if(found == validation_return_unpacked)
            {
                // this is either an error or we can mark that package as configure
                if(get_parameter(wpkgar_install_force_configure_any, false))
                {
                    f_packages[unpacked_idx].set_type(package_item_t::package_type_configure);
                    found = validation_return_success;
                }
            }

            // kind of a hackish test here... if not Depends field
            // and it is a binary package, that's an error
            if(!is_source && f != f_field_names.begin()
            && found == validation_return_success)
            {
                // similar to having a missing dependency error wise
                found = validation_return_missing;

                // this is an error no matter what, we may end up printing it
                // many times though...
                wpkg_output::log("%1 is a binary package and yet it includes build dependencies.")
                        .quoted_arg(filename)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(tree[idx].get_name())
                    .action("install-validation");
            }

            if( found == validation_return_missing )
            {
                missing.push_back(d);
            }
            else if( found == validation_return_held )
            {
                held.push_back(d);
            }
        }
    }
}


bool wpkgar_install::verify_tree( wpkgar_package_list_t& tree, wpkgar_dependency_list_t& missing, wpkgar_dependency_list_t& held )
{
    // if reconfiguring we have a good tree (i.e. the existing installation
    // tree is supposed to be proper)
    if(f_reconfiguring_packages)
    {
        return true;
    }

    // save so we know whether any dependencies are missing
    wpkgar_dependency_list_t::size_type missing_count(missing.size());
    wpkgar_dependency_list_t::size_type held_count(held.size());

    // verifying means checking that all dependencies are satisfied
    // also, in this case "available" dependencies that are required
    // get the new type "implicit" so we know we have to install them
    // and we can save the correct status in the package once installed
    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < tree.size();
                                         ++idx)
    {
        if(tree[idx].get_type() == package_item_t::package_type_explicit)
        {
            find_dependencies(tree, idx, missing, held );
        }
    }

    return missing_count == missing.size() && held_count == held.size();
}


/** \brief Compare two trees to see whether they are practically identical.
 *
 * Tests whether two installation trees are "practically identical".
 * For our purposes, "practically identical" means that the two trees
 * will install the same versions of the same packages.
 *
 * \todo
 * This function is not exactly logical. We may want to look into the
 * exact reason why we need to do this test. Could it be that the
 * compare_trees() should return that trees are equal and not generate
 * an error in that case?
 *
 * \param[in] left  The first tree to compare
 * \param[in] right  The second tree to compare
 *
 * \returns Returns \c true if both trees will install the same versions
 *          of the same packages, \c false if not.
 */
bool wpkgar_install::trees_are_practically_identical(
    const wpkgar_package_list_t& left,
    const wpkgar_package_list_t& right) const
{
    // A functor for testing the equivalence of package versions. This is
    // hoisted out here to make the comparison loop below easier to read.
    // Ideally this would be floating outside the function altogether,
    // but there's not much point while the types it references are only
    // defined inside wpkgar_install.
    struct is_equivalent : std::unary_function<const package_item_t&, bool>
    {
        is_equivalent(const package_item_t& pkg)
            : f_lhs(pkg)
        {
        }

        // equality means:
        //   both are marked for installation
        //   exact same name
        //   exact same version
        bool operator () (const package_item_t& rhs) const
        {
            if(rhs.is_marked_for_install())
            {
                if(f_lhs.get_name() == rhs.get_name())
                {
                    int cmp(wpkg_util::versioncmp(f_lhs.get_version(),
                                                    rhs.get_version()));
                    return cmp == 0;
                }
            }
            return false;
        }

        // this is ugly, we use this function to count the number of
        // packages to be installed...
        static bool is_marked_for_install(const package_item_t& p)
        {
            return p.is_marked_for_install();
        }

    private:
        const package_item_t& f_lhs;
    };

    // The function proper actually begins here.

    // Check the number of installable packages on either side; if they do not
    // match then they *cannot possibly* be considered identical.
    const size_t left_count(std::count_if(left.begin(), left.end(), is_equivalent::is_marked_for_install));
    const size_t right_count(std::count_if(right.begin(), right.end(), is_equivalent::is_marked_for_install));
    if(left_count != right_count)
    {
        return false;
    }

    // If we get to here, then we have the same number of packages to install
    // on either side. Let's run through the LHS and check whether each installable
    // pkg has an equivalent on the RHS.
    for(wpkgar_package_index_t i(0); i < left.size(); ++i)
    {
        const package_item_t& left_pkg(left[i]);
        if(left_pkg.is_marked_for_install())
        {
            wpkgar_package_list_t::const_iterator rhs = find_if(right.begin(),
                                                                right.end(),
                                                                is_equivalent(left_pkg));
            if(rhs == right.end())
            {
                return false;
            }
        }
    }

    return true;
}


int wpkgar_install::compare_trees(const wpkgar_package_list_t& left, const wpkgar_package_list_t& right) const
{
    // comparing both trees we keep the one that has packages
    // with larger versions; if left has the largest then the
    // function returns 1, if right has the largest then the
    // function returns -1 (similar to a strcmp() call.)
    //
    // if both trees have larger versions then it's a tie and
    // we return 0 instead; this happens as many packages are
    // included and if package left.A > right.A but
    // left.B < right.B then the computer cannot select
    // automatically...
    //
    // note that this test ignores the fact that a package is
    // on the left and not on the right or vice versa. As far
    // as I can tell it is not really possible to distinguish
    // A from B when unmatched packages are found on one side
    // or the other

    int result(0);
    for(wpkgar_package_list_t::size_type i(0);
                                         i < left.size();
                                         ++i)
    {
        f_manager->check_interrupt();

        switch(left[i].get_type())
        {
        case package_item_t::package_type_explicit:
        case package_item_t::package_type_implicit:
        case package_item_t::package_type_configure:
        case package_item_t::package_type_upgrade:
        case package_item_t::package_type_upgrade_implicit:
        case package_item_t::package_type_downgrade:
            {
                std::string name(left[i].get_name());
                for(wpkgar_package_list_t::size_type j(0);
                                                     j < right.size();
                                                     ++j)
                {
                    switch(right[j].get_type())
                    {
                    case package_item_t::package_type_explicit:
                    case package_item_t::package_type_implicit:
                    case package_item_t::package_type_configure:
                    case package_item_t::package_type_upgrade:
                    case package_item_t::package_type_upgrade_implicit:
                    case package_item_t::package_type_downgrade:
                        if(name == right[j].get_name())
                        {
                            // found similar packages, check versions
                            int r(wpkg_util::versioncmp(left[i].get_version(), right[j].get_version()));
                            if(r != 0) // ignore if equal
                            {
                                if(result == 0)
                                {
                                    result = r;
                                }
                                else if(result != r)
                                {
                                    // computer indecision...
                                    return 0;
                                }
                            }
                        }
                        break;

                    default:
                        // other types are not valid for installation
                        break;

                    }
                }
            }
            break;

        default:
            // other types are not marked for installation
            break;

        }
    }

    return result;
}


void wpkgar_install::output_tree(int file_count, const wpkgar_package_list_t& tree, const std::string& sub_title)
{
    memfile::memory_file dot;
    dot.create(memfile::memory_file::file_format_other);
    time_t now(time(NULL));
    dot.printf("// created by wpkg on %s\n"
               "digraph {\n"
               "rankdir=BT;\n"
               "label=\"Packager Dependency Graph (%s)\";\n",
                    ctime(&now),
                    sub_title.c_str());

    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < tree.size();
                                         ++idx)
    {
        f_manager->check_interrupt();

        const char *name(tree[idx].get_name().c_str());
        const char *version(tree[idx].get_version().c_str());
        switch(tree[idx].get_type())
        {
        case package_item_t::package_type_explicit:
            dot.printf("n%d [label=\"%s (exp)\\n%s\",shape=box,color=black]; // EXPLICIT\n", idx, name, version);
            break;

        case package_item_t::package_type_implicit:
            dot.printf("n%d [label=\"%s (imp)\\n%s\",shape=box,color=\"#aa5500\"]; // IMPLICIT\n", idx, name, version);
            break;

        case package_item_t::package_type_available:
            dot.printf("n%d [label=\"%s (avl)\\n%s\",shape=ellipse,color=\"#cccccc\"]; // AVAILABLE\n", idx, name, version);
            break;

        case package_item_t::package_type_not_installed:
            dot.printf("n%d [label=\"%s (not)\\n%s\",shape=box,color=\"#cccccc\"]; // NOT INSTALLED\n", idx, name, version);
            break;

        case package_item_t::package_type_installed:
            dot.printf("n%d [label=\"%s (ins)\\n%s\",shape=box,color=black]; // INSTALLED\n", idx, name, version);
            break;

        case package_item_t::package_type_unpacked:
            dot.printf("n%d [label=\"%s (upk)\\n%s\",shape=ellipse,color=red]; // UNPACKED\n", idx, name, version);
            break;

        case package_item_t::package_type_configure:
            dot.printf("n%d [label=\"%s (cfg)\\n%s\",shape=box,color=purple]; // CONFIGURE\n", idx, name, version);
            break;

        case package_item_t::package_type_upgrade:
            dot.printf("n%d [label=\"%s (upg)\\n%s\",shape=box,color=blue]; // UPGRADE\n", idx, name, version);
            break;

        case package_item_t::package_type_upgrade_implicit:
            dot.printf("n%d [label=\"%s (iup)\\n%s\",shape=box,color=blue]; // UPGRADE IMPLICIT\n", idx, name, version);
            break;

        case package_item_t::package_type_downgrade:
            dot.printf("n%d [label=\"%s (dwn)\\n%s\",shape=box,color=orange]; // DOWNGRADE\n", idx, name, version);
            break;

        case package_item_t::package_type_invalid:
            dot.printf("n%d [label=\"%s (inv)\\n%s\",shape=ellipse,color=red]; // INVALID\n", idx, name, version);
            break;

        case package_item_t::package_type_same:
            dot.printf("n%d [label=\"%s (same)\\n%s\",shape=ellipse,color=black]; // SAME\n", idx, name, version);
            break;

        case package_item_t::package_type_older:
            dot.printf("n%d [label=\"%s (old)\\n%s\",shape=ellipse,color=black]; // OLDER\n", idx, name, version);
            break;

        case package_item_t::package_type_directory:
            dot.printf("n%d [label=\"%s (dir)\\n%s\",shape=ellipse,color=\"#aa5500\"]; // DIRECTORY\n", idx, name, version);
            break;

        }
        for(wpkgar_list_of_strings_t::const_iterator f(f_field_names.begin());
                                                     f != f_field_names.end();
                                                     ++f)
        {
            if(!tree[idx].field_is_defined(*f))
            {
                continue;
            }

            // check the dependencies
            wpkg_dependencies::dependencies depends(tree[idx].get_field(*f));
            for(int i(0); i < depends.size(); ++i)
            {
                const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(i));

                for(wpkgar_package_list_t::size_type j(0);
                                                     j < tree.size();
                                                     ++j)
                {
                    if(d.f_name == tree[j].get_name())
                    {
                        if(match_dependency_version(d, tree[j]) == 1)
                        {
                            dot.printf("n%d -> n%d;\n", idx, j);
                        }
                    }
                }
            }
        }
    }
    dot.printf("}\n");
    std::stringstream stream;
    stream << "install-graph-";
    stream.width(3);
    stream.fill('0');
    stream << file_count;
    stream << ".dot";
    dot.write_file(stream.str());
}


/** \brief Validate the dependency tree.
 *
 * This function creates all the possible dependency trees it can in
 * order to select the best one for installation.
 *
 * As the number of packages increases the number of trees increases
 * quickly so we want to keep the best tree of the moment in memory.
 * So in effect we have a maximum of two trees, the best one and the
 * current one being created.
 *
 * In order to go as fast as possible, we make the assumption that it
 * will all work simply using the best packages (and that's certainly
 * the case 99% of the time). If that fails, then we try again with the
 * next as good as possible tree.
 *
 * Whenever creating the tree, if there is a choice, then we set a flag
 * to true. This way we know we get at least one more chance. When
 * testing the next possibility we use a counter. The counter is
 * decreased by one until the counter returns to zero.
 */
void wpkgar_install::validate_dependencies()
{
    // self-contained with explicit and installed dependencies?
    if(validate_installed_dependencies() != validation_return_missing)
    {
        if((wpkg_output::get_output_debug_flags() & wpkg_output::debug_flags::debug_depends_graph) != 0)
        {
            // output the verified tree
            output_tree(1, f_packages, "no implied packages");
        }
        // although we're not going to have implied targets we
        // still want to run the trimming because it checks
        // things that we need to have validated (conflicts,
        // breaks, etc.)
        f_install_includes_choices = false;
        f_tree_max_depth = 0;
        trim_available_packages();
        return;
    }

    // load all the repository files so we have a complete
    // list in memory which is easier and faster to handle
    //
    // NOTE: if there are no repositories defined on the command line then
    //       this read does nothing and thus the following code will
    //       generate trees and end up with a list of the missing
    //       dependencies which we can then list to the user
    read_repositories();

    if((wpkg_output::get_output_debug_flags() & wpkg_output::debug_flags::debug_depends_graph) != 0)
    {
        // output the verified tree
        output_tree(0, f_packages, "tree with repositories");
    }

    // recursively remove all the "available" (implicit) packages that
    // do not match an explicit/implicit package requirement in terms of
    // version (i.e. version too large or small.) this trimming reduces
    // the number of choices dramatically, assuming we are faced with
    // such choices
    //
    // as a side effect the trimming process detects:
    //
    // \li circular dependencies
    // \li missing dependencies
    // \li whether we have choices
    f_install_includes_choices = false;
    f_tree_max_depth = 0;
    trim_available_packages();

    // if we did not find choices while running through the available
    // packages, then our current tree is enough and we can simple use
    // it for the next step and if it fails, we're done...
    if(!f_install_includes_choices)
    {
        wpkgar_dependency_list_t missing;
        wpkgar_dependency_list_t held;
        if(!verify_tree(f_packages, missing, held))
        {
            std::stringstream ss;
            if( missing.size() > 0 )
            {
                // Tell the user which dependencies are missing...
                //
                ss << "Missing dependencies: [";
                std::string comma;
                std::for_each( missing.begin(), missing.end(), [&ss,&comma]( wpkg_dependencies::dependencies::dependency_t dep )
                {
                    ss << comma;
					ss << dep.f_name << " (" << dep.f_version << ")";
                    comma = ", ";
                });
                ss << "]. Package not installed!";
            }
            else if( held.size() > 0 )
            {
                // Tell the user which dependencies are missing...
                //
                ss << "The following dependencies are in a held state: [";
                std::string comma;
                std::for_each( held.begin(), held.end(), [&ss,&comma]( wpkg_dependencies::dependencies::dependency_t dep )
                {
                    ss << comma;
                    ss << dep.f_name << " (" << dep.f_version << ")";
                    comma = ", ";
                });
                ss << "]. Package not installed!";
            }
            else
            {
                ss << "could not create a complete tree, some dependencies are in conflict, or have incompatible versions (see --debug 4)";
            }

            // dependencies are missing
            wpkg_output::log(ss.str())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .action("install-validation");
        }
        if((wpkg_output::get_output_debug_flags() & wpkg_output::debug_flags::debug_depends_graph) != 0)
        {
            // output the verified tree
            output_tree(1, f_packages, "no choices");
        }
        return;
    }

    // note that here when we want to add dependencies we add them to the
    // implicit list of packages and from now on we have to check both
    // lists to be complete... (explicit + implicit); other lists are
    // ignored except the available while we search for dependencies

    wpkgar_package_list_t best;
    for(tree_generator tree_gen(f_packages);;)
    {
        wpkgar_package_list_t tree(tree_gen.next());
        if(tree.empty())
        {
            if(0 == tree_gen.tree_number())
            {
                // the very first tree cannot fail because count is set to 0
                throw std::logic_error("somehow the very first tree cannot be built properly!?");
            }
            // we're done!
            break;
        }

        wpkgar_dependency_list_t missing;
        wpkgar_dependency_list_t held;
        bool verified(verify_tree(tree, missing, held));

        if((wpkg_output::get_output_debug_flags() & wpkg_output::debug_flags::debug_depends_graph) != 0)
        {
            // output the verified tree
            output_tree(static_cast<int>(tree_gen.tree_number()), tree, verified ? "verified tree" : "failed tree");
        }

        if(verified)
        {
            // it worked, keep it if it is the best
            if(best.empty())
            {
                best = tree;
            }
            else if(!trees_are_practically_identical(tree, best))
            {
                // if both trees are to install the same versions of the
                // same packages, then they are identical for our purposes;
                // so in that case we do not need to compare anything

                int r(compare_trees(tree, best));
                if(r == 0)
                {
                    // we've got a problem!
                    // TODO: from what I can see, tree & best could both be
                    //       eliminated by another tree that has only larger
                    //       packages than both tree & best so this
                    //       error is coming up too early at this point...
                    //       however to support such we'd have to memorize
                    //       all those trees and that could be quite a lot
                    //       of them!
                    wpkg_output::log("found two trees that are considered similar. This means the computer cannot choose between two implicit dependencies. You will have to add dependencies to your command line to resolve the issue.")
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .action("install-validation");
                }
                else if(r > 0)
                {
                    // tree is viewed as better so keep that instead
                    best = tree;
                }
            }
        }
    }
    if(best.empty())
    {
        // some dependencies are missing...
        wpkg_output::log("could not create a complete tree, some dependencies are missing")
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_validate_installation)
            .action("install-validation");
        return;
    }

    // just keep the best, all the other trees we can discard
    f_packages = best;
}


#if !defined(MO_DARWIN) && !defined(MO_SUNOS) && !defined(MO_FREEBSD)
namespace details
{


/** \brief The disk class to manage installation sizes.
 *
 * This class handles disk metadeta when computing the installation
 * sizes of a package (i.e. evaluating whether the destination disk
 * is large enough to accept the packages that are about to be
 * installed on it.)
 *
 * This class has a function called match() which detects whether
 * a given filename represents a file to be installed on that disk.
 *
 * \note
 * At this time this functionality is not available at all on any system
 * because there are problems on all except Linux. The current Microsoft
 * Windows implementation does not properly detect what a letter drive
 * really represents and as such would revert to C:\\ and often generate
 * and error when the other drive(s) where the packages would really
 * be installed were more than large enough.
 */
class disk_t
{
public:
    disk_t(const wpkg_filename::uri_filename& path);

    const wpkg_filename::uri_filename& get_path() const;
    bool match(const wpkg_filename::uri_filename& path);
    void add_size(int64_t size);
    //int64_t size() const;
    void set_block_size(uint64_t block);
    void set_free_space(uint64_t space);
    //uint64_t free_space() const;
    void set_readonly();
    bool is_valid() const;

private:
    wpkg_filename::uri_filename     f_path;
    controlled_vars::zint64_t       f_size;
    controlled_vars::zint64_t       f_block_size;
    controlled_vars::zuint64_t      f_free_space;
    controlled_vars::fbool_t        f_readonly;
};

disk_t::disk_t(const wpkg_filename::uri_filename& path)
    : f_path(path)
    //, f_size(0) -- auto-init
    //, f_block(0) -- auto-init
    //, f_free_space(0) -- auto-init
    //, f_readonly(false) -- auto-init
{
}

const wpkg_filename::uri_filename& disk_t::get_path() const
{
    return f_path;
}

bool disk_t::match(const wpkg_filename::uri_filename& path)
{
    return f_path.full_path() == path.full_path().substr(0, f_path.full_path().length());
}

void disk_t::add_size(int64_t size)
{
    if(f_readonly)
    {
        throw wpkgar_exception_io("package cannot be installed on " + f_path.original_filename() + " since it is currently mounted as read-only");
    }
    // use the ceiling of (size / block size)
    // Note: size may be negative when the file is being removed or upgraded
    f_size += (size + f_block_size - 1) / f_block_size;

    // note: we do not add anything for the directory entry which is most
    // certainly wrong although I think that the size very much depends
    // on the file system and for very small files it may even use the
    // directory entry to save the file data (instead of a file chain
    // as usual)
}

//int64_t disk_t::size() const
//{
//    return f_size * f_block_size;
//}

void disk_t::set_block_size(uint64_t block_size)
{
    f_block_size = block_size;
}

void disk_t::set_free_space(uint64_t space)
{
    f_free_space = space;
}

//uint64_t disk_t::free_space() const
//{
//    return f_free_space;
//}

void disk_t::set_readonly()
{
    // size should still be zero when we call this function, but if not
    // we still want an error
    if(0 != f_size)
    {
        throw wpkgar_exception_io("package cannot be installed on " + f_path.original_filename() + " since it is currently mounted as read-only");
    }
    f_readonly = true;
}

bool disk_t::is_valid() const
{
    // if we're saving space then it's always valid
    if(f_size <= 0)
    {
        return true;
    }
    // leave a 10% margin for all the errors in our computation
    return static_cast<uint64_t>(f_size * f_block_size) < f_free_space * 9 / 10;
}



/** \brief The list of all the disks accessible on this system.
 *
 * As we are installing files on one or more disks, this object holds a
 * list of all the disks that were accessed so far.
 *
 * This class offers an add_size() function which checks by going through
 * the list of disks which one is a match. Once the match was found, it
 * then calls the add_size() on that disk.
 *
 * The compute_size_and_verify_overwrite() function is used to find out
 * whether all the disks have enough room for all the data being added
 * to each one of them.
 */
class disk_list_t
{
public:
    disk_list_t(wpkgar_manager *manager, wpkgar_install *install);
    void add_size(const std::string& path, int64_t size);
    void compute_size_and_verify_overwrite(const wpkgar_install::wpkgar_package_list_t::size_type idx, const wpkgar_install::package_item_t& item, const wpkg_filename::uri_filename& root, memfile::memory_file *data, memfile::memory_file *upgrade, int factor);
    bool are_valid() const;

private:
    typedef std::map<std::string, memfile::memory_file::file_info> filename_info_map_t;

    disk_t *find_disk(const std::string& path);

    wpkgar_manager *        f_manager;
    wpkgar_install *        f_install;
    std::vector<disk_t>     f_disks;
    disk_t *                f_default_disk; // used in win32 only
    filename_info_map_t     f_filenames;
};


disk_list_t::disk_list_t(wpkgar_manager *manager, wpkgar_install *install)
    : f_manager(manager),
      f_install(install),
      //f_disks() -- auto-init
      f_default_disk(NULL)
      //f_filenames() -- auto-init
{
#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
    // limit ourselves to regular drives as local drives are all
    // defined in this way
    DWORD drives(GetLogicalDrives());
    for(int d(0); d < 26; ++d)
    {
        if((drives & (1 << d)) != 0)
        {
            // drive is defined!
            std::string letter;
            letter = static_cast<char>('a' + d);
            letter += ":/";
            if(GetDriveTypeA(letter.c_str()) == DRIVE_FIXED)
            {
                // only accept hard drives
                DWORD sectors_per_cluster;
                DWORD bytes_per_sector;
                DWORD number_of_free_clusters;
                DWORD total_number_of_clusters;
                if(GetDiskFreeSpace(letter.c_str(), &sectors_per_cluster, &bytes_per_sector, &number_of_free_clusters, &total_number_of_clusters))
                {
                    // we could gather the total size, keep this entry
                    disk_t disk(letter);
                    disk.set_free_space(bytes_per_sector * sectors_per_cluster * number_of_free_clusters);
                    disk.set_block_size(bytes_per_sector);

                    // check whether that partition is read-only
                    DWORD volume_serial_number;
                    DWORD maximum_component_length;
                    DWORD file_system_flags;
                    if(GetVolumeInformation(letter.c_str(), NULL, 0, &volume_serial_number, &maximum_component_length, &file_system_flags, NULL, 0))
                    {
                        if((file_system_flags & FILE_READ_ONLY_VOLUME) != 0)
                        {
                            disk.set_readonly();
                        }
                    }

                    // save that disk in our vector
                    f_disks.push_back(disk);
                }
            }
        }
    }

    // include the / folder using the correct information
    wchar_t cwd[4096];
    if(GetCurrentDirectoryW(sizeof(cwd) / sizeof(cwd[0]), cwd) == 0)
    {
        throw std::runtime_error("failed reading current working directory (more than 4096 character long?)");
    }
    if(wcslen(cwd) < 3)
    {
        throw std::runtime_error("the name of the current working directory is too short");
    }
    if(cwd[1] != L':' || cwd[2] != L'\\'
    || ((cwd[0] < L'a' || cwd[0] > L'z') && (cwd[0] < L'A' || cwd[0] > L'Z')))
    {
        // TODO: add support for \\foo\blah (network drives)
        throw std::runtime_error("the name of the current working directory does not start with a drive name (are you on a network drive? this is not currently supported.)");
    }
    cwd[3] = L'\0';     // end the string
    cwd[2] = L'/';      // change \\ to /
    cwd[0] |= 0x20;     // lowercase (this works in UCS-2 as well since this is an ASCII letter)
    disk_t *def(find_disk(libutf8::wcstombs(cwd)));
    if(def == NULL)
    {
        throw std::runtime_error("the name of the drive found in the current working directory \"" + libutf8::wcstombs(cwd) + "\" is not defined in the list of existing directories");
    }
    f_default_disk = def;

#elif defined(MO_LINUX)
    // RAII on f_mounts
    class mounts
    {
    public:
        mounts()
            : f_mounts(setmntent("/etc/mtab", "r")),
              f_entry(NULL)
        {
            if(f_mounts == NULL)
            {
                throw std::runtime_error("packager could not open /etc/mtab for reading");
            }
        }

        ~mounts()
        {
            endmntent(f_mounts);
        }

        struct mntent *next()
        {
            f_entry = getmntent(f_mounts);
            // TODO: if NULL we reached the end or had an error
            return f_entry;
        }

        bool has_option(const char *opt)
        {
            if(f_entry == NULL)
            {
                std::logic_error("has_option() cannot be called before next()");
            }
            return hasmntopt(f_entry, opt) != NULL;
        }

    private:
        FILE *              f_mounts;
        struct mntent *     f_entry;
    };

    mounts m;
    for(;;)
    {
        struct mntent *e(m.next());
        if(e == NULL)
        {
            // EOF
            break;
        }
        // ignore unusable disks and skip network disks too
        if(strcmp(e->mnt_type, MNTTYPE_IGNORE) != 0
        && strcmp(e->mnt_type, MNTTYPE_NFS) != 0
        && strcmp(e->mnt_type, MNTTYPE_SWAP) != 0)
        {
            struct statvfs s;
            if(statvfs(e->mnt_dir, &s) == 0)
            {
                if(s.f_bfree != 0)
                {
                    disk_t disk(e->mnt_dir);
                    // Note: f_bfree is larger than f_bavail and most certainly
                    //       includes kernel reserved blocks that even root
                    //       cannot access while installing packages
                    disk.set_free_space(s.f_bavail * s.f_bsize);
                    disk.set_block_size(s.f_bsize);
                    if(m.has_option("ro"))
                    {
                        disk.set_readonly();
                    }
//printf("Got disk [%s] %ld, %ld, %s\n", e->mnt_dir, s.f_bavail, s.f_frsize, m.has_option("ro") ? "ro" : "r/w");
                    f_disks.push_back(disk);
                }
            }
        }
    }
#else
#   error This platform is not yet supported!
#endif
}

disk_t *disk_list_t::find_disk(const std::string& path)
{
    // we want to keep the longest as it represents the real mount point
    // (i.e. we must select /usr and not /)
#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
    std::string drive(path.substr(0, 3));
    if(drive.length() < 3
    || drive[1] != ':' || drive[2] != '/'
    || ((drive[0] < 'a' || drive[0] > 'z') && (drive[0] < 'A' || drive[0] > 'Z')))
    {
        // no drive is specified in that path, use the default drive instead!
        // TODO: support //network/folder syntax for network drives
        return f_default_disk;
    }
#endif
    std::string::size_type max(0);
    std::vector<disk_t>::size_type found(static_cast<std::vector<disk_t>::size_type>(-1));
    for(std::vector<disk_t>::size_type i(0); i < f_disks.size(); ++i)
    {
        std::string::size_type l(f_disks[i].get_path().full_path().length());
        if(l > max && f_disks[i].match(path))
        {
            max = l;
            found = i;
        }
    }
    if(found != static_cast<std::vector<disk_t>::size_type>(-1))
    {
        return &f_disks[found];
    }
    return NULL;
}

void disk_list_t::add_size(const std::string& path, int64_t size)
{
    disk_t *d(find_disk(path));
    if(d != NULL)
    {
        d->add_size(size);
    }
    else
    {
        wpkg_output::log("cannot find partition for %1.")
                .quoted_arg(path)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_validate_installation)
            .action("install-validation");
    }
}

void disk_list_t::compute_size_and_verify_overwrite(const wpkgar_install::wpkgar_package_list_t::size_type idx,
                                                    const wpkgar_install::package_item_t& item,
                                                    const wpkg_filename::uri_filename& root,
                                                    memfile::memory_file *data,
                                                    memfile::memory_file *upgrade,
                                                    const int factor)
{
    const wpkg_filename::uri_filename package_name(item.get_filename());
    memfile::memory_file::file_info info;

    // if we have an upgrade package then we want to get all the filenames
    // first to avoid searching that upgrade package for every file we find
    // in the new package being installed; we use that data file only to
    // determine whether an overwrite is normal or not
    std::map<std::string, memfile::memory_file::file_info> upgrade_files;
    if(upgrade != NULL)
    {
        upgrade->dir_rewind();
        while(upgrade->dir_next(info))
        {
            upgrade_files[info.get_filename()] = info;
        }
    }

    // placed here because VC++ "needs" an initialization (which is
    // wrong, but instead of hiding the warning...)
    wpkg_filename::uri_filename::file_stat s;

    data->dir_rewind();
    while(data->dir_next(info))
    {
        std::string path(info.get_filename());
        if(path[0] != '/')
        {
            // files that do not start with a slash are part of the database
            // only so we ignore them here
            continue;
        }
        if(factor == 1)
        {
            filename_info_map_t::const_iterator it(f_filenames.find(path));
            if(it != f_filenames.end())
            {
                // this is not an upgrade (downgrade) so the filename must be
                // unique otherwise two packages being installed are in conflict
                // note that in this case we do not check for the
                // --force-overwrite flags... (should we allow such here?)
                if(info.get_file_type() != memfile::memory_file::file_info::directory
                || it->second.get_file_type() != memfile::memory_file::file_info::directory)
                {
                    wpkg_output::log("file %1 from package %2 also exists in %3.")
                            .quoted_arg(info.get_filename())
                            .quoted_arg(package_name)
                            .quoted_arg(it->second.get_package_name())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(package_name)
                        .action("install-validation");
                }
            }
            else
            {
                info.set_package_name(package_name.original_filename());
                f_filenames[path] = info;
            }
        }
        int64_t size(info.get_size());
        switch(info.get_file_type())
        {
        case memfile::memory_file::file_info::regular_file:
        case memfile::memory_file::file_info::continuous:
            // if regular, then the default size works
            break;

        case memfile::memory_file::file_info::directory:
            // at this time we may not remove directories so
            // we keep this size if upgrading
            if(factor < 0)
            {
                size = 0;
                break;
            }
        default:
            // otherwise use at least 1 block
            if(size < 512)
            {
                // note that this adds 1 block whether it is 512,
                // 1024, 2048, 4096, etc. (we assume blocks are
                // never less than 512 though.)
                size = 512;
            }
            break;

        }
        // note that we want to call add_size() even if the
        // size is zero because the add_size() function
        // verifies that path is writable
        add_size(path, size * factor);

        // check whether the file already exists, and if so whether
        // we're upgrading because if so, we're fine--note that we
        // allow an overwrite only of a file from the same package
        // (same Package field name); later we may support a Replace
        // in which case the names could differ
        //
        // IMPORTANT NOTE: if the file is a configuration file, then
        // it shouldn't exist if we are installing that package for
        // the first time and if that's an upgrade then we need
        // the file to be present in the package being upgraded
        //
        // note that any number of packages can have the same
        // directory defined and that is silently "overwritten";
        // however, a directory cannot be overwritten by a regular
        // file and vice versa unless you have the --force-overwrite-dir
        // and that's certainly a very bad idea
        //
        // We run that test only against explicit and implicit packages
        // since installed (upgrade) packages are... installed and thus
        // their files exist on the target!
        if(factor > 0 && root.append_child(path).os_stat(s) == 0)
        {
            // it already exists, so we're overwriting it...
            bool a(info.get_file_type() != memfile::memory_file::file_info::directory);
            bool b(!s.is_dir());
            if(a && b)
            {  // both are regular files
                // are we upgrading?
                if(upgrade_files.find(info.get_filename()) == upgrade_files.end())
                {
                    // first check whether this is a file in an Essential package
                    // because if so we ALWAYS prevent the overwrite
                    if(f_install->find_essential_file(info.get_filename(), idx))
                    {
                        // use a fatal error because that's pretty much what it is
                        // (i.e. there isn't a way to prevent the error from occurring)
                        wpkg_output::log("file %1 from package %2 already exists on your target system and it cannot be overwritten because the owner is an essential package.")
                                .quoted_arg(info.get_filename())
                                .quoted_arg(package_name)
                            .level(wpkg_output::level_fatal)
                            .module(wpkg_output::module_validate_installation)
                            .package(package_name)
                            .action("install-validation");
                    }
                    else
                    {
                        // last chance, is that a configuration file?
                        // if so we deal with those later...
                        if(!item.is_conffile(path))
                        {
                            // bad bad bad!
                            if(f_install->get_parameter(wpkgar_install::wpkgar_install_force_overwrite, false))
                            {
                                wpkg_output::log("file %1 from package %2 already exists on your target system and it will get overwritten.")
                                        .quoted_arg(info.get_filename())
                                        .quoted_arg(package_name)
                                    .level(wpkg_output::level_warning)
                                    .module(wpkg_output::module_validate_installation)
                                    .package(package_name)
                                    .action("install-validation");
                            }
                            else
                            {
                                wpkg_output::log("file %1 from package %2 already exists on your target system.")
                                        .quoted_arg(info.get_filename())
                                        .quoted_arg(package_name)
                                    .level(wpkg_output::level_error)
                                    .module(wpkg_output::module_validate_installation)
                                    .package(package_name)
                                    .action("install-validation");
                            }
                        }
                    }
                }
            }
            else if(a ^ b)
            {  // one is a directory and the other is not
                // are we upgrading?
                if(upgrade_files.find(info.get_filename()) == upgrade_files.end())
                {
                    if(f_install->get_parameter(wpkgar_install::wpkgar_install_force_overwrite_dir, false))
                    {
                        // super bad!
                        if(a)
                        {
                            // TODO:
                            // forbid this no matter what when the directory to
                            // be overwritten is defined in an essential package
                            wpkg_output::log("file %1 from package %2 will replace directory of the same name (IMPORTANT NOTE: the contents of that directory will be lost!)")
                                    .quoted_arg(info.get_filename())
                                    .quoted_arg(package_name)
                                .level(wpkg_output::level_warning)
                                .module(wpkg_output::module_validate_installation)
                                .package(package_name)
                                .action("install-validation");
                        }
                        else
                        {
                            wpkg_output::log("directory %1 from package %2 will replace the regular file of the same name.")
                                    .quoted_arg(info.get_filename())
                                    .quoted_arg(package_name)
                                .level(wpkg_output::level_warning)
                                .module(wpkg_output::module_validate_installation)
                                .package(package_name)
                                .action("install-validation");
                        }
                    }
                    else
                    {
                        if(b)
                        {
                            wpkg_output::log("file %1 already exists on your target system and package %2 would like to create a directory in its place.")
                                    .quoted_arg(path)
                                    .quoted_arg(package_name)
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_validate_installation)
                                .package(package_name)
                                .action("install-validation");
                        }
                        else
                        {
                            wpkg_output::log("directory %1 already exists on your target system and package %2 would like to create a regular file in its place.")
                                    .quoted_arg(path)
                                    .quoted_arg(package_name)
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_validate_installation)
                                .package(package_name)
                                .action("install-validation");
                        }
                    }
                }
                else
                {
                    // in this case we emit a warning because a package
                    // should not transform a file into a directory or
                    // vice versa (bad practice!) but we still allow it
                    if(a)
                    {
                        wpkg_output::log("package %1 is requesting directory %2 to be replaced by a regular file.")
                                .quoted_arg(package_name)
                                .quoted_arg(info.get_filename())
                            .level(wpkg_output::level_warning)
                            .module(wpkg_output::module_validate_installation)
                            .package(package_name)
                            .action("install-validation");
                    }
                    else
                    {
                        wpkg_output::log("package %1 is requesting file %2 to be replaced by a directory.")
                                .quoted_arg(package_name)
                                .quoted_arg(info.get_filename())
                            .level(wpkg_output::level_warning)
                            .module(wpkg_output::module_validate_installation)
                            .package(package_name)
                            .action("install-validation");
                    }
                }
            }
            // else -- both are directories so we can ignore the "overwrite"
        }
    }
}

bool disk_list_t::are_valid() const
{
    for(std::vector<disk_t>::size_type i(0); i < f_disks.size(); ++i)
    {
        if(!f_disks[i].is_valid())
        {
            return false;
        }
    }
    return true;
}






}   // no-name namespace
#endif //!MO_DARWIN && !MO_SUNOS && !MO_FREEBSD




bool wpkgar_install::find_essential_file(std::string filename, const size_t skip_idx)
{
    // filename should never be empty
    if(filename.empty())
    {
        throw std::runtime_error("somehow a package filename is the empty string");
    }
    if(filename[0] != '/')
    {
        filename = "/" + filename;
    }

    if(!f_read_essentials)
    {
        f_read_essentials = true;
        for(wpkgar_package_list_t::size_type idx(0);
                                             idx < f_packages.size();
                                             ++idx)
        {
            if(static_cast<wpkgar_package_list_t::size_type>(skip_idx) == idx)
            {
                // this is the package we're working on and obviously
                // filename will exist in this package
                continue;
            }
            // any package that is already installed or unpacked
            // or that is about to be installed is checked
            switch(f_packages[idx].get_type())
            {
            case package_item_t::package_type_explicit:
            case package_item_t::package_type_implicit:
            case package_item_t::package_type_installed:
            case package_item_t::package_type_unpacked:
            case package_item_t::package_type_configure:
            case package_item_t::package_type_upgrade:
            case package_item_t::package_type_upgrade_implicit:
            case package_item_t::package_type_downgrade:
                break;

            default:
                // invalid packages can be ignored
                continue;

            }

            // Is this an Essential package?
            if(!f_packages[idx].field_is_defined(wpkg_control::control_file::field_essential_factory_t::canonicalized_name())
            || !f_packages[idx].get_boolean_field(wpkg_control::control_file::field_essential_factory_t::canonicalized_name()))
            {
                // the default value of the Essential field is "No" (false)
                continue;
            }

            // TODO: change this load and use the Files field instead
            // make sure the package is loaded
            f_manager->load_package(f_packages[idx].get_filename());

            // check all the files defined in the data archive
            memfile::memory_file *wpkgar_file;
            f_manager->get_wpkgar_file(f_packages[idx].get_filename(), wpkgar_file);

            wpkgar_file->dir_rewind();
            for(;;)
            {
                memfile::memory_file::file_info info;
                if(!wpkgar_file->dir_next(info, NULL))
                {
                    break;
                }
                std::string file(info.get_filename());
                if(file[0] == '/')
                {
                    // only keep filenames from the data archive
                    f_essential_files.push_back(file);
                }
            }
        }
    }

    // in case we have many files, we memorized the list of essential
    // files so that way we can quickly search that list in memory
    return std::find(f_essential_files.begin(), f_essential_files.end(), filename) != f_essential_files.end();
}



void wpkgar_install::validate_packager_version()
{
    // note: at this point we have one valid tree to be installed

    // already installed packages are ignore here
    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < f_packages.size();
                                         ++idx)
    {
        switch(f_packages[idx].get_type())
        {
        case package_item_t::package_type_explicit:
        case package_item_t::package_type_implicit:
            {
                // full path to package
                const wpkg_filename::uri_filename filename(f_packages[idx].get_filename());

                // get list of dependencies if any
                if(f_packages[idx].field_is_defined(wpkg_control::control_file::field_packagerversion_factory_t::canonicalized_name()))
                {
                    const std::string build_version(f_packages[idx].get_field(wpkg_control::control_file::field_packagerversion_factory_t::canonicalized_name()));
                    int c(wpkg_util::versioncmp(debian_packages_version_string(), build_version));
                    // our version is expected to be larger or equal in which case
                    // we're good; if we're smaller, then we may not be 100%
                    // compatible (and in some cases, 0%... which will be caught
                    // here once we have such really bad cases.)
                    if(c < 0)
                    {
                        wpkg_output::log("package %1 was build with packager v%2 which may not be 100%% compatible with this packager v%3.")
                                .quoted_arg(f_packages[idx].get_name())
                                .arg(build_version)
                                .arg(debian_packages_version_string())
                            .module(wpkg_output::module_validate_installation)
                            .package(filename)
                            .action("install-validation");
                    }
                }
                else
                {
                    wpkg_output::log("package %1 does not define a Packager-Version field. It was not created using wpkg and it may not install properly.")
                            .quoted_arg(f_packages[idx].get_name())
                        .level(wpkg_output::level_warning)
                        .module(wpkg_output::module_validate_installation)
                        .package(filename)
                        .action("install-validation");
                }
            }
            break;

        default:
            // other packages are already installed so it's not our
            // concern
            break;

        }
    }
}



/** \brief Ensure that enough space is available and no file gets overwritten.
 *
 * The installed size requires us to determine the list of drives
 * and the size of each drive and then to check the path of each
 * file and compute an approximative amount of space that each
 * file is likely to take on each drive.
 *
 * Note that we do NOT use the Installed-Size field since it does
 * not represent a size in blocks for the target machine (not
 * likely) and it does not represent a size specific to each
 * drive where data is to be installed.
 *
 * Under Linux, we can use the setmntent(), getmntent(), and
 * endmntent() functions to retrieve the current list of mounted
 * partitions/drives. The statvfs() function can then be used
 * to have information about the available space on each one of
 * those drives. Use man for details about those functions.
 *
 * Under MS-Windows, we want to call the GetLogicalDrives()
 * function to get a complete list of all the drives available
 * (A:, B:, C:, etc.) We can then use the GetDriveType() and
 * limit the installation to local drives (fixed). Finally, we
 * can call GetDiskFreeSpace() to get the number of free clusters
 * and their size. The decimation is not as precise as under Linux,
 * but it should be enough for our function which is not perfect
 * anyway.
 *
 * Details about the MS-Windows API in that regard can be found
 * here: http://msdn.microsoft.com/en-US/library/aa365730.aspx
 *
 * Note that as a side effect, since we know whether a volume
 * was mounted in read/write or read-only mode, we right away
 * fail if it were read-only.
 *
 * This function also determines whether a package being installed
 * is actually upgrading an already installed package.
 */
void wpkgar_install::validate_installed_size_and_overwrite()
{
#if !defined(MO_DARWIN) && !defined(MO_SUNOS) && !defined(MO_FREEBSD)
    details::disk_list_t     disks(f_manager, this);
#endif

    const wpkg_filename::uri_filename root(f_manager->get_inst_path());
    controlled_vars::zuint32_t total;
    int32_t idx = -1;
    for( auto& outer_pkg : f_packages )
    {
        ++idx;
        int factor(0);
        memfile::memory_file *upgrade(NULL);
        switch(outer_pkg.get_type())
        {
        case package_item_t::package_type_upgrade:
        case package_item_t::package_type_upgrade_implicit:
        case package_item_t::package_type_downgrade:
            // here the factor is -1 as we remove the size of
            // this package from the installation
            factor = -1;
            break;

        case package_item_t::package_type_explicit:
        case package_item_t::package_type_implicit:
            factor = 1;
            {
                // we want the corresponding upgrade (downgrade) package
                // because we use that for our overwrite test
                const std::string& name(outer_pkg.get_name());
                int32_t j = -1;
                for( auto& inner_pkg : f_packages )
                {
                    ++j;
                    switch(inner_pkg.get_type())
                    {
                    case package_item_t::package_type_upgrade:
                    case package_item_t::package_type_downgrade:
                        if(name == inner_pkg.get_name())
                        {
                            f_manager->check_interrupt();

                            // make sure the package is loaded
                            // TODO: change this load and use the Files field instead
                            f_manager->load_package(inner_pkg.get_filename());

                            f_manager->get_wpkgar_file(inner_pkg.get_filename(), upgrade);
                            if(outer_pkg.get_upgrade() != -1
                            || inner_pkg.get_upgrade() != -1)
                            {
                                throw std::logic_error("somehow more than two packages named \"" + name + "\" were marked for upgrade.");
                            }
                            // link these packages together
                            inner_pkg.set_upgrade( idx );
                            outer_pkg.set_upgrade( j   );

                            // in case we're a self package add ourselves since
                            // we're being upgraded
                            f_manager->include_self(name);
                        }
                        break;

                    default:
                        // we're looking for upgrades only
                        break;

                    }
                }
            }
            break;

        default:
            // other packages are either already installed,
            // not installed, or marked invalid in some ways
            break;
        }
//
// TODO: There is no drive detection under Darwin / SunOS presently implemented!
//
#if !defined(MO_DARWIN) && !defined(MO_SUNOS) && !defined(MO_FREEBSD)
        if(factor != 0 && (f_installing_packages || f_unpacking_packages))
        {
            f_manager->check_interrupt();

            // TODO: change this load and use the Files field instead
            //       make sure the package is loaded
            // (as far as I can tell this is really fast if the package
            // was already loaded so we're certainly safe doing again.)
            f_manager->load_package(outer_pkg.get_filename());

            memfile::memory_file *data(0);
            f_manager->get_wpkgar_file(outer_pkg.get_filename(), data);
            disks.compute_size_and_verify_overwrite( idx, outer_pkg, root, data, upgrade, factor );
        }
#endif //!MO_DARWIN && !MO_SUNOS && !MO_FREEBSD
    }
    // for( auto& outer_pkg : f_packages )

    // got all the totals, make sure its valid
    //if(!disks.are_valid())
    //{
    //    wpkg_output::log("the space available on your disks is not enough to support this installation.")
    //        .level(wpkg_output::level_error)
    //        .module(wpkg_output::module_validate_installation)
    //        .action("install-validation");
    //}
}


/** \brief Ensure that enough space is available and no file gets overwritten.
 *
 * The installed size requires us to determine the list of drives
 * and the size of each drive and then to check the path of each
 * file and compute an approximative amount of space that each
 * file is likely to take on each drive.
 *
 * Note that we do NOT use the Installed-Size field since it does
 * not represent a size in blocks for the target machine (not
 * likely) and it does not represent a size specific to each
 * drive where data is to be installed.
 *
 * Under Linux, we can use the setmntent(), getmntent(), and
 * endmntent() functions to retrieve the current list of mounted
 * partitions/drives. The statvfs() function can then be used
 * to have information about the available space on each one of
 * those drives. Use man for details about those functions.
 *
 * Under MS-Windows, we want to call the GetLogicalDrives()
 * function to get a complete list of all the drives available
 * (A:, B:, C:, etc.) We can then use the GetDriveType() and
 * limit the installation to local drives (fixed). Finally, we
 * can call GetDiskFreeSpace() to get the number of free clusters
 * and their size. The decimation is not as precise as under Linux,
 * but it should be enough for our function which is not perfect
 * anyway.
 *
 * Details about the MS-Windows API in that regard can be found
 * here: http://msdn.microsoft.com/en-US/library/aa365730.aspx
 *
 * Note that as a side effect, since we know whether a volume
 * was mounted in read/write or read-only mode, we right away
 * fail if it were read-only.
 */
void wpkgar_install::validate_fields()
{
    // if there are not validations, return immediately
    if(f_field_validations.empty())
    {
        return;
    }

    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < f_packages.size();
                                         ++idx)
    {
        switch(f_packages[idx].get_type())
        {
        case package_item_t::package_type_explicit:
        case package_item_t::package_type_implicit:
            // we want the corresponding upgrade (downgrade) package
            // because we use that for our overwrite test
            for(wpkgar_package_list_t::size_type j(0);
                                                 j < f_field_validations.size();
                                                 ++j)
            {
                f_manager->check_interrupt();

                const std::string& name(f_packages[idx].get_name());
                if(!f_packages[idx].validate_fields(f_field_validations[j]))
                {
                    wpkg_output::log("package %1 did not validate against %2.")
                            .quoted_arg(name)
                            .quoted_arg(f_field_validations[j])
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(name)
                        .action("install-validation");
                }
            }
            break;

        default:
            // other packages are either already installed,
            // not installed, or marked invalid in some ways
            break;

        }
    }
}





void wpkgar_install::sort_package_dependencies(const std::string& name, wpkgar_package_listed_t& listed)
{
    // note: we do not check the depth limit here because we already
    //       have done so in a validation function

    // check whether this package was already handled
    if(listed.find(name) != listed.end())
    {
        return;
    }

    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < f_packages.size();
                                         ++idx)
    {
        if(f_packages[idx].get_name() == name)
        {
            f_manager->check_interrupt();

            switch(f_packages[idx].get_type())
            {
            case package_item_t::package_type_explicit:
            case package_item_t::package_type_implicit:
                // check dependencies because they need to be added first
                for(wpkgar_list_of_strings_t::const_iterator f(f_field_names.begin());
                                                             f != f_field_names.end();
                                                             ++f)
                {
                    if(f_packages[idx].field_is_defined(*f))
                    {
                        wpkg_dependencies::dependencies depends(f_packages[idx].get_field(*f));
                        for(int i(0); i < depends.size(); ++i)
                        {
                            const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(i));
                            sort_package_dependencies(d.f_name, listed);
                        }
                    }
                }
                // done with dependencies, we can add this package to the list
                // if it wasn't added already
                listed[name] = true;
                f_sorted_packages.push_back(idx);
                break;

            default:
                // at this point all the other packages can be ignored
                // although we keep them in the list in case someone
                // wanted to list them (specifically in a GUI app.)
                // however we do not have to sort them in any way
                break;

            }
        }
    }
}


/** \brief Sort the packages.
 *
 * This function sorts the packages with the package that does not
 * depend on any others first. Then packages that depend on that
 * package, and so on until all the packages are added to the list.
 *
 * Packages without dependencies are added as is since the order
 * is not important for them.
 */
void wpkgar_install::sort_packages()
{
    wpkgar_package_listed_t listed;

    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < f_packages.size();
                                         ++idx)
    {
        sort_package_dependencies(f_packages[idx].get_name(), listed);
    }
}


/** \brief Run user defined validation scripts.
 *
 * At times it may be useful to run scripts before the system is ready
 * to run the unpack command. The scripts defined here are called
 * validation scripts for that reason. These scripts are expected to
 * test things and modify nothing.
 *
 * One \em problem with validation scripts at unpack/install time is that
 * none of the dependencies are installed when running these scripts.
 * This may cause problems where the user ends up having to install a
 * dependency before it is possible for him/her to install the main
 * package he's interested in installing (see the Pre-Depends field).
 *
 * \note
 * This function loops through the list of explicit and implicit packages
 * to run their validate scripts explicitly. This is done that way because
 * these scripts are not yet considered installed. However, packages being
 * upgraded may get their scripts ran twice (the new version first and then
 * their old version.)
 */
void wpkgar_install::validate_scripts()
{
    // run the package validation script of the packages being installed
    // or upgraded and as we're at it generate the list of package names
    int errcnt(0);
    std::string package_names;
    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < f_packages.size();
                                         ++idx)
    {
        switch(f_packages[idx].get_type())
        {
        case package_item_t::package_type_explicit:
        case package_item_t::package_type_implicit:
            {
                package_names += f_packages[idx].get_filename().full_path() + " ";

                // new-validate install <new-version> [<old-version>]
                wpkgar_manager::script_parameters_t params;
                params.push_back("install");
                params.push_back(f_packages[idx].get_version());
                int32_t upgrade_idx(f_packages[idx].get_upgrade());
                if(upgrade_idx != -1)
                {
                    params.push_back(f_packages[upgrade_idx].get_version());
                }
                if(!f_manager->run_script(f_packages[idx].get_filename(), wpkgar_manager::wpkgar_script_validate, params))
                {
                    wpkg_output::log("the validate script of package %1 returned with an error, installation aborted.")
                            .quoted_arg(f_packages[idx].get_name())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(f_packages[idx].get_name())
                        .action("install-validation");
                    ++errcnt;
                }
            }
            break;

        default:
            // other packages are not going to be installed
            break;

        }
    }

    // if no errors occured, validate with the already installed
    // installation scripts
    if(errcnt == 0)
    {
        // old-validate install <package-names>
        wpkgar_manager::script_parameters_t params;
        params.push_back("install");
        params.push_back(package_names);
        if(!f_manager->run_script("core", wpkgar_manager::wpkgar_script_validate, params))
        {
            wpkg_output::log("a global validation hook failed, the installation is canceled.")
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .action("install-validation");
            //++errcnt; -- not necessary here
        }
    }
}


/** \brief Validate one or more packages for installation.
 *
 * The --install, --unpack, --configure, --check-install commands means the
 * user expects packages to be installed or updated, unpacked, configured, or
 * validated for installation.
 *
 * This function runs all the validations to ensure that the resulting
 * installation remains consistent.
 *
 * As much as possible, validation failures are recorded but do not stop
 * the process until the actual extraction of the packages happens. This
 * allows us to give all the information available for the user to correct
 * his command line at once.
 *
 *   - Check the type of all the package names; if --configure and the
 *     name is not of an installed package, it fails; vice versa, if
 *     not --configure and the name is an installed package, it fails;
 *     these failures are pretty immediate since we cannot really validate
 *     incorrect files any further
 *   - Check whether we're installing source or binary packages;
 *     if source, then we fail at this point as we don't support installing
 *     those yet
 *   - Verify that the database is ready for an installation, if not
 *     we fail without further validation
 *   - Check all the installed packages to make sure that they are in a
 *     valid state (installed, uninstalled, or unpacked); if not
 *     the process fails since the existing installation is not stable
 *     the user may have to use the --repair command instead
 *   - Check the database current status, if not "ready", then the
 *     process fails because a previous command failed
 *   - Read all the packages specified on the command line; if a folder
 *     was specified, read all the packages present in that folder.
 *   - If any package fails to load, the process fail
 *   - If any one of those packages is for a different architecture, fail
 *     unless --force-architecture is set
 *   - Build the tree of packages based on their dependencies
 *   - If installing a source package, check the Build-Depends too
 *   - If wpkg detects a circular dependency, the process fails
 *   - If dependencies are missing, try to load the corresponding installed
 *     package
 *   - If the command is --unpack and some pre-dependencies are not yet
 *     installed, the process fails
 *   - If dependencies are still not fully satisfied, the process fails
 *     unless the --force-depends or --force-depends-version are used.
 *   - If any package breaks another, the process fails unless
 *     the --force-breaks is set
 *   - If the packages create a conflict, the process fails unless
 *     the --force-conflicts is set
 *   - Determine whether fields are valid (i.e. apply
 *     the  --validate-fields expression to all the packages being
 *     installed), if it does not validate the process fails
 *   - Check whether any file is going to get overwritten on the target
 *     - If not, we're good and proceed
 *     - If so, check whether the file is part of an already installed
 *       package
 *       - If so and the name of both packages match then we are upgrading
 *         so check the following:
 *         - If the version of the new package is the same as
 *           the new package; then it's already installed and we
 *           simply drop that package from the list if --skip-same-version
 *           was specified (This especially happens when installing from
 *           a folder).
 *         - If the version of the new package is larger (newer) then
 *           we're doing an update; we keep the package for installation
 *         - If the version of the new package is smaller (older) then
 *           we're downgrading which is not permission so the process
 *           fails unless --force-downgrade was used.
 *       - If the overwrite is not of the same package, then the
 *         installation fails unless --force-overwrite was used
 *         (remember that this test does not apply to files getting
 *         updated)
 *       - If the overwrite is not of any package, then the installation
 *         fails; we do not offer a --force-... in that case (not
 *         even --force-all); you have to manually remove the file(s) if
 *         you want to install the package
 *       - Test whether the file can be open for writing but do not truncate
 *         the file; if this fails then the validation fails because the
 *         unpack() process would otherwise fail; note that the configure()
 *         process does not require this step although under MS-Windows it
 *         proves that none of the files are in use and thus that the
 *         configuration process is safe
 *   - If more than one package is being installed, check whether any
 *     of the files to be installed are named exactly the same (using a
 *     full path); if so the process fails; we do not allow
 *     the --force-overwrite or --force-overwrite-dir in this case
 *     because we cannot know which of the two files should be kept
 *   - Compute the total amount of space necessary to install all of
 *     these new packages (i.e. Installed-Size of the new package, minus
 *     Installed-Size of the old package when upgrading, plus database size)
 *     If the total amount of space necessary to install the new packages
 *     is larger than the available disk space, the installation fails
 *
 * If anything fails and no corresponding --force-... flag was used,
 * then the validation process fails.
 *
 * \note
 * If --force-all was used, it will be as if all the --force-... were
 * used. Although if the corresponding --no-force-... or --refuse-...
 * option was used, then the --force-... is ignored, whatever the order
 * in which both options were specified (i.e. the no-force or refuse
 * options have higher priority.)
 */
bool wpkgar_install::validate()
{
    // the caller is responsible for locking the database
    if(!f_manager->was_locked())
    {
        throw std::logic_error("the manager must be locked before calling wpkgar_install::validate()");
    }

    // list of all the dependency fields to test here
    //
    // TODO: select the Build-Depends-Arch or Build-Depends-Indep depending
    //       on the build mode we're in (we do not support such distinction
    //       at the CMakeLists.txt level yet!)
    f_field_names.clear();
    // WARNING: the Depends field MUST be the first entry
    f_field_names.push_back(wpkg_control::control_file::field_depends_factory_t::canonicalized_name());

    // installation architecture
    // (note that dpkg can be setup to support multiple architecture;
    // at this point we support just one.)
    f_manager->load_package("core");
    f_architecture = f_manager->get_field("core", wpkg_control::control_file::field_architecture_factory_t::canonicalized_name());

    // some of the package names may be directory names, make sure we
    // know what's what and actually replace all the directory names
    // with their content so we don't have to know about those later
    // all of those are considered explicitly defined packages
    wpkg_output::log("validate directories")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    if(!validate_directories())
    {
        // the list of packages may end up being empty in which case we
        // just return since there is really nothing more we can do
        return false;
    }
//printf("list packages (nearly) at the start:\n");
//for(wpkgar_package_list_t::size_type idx(0); idx < f_packages.size(); ++idx)
//{
//printf("  %d - [%s]\n", (int)f_packages[idx].get_type(), f_packages[idx].get_filename().c_str());
//}

    // make sure package names correspond to the type of installation
    // (i.e. in --configure all the names must be installed packages, in
    // all other cases, it must not be.)
    wpkg_output::log("validate package name")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_package_names();

    // check whether some packages are source packages;
    wpkg_output::log("validate installation type (source/binary)")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    installing_source();
    if(f_install_source)
    {
        // IMPORTANT NOTE:
        // I have a validation that checks whether binary fields include one
        // of those Build dependency fields; that validation is void when
        // none of the packages are source packages. So if that package is
        // never necessary to build any package source, it will never be
        // checked for such (but it is made valid by the --build command!)
        f_field_names.push_back(wpkg_control::control_file::field_builddepends_factory_t::canonicalized_name());
        f_field_names.push_back(wpkg_control::control_file::field_builddependsarch_factory_t::canonicalized_name());
        f_field_names.push_back(wpkg_control::control_file::field_builddependsindep_factory_t::canonicalized_name());
        f_field_names.push_back(wpkg_control::control_file::field_builtusing_factory_t::canonicalized_name());
    }

    // make sure that the currently installed packages are in the
    // right state for a new installation to occur
    wpkg_output::log("validate installed packages")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_installed_packages();

    // make sure that all the packages to be installed have the same
    // architecture as defined in the core package
    // (note: this is done before checking dependencies because it is
    // assumed that implicit packages are added only if their architecture
    // matches the core architecture, and of course already installed
    // packages have the right architecture.)
    wpkg_output::log("validate architecture")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_architecture();

    // if any Pre-Depends is not satisfied in the explicit packages then
    // the installation will fail (although we can go on with validations)
    wpkg_output::log("validate pre-dependencies")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_predependencies();

    // before we can check a complete list of what is going to be installed
    // we first need to make sure that this list is complete; this means we
    // need to determine whether all the dependencies are satisfied this
    // adds the dependencies to the list and at the end we have a long list
    // that includes all the packages we need to check further
    wpkg_output::log("validate dependencies")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_dependencies();

    // when marking a target with a specific distribution then only
    // packages with the same distribution informations should be
    // installed on that target; otherwise packages may not be 100%
    // compatible (i.e. incompatible compiler used to compile two
    // libraries running together...)
    wpkg_output::log("validate distribution name")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_distribution();

    // check that the packager used to create the explicit and implicit
    // packages was the same or an older version; if newer, we print out
    // a message in verbose mode; at this point this does not generate
    // error (it may later if incompatibilities are found)
    wpkg_output::log("validate packager version")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_packager_version();

    // check user defined C-like expressions against the control file
    // fields of all the packages being installed (implicitly or
    // explicitly)
    wpkg_output::log("validate fields")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_fields();

    // TODO:
    // avoid the overwrite test for now because it loads packages and if
    // we already had errors, it becomes more of a waste right now; remove
    // this test once we have the Files field available in control files
    // of indexes
    if(wpkg_output::get_output_error_count() == 0)
    {
        // check that the new installation size is going to fit the hard drive
        // (this needs a lot of work to properly take the database in account!)
        // and since we read all the data files, check whether any file gets
        // overwritten as we're at it
        wpkg_output::log("validate size and overwrites")
            .debug(wpkg_output::debug_flags::debug_progress)
            .module(wpkg_output::module_validate_installation);
        validate_installed_size_and_overwrite();
    }

    if(wpkg_output::get_output_error_count() == 0)
    {
        // run user defined validation scripts found in the implicit and
        // explicit packages
        wpkg_output::log("validate hooks")
            .debug(wpkg_output::debug_flags::debug_progress)
            .module(wpkg_output::module_validate_installation);
        validate_scripts();
    }

    if(wpkg_output::get_output_error_count() == 0)
    {
        // at this point the order in which we have the packages in our array
        // is the command line order for explicit packages and alphabetical
        // order for implicit packages; the order must be dependencies first
        // (if a depends on b, then b must be installed first even if a
        // appears first in the current list of packages); the following
        // function ensures the order so we can unpack and configure in the
        // correct order
        sort_packages();
    }

//printf("list packages after deps:\n");
//for(wpkgar_package_list_t::size_type idx(0); idx < f_packages.size(); ++idx)
//{
//printf("  %d - [%s]\n", (int)f_packages[idx].get_type(), f_packages[idx].get_filename().c_str());
//}

    return wpkg_output::get_output_error_count() == 0;
}





wpkgar_install::install_info_t::install_info_t()
    //: f_name("") -- auto-init
    //, f_version("") -- auto-init
    : f_install_type(install_type_undefined)
    //, f_is_upgrade(false) -- auto-init
{
}


std::string   wpkgar_install::install_info_t::get_name() const
{
    return f_name;
}


std::string   wpkgar_install::install_info_t::get_version() const
{
    return f_version;
}


wpkgar_install::install_info_t::install_type_t wpkgar_install::install_info_t::get_install_type() const
{
    return f_install_type;
}


bool wpkgar_install::install_info_t::is_upgrade() const
{
    return f_is_upgrade;
}


/** \brief Get list of packages to be installed.
 *
 * First, call the validate() method, but before the unpack() method. Then this
 * method gives you a list of the files to be installed. This includes those
 * files that were requested (explicit), and any required dependencies
 * (implicit), or all those files needing upgrading.
 *
 * Each package is wrapped in an install_info_list_t object value, which allows you to
 * differentiate the type of install it is going to be. Useful for informing
 * the user of the pending database changes and any new packages to be installed.
 */
wpkgar_install::install_info_list_t wpkgar_install::get_install_list()
{
    install_info_list_t list;

    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < f_packages.size();
                                         ++idx)
    {
        switch(f_packages[idx].get_type())
        {
        case package_item_t::package_type_explicit:
        case package_item_t::package_type_implicit:
            {
                install_info_t info;
                info.f_name    = f_packages[idx].get_name();
                info.f_version = f_packages[idx].get_version();

                switch( f_packages[idx].get_type() )
                {
                case package_item_t::package_type_explicit:
                    info.f_install_type = install_info_t::install_type_explicit;
                    break;

                case package_item_t::package_type_implicit:
                    info.f_install_type = install_info_t::install_type_implicit;
                    break;

                default:
                    // Should never happen!
                    //
                    throw std::logic_error("package_type is unknown!");

                }

                const int32_t upgrade_idx(f_packages[idx].get_upgrade());
                info.f_is_upgrade = (upgrade_idx != -1);
                list.push_back( info );
            }
            break;

        default:
            // anything else is already unpacked or ignored
            break;

        }
    }

    return list;
}






bool wpkgar_install::preupgrade_scripts(package_item_t *item, package_item_t *upgrade)
{
    f_manager->set_field(upgrade->get_filename(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Half-Installed", true);

    // run the prerm only if the old version is currently installed
    // (opposed to just unpacked, half-installed, etc.)
    if(f_original_status != wpkgar_manager::installed)
    {
        return true;
    }

    // old-hooks-prerm upgrade <package-name> <new-version>
    wpkgar_manager::script_parameters_t hook_params;
    hook_params.push_back("upgrade");
    hook_params.push_back(item->get_name());
    hook_params.push_back(item->get_version());
    if(!f_manager->run_script("core", wpkgar_manager::wpkgar_script_prerm, hook_params))
    {
        wpkg_output::log("a prerm global validation hook failed for package %1, the installation is canceled.")
                .quoted_arg(item->get_name())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_unpack_package)
            .action("install-unpack");
        return false;
    }

    // old-prerm upgrade <new-version>
    wpkgar_manager::script_parameters_t params;
    params.push_back("upgrade");
    params.push_back(item->get_version());
    if(!f_manager->run_script(upgrade->get_filename(), wpkgar_manager::wpkgar_script_prerm, params))
    {
        // new-prerm failed-upgrade <old-version>
        params.clear();
        params.push_back("failed-upgrade");
        params.push_back(upgrade->get_version());
        if(!f_manager->run_script(item->get_filename(), wpkgar_manager::wpkgar_script_prerm, params))
        {
            f_manager->set_field(upgrade->get_filename(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Half-Configured", true);

            // old-postinst abort-upgrade <new-version>
            params.clear();
            params.push_back("abort-upgrade");
            params.push_back(item->get_version());
            if(!f_manager->run_script(upgrade->get_filename(), wpkgar_manager::wpkgar_script_postinst, params))
            {
                wpkg_output::log("the upgrade scripts failed to prepare the upgrade, package %1 is now Half-Configured.")
                        .quoted_arg(upgrade->get_name())
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_unpack_package)
                    .package(item->get_name())
                    .action("install-unpack");
            }
            else
            {
                // restore the status, but stop the upgrade
                // note: we can hard code "Installed" because we run this code only if
                //       the package was installed
                f_manager->set_field(upgrade->get_filename(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Installed", true);
                wpkg_output::log("the upgrade scripts failed to prepare the upgrade, however it could restore the package state so %1 is marked as Installed.")
                        .quoted_arg(upgrade->get_name())
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_unpack_package)
                    .package(item->get_name())
                    .action("install-unpack");
            }
            return false;
        }
        // the old package script failed, but the new package script
        // succeeded so we proceed anyway
    }

    return true;
}

bool wpkgar_install::postupgrade_scripts(package_item_t *item, package_item_t *upgrade, wpkg_backup::wpkgar_backup& backup)
{
    // old-postrm upgrade <new-version>
    wpkgar_manager::script_parameters_t params;
    params.push_back("upgrade");
    params.push_back(item->get_version());
    if(!f_manager->run_script(upgrade->get_filename(), wpkgar_manager::wpkgar_script_postrm, params))
    {
        // new-postrm failed-upgrade <old-version>
        params.clear();
        params.push_back("failed-upgrade");
        params.push_back(upgrade->get_version());
        if(!f_manager->run_script(item->get_filename(), wpkgar_manager::wpkgar_script_postrm, params))
        {
            cancel_upgrade_scripts(item, upgrade, backup);
            return false;
        }
        // the old package script failed, but the new package script
        // succeeded so we proceed anyway
    }

    // hooks-postrm upgrade <package-name> <new-version> <old-version>
    wpkgar_manager::script_parameters_t hooks_params;
    hooks_params.push_back("upgrade");
    hooks_params.push_back(item->get_name());
    hooks_params.push_back(item->get_version());
    hooks_params.push_back(upgrade->get_version());
    if(!f_manager->run_script("core", wpkgar_manager::wpkgar_script_postrm, hooks_params))
    {
        wpkg_output::log("a postrm global validation hook failed for package %1, the installation is canceled.")
                .quoted_arg(item->get_name())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_unpack_package)
            .action("install-unpack");
        return false;
    }

    return true;
}

void wpkgar_install::cancel_upgrade_scripts(package_item_t *item, package_item_t *upgrade, wpkg_backup::wpkgar_backup& backup)
{
    f_manager->set_field(upgrade->get_filename(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Half-Installed", true);
    backup.restore(); // restore as many files as possible

    // old-preinst abort-upgrade <new-version>
    wpkgar_manager::script_parameters_t params;
    params.push_back("abort-upgrade");
    params.push_back(item->get_version());
    if(!f_manager->run_script(upgrade->get_filename(), wpkgar_manager::wpkgar_script_preinst, params))
    {
        wpkg_output::log("the upgrade scripts failed to cancel the upgrade, package %1 is now half-installed.")
                .quoted_arg(upgrade->get_name())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_unpack_package)
            .package(item->get_name())
            .action("install-unpack");
        return;
    }

    // new-postrm abort-upgrade <old-version>
    params.clear();
    params.push_back("abort-upgrade");
    params.push_back(upgrade->get_version());
    if(!f_manager->run_script(item->get_filename(), wpkgar_manager::wpkgar_script_postrm, params))
    {
        wpkg_output::log("the upgrade scripts failed to cancel the upgrade, and it could not properly restore the state of %1 (half-installed).")
                .quoted_arg(upgrade->get_name())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_unpack_package)
            .package(item->get_name())
            .action("install-unpack");
        return;
    }

    // old-postinst abort-upgrade <new-version>
    params.clear();
    params.push_back("abort-upgrade");
    params.push_back(item->get_version());
    if(!f_manager->run_script(upgrade->get_filename(), wpkgar_manager::wpkgar_script_postinst, params))
    {
        // could not reconfigure...
        f_manager->set_field(upgrade->get_filename(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Unpacked", true);
        wpkg_output::log("the upgrade scripts failed to cancel the upgrade, and it could not properly restore the state of %1 (unpacked).")
                .quoted_arg(upgrade->get_name())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_unpack_package)
            .package(item->get_name())
            .action("install-unpack");
        return;
    }

    // cancel successful!
    // use the original status: Installed or Unpacked
    f_manager->set_field(upgrade->get_filename(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), f_original_status == wpkgar_manager::installed ? "Installed" : "Unpacked", true);
    wpkg_output::log("the upgrade was canceled, yet it could properly restore the package state so %1 is marked as installed.")
            .quoted_arg(upgrade->get_name())
        .level(wpkg_output::level_error)
        .module(wpkg_output::module_unpack_package)
        .package(item->get_name())
        .action("install-unpack");
}

bool wpkgar_install::preinst_scripts(package_item_t *item, package_item_t *upgrade, package_item_t *& conf_install)
{
    wpkgar_manager::script_parameters_t params;
    if(upgrade == NULL)
    {
        // new-preinst install [<old-version>]
        std::string old_version;
        for(wpkgar_package_list_t::size_type idx(0);
                                             idx < f_packages.size();
                                             ++idx)
        {
            if(f_packages[idx].get_type() == package_item_t::package_type_not_installed
            && f_packages[idx].get_name() == item->get_name())
            {
                if(f_manager->package_status(f_packages[idx].get_filename()) == wpkgar_manager::config_files)
                {
                    // we found a package that is being re-installed
                    // (i.e. package configuration files exist,
                    // and it's not being upgraded)
                    old_version = f_packages[idx].get_version();
                    conf_install = &f_packages[idx];
                }
                break;
            }
        }
        params.push_back("install");
        if(conf_install != NULL)
        {
            params.push_back(old_version);
        }
        else
        {
            // create new package entry so we can have a current status
            item->copy_package_in_database();
        }
        set_status(item, upgrade, conf_install, "Half-Installed");

        // hooks-preinst install <package-name> <new-version> [<old-version>]
        wpkgar_manager::script_parameters_t hooks_params;
        hooks_params.push_back("install");
        hooks_params.push_back(item->get_name());
        hooks_params.push_back(item->get_version());
        if(conf_install != NULL)
        {
            hooks_params.push_back(old_version);
        }
        if(!f_manager->run_script("core", wpkgar_manager::wpkgar_script_preinst, hooks_params))
        {
            wpkg_output::log("a preinst global validation hook failed for package %1, the installation is canceled.")
                    .quoted_arg(item->get_name())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_unpack_package)
                .action("install-unpack");
            return false;
        }

        if(!f_manager->run_script(item->get_filename(), wpkgar_manager::wpkgar_script_preinst, params))
        {
            wpkg_output::log("the preinst install script failed to initialize package %1.")
                    .quoted_arg(item->get_name())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_unpack_package)
                .package(item->get_name())
                .action("install-unpack");

            // new-postrm abort-install [<old-version>]
            params.clear();
            params.push_back("abort-install");
            if(!old_version.empty())
            {
                params.push_back(old_version);
            }
            if(f_manager->run_script(item->get_filename(), wpkgar_manager::wpkgar_script_postrm, params))
            {
                // the error unwind worked so we switch the state back
                // to normal
                if(conf_install)
                {
                    f_manager->set_field(conf_install->get_filename(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Config-Files", true);
                }
                else
                {
                    // note: we could also remove the whole thing since
                    // it's not installed although this is a signal that
                    // an attempt was made and failed
                    f_manager->set_field(item->get_name(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Not-Installed", true);
                }

                wpkg_output::log("the postrm abort-install script succeeded for package %1;; its previous status was restored.")
                        .quoted_arg(item->get_name())
                    .module(wpkg_output::module_unpack_package)
                    .package(item->get_name())
                    .action("install-unpack");
            }
            else
            {
                wpkg_output::log("the postrm abort-install script failed for package %1;; package is marked as Half-Installed, although it was not yet unpacked.")
                        .quoted_arg(item->get_name())
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_unpack_package)
                    .package(item->get_name())
                    .action("install-unpack");
            }
            // else the package stays in an half-installed state
            return false;
        }
    }
    else
    {
        // if the package was marked for upgrade then its status is
        // "installed" so we do not have to check that here

        // hooks-preinst upgrade <package-name> <new-version> <old-version>
        wpkgar_manager::script_parameters_t hooks_params;
        hooks_params.push_back("upgrade");
        hooks_params.push_back(item->get_name());
        hooks_params.push_back(item->get_version());
        hooks_params.push_back(upgrade->get_version());
        if(!f_manager->run_script("core", wpkgar_manager::wpkgar_script_preinst, hooks_params))
        {
            wpkg_output::log("a preinst global validation hook failed for package %1, the installation is canceled.")
                    .quoted_arg(item->get_name())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_unpack_package)
                .action("install-unpack");
            return false;
        }

        // new-preinst upgrade <old-version>
        params.push_back("upgrade");
        params.push_back(upgrade->get_version());
        if(!f_manager->run_script(item->get_filename(), wpkgar_manager::wpkgar_script_preinst, params))
        {
            // new-postrm abort-upgrade <old-version>
            params.clear();
            params.push_back("abort-upgrade");
            params.push_back(upgrade->get_version());
            if(f_manager->run_script(item->get_filename(), wpkgar_manager::wpkgar_script_postrm, params))
            {
                f_manager->set_field(upgrade->get_filename(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Unpacked", true);

                // old-postinst abort-upgrade <new-version>
                params.clear();
                params.push_back("abort-upgrade");
                params.push_back(item->get_version());
                if(!f_manager->run_script(upgrade->get_filename(), wpkgar_manager::wpkgar_script_postinst, params))
                {
                    wpkg_output::log("the upgrade scripts failed to initialize the upgrade, package %1 is now unpacked.")
                            .quoted_arg(upgrade->get_name())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_unpack_package)
                        .package(item->get_name())
                        .action("install-unpack");
                }
                else
                {
                    // restore the status, but stop the upgrade
                    // note: the original status may be Installed or Unpacked
                    f_manager->set_field(upgrade->get_filename(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), f_original_status == wpkgar_manager::installed ? "Installed" : "Unpacked", true);
                    wpkg_output::log("the upgrade scripts failed to initialize the upgrade, however it could restore the package state so %1 is marked as installed.")
                            .quoted_arg(upgrade->get_name())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_unpack_package)
                        .package(item->get_name())
                        .action("install-unpack");
                }
                return false;
            }
            // restoring the old package failed, we're Half-Installed
            wpkg_output::log("the \"preinst install/upgrade %1\" script of %2 failed and restoring with \"new-postrm abort-upgrade %1\" did not restore the state properly.")
                    .arg(upgrade->get_version())
                    .quoted_arg(item->get_name())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_unpack_package)
                .package(item->get_name())
                .action("install-unpack");
            return false;
        }
    }

    return true;
}

void wpkgar_install::cancel_install_scripts(package_item_t *item, package_item_t *conf_install, wpkg_backup::wpkgar_backup& backup)
{
    // restore the backed up files (it has to happen before running the scripts)
    backup.restore();

    // new-postrm abort-install [<old-version>]
    wpkgar_manager::script_parameters_t params;
    params.push_back("abort-install");
    if(conf_install != NULL)
    {
        params.push_back(conf_install->get_version());
    }
    if(f_manager->run_script(item->get_filename(), wpkgar_manager::wpkgar_script_postrm, params))
    {
        // the error unwind worked so we switch the state back to normal
        if(conf_install)
        {
            f_manager->set_field(conf_install->get_filename(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Config-Files", true);
        }
        else
        {
            // note: we could also remove the whole thing since
            // it's not installed although this is a signal that
            // an attempt was made and failed
            f_manager->set_field(item->get_name(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Not-Installed", true);
        }
    }
    else
    {
        // else the package stays in an Half-Installed state
        wpkg_output::log("installation cancellation of package %1 failed, it will remain in the Half-Installed state.")
                .quoted_arg(item->get_name())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_unpack_package)
            .package(item->get_name())
            .action("install-unpack");
    }
}

void wpkgar_install::set_status(package_item_t *item, package_item_t *upgrade, package_item_t *conf_install, const std::string& status)
{
    if(conf_install)
    {
        f_manager->set_field(conf_install->get_filename(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), status, true);
    }
    else if(upgrade == NULL)
    {
        // IMPORTANT: Note that we're using get_name() here because we
        //            want to change the status in the database and not
        //            the temporary version of this package
        f_manager->set_field(item->get_name(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), status, true);
    }
    else
    {
        f_manager->set_field(upgrade->get_filename(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), status, true);
    }
}

void wpkgar_install::unpack_file(package_item_t *item, const wpkg_filename::uri_filename& destination, const memfile::memory_file::file_info& info)
{
    int file_info_err(get_parameter(wpkgar_install_force_file_info, false) ? memfile::memory_file::file_info_return_errors : memfile::memory_file::file_info_throw);

    // apply the file info
    memfile::memory_file::info_to_disk_file(destination, info, file_info_err);

    if(file_info_err & memfile::memory_file::file_info_permissions_error)
    {
        if(get_parameter(wpkgar_install_quiet_file_info, false) == 0)
        {
            wpkg_output::log("file %1 permissions could not be setup up, chmod() failed.")
                    .quoted_arg(info.get_filename())
                .level(wpkg_output::level_warning)
                .module(wpkg_output::module_unpack_package)
                .package(item->get_name())
                .action("install-unpack");
        }
    }

    if(file_info_err & memfile::memory_file::file_info_owner_error)
    {
        if(get_parameter(wpkgar_install_quiet_file_info, false) == 0)
        {
            wpkg_output::log("file %1 ownership could not be setup up, chown() failed.")
                    .quoted_arg(info.get_filename())
                .level(wpkg_output::level_warning)
                .module(wpkg_output::module_unpack_package)
                .package(item->get_name())
                .action("install-unpack");
        }
    }
}


/** \brief Unpack the files of a package.
 *
 * This function actually extracts the files from the data.tar.gz tarball.
 * If the package has configuration files, those are extracted with the
 * special .wpkg-new extension, meaning that the package is not yet installed
 * (it is considered unpacked, but not configured.)
 *
 * When upgrading, the system runs upgrade specific scripts and allows for
 * overwriting files that existed in the previous version. Different fields
 * are also setup in the status file. Finally, files that existed in the old
 * package but are not present in the new package get removed.
 *
 * If the process fails, then the package stays in an Half-Installed status.
 *
 * \param[in] item  The package to unpack.
 * \param[in] upgrade  The package to upgrade or NULL if we are not upgrading.
 */
bool wpkgar_install::do_unpack(package_item_t *item, package_item_t *upgrade)
{
    f_original_status = wpkgar_manager::not_installed;

    if(upgrade != NULL)
    {
        f_original_status = upgrade->get_original_status();

        if(!preupgrade_scripts(item, upgrade))
        {
            return false;
        }
    }

    // IMPORTANT: the preinst_script() function creates the database
    //            for this package if it was not installed yet
    package_item_t *conf_install(NULL);
    if(!f_reconfiguring_packages)
    {
        // the reconfigure does not re-run the preinst script
        // (it could because of the expected idempotency of scripts)
        if(!preinst_scripts(item, upgrade, conf_install))
        {
            return false;
        }
    }

    // RAII backup, by default we restore the backup files;
    // if everything works as expected we call success() which
    // prevents the restore; either way the object deletes the
    // backup files it creates (see the wpkgar_backup::backup()
    // function for details)
    wpkg_backup::wpkgar_backup backup(f_manager, item->get_name(), "install-unpack");

    long count_files(0);
    long count_directories(0);

    // get the data archive of item (new package) and unpack it
    try
    {
        if(upgrade != NULL)
        {
            set_status(item, upgrade, conf_install, "Upgrading");
            if(f_reconfiguring_packages)
            {
                f_manager->set_field(item->get_name(), "X-Last-Reconfigure-Date", wpkg_util::rfc2822_date(), true);
                f_manager->set_field(item->get_name(), "X-Last-Reconfigure-Packager-Version", debian_packages_version_string(), true);
            }
            else
            {
                f_manager->set_field(item->get_name(), "X-Last-Upgrade-Date", wpkg_util::rfc2822_date(), true);
                f_manager->set_field(item->get_name(), "X-Last-Upgrade-Packager-Version", debian_packages_version_string(), true);
                if(item->get_type() != package_item_t::package_type_implicit)
                {
                    f_manager->set_field(item->get_name(), "X-Explicit", "Yes", true);
                }
            }
        }
        else
        {
            set_status(item, upgrade, conf_install, "Installing");
            f_manager->set_field(item->get_name(), "X-Install-Date", wpkg_util::rfc2822_date(), true);
            f_manager->set_field(item->get_name(), "X-Install-Packager-Version", debian_packages_version_string(), true);
            if(item->get_type() != package_item_t::package_type_implicit)
            {
                f_manager->set_field(item->get_name(), "X-Explicit", "Yes", true);
            }
            else
            {
                // Implicit
                f_manager->set_field(item->get_name(), "X-Explicit", "No", true);
            }
        }
        {
            const wpkg_filename::uri_filename package_name(item->get_filename());
            memfile::memory_file data;
            std::string data_filename("data.tar");
            wpkg_filename::uri_filename database(f_manager->get_database_path());
            const int segment_max(database.segment_size());
            f_manager->get_control_file(data, item->get_filename(), data_filename, false);
            for(;;)
            {
                memfile::memory_file::file_info info;
                memfile::memory_file file;
                if(!data.dir_next(info, &file))
                {
                    break;
                }
                const std::string filename(info.get_filename());
                if(filename.empty())
                {
                    throw std::runtime_error("a filename in the data.tar archive file cannot be empty");
                }
                if(filename[0] == '/' || filename[0] == '\\')
                {
                    throw std::runtime_error("a filename in the data.tar archive file cannot start with \"/\"");
                }
                // get the destination filename and make sure it doesn't
                // match the database path
                wpkg_filename::uri_filename destination(f_manager->get_inst_path().append_child(filename));
                if(destination.segment_size() >= segment_max)
                {
                    int i(0);
                    for(; i < segment_max; ++i)
                    {
                        if(destination.segment(i) != database.segment(i))
                        {
                            break;
                        }
                    }
                    if(i == segment_max)
                    {
                        std::string msg;
                        wpkg_output::log(msg, "file %1 has a path that would place it in your administration directory; this is not allowed and the unpack process must be canceled.")
                                .quoted_arg(destination);
                        throw std::runtime_error(msg);
                    }
                }
                switch(info.get_file_type())
                {
                case memfile::memory_file::file_info::regular_file:
                case memfile::memory_file::file_info::continuous:
                    {
                        const bool is_config(f_manager->is_conffile(package_name, filename));
                        if(is_config)
                        {
                            // configuration files are renamed at this point
                            destination = destination.append_path(".wpkg-new");
                        }
                        if(is_config || !f_reconfiguring_packages)
                        {
                            // do a backup no matter what
                            backup.backup(destination);
                            // write that file on disk
                            file.write_file(destination, true, true);
                            unpack_file(item, destination, info);
                            ++count_files;

                            wpkg_output::log("%1 unpacked...")
                                    .quoted_arg(destination)
                                .debug(wpkg_output::debug_flags::debug_files)
                                .module(wpkg_output::module_unpack_package)
                                .package(package_name);
                        }
                    }
                    break;

                case memfile::memory_file::file_info::directory:
                    if(!f_reconfiguring_packages)
                    {
                        // TODO: we need to support copying directories recursively
                        //       (and of course restore them too!)
                        // do a backup no matter what
                        //backup.backup(destination); -- not implemented yet!
                        // create directory if it doesn't exist yet
                        destination.os_mkdir_p();
                        unpack_file(item, destination, info);
                        ++count_directories;
                    }
                    break;

                case memfile::memory_file::file_info::symbolic_link:
                    if(!f_reconfiguring_packages)
                    {
                        const wpkg_filename::uri_filename dest(f_manager->get_inst_path().append_child(info.get_filename()));
                        wpkg_filename::uri_filename path(dest.dirname());

                        const wpkg_filename::uri_filename source( path.append_child( info.get_link() ) );
                        backup.backup(dest);

                        source.os_symlink(dest);

                        //std::cout << "symlinked " << source << " to " << dest << std::endl;
                        //
                        // TODO: this is not done because the symlink is mistaken for a file...
                        // this fails because the symlink might precede the real file in the archive.
                        // So what we need is a method that can detect the symlink and alter the permissions
                        // for the symlink only, not the file it points to (which might not exist yet and
                        // which should have its own permissions/owner/group information anyway!).
                        //
                        // unpack_file(item, destination, info);
                        //
                        ++count_files;
                        wpkg_output::log("%1 --> %2 symlinked...")
                                .quoted_arg(source)
                                .quoted_arg(dest)
                            .debug(wpkg_output::debug_flags::debug_files)
                            .module(wpkg_output::module_unpack_package)
                            .package(package_name);
                    }
                    break;

                // TODO: To get hard links to work we need to memorize what
                //       the file before this entry was (i.e. tarball have
                //       hard links right after the file it is linking to.)
                //       Also we must prevent hard links to configuration
                //       files if we couldn't catch that problem with the
                //       --build command.
                //case memfile::memory_file::file_info::hard_link:

                default:
                    if(!f_reconfiguring_packages)
                    {
                        // at this point we ignore other file types because they
                        // are not supported under MS-Windows so we don't have
                        // to do anything with them anyway
                        wpkg_output::log( "file %1 is not a regular file or a directory, it will be ignored.")
                                .quoted_arg(destination)
                            .level(wpkg_output::level_warning)
                            .module(wpkg_output::module_unpack_package)
                            .package(item->get_name())
                            .action("install-unpack");
                    }
                    break;

                }
            }
        }

        // the post upgrade script is run before we delete the files that
        // the upgrade may invalidate (because they are not available
        // in the new version of the package)
        set_status(item, upgrade, conf_install, "Half-Installed");
        if(upgrade != NULL)
        {
            if(!postupgrade_scripts(item, upgrade, backup))
            {
                return false;
            }
        }

        // if upgrading, now we want to delete files that "disappeared" from
        // the old package
        if(upgrade != NULL)
        {
            set_status(item, upgrade, conf_install, "Upgrading"); // could it be Removing?
            const wpkg_filename::uri_filename package_name(upgrade->get_filename());
            memfile::memory_file data;
            std::string data_filename("data.tar");
            f_manager->get_control_file(data, item->get_name(), data_filename, false);
            for(;;)
            {
                // in this case we don't need the data
                memfile::memory_file::file_info info;
                if(!data.dir_next(info, NULL))
                {
                    break;
                }
                std::string filename(info.get_filename());
                if(filename.empty())
                {
                    throw std::runtime_error("a filename in the data.tar archive file cannot be empty");
                }
                if(filename[0] == '/' || filename[0] == '\\')
                {
                    throw std::runtime_error("a filename in the data.tar archive file cannot start with \"/\"");
                }
                // backup any regular file (we can restore anything else without the need of a full backup)
                // configuration files are silently skipped in the unpack process
                if((info.get_file_type() == memfile::memory_file::file_info::regular_file
                        || info.get_file_type() == memfile::memory_file::file_info::continuous)
                && !f_manager->is_conffile(package_name, filename))
                {
                    const wpkg_filename::uri_filename destination(f_manager->get_inst_path().append_child(filename));
                    // if saving a backup succeeds then we want to delete
                    // the file on the target (i.e. it's not present in the
                    // new version of the package)
                    if(backup.backup(destination))
                    {
                        // delete that file as we're upgrading
                        try
                        {
                            if(!destination.os_unlink())
                            {
                                // the file did not exist, post a log, but ignore otherwise
                                wpkg_output::log("file %1 could not be removed while upgrading because it did not exist.")
                                        .quoted_arg(destination)
                                    .debug(wpkg_output::debug_flags::debug_detail_files)
                                    .module(wpkg_output::module_unpack_package)
                                    .package(package_name);
                            }
                        }
                        catch(const wpkg_filename::wpkg_filename_exception_io&)
                        {
                            // we capture the exception so we can continue
                            // to process the installation but we generate
                            // an error so in the end it fails
                            wpkg_output::log("file %1 from the previous version of the package could not be deleted.")
                                    .quoted_arg(destination)
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_unpack_package)
                                .package(item->get_name())
                                .action("install-unpack");
                        }
                    }
                }
            }
        }
    }
    catch(const std::runtime_error&)
    {
        // we are not annihilating the catch but we want to run scripts
        // to cancel the process when an error occurs;
        set_status(item, upgrade, conf_install, "Half-Installed");
        if(upgrade != NULL)
        {
            cancel_upgrade_scripts(item, upgrade, backup);
        }
        else
        {
            cancel_install_scripts(item, conf_install, backup);
        }
        throw;
    }

    if(!f_reconfiguring_packages && (upgrade != NULL || conf_install != NULL))
    {
        item->copy_package_in_database();
    }

    set_status(item, upgrade, conf_install, "Unpacked");

    if(f_reconfiguring_packages)
    {
        f_manager->set_field(item->get_name(), "X-Reconfigure-Date", wpkg_util::rfc2822_date(), true);
    }
    else
    {
        f_manager->set_field(item->get_name(), "X-Unpack-Date", wpkg_util::rfc2822_date(), true);
        f_manager->set_field(item->get_name(), "X-Installed-Files", count_files, true);
        f_manager->set_field(item->get_name(), "X-Created-Directories", count_directories, true);
    }

    // just delete all those backups but don't restore!
    backup.success();

    // it worked
    item->mark_unpacked();

    return true;
}


/** \brief Pre-configure packages.
 *
 * This function is run to pre-configure all the packages that were unpacked
 * earlier and not yet configured but that require to be configured before
 * we can proceed with other installation processes.
 *
 * The function can safely be called if no packages need to be pre-configured.
 * In that case nothing happens and the function returns true.
 *
 * \return The function returns false if an error was detected.
 */
bool wpkgar_install::pre_configure()
{
    // the caller is responsible for locking the database
    if(!f_manager->was_locked())
    {
        throw std::logic_error("the manager must be locked before calling wpkgar_install::pre_configure()");
    }

    // TODO: We have to respect the order which at this point we do not
    //       (i.e. if many packages were unpacked and not yet configured
    //       the one that only depends on already installed packages has
    //       to be configured first)
    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < f_packages.size();
                                         ++idx)
    {
        if(f_packages[idx].get_type() == package_item_t::package_type_configure)
        {
            std::string package_name(f_packages[idx].get_name());
            wpkg_output::log("pre-configuring %1")
                        .quoted_arg(package_name)
                .debug(wpkg_output::debug_flags::debug_progress)
                .module(wpkg_output::module_validate_installation);

            f_manager->track("deconfigure " + package_name, package_name);
            if(!configure_package(&f_packages[idx]))
            {
                f_manager->track("failed");
                return false;
            }
        }
    }

    return true;
}



/** \brief Unpack the files from a package.
 *
 * This process is equivalent to:
 *
 * \code
 * tar xzf data.tar.gz
 * \endcode
 *
 * Except that configuration files (those listed in conffiles) are not
 * extracted.
 *
 * The function searches for the first package that has all of its
 * dependencies satisfied and returns its index when successfully
 * unpacked. The index can be used to call the configure() function
 * in order to finish the installation by configuring the package.
 *
 * In case of an update, the function first backs up the existing
 * files. These files are restored if an error occurs before the
 * extraction is complete or if some of the upgrade scripts fail.
 * Note that under MS-Windows a file that is currently opened
 * cannot be overwritten so such errors can occur more frequently
 * under that operating system.
 *
 *   - Starting here, if the process fails we first try to restore
 *     the files saved before the extraction process
 *     - Sort the packages so the packages that do not have dependencies
 *       or dependencies already installed are installed first;
 *       especially, pre-dependencies are fully installed before
 *       anything else
 *     - Extraction process run against one package at a time
 *       - Set the database status to "working"
 *       - In case we're upgrading, backup the existing files using hard
 *         links if possible
 *       - If the package is being installed, move the temporary folder
 *         to the installed folder (one directory up)
 *       - Change the package status, if upgrading, set it to "upgrading",
 *         otherwise set it to "installing"
 *       - If upgrading, run the prerm of the old package
 *       - Run the preinst of the old package
 *       - Extract all the files except the configuration files
 *       - If we're upgrading delete any file that was present in the
 *         old package but is not available in the new package
 *       - If we're upgrading, run the postrm of the old package
 *       - If we're upgrading, copy the new package data to the parent
 *         folder (in the database)
 *       - Set the package status to "half-installed"
 *       - Set the database status to "ready"
 *
 * \note
 * Since MS-Windows does not offer a very safe way to rename files we
 * directly extract all the files in their destination file; this
 * means the target should not be in use when we do an update (anyway
 * you cannot open a file for writing if already in use under MS-Windows
 * so that's hardly a limitation of the packager.)
 *
 * \note
 * When the installation process fails while upgrading a package we can
 * attempt to restore that package. However, if upgrading package A
 * succeeds and then upgrading package B fails, then we may end up in
 * a situation where package A is now installed but is the wrong version
 * for the current installation (i.e. because the old package B cannot
 * work right with the new package A.)
 *
 * \note
 * The detailed Debian policy about the maintainer scripts:
 * http://www.debian.org/doc/debian-policy/ch-maintainerscripts.html
 *
 * \return The index of the item that got unpacked (a positive value,)
 * this index can be used to call the configure() function, or
 * WPKGAR_EOP (end of packages) when all packages were unpacked,
 * or WPKGAR_ERROR when an error occured and the whole process
 * should stop.
 */
int wpkgar_install::unpack()
{
    // the caller is responsible for locking the database
    if(!f_manager->was_locked())
    {
        throw std::logic_error("the manager must be locked before calling wpkgar_install::unpack()");
    }

    for( auto idx : f_sorted_packages )
    {
        auto& package( f_packages[idx] );
        if(!package.is_unpacked())
        {
            switch(package.get_type())
            {
            case package_item_t::package_type_explicit:
            case package_item_t::package_type_implicit:
                {
                    const std::string package_name(package.get_name());
                    wpkg_output::log("unpacking %1")
                                .quoted_arg(package_name)
                        .debug(wpkg_output::debug_flags::debug_progress)
                        .module(wpkg_output::module_validate_installation);

                    package_item_t *upgrade(NULL);
                    const int32_t upgrade_idx(package.get_upgrade());
                    if(upgrade_idx != -1)
                    {
                        upgrade = &f_packages[upgrade_idx];

                        // restore in case of an upgrade requires an
                        // original package from a repository
                        std::string restore_name(package_name + "_" + upgrade->get_version());
                        if(upgrade->get_architecture() != "src"
                        && upgrade->get_architecture() != "source")
                        {
                            restore_name += upgrade->get_architecture();
                        }
                        restore_name += ".deb ";
                        f_manager->track("downgrade " + restore_name, package_name);
                    }
                    else
                    {
                        // it was not installed yet, just purge the whole thing
                        f_manager->track("purge " + package_name, package_name);
                    }
                    if(!do_unpack(&package, upgrade))
                    {
                        // an error occured, we cannot continue
                        // TBD: should we throw?
                        return WPKGAR_ERROR;
                    }
                    return static_cast<int>(idx);
                }
                break;

            default:
                // anything else is already unpacked or ignored
                break;

            }
        }
    }

    // End of Packages
    return WPKGAR_EOP;
}



bool wpkgar_install::configure_package(package_item_t *item)
{
    // count errors that occur here
    int err(0);

    // get the list of configuration files
    wpkgar_manager::conffiles_t files;
    f_manager->conffiles(item->get_name(), files);

    wpkg_util::md5sums_map_t sums;
    std::string data_filename("md5sums.wpkg-old");
    if(f_manager->has_control_file(item->get_name(), data_filename))
    {
        // we use item->get_name() because at this point we want to read from
        // the installed package, not the temporary package
        memfile::memory_file old_md5sums;
        f_manager->get_control_file(old_md5sums, item->get_name(), data_filename, false);
        wpkg_util::parse_md5sums(sums, old_md5sums);
    }

    f_manager->set_field(item->get_name(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Half-Configured", true);

    f_manager->track("deconfigure " + item->get_name(), item->get_name());

    const wpkg_filename::uri_filename root(f_manager->get_inst_path());
    for(std::vector<std::string>::iterator it(files.begin());
                                           it != files.end();
                                           ++it)
    {
        const wpkg_filename::uri_filename confname(root.append_child(*it));
        if(!confname.exists())
        {
            // file doesn't exist, rename the .wpkg-new configuration
            // file as the main file (otherwise simply ignore)
            try
            {
                const wpkg_filename::uri_filename user(confname.append_path(".wpkg-user"));
                if(user.exists())
                {
                    // in case the package was --deconfigured in between
                    user.os_rename(confname);
                }
                else
                {
                    confname.append_path(".wpkg-new").os_rename(confname);
                }
            }
            catch(const wpkg_filename::wpkg_filename_exception_io&)
            {
                ++err;
                // an error occured, we won't mark the package as installed
                int e(errno);
                wpkg_output::log("configuration file %1 could not be renamed %2 (errno: %3).")
                        .quoted_arg(confname.original_filename() + ".wpkg-new")
                        .quoted_arg(confname)
                        .arg(e)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_configure_package)
                    .package(item->get_name())
                    .action("install-configure");
            }
        }
        else
        {
            // we're upgrading it looks like and there are some old
            // md5sums that apply to the old (existing) configuration
            // files so we want to check that against its md5sum
            std::string confbasename(it->c_str());
            if(confbasename[0] == '/')
            {
                confbasename = confbasename.substr(1);
            }
            wpkg_util::md5sums_map_t::const_iterator old_md5sum(sums.find(confbasename));
            if(old_md5sum == sums.end())
            {
                // no old sums, maybe it's a user created file, no upgrade
                // (should this be an error?!)
                wpkg_output::log("configuration file %1 already exists but it does not appear to be part of the old package.")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_warning)
                    .module(wpkg_output::module_configure_package)
                    .package(item->get_name())
                    .action("install-configure");
            }
            else
            {
                memfile::memory_file old_conf;
                old_conf.read_file(confname);
                if(old_md5sum->second == old_conf.md5sum())
                {
                    const wpkg_filename::uri_filename new_confname(confname.append_path(".wpkg-new"));
                    if(new_confname.exists())
                    {
                        // the old configuration file was never modified, replace
                        // it silently
                        wpkg_output::log("replacing configuration file %1 from package %2 because it was never modified (md5sum is still the same).")
                                .quoted_arg(confname)
                                .quoted_arg(item->get_name())
                            .debug(wpkg_output::debug_flags::debug_detail_config)
                            .module(wpkg_output::module_configure_package)
                            .package(item->get_name());
                        const wpkg_filename::uri_filename old_confname(confname.append_path(".wpkg-old"));
                        // the unlink() can fail if no .wpkg-old already exists
                        old_confname.os_unlink();
                        const char *ext_old("");
                        const char *ext_new(".wpkg-old");
                        try
                        {
                            confname.os_rename(old_confname, true);
                            ext_old = ".wpkg-new";
                            ext_new = "";
                            new_confname.os_rename(confname);
                        }
                        catch(const wpkg_filename::wpkg_filename_exception_io&)
                        {
                            ++err;
                            wpkg_output::log("configuration file %1 could not be renamed %2, package %3 not marked installed.")
                                    .quoted_arg(confname.original_filename() + ext_old)
                                    .quoted_arg(confname.original_filename() + ext_new)
                                    .quoted_arg(item->get_name())
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_configure_package)
                                .package(item->get_name())
                                .action("install-configure");
                        }
                    }
                    else
                    {
                        // new configuration is missing! we do not want to
                        // copy it because we may smash a user file! (even
                        // if the md5sum is equal...) this happens when
                        // the user runs --configure twice; although we
                        // should not get here because the 2nd time it
                        // should be ignored...
                        wpkg_output::log("configuration file %1 is missing, package %2 still marked as installed.")
                                .quoted_arg(new_confname)
                                .quoted_arg(item->get_name())
                            .level(wpkg_output::level_warning)
                            .module(wpkg_output::module_configure_package)
                            .package(item->get_name())
                            .action("install-configure");
                    }
                }
                else
                {
                    // the old configuration file was modified, we do not
                    // replace, but give the user a message if in verbose
                    // mode (should this be a warning? it's a rather standard
                    // thing so not really an error or even a warning...)
                    wpkg_output::log("configuration file %1 from package %2 was modified so the configuration process did not touch it. The new configuration is available in %3.")
                            .quoted_arg(*it)
                            .quoted_arg(item->get_name())
                            .quoted_arg(confname.original_filename() + ".wpkg-new")
                        .module(wpkg_output::module_configure_package)
                        .package(item->get_name())
                        .action("install-configure");
                }
            }
        }
    }

    // new-postinst configure <new-version>
    wpkgar_manager::script_parameters_t params;
    params.push_back("configure");
    params.push_back(item->get_version());
    if(!f_manager->run_script(item->get_name(), wpkgar_manager::wpkgar_script_postinst, params))
    {
        // errors are reported but there is no unwind for configuration failures
        ++err;
        wpkg_output::log("postinst script failed configuring the package.")
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_configure_package)
            .package(item->get_name())
            .action("install-configure");
    }
    else
    {
        // hooks-postinst configure <package-name> <new-version>
        wpkgar_manager::script_parameters_t hooks_params;
        hooks_params.push_back("configure");
        hooks_params.push_back(item->get_name());
        hooks_params.push_back(item->get_version());
        if(!f_manager->run_script("core", wpkgar_manager::wpkgar_script_postinst, hooks_params))
        {
            ++err;
            wpkg_output::log("a postinst global hook failed for package %1, the installation is canceled.")
                    .quoted_arg(item->get_name())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_unpack_package)
                .action("install-configure");
        }
    }

    if(err == 0)
    {
        // mark the package as installed!
        f_manager->set_field(item->get_name(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Installed", true);
        f_manager->set_field(item->get_name(), "X-Configure-Date", wpkg_util::rfc2822_date(), true);
    }

    return err == 0;
}


/** \brief Configure the specified packages.
 *
 * This function configures the specified packages which means extracting
 * the configuration files from the data.tar.gz archive.
 *
 *         - Extract the configuration files; if we are upgrading and the
 *           destination already exists, extract the file with the .wpkg
 *           extension added so we do not overwrite the file (we do
 *           overwrite an existing .wpkg though)
 *         - Run the postinst script of the new package
 *         - Set the package status to "installed"
 *
 * \param[in] idx  The index as returned by unpack(), if you're not using
 * unpack() then use an index from zero to count() - 1.
 *
 * \return true if no error occured, false otherwise
 */
bool wpkgar_install::configure(int idx)
{
    // the caller is responsible for locking the database
    if(!f_manager->was_locked())
    {
        throw std::logic_error("the manager must be locked before calling wpkgar_install::configure()");
    }

    if(idx < 0 || static_cast<wpkgar_package_list_t::size_type>(idx) >= f_packages.size())
    {
        throw std::range_error("index out of range in wpkgar_install::configure()");
    }

    switch(f_packages[idx].get_type())
    {
    case package_item_t::package_type_explicit:
    case package_item_t::package_type_implicit:
        if(!f_packages[idx].is_unpacked())
        {
            throw std::logic_error("somehow the wpkgar_install::configure() function was called on a package that is not yet unpacked.");
        }
        break;

    case package_item_t::package_type_unpacked:
        // in this case we're configuring a package that was unpacked
        // earlier and not configured immediately
        break;

    case package_item_t::package_type_same:
        // --configure on an Installed package is ignored here
        return true;

    default:
        throw std::logic_error("the wpkgar_install::configure() function cannot be called with an index representing a package other than explicit or implicit.");

    }

    wpkg_output::log("configuring %1")
            .quoted_arg(f_packages[idx].get_name())
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);

    return configure_package(&f_packages[idx]);
}


/** \brief Reconfigure a package.
 *
 * This function reconfigures a package which includes 3 steps:
 *
 * \li Deconfigure the package (i.e. prerm upgrade <version>)
 * \li Reinstall (unpack) clean configuration files
 * \li Reconfigure the files (i.e. postinst configure <version>)
 *
 * However, this function does not actually call the configure() function,
 * you are responsible for doing so:
 *
 * \code
 *      ...
 *      installer->set_reconfiguring();
 *      ...
 *      if(!installer->validate())
 *      {
 *          // the validation did not work
 *          return false;
 *      }
 *      ...
 *      for(;;)
 *      {
 *          idx = installer->reconfigure()
 *          if(idx == WPKGAR_ERROR)
 *          {
 *              // reconfiguration failed
 *              break;
 *          }
 *          if(idx == WPKGAR_EOP)
 *          {
 *              // reconfiguration ended normally
 *              break;
 *          }
 *          if(!installed->configure(idx))
 *          {
 *              // configuration failed
 *              break;
 *          }
 *      }
 * \endcode
 *
 * This is very similar to the unpack() function.
 *
 * \return the index of the next package to reconfigure after we processed
 *         the unpack part (prerm + unpack of configuration files)
 */
int wpkgar_install::reconfigure()
{
    // the caller is responsible for locking the database
    if(!f_manager->was_locked())
    {
        throw std::logic_error("the manager must be locked before calling wpkgar_install::reconfigure()");
    }

    for(wpkgar_package_list_t::size_type i(0);
                                         i < f_sorted_packages.size();
                                         ++i)
    {
        // the sort is probably not useful here
        wpkgar_package_list_t::size_type idx(f_sorted_packages[i]);
        if(!f_packages[idx].is_unpacked())
        {
            switch(f_packages[idx].get_type())
            {
            case package_item_t::package_type_explicit:
                wpkg_output::log("reconfiguring %1")
                        .quoted_arg(f_packages[idx].get_name())
                    .debug(wpkg_output::debug_flags::debug_progress)
                    .module(wpkg_output::module_validate_installation);

                if(!do_unpack(&f_packages[idx], &f_packages[idx]))
                {
                    // an error occured, we cannot continue
                    return WPKGAR_ERROR;
                }

                return static_cast<int>(idx);

            default:
                // anything else cannot be reconfigured anyway
                break;

            }
        }
    }

    // End of Packages
    return WPKGAR_EOP;
}



}
// vim: ts=4 sw=4 et
