/*    wpkgar_remove.cpp -- remove a debian package
 *    Copyright (C) 2006-2013  Made to Order Software Corporation
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
 * \brief Implementation of the wpkg --remove command.
 *
 * This file includes the necessary functions to execute the remove and other
 * removal commands such as --purge, --autoremove, and --deconfigure.
 */
#include    "libdebpackages/wpkgar_remove.h"
#include    "libdebpackages/debian_version.h"
#include    "libdebpackages/wpkg_backup.h"
#include    "libdebpackages/wpkg_util.h"
#include    "libdebpackages/debian_packages.h"
#include    <sstream>
#include    <fstream>
#include    <algorithm>
#include    <stdarg.h>
#include    <errno.h>
#if defined(MO_LINUX)
#   include    <mntent.h>
#   include    <sys/statvfs.h>
#   include    <unistd.h>
#endif


namespace wpkgar
{


/** \class wpkgar_remove
 * \brief Class implemented to handle removal of packages.
 *
 * This class allows for the removal of packages from a target system.
 * It handles the validation of the removal, the deletion of the files,
 * the deconfiguration of the package as required.
 */


/** \class wpkgar_remove::package_item_t
 * \brief A package manager for the remove feature.
 *
 * This class handles packages internally for the removal process to know
 * what it is doing with each package.
 *
 * These packages may or may not exist. For example, the user may enter the
 * name of a non-existent package on the command line:
 *
 * \code
 * wpkg --remove unknown-package
 * \endcode
 *
 * Contrary to the install feature, the remove feature does not create a list
 * of repository packages.
 */





wpkgar_remove::package_item_t::package_item_t(wpkgar_manager *manager, const std::string& filename, package_type_t type = package_type_explicit)
    : f_manager(manager)
    , f_filename(filename)
    //, f_new_filename("") -- auto-init
    , f_type(type)
    //, f_depends_done(false) -- auto-init
    //, f_loaded(false) -- auto-init
    //, f_removed(false) -- auto-init
    //, f_configured(false) -- auto-init
    //, f_installed(false) -- auto-init
    //, f_name("") -- auto-init
    //, f_architecture("") -- auto-init
    //, f_version("") -- auto-init
    //, f_status("") -- auto-init
    , f_original_status(wpkgar_manager::unknown)
    , f_upgrade(-1) // no upgrade
{
}

void wpkgar_remove::package_item_t::load()
{
    if(!f_loaded)
    {
        f_loaded = true;
        f_manager->load_package(f_filename);
        f_name = f_manager->get_field(f_filename, wpkg_control::control_file::field_package_factory_t::canonicalized_name());
        f_architecture = f_manager->get_field(f_filename, wpkg_control::control_file::field_architecture_factory_t::canonicalized_name());
        f_version = f_manager->get_field(f_filename, wpkg_control::control_file::field_version_factory_t::canonicalized_name());
        f_status = f_manager->get_field(f_filename, wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name());
        f_original_status = f_manager->package_status(f_filename);
    }
}

const std::string& wpkgar_remove::package_item_t::get_filename() const
{
    return f_filename;
}

void wpkgar_remove::package_item_t::set_type(const package_type_t type)
{
    f_type = type;
}

wpkgar_remove::package_item_t::package_type_t wpkgar_remove::package_item_t::get_type() const
{
    return f_type;
}

const std::string& wpkgar_remove::package_item_t::get_name() const
{
    const_cast<package_item_t *>(this)->load();
    return f_name;
}

const std::string& wpkgar_remove::package_item_t::get_architecture() const
{
    const_cast<package_item_t *>(this)->load();
    return f_architecture;
}

const std::string& wpkgar_remove::package_item_t::get_version() const
{
    const_cast<package_item_t *>(this)->load();
    return f_version;
}

wpkgar_manager::package_status_t wpkgar_remove::package_item_t::get_original_status() const
{
    const_cast<package_item_t *>(this)->load();
    return f_original_status;
}

void wpkgar_remove::package_item_t::reset_original_status()
{
    // we must make sure that the package was loaded
    const_cast<package_item_t *>(this)->load();
    // read the current status
    f_original_status = f_manager->package_status(f_filename);
}

void wpkgar_remove::package_item_t::restore_original_status()
{
    // if f_status is empty then it was never loaded so it wasn't modified either
    if(!f_status.empty()) {
        f_manager->set_field(f_filename, wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), f_status, true);
    }
}

void wpkgar_remove::package_item_t::set_upgrade(int32_t upgrade)
{
    f_upgrade = upgrade;
}

int32_t wpkgar_remove::package_item_t::get_upgrade() const
{
    return f_upgrade;
}

void wpkgar_remove::package_item_t::mark_removed()
{
    f_removed = true;
}

bool wpkgar_remove::package_item_t::is_removed() const
{
    return f_removed;
}

void wpkgar_remove::package_item_t::mark_configured()
{
    f_configured = true;
}

bool wpkgar_remove::package_item_t::is_configured() const
{
    return f_configured;
}

bool wpkgar_remove::package_item_t::get_installed() const
{
    return f_installed;
}









wpkgar_remove::wpkgar_remove(wpkgar_manager *manager)
    : f_manager(manager)
      //f_flags() -- auto-init
      //f_instdir("") -- auto-init
      //f_packages() -- auto-init
      //f_purging_packages(false) -- auto-init
{
}


void wpkgar_remove::set_parameter(parameter_t flag, int value)
{
    f_flags[flag] = value;
}


int wpkgar_remove::get_parameter(parameter_t flag, int default_value) const
{
    wpkgar_flags_t::const_iterator it(f_flags.find(flag));
    if(it == f_flags.end()) {
        return default_value;
    }
    return it->second;
}


void wpkgar_remove::set_instdir(const std::string& instdir)
{
    f_instdir = instdir;
}


void wpkgar_remove::set_purging()
{
    f_purging_packages = true;
    f_deconfiguring_packages = false;
}

bool wpkgar_remove::get_purging() const
{
    return f_purging_packages;
}


void wpkgar_remove::set_deconfiguring()
{
    f_purging_packages = false;
    f_deconfiguring_packages = true;
}

bool wpkgar_remove::get_deconfiguring() const
{
    return f_deconfiguring_packages;
}


std::string wpkgar_remove::get_package_name(const int i) const
{
    return f_packages[i].get_name();
}


wpkgar_remove::wpkgar_package_list_t::const_iterator wpkgar_remove::find_package_item(const std::string& filename) const
{
    for(wpkgar_package_list_t::size_type i(0); i < f_packages.size(); ++i)
    {
        if(f_packages[i].get_filename() == filename)
        {
            return f_packages.begin() + i;
        }
    }
    return f_packages.end();
}


wpkgar_remove::wpkgar_package_list_t::iterator wpkgar_remove::find_package_item_by_name(const std::string& name)
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


void wpkgar_remove::add_package(const std::string& package)
{
    const wpkg_filename::uri_filename pck(package);
    wpkgar_package_list_t::const_iterator item(find_package_item(pck.full_path()));
    if(item != f_packages.end())
    {
        // the user is trying to remove the same package twice
        // (ignore if explicit, it doesn't hurt to have the same
        // name twice on the command line)
        if(item->get_type() != package_item_t::package_type_explicit)
        {
            wpkg_output::log("package %1 defined twice on your command line using two different paths.")
                    .quoted_arg(package)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_removal)
                .package(package)
                .action("remove-validation");
        }
    }
    else
    {
        package_item_t package_item(f_manager, pck.full_path(), package_item_t::package_type_explicit);
        f_packages.push_back(package_item);
    }
}


int wpkgar_remove::count() const
{
    return static_cast<int>(f_packages.size());
}


void wpkgar_remove::validate_package_names()
{
    for(wpkgar_package_list_t::const_iterator it(f_packages.begin());
                                              it != f_packages.end();
                                              ++it)
    {
        f_manager->check_interrupt();

        if(it->get_filename().find_first_of("_/") != std::string::npos)
        {
            // this is a full package name (a .deb file)
            wpkg_output::log("package name %1 cannot be used for removal.")
                    .quoted_arg(it->get_filename())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_removal)
                .package(it->get_filename())
                .action("remove-validation");
        }
    }
}


void wpkgar_remove::validate_explicit_packages()
{
    for(wpkgar_package_list_t::const_iterator it(f_packages.begin());
                                              it != f_packages.end();
                                              ++it)
    {
        f_manager->check_interrupt();

        // only already installed packages can be loaded so get_name()
        // is enough here (and if the load fails then it's not valid
        // anyway)
        f_manager->load_package(it->get_name());
        switch(f_manager->package_status(it->get_name())) {
        case wpkgar_manager::no_package:
            wpkg_output::log("package %1 is not known on this target.")
                    .quoted_arg(it->get_filename())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_removal)
                .package(it->get_filename())
                .action("remove-validation");
            break;

        case wpkgar_manager::unknown:
            wpkg_output::log("package %1 is in an unknown state and it will not be removed.")
                    .quoted_arg(it->get_filename())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_removal)
                .package(it->get_filename())
                .action("remove-validation");
            break;

        case wpkgar_manager::not_installed:
            wpkg_output::log("package %1 cannot be removed because it is not even installed.")
                    .quoted_arg(it->get_filename())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_removal)
                .package(it->get_filename())
                .action("remove-validation");
            break;

        case wpkgar_manager::installing:
        case wpkgar_manager::upgrading:
        case wpkgar_manager::removing:
        case wpkgar_manager::purging:
        case wpkgar_manager::listing:
        case wpkgar_manager::verifying:
        case wpkgar_manager::ready:
            wpkg_output::log("package %1 has an unexpected status for a --remove, --purge, or --deconfigure command.")
                    .quoted_arg(it->get_filename())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_removal)
                .package(it->get_filename())
                .action("remove-validation");
            break;

        case wpkgar_manager::config_files:
            if(!f_deconfiguring_packages && !f_purging_packages) {
                wpkg_output::log("package %1 is not installed, --remove will have no effect (--purge or --deconfigure?).")
                        .quoted_arg(it->get_filename())
                    .level(wpkg_output::level_warning)
                    .module(wpkg_output::module_validate_removal)
                    .package(it->get_filename())
                    .action("remove-validation");
            }
            break;

        case wpkgar_manager::unpacked:
            if(f_deconfiguring_packages) {
                wpkg_output::log("package %1 is only unpacked, --deconfigure will have no effect (--purge or --remove?).")
                        .quoted_arg(it->get_filename())
                    .level(wpkg_output::level_warning)
                    .module(wpkg_output::module_validate_removal)
                    .package(it->get_filename())
                    .action("remove-validation");
            }
            break;

        case wpkgar_manager::half_installed:
        case wpkgar_manager::half_configured:
        case wpkgar_manager::installed:
            // these are valid on the command line
            break;

        }
    }
}


void wpkgar_remove::validate_installed_packages()
{
    // read the names of all the installed packages
    wpkgar_manager::package_list_t list;
    f_manager->list_installed_packages(list);
    for(wpkgar_manager::package_list_t::const_iterator it(list.begin());
                                                       it != list.end();
                                                       ++it)
    {
        f_manager->check_interrupt();

        try
        {
            package_item_t::package_type_t type(package_item_t::package_type_invalid);
            switch(f_manager->package_status(*it))
            {
            case wpkgar_manager::not_installed:
                // already removed
                type = package_item_t::package_type_not_installed;
                break;

            case wpkgar_manager::config_files:
                // may still be purged
                type = package_item_t::package_type_configured;
                break;

            case wpkgar_manager::installed:
                // accepted as valid, be silent about all of those
                type = package_item_t::package_type_installed;
                break;

            case wpkgar_manager::unpacked:
                // nearly like installed when removing packages
                type = package_item_t::package_type_unpacked;
                break;

            case wpkgar_manager::no_package:
                wpkg_output::log("somehow a folder named %1 found in your database does not represent an existing package.")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_removal)
                    .package(*it)
                    .action("remove-validation");
                break;

            case wpkgar_manager::unknown:
                wpkg_output::log("package %1 has an unexpected status of \"unknown\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_removal)
                    .package(*it)
                    .action("remove-validation");
                break;

            case wpkgar_manager::half_installed:
                type = package_item_t::package_type_need_repair;
                wpkg_output::log("trying to repair half-installed package %1.")
                        .quoted_arg(*it)
                    .module(wpkg_output::module_validate_removal)
                    .package(*it)
                    .action("remove-validation");
                break;

            case wpkgar_manager::installing:
                wpkg_output::log("package %1 has an unexpected status of \"installing\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_removal)
                    .package(*it)
                    .action("remove-validation");
                break;

            case wpkgar_manager::upgrading:
                wpkg_output::log("package %1 has an unexpected status of \"upgrading\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_removal)
                    .package(*it)
                    .action("remove-validation");
                break;

            case wpkgar_manager::half_configured:
                type = package_item_t::package_type_need_repair;
                wpkg_output::log("trying to repair half-configured package %1.")
                        .quoted_arg(*it)
                    .module(wpkg_output::module_validate_removal)
                    .package(*it)
                    .action("remove-validation");
                break;

            case wpkgar_manager::removing:
                wpkg_output::log("package %1 has an unexpected status of \"removing\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_removal)
                    .package(*it)
                    .action("remove-validation");
                break;

            case wpkgar_manager::purging:
                wpkg_output::log("package %1 has an unexpected status of \"purging\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_removal)
                    .package(*it)
                    .action("remove-validation");
                break;

            case wpkgar_manager::listing:
                wpkg_output::log("package %1 has an unexpected status of \"listing\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_removal)
                    .package(*it)
                    .action("remove-validation");
                break;

            case wpkgar_manager::verifying:
                wpkg_output::log("package %1 has an unexpected status of \"verifying\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_removal)
                    .package(*it)
                    .action("remove-validation");
                break;

            case wpkgar_manager::ready:
                wpkg_output::log("package %1 has an unexpected status of \"ready\".")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_removal)
                    .package(*it)
                    .action("remove-validation");
                break;

            }

            // IMPORTANT: note that *it is a name (Package field), not a path, in this case
            wpkgar_package_list_t::iterator item(find_package_item_by_name(*it));
            if(item != f_packages.end())
            {
                // we should always get here for the packages that were
                // specified on the command line
                switch(item->get_type())
                {
                case package_item_t::package_type_explicit:
                    switch(type) {
                    case package_item_t::package_type_not_installed:
                        // we cannot remove a package that isn't installed
                        wpkg_output::log("package %1 is not installed and thus it cannot be removed, purged, or deconfigured. (1)")
                                .quoted_arg(*it)
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_removal)
                            .package(*it)
                            .action("remove-validation");
                        break;

                    case package_item_t::package_type_need_repair:
                        if(f_deconfiguring_packages)
                        {
                            wpkg_output::log("package %1 needs repair, --deconfigure is not enough, use --remove or --purge.")
                                    .quoted_arg(*it)
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_validate_removal)
                                .package(*it)
                                .action("remove-validation");
                        }
                        /*FALLTHROUGH*/
                    case package_item_t::package_type_installed:
                        // this means we're working on that package
                        item->set_type(package_item_t::package_type_removing);
                        f_manager->include_self(*it);
                        break;

                    case package_item_t::package_type_unpacked:
                        if(f_deconfiguring_packages)
                        {
                            // we already emitted a warning so we do not have to do it again here
                            item->set_type(package_item_t::package_type_same);
                        }
                        else
                        {
                            item->set_type(package_item_t::package_type_removing);
                            f_manager->include_self(*it);
                        }
                        break;

                    case package_item_t::package_type_configured:
                        if(!f_deconfiguring_packages && !f_purging_packages)
                        {
                            item->set_type(package_item_t::package_type_same);
                        }
                        else
                        {
                            item->set_type(package_item_t::package_type_removing);
                            f_manager->include_self(*it);
                        }
                        break;

                    case package_item_t::package_type_invalid:
                        // in this case we already output an error
                        break;

                    default:
                        throw std::logic_error("somehow the new type is not accounted for, please fix the code");

                    }
                    break;

                case package_item_t::package_type_removing:
                    // something is wrong in the algorithm if we get this one here!
                    wpkg_output::log("package %1 found twice in the existing installation.")
                            .quoted_arg(*it)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_removal)
                        .package(*it)
                        .action("remove-validation");
                    break;

                default:
                    // something wrong in the algorithm if we get this one here!
                    wpkg_output::log("package %1 found with an unexpected package type.")
                            .quoted_arg(*it)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_removal)
                        .package(*it)
                        .action("remove-validation");
                    break;

                }
            }
            else
            {
                // add that installed package to the list for reference
                switch(type)
                {
                case package_item_t::package_type_not_installed:
                case package_item_t::package_type_invalid:
                    // not necessary to keep in our list
                    break;

                case package_item_t::package_type_installed:
                case package_item_t::package_type_unpacked:
                case package_item_t::package_type_configured:
                    // we save those package entries because we'll use
                    // them to test all the dependencies
                    {
                        package_item_t package_item(f_manager, *it, type);
                        f_packages.push_back(package_item);
                    }
                    break;

                case package_item_t::package_type_need_repair:
                    wpkg_output::log("package %1 needs repair, it must be included in the list of packages to be removed or purged.")
                            .quoted_arg(*it)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_removal)
                        .package(*it)
                        .action("remove-validation");
                    break;

                default:
                    throw std::logic_error("somehow the new type is not accounted for, please fix the code");

                }
            }
        }
        catch(const std::exception& e)
        {
            wpkg_output::log("installed package %1 could not be loaded (%2).")
                    .quoted_arg(*it)
                    .arg(e.what())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_removal)
                .package(*it)
                .action("remove-validation");
        }
    }
}


// if invalid, return -1
// if valid but not in range, return 0
// if valid and in range, return 1
int wpkgar_remove::match_dependency_version(const wpkg_dependencies::dependencies::dependency_t& d, const std::string& name)
{
    // check the version if necessary
    if(!d.f_version.empty()
    && d.f_operator != wpkg_dependencies::dependencies::operator_any)
    {
        std::string version(f_manager->get_field(name, wpkg_control::control_file::field_version_factory_t::canonicalized_name()));

        int c(wpkg_util::versioncmp(version, d.f_version));

        bool r(false);
        switch(d.f_operator)
        {
        case wpkg_dependencies::dependencies::operator_any:
            throw std::logic_error("this cannot happen here unless the if() checking this value is invalid");

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


bool wpkgar_remove::can_package_be_removed(const std::string& filename, bool cannot_force)
{
    bool result(true);

    // 100% prevent required packages from being removed
    // (there is no --force-... for this one!)
    if(f_manager->field_is_defined(filename, wpkg_control::control_file::field_priority_factory_t::canonicalized_name()))
    {
        case_insensitive::case_insensitive_string priority(f_manager->get_field(filename, wpkg_control::control_file::field_priority_factory_t::canonicalized_name()));
        if(priority == "required")
        {
            // this is a fatal error, there is not way around it
            // (other than messing around with the content of the
            // wpkg database.)
            wpkg_output::log("package %1 is a required package and it cannot be removed, purged, or deconfigured (and there are no options to circumvent this case.).")
                    .quoted_arg(filename)
                .level(wpkg_output::level_fatal)
                .module(wpkg_output::module_validate_removal)
                .package(filename)
                .action("remove-validation");
            result = false;
        }
    }

    // prevent Essential packages from being removed
    // unless user entered --force-remove-essential
    if(f_manager->field_is_defined(filename, wpkg_control::control_file::field_essential_factory_t::canonicalized_name())
    && f_manager->get_field_boolean(filename, wpkg_control::control_file::field_essential_factory_t::canonicalized_name()))
    {
        if(!cannot_force && get_parameter(wpkgar_remove_force_remove_essentials, false))
        {
            wpkg_output::log("package %1 is an essential package and it is going to be removed, purged, or deconfigured.")
                    .quoted_arg(filename)
                .level(wpkg_output::level_warning)
                .module(wpkg_output::module_validate_removal)
                .package(filename)
                .action("remove-validation");
        }
        else
        {
            wpkg_output::log("package %1 is an essential package and it will not be removed, purged, or deconfigured (use --force-remove-essentials to circumvent the situation, also if the package was not specified on the command line, it is required there because implicit packages marked as essential cannot automatically be removed).")
                    .quoted_arg(filename)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_removal)
                .package(filename)
                .action("remove-validation");
            result = false;
        }
    }

    // prevent packages that are marked as "hold" from being removed
    // unless user entered --force-hold
    if(f_manager->field_is_defined(filename, wpkg_control::control_file::field_xselection_factory_t::canonicalized_name()))
    {
        wpkg_control::control_file::field_xselection_t::selection_t selection(wpkg_control::control_file::field_xselection_t::validate_selection(f_manager->get_field(filename, wpkg_control::control_file::field_xselection_factory_t::canonicalized_name())));
        if(selection == wpkg_control::control_file::field_xselection_t::selection_hold)
        {
            if(!cannot_force && get_parameter(wpkgar_remove_force_hold, false))
            {
                wpkg_output::log("package %1 is being %2 even though it is on hold.")
                        .quoted_arg(filename)
                        .arg(f_deconfiguring_packages ? "deconfigured" : "removed")
                    .level(wpkg_output::level_warning)
                    .module(wpkg_output::module_validate_removal)
                    .package(filename)
                    .action("remove-validation");
            }
            else
            {
                wpkg_output::log("package %1 is not getting %2 because it is on hold. If you used --recursive and this is an implicit package, you will also have to specify its name on the command line.")
                        .quoted_arg(filename)
                        .arg(f_deconfiguring_packages ? "deconfigured" : "removed")
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_removal)
                    .package(filename)
                    .action("remove-validation");
                result = false;
            }
        }
    }

    return result;
}


void wpkgar_remove::validate_removal()
{
    // all explicit packages must now be marked as removing
    // if we still have some explicit packages then that's an
    // error (i.e. specified package is not installed)
    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < f_packages.size();
                                         ++idx)
    {
        f_manager->check_interrupt();

        switch(f_packages[idx].get_type())
        {
        case package_item_t::package_type_explicit:
            // still explicit!
            wpkg_output::log("package %1 is not installed and thus it cannot be removed, purged, or deconfigured. (2)")
                    .quoted_arg(f_packages[idx].get_name())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_removal)
                .package(f_packages[idx].get_name())
                .action("remove-validation");
            break;

        case package_item_t::package_type_removing:
            // prevent removal of essential, required, and held packages
            // (unless --force-remove-... was used)
            can_package_be_removed(f_packages[idx].get_filename(), false);
            break;

        case package_item_t::package_type_need_repair:
            break;

        default:
            // ignore everything else here
            break;

        }
    }
}


void wpkgar_remove::output_tree(int file_count, const wpkgar_package_list_t& tree, const std::string& sub_title)
{
    memfile::memory_file dot;
    dot.create(memfile::memory_file::file_format_other);
    dot.printf("digraph {\nrankdir=BT;\nlabel=\"Packager Dependency Graph (%s)\";\n", sub_title.c_str());

    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < tree.size();
                                         ++idx)
    {
        const char *name(tree[idx].get_name().c_str());
        const char *version(tree[idx].get_version().c_str());
        switch(tree[idx].get_type())
        {
        case package_item_t::package_type_explicit:
            dot.printf("n%d [label=\"%s (exp)\\n%s\",shape=box,color=black];\n", idx, name, version);
            break;

        case package_item_t::package_type_removing:
            dot.printf("n%d [label=\"%s (rmp)\\n%s\",shape=box,color=black];\n", idx, name, version);
            break;

        case package_item_t::package_type_not_installed:
            dot.printf("n%d [label=\"%s (not)\\n%s\",shape=box,color=\"#cccccc\"];\n", idx, name, version);
            break;

        case package_item_t::package_type_installed:
            dot.printf("n%d [label=\"%s (ins)\\n%s\",shape=box,color=black];\n", idx, name, version);
            break;

        case package_item_t::package_type_unpacked:
            dot.printf("n%d [label=\"%s (upk)\\n%s\",shape=ellipse,color=red];\n", idx, name, version);
            break;

        case package_item_t::package_type_configured:
            dot.printf("n%d [label=\"%s (cfg)\\n%s\",shape=box,color=purple];\n", idx, name, version);
            break;

        case package_item_t::package_type_implicit:
            dot.printf("n%d [label=\"%s (imp)\\n%s\",shape=box,color=\"#aa5500\"];\n", idx, name, version);
            break;

        case package_item_t::package_type_need_repair:
            dot.printf("n%d [label=\"%s (nrp)\\n%s\",shape=ellipse,color=\"#cccccc\"];\n", idx, name, version);
            break;

        case package_item_t::package_type_invalid:
            dot.printf("n%d [label=\"%s (inv)\\n%s\",shape=ellipse,color=red];\n", idx, name, version);
            break;

        case package_item_t::package_type_same:
            dot.printf("n%d [label=\"%s (sam)\\n%s\",shape=box,color=\"#cccccc\"];\n", idx, name, version);
            break;

        }
        std::string filename(tree[idx].get_filename());
        if(!f_manager->field_is_defined(filename, wpkg_control::control_file::field_depends_factory_t::canonicalized_name()))
        {
            continue;
        }

        // check the dependencies
        wpkg_dependencies::dependencies depends(f_manager->get_field(filename, wpkg_control::control_file::field_depends_factory_t::canonicalized_name()));
        for(int i(0); i < depends.size(); ++i)
        {
            const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(i));

            for(wpkgar_package_list_t::size_type j(0);
                                                 j < tree.size();
                                                 ++j)
            {
                if(d.f_name == tree[j].get_name())
                {
                    dot.printf("n%d -> n%d;\n", idx, j);
                }
            }
        }
    }
    dot.printf("}\n");
    std::stringstream stream;
    stream << "remove-graph-";
    stream << file_count;
    stream << ".dot";
    dot.write_file(stream.str());
}


/** \brief Validate the dependency tree.
 *
 * For the removal of packages the dependency tree is much easier to
 * deal with since it is unique and clearly defined as the set of
 * installed packages (although there could be dangling dependencies
 * if the user used --force-depends before and thus some dependencies
 * are missing.)
 *
 * So unless the administrator used the --recursive command line option,
 * the parsing for dependency validity is a simple one pass through all
 * the installed packages to see whether one of their dependencies
 * is a package being removed. The --recursive means that the loop must
 * be run again if any one package becomes an implicit package.
 *
 * The result is one of the following:
 *
 * 1) no packages depend on the packages being removed (this is
 *    the expected case) or both packages are already marked for
 *    removal
 *
 * 2) one or more packages depend on the packages being removed and
 *    the --recursive was specified, all those packages get marked
 *    for removal too (become implicit targets)
 *
 * 3) one or more packages depend on the packages being removed,
 *    the --recursive was specified, and the package is an essential
 *    package, then the removal fails
 *
 * 4) one or more packages depend on the packages being removed and
 *    the --force-depends was specified, the removal happens as if
 *    no packages were depending on the packages being removed
 *
 * 5) one or more packages depend on the packages being removed and
 *    --recursive and --force-depends were not specified, the removal
 *    fails since it would break the existing tree (this is the default
 *    unless you are in case (1): the expected case)
 */
void wpkgar_remove::validate_dependencies()
{
    // in case --recursive is used we repeat the test on the newly marked
    // implicit packages, this is done by adding them to the package_indexes
    // vector initialized below with targets to be removed
    std::vector<wpkgar_package_list_t::size_type> package_indexes;
    wpkgar_package_list_t::size_type idx;
    for(idx = 0; idx < f_packages.size(); ++idx)
    {
        switch(f_packages[idx].get_type())
        {
        //case package_item_t::package_type_explicit: -- these are "removing" if they need to be checked
        case package_item_t::package_type_implicit: // implicit targets need to be checked too, although there shouldn't be any yet
        case package_item_t::package_type_removing: // explicit to be removed were changed into removing
            package_indexes.push_back(idx);
            break;

        default:
            // explicit / need-repair already generated an error if needed
            // not installed / invalid do not apply at all
            break;

        }
    }

    while(!package_indexes.empty())
    {
        idx = package_indexes.back();
        package_indexes.pop_back();
        std::string name(f_packages[idx].get_name());

        // look for any one package that would depend on the package
        // that is proposed for removal
        for(wpkgar_package_list_t::size_type j(0); j < f_packages.size(); ++j)
        {
            f_manager->check_interrupt();

            if(idx == j)
            {
                // skip ourself
                continue;
            }
            switch(f_packages[j].get_type())
            {
            case package_item_t::package_type_explicit:
            case package_item_t::package_type_not_installed:
            case package_item_t::package_type_invalid:
            case package_item_t::package_type_same:
            case package_item_t::package_type_configured: // this is not an installed package
            case package_item_t::package_type_need_repair: // this is a form of "unpacked" in most cases (i.e. no configuration files)
                // ignore these packages as they are not linked to this item
                break;

            case package_item_t::package_type_removing:
            case package_item_t::package_type_implicit:
            case package_item_t::package_type_installed:
            case package_item_t::package_type_unpacked:
                // full path to package
                std::string filename(f_packages[j].get_name());

                // make sure it's loaded
                f_manager->load_package(filename);

                // get list of all dependencies
                std::string dependencies;
                if(f_manager->field_is_defined(filename, wpkg_control::control_file::field_predepends_factory_t::canonicalized_name()))
                {
                    // not necessary for the first one, it's always empty
                    //if(!dependencies.empty()) {
                    //    dependencies += ",";
                    //}
                    dependencies = f_manager->get_field(filename, wpkg_control::control_file::field_predepends_factory_t::canonicalized_name());
                }
                if(f_manager->field_is_defined(filename, wpkg_control::control_file::field_depends_factory_t::canonicalized_name()))
                {
                    if(!dependencies.empty())
                    {
                        dependencies += ",";
                    }
                    dependencies += f_manager->get_field(filename, wpkg_control::control_file::field_depends_factory_t::canonicalized_name());
                }
                if(!dependencies.empty())
                {
                    wpkg_dependencies::dependencies depends(dependencies);
                    for(int i(0); i < depends.size(); ++i)
                    {
                        f_manager->check_interrupt();

                        const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(i));
                        if(name == d.f_name)
                        {
                            // case 1: that other package is already marked for removal
                            bool marked_for_removal_too(false);
                            switch(f_packages[j].get_type())
                            {
                            //case package_item_t::package_type_explicit: -- should not happen here anymore
                            case package_item_t::package_type_implicit: // implicitly marked for removal
                            case package_item_t::package_type_removing: // explicit that was already changed into removing
                                // this is not a problem
                                marked_for_removal_too = true;
                                break;

                            default:
                                // any of the other packages may cause a problem
                                break;

                            }
                            if(!marked_for_removal_too)
                            {
                                // case 2 to 5: problem!
                                if(get_parameter(wpkgar_remove::wpkgar_remove_force_depends, false))
                                {
                                    // case 4: ignore the problem
                                    ;
                                }
                                else if(get_parameter(wpkgar_remove::wpkgar_remove_recursive, false))
                                {
                                    // case 3: --recursive remove fails if one
                                    //         of the following is true
                                    //             (a) essential;
                                    //             (b) required;
                                    //             (c) hold packages.
                                    if(can_package_be_removed(filename, true))
                                    {
                                        // case 2: automatically remove this dependent
                                        wpkg_output::log("%1 is a dependent of %2 which will automatically be removed because you used --recursive.")
                                                .quoted_arg(name)
                                                .quoted_arg(f_packages[j].get_name())
                                            .module(wpkg_output::module_validate_removal)
                                            .package(name)
                                            .action("remove-validation");
                                        f_packages[j].set_type(package_item_t::package_type_implicit);
                                        package_indexes.push_back(j);
                                    }
                                }
                                else
                                {
                                    // case 5: generate an error
                                    wpkg_output::log("package %1 depends on %2 preventing its removal (try --recursive).")
                                            .quoted_arg(f_packages[j].get_name())
                                            .quoted_arg(name)
                                        .level(wpkg_output::level_error)
                                        .module(wpkg_output::module_validate_removal)
                                        .package(name)
                                        .action("remove-validation");
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}


/** \brief Run user defined validation scripts.
 *
 * At times it may be useful to run scripts before the system is ready
 * to run the remove command. The scripts defined here are called
 * validation scripts for that reason. These scripts are expected to
 * test things and modify nothing.
 *
 * Note that when removing a package you may actually be repairing that
 * package, meaning that the package was never properly installed in the
 * first place. Similarly, the scripts are run against packages that are
 * there only because they have some configuration scripts. In that case
 * too these packages are not considered installed.
 */
void wpkgar_remove::validate_scripts()
{
    // run the package validation script of the packages being removed or
    // purged and as we're at it generate the list of package names
    int errcnt(0);
    std::string package_names;
    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < f_packages.size();
                                         ++idx)
    {
        switch(f_packages[idx].get_type())
        {
        case package_item_t::package_type_removing:
        case package_item_t::package_type_implicit:
            {
                package_names += f_packages[idx].get_filename() + " ";

                // old-validate remove <version>
                wpkgar_manager::script_parameters_t params;
                params.push_back("remove");
                params.push_back(f_packages[idx].get_version());
                if(!f_manager->run_script(f_packages[idx].get_filename(), wpkgar_manager::wpkgar_script_validate, params))
                {
                    wpkg_output::log("the validate script of package %1 returned with an error, removal aborted.")
                            .quoted_arg(f_packages[idx].get_name())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_removal)
                        .package(f_packages[idx].get_name())
                        .action("remove-validation");
                    ++errcnt;
                }
            }
            break;

        default:
            // other packages are not going to be installed
            break;

        }
    }

    // if no error occured also check the global hooks
    if(errcnt == 0)
    {
        // old-validate remove <package-names>
        wpkgar_manager::script_parameters_t params;
        params.push_back("remove");
        params.push_back(package_names);
        if(!f_manager->run_script("core", wpkgar_manager::wpkgar_script_validate, params))
        {
            wpkg_output::log("a global validation hook failed, the removal is canceled.")
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_removal)
                .action("remove-validation");
            //++errcnt; -- not necessary here
        }
    }
}



/** \brief Validate one or more packages for removal.
 *
 * The --remove, --purge, --deconfigure commands means the
 * user expects packages to be removed, purged, or deconfigured from
 * a target machine.
 *
 * This function runs all the validations to ensure that the resulting
 * removals keep the system consistent.
 *
 * As much as possible, validation failures are recorded but do not stop
 * the process until the actual removal of the packages happens. This
 * allows us to give all the information available for the user to correct
 * his command line at once.
 *
 *   - Check the type of all the package names; if the name is not of an
 *     installed package, it fails; these failures are pretty immediate
 *     since we cannot really validate incorrect files any further
 *   - Verify that the database is ready for a removal, if not
 *     we fail without further validation
 *   - Check all the installed packages to make sure that they are in a
 *     valid state (installed, uninstalled, or unpacked); if not
 *     the process fails except when the package was also specified on
 *     the command line (explicit) since removing it is likely to fix
 *     the state as a whole
 *   - Check the database current status, if not "ready", then the
 *     process fails because a previous command failed
 *   - Read all the packages specified on the command line
 *   - If any package fails to load, the process fail
 *   - Build the tree of packages based on their dependencies
 *   - If the one or more packages depend on any one of the packages
 *     being removed; fail unless --recursive or --force-depends was
 *     used
 *   - If we found some dependency problems and --recursive was
 *     specified, then mark all the necessary packages for removal
 *   - Check whether any of the packages marked for removal is an
 *     essential package, if so and --force-remove-essentials was
 *     not specified then the process fails
 *   - Determine whether fields are valid (i.e. apply
 *     the  --validate-fields expression to all the modules being
 *     removed), if it does not validate the process fails
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
 *
 * \return true if the validation succeeded and the removal can proceed.
 * false if one or more error was generated.
 */
bool wpkgar_remove::validate()
{
    // the caller is responsible for locking the database
    if(!f_manager->was_locked())
    {
        throw std::logic_error("the manager must be locked before calling wpkgar_remove::validate()");
    }

    // --force-depends means don't recurse so it is not possible
    // to use it with the --recursive command line option
    if(get_parameter(wpkgar_remove_force_depends, false)
    && get_parameter(wpkgar_remove_recursive, false))
    {
        throw std::runtime_error("the --force-depends and --recursive flags are mutually exclusive, you must use one or the other");
    }

    // load the core page as we need some info here and there from it
    // (although at this point I don't think I need it... anyway...)
    wpkg_output::log("validate core package")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_removal);
    f_manager->load_package("core");

    // make sure that the package names specified on the command line are valid
    wpkg_output::log("validate package names")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_removal);
    validate_package_names();

    // explicit packages must be "mostly installed" packages
    wpkg_output::log("validate explicit packages")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_removal);
    validate_explicit_packages();

    // make sure that the currently installed packages are in an understandable
    // state for a removal to occur (some states are still completely invalid
    // for installed packages, opposed to the core package)
    wpkg_output::log("validate installed packages")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_removal);
    validate_installed_packages();

    // if any of the explicit packages are still explicit then it is not
    // installed (or in need of repair) so it shouldn't be on the command
    // line -- err in this case!
    //
    // also, for those being removed, check whether they are essential
    // packages and if so prevent the removal (unless --force-remove-essentials
    // was specified on the command line)
    wpkg_output::log("validate removal")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_removal);
    validate_removal();

    // for removal to be acceptable, we need to make sure that dependencies
    // do not prevent one of the modules from being removed. The problem
    // we encounter is that the removal of A is prevented if B depends on
    // A and B is not being removed. This would otherwise break the tree.
    //
    // There are two ways to resolve the problems:
    //
    // 1) Use --recursive which means the system implicitly removes the
    //    dependents (B in our example) assuming those are not marked as
    //    essential packages
    //
    // 2) Use --force-depends and the error is ignored
    //
    wpkg_output::log("validate dependencies")
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_removal);
    validate_dependencies();

    // run user defined validation scripts found in the implicit and
    // explicit packages
    if(wpkg_output::get_output_error_count() == 0)
    {
        wpkg_output::log("validate hooks")
            .debug(wpkg_output::debug_flags::debug_progress)
            .module(wpkg_output::module_validate_removal);
        validate_scripts();
    }


//printf("list packages after deps:\n");
//for(wpkgar_package_list_t::size_type idx(0); idx < f_packages.size(); ++idx) {
//printf("  %d - [%s]\n", (int)f_packages[idx].get_type(), f_packages[idx].get_filename().c_str());
//}

    return wpkg_output::get_output_error_count() == 0;
}









bool wpkgar_remove::prerm_scripts(package_item_t *item, const std::string& command)
{
    std::string new_status;
    wpkgar_manager::package_status_t status(f_manager->package_status(item->get_name()));
    switch(status)
    {
    case wpkgar_manager::installed:
    case wpkgar_manager::unpacked:
        new_status = "Half-Installed";
        break;

    case wpkgar_manager::config_files:
        new_status = "Half-Configured";
        break;

    default:
        // skip on running the remove script because it was not
        // properly installed anyway... the other unwind would
        // have taken care of proper clean up if necessary
        // we do not want to change the state to Half-Installed
        // or Half-Configured in this case either...
        return true;

    }

    f_manager->set_field(item->get_filename(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), new_status, true);

    // hooks-prerm remove|deconfigure <package-name> <version>
    wpkgar_manager::script_parameters_t hook_params;
    hook_params.push_back(command);
    hook_params.push_back(item->get_name());
    hook_params.push_back(item->get_version());
    if(!f_manager->run_script("core", wpkgar_manager::wpkgar_script_prerm, hook_params))
    {
        wpkg_output::log("a prerm global validation hook failed for package %1, the removal is canceled.")
                .quoted_arg(item->get_name())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_remove_package)
            .action("remove-scripts");
        return false;
    }

    // prerm remove  or  prerm deconfigure
    wpkgar_manager::script_parameters_t params;
    params.push_back(command);
    if(!f_manager->run_script(item->get_filename(), wpkgar_manager::wpkgar_script_prerm, params))
    {
        wpkg_output::log("the prerm script failed for package %1, the removal is canceled.")
                .quoted_arg(item->get_name())
            .level(wpkg_output::level_warning)
            .module(wpkg_output::module_remove_package)
            .action("remove-scripts");

        // postinst abort-remove  or  postinst abort-deconfigure
        // (here we're not deconfiguring in favor of another package, it's the direct --deconfigure call)
        params.clear();
        params.push_back("abort-" + command);
        if(f_manager->run_script(item->get_filename(), wpkgar_manager::wpkgar_script_postinst, params))
        {
            // the error unwind worked so we switch the state back
            // to what it was on entry
            item->restore_original_status();
        }
        else
        {
            // the package stays in an half-installed or half-configured state
            wpkg_output::log("the postinst script failed to restore package %1 state, the package is now half-installed or half-configured.")
                    .quoted_arg(item->get_name())
                .level(wpkg_output::level_warning)
                .module(wpkg_output::module_remove_package)
                .action("remove-scripts");
        }
        return false;
    }

    return true;
}


/** \brief Remove the files attached to the specified package.
 *
 * This function goes through the list of files defined in the given package
 * and delete them from the disk.
 *
 * The function updates the package status as required.
 *
 * As expected, the function executes the user scripts before and after
 * the removal process. Note that the postrm does not happen unless you
 * used --purge and it really only happens at the end of the deconfigure
 * step.
 *
 * This function actually creates a backup of all the files it is expected
 * to delete in case an error occur.
 *
 * \param[in] item  The item representing the package to be removed.
 */
bool wpkgar_remove::do_remove(package_item_t *item)
{
    if(!prerm_scripts(item, "remove"))
    {
        return false;
    }

    // RAII backup, by default we restore the backup files;
    // if everything works as expected we call success() which
    // prevents the restore; either way the object deletes the
    // backup files it creates (see backup() function)
    wpkg_backup::wpkgar_backup backup(f_manager, item->get_name(), "remove-remove");

    // get the data archive of item (new package) and remove
    // all the corresponding files
    try
    {
        f_manager->set_field(item->get_name(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Removing", true);
        f_manager->set_field(item->get_name(), "X-Remove-Date", wpkg_util::rfc2822_date(), true);

        {
            std::string package_name(item->get_filename());
            memfile::memory_file data;
            std::string data_filename("data.tar");
            f_manager->get_control_file(data, item->get_filename(), data_filename, false);
            for(;;)
            {
                memfile::memory_file::file_info info;
                memfile::memory_file file;
                if(!data.dir_next(info, &file))
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
                const wpkg_filename::uri_filename destination(f_manager->get_inst_path().append_child(filename));
                switch(info.get_file_type())
                {
                case memfile::memory_file::file_info::regular_file:
                case memfile::memory_file::file_info::continuous:
                case memfile::memory_file::file_info::symbolic_link:
                    // keep all configuration files
                    if(f_manager->is_conffile(package_name, filename))
                    {
                        break;
                    }
                    // do a backup no matter what
                    backup.backup(destination);
                    // delete that file
                    destination.os_unlink();

                    wpkg_output::log("%1 removed...")
                            .quoted_arg(destination)
                        .debug(wpkg_output::debug_flags::debug_files)
                        .module(wpkg_output::module_remove_package)
                        .package(package_name);
                    break;

                case memfile::memory_file::file_info::directory:
                    // TODO: check whether the directory is empty, if so
                    //       remove it too
                    break;

                default:
                    // at this point we ignore other file types because they
                    // are not supported under MS-Windows so we don't have
                    // to do anything with them anyway
                    wpkg_output::log("file %1 is not a regular file or a directory, it will be ignored.")
                            .quoted_arg(destination)
                        .level(wpkg_output::level_warning)
                        .module(wpkg_output::module_remove_package)
                        .package(item->get_name())
                        .action("remove-delete");
                    break;

                }
            }
        }

        // the post remove script is run before we delete the configuration
        // files (in case we're --purge-ing)
        f_manager->set_field(item->get_name(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Half-Installed", true);

        // TODO:
        // TBD -- should we NOT run the postrm script if we do not run the
        // prerm script? (after all the preinst was probably run...)
        wpkgar_manager::script_parameters_t params;
        params.push_back("remove");
        if(!f_manager->run_script(item->get_filename(), wpkgar_manager::wpkgar_script_postrm, params))
        {
            // NOTE: we ignore the result and proceed, but we emit a warning
            wpkg_output::log("a prerm script failed for package %1, the failure will be ignored though.")
                    .quoted_arg(item->get_name())
                .level(wpkg_output::level_warning)
                .module(wpkg_output::module_remove_package)
                .action("remove-delete");
        }

        // hooks-postrm remove <package-name> <version>
        wpkgar_manager::script_parameters_t hook_params;
        hook_params.push_back("remove");
        hook_params.push_back(item->get_name());
        hook_params.push_back(item->get_version());
        if(!f_manager->run_script("core", wpkgar_manager::wpkgar_script_postrm, hook_params))
        {
            wpkg_output::log("a postrm global hook failed for package %1, the failure will be ignored though.")
                    .quoted_arg(item->get_name())
                .level(wpkg_output::level_warning)
                .module(wpkg_output::module_remove_package)
                .action("remove-delete");
        }
    }
    catch(...)
    {
        // we are not annihilating the catch but we want to run scripts if
        // an error occurs to cancel the process;
        // note that installations are canceled without running a script,
        // upgrades are though
        f_manager->set_field(item->get_name(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Half-Installed", true);

        // dpkg doesn't seem to do this but it seems logical
        wpkgar_manager::script_parameters_t params;
        params.push_back("abort-remove");
        f_manager->run_script(item->get_filename(), wpkgar_manager::wpkgar_script_postinst, params);
        throw;
    }

    f_manager->remove_hooks(item->get_name());

    // the configuration files are still there, the rest is gone
    // (XXX what is the new status when starting from Half-Installed or Half-Configured?)
    const std::string final_status(item->get_original_status() == wpkgar_manager::unpacked || item->get_original_status() == wpkgar_manager::not_installed ? "Not-Installed" : "Config-Files");
    f_manager->set_field(item->get_name(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), final_status, true);
    f_manager->set_field(item->get_name(), "X-Removed-Date", wpkg_util::rfc2822_date(), true);

    // change the original so the deconfigure does the right thing!
    item->reset_original_status();

    // just delete all those backups!
    backup.success();

    return true;
}


/** \brief Remove the files previously installed from a package.
 *
 * This process is equivalent to looking for all the files in the
 * data.tar.gz file and doing rm -rf on each one of them. More or
 * less, something like this:
 *
 * \code
 * rm -rf `tar tzf data.tar.gz`
 * \endcode
 *
 * However, the remove function does not delete the configuration
 * files (it actually does not touch them at all.) The list of
 * configuration files are found in the conffiles found in the
 * control.tar.gz file. The deconfigure handles those files.
 *
 * The process involves running the prerm script as well (and postinst
 * in case of failure of the prerm script.)
 *
 *   - Set the status to Half-Installed (unpacked or installed packages)
 *   - Run the "prerm remove" command
 *     - on failure, run "postinst abort-remove" command
 *       - on failure leave the package "Half-Installed"
 *       - on success return the package to "Installed" or "Unpacked"
 *     - on success, remove all the files except the configuration files
 *       - on failure, run "postinst abort-remove" command (in this case
 *         we may have missing files!) Status stay Half-Installed
 *       - on success, run "postrm remove"
 *         - ignore results of postrm except an exception:
 *           - run "postinst abort-remove" on exceptions
 *         - on success, state becomes "Conf-Files" or "Not-Installed"
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
int wpkgar_remove::remove()
{
    // the caller is responsible for locking the database
    if(!f_manager->was_locked())
    {
        throw std::logic_error("the manager must be locked before calling wpkgar_remove::remove()");
    }

    for(wpkgar_package_list_t::size_type idx(0);
                                         idx < f_packages.size();
                                         ++idx)
    {
        if(!f_packages[idx].is_removed())
        {
            switch(f_packages[idx].get_type())
            {
            case package_item_t::package_type_removing:
            case package_item_t::package_type_implicit:
                {
                    std::string package_name(f_packages[idx].get_name());
                    wpkg_output::log("removing %1")
                            .quoted_arg(f_packages[idx].get_name())
                        .debug(wpkg_output::debug_flags::debug_progress)
                        .module(wpkg_output::module_validate_removal)
                        .package(f_packages[idx].get_name());

                    // restore in case of a remove or a purge requires an
                    // original package from a repository; if the package
                    // was configured we need an install, otherwise unpack
                    std::string restore_cmd(f_packages[idx].is_configured() ? "install " : "unpack ");
                    restore_cmd += package_name + "_" + f_packages[idx].get_version();
                    if(f_packages[idx].get_architecture() != "src"
                    && f_packages[idx].get_architecture() != "source")
                    {
                        restore_cmd += f_packages[idx].get_architecture();
                    }
                    restore_cmd += ".deb";
                    f_manager->track(restore_cmd, package_name);

                    if(!do_remove(&f_packages[idx]))
                    {
                        // an error occured, we cannot continue
                        // TBD: should we throw?
                        return WPKGAR_ERROR;
                    }
                    f_packages[idx].mark_removed();
                    return static_cast<int>(idx);
                }
                break;

            default:
                // anything else is ignored
                break;

            }
        }
    }

    // End of Packages
    return WPKGAR_EOP;
}


/** \brief Automatically remove packages.
 *
 * This function implements the --autoremove functionality by removing
 * all the packages that are marked "auto" (as per their selection) and
 * that are not currently depended on. Note that the removal is recursive
 * so if A depends on B and both are marked "auto" and none of the other
 * packages depends on A or B, then both are removed.
 *
 * \param[in] dryrun  Whether the removal is effectively done or not.
 */
void wpkgar_remove::autoremove(bool dryrun)
{
    typedef std::vector<std::string> depend_field_names_t;
    depend_field_names_t depend_names;
    depend_names.push_back(wpkg_control::control_file::field_depends_factory_t::canonicalized_name());
    depend_names.push_back(wpkg_control::control_file::field_predepends_factory_t::canonicalized_name());
    depend_names.push_back(wpkg_control::control_file::field_builddepends_factory_t::canonicalized_name());
    depend_names.push_back(wpkg_control::control_file::field_builddependsarch_factory_t::canonicalized_name());
    depend_names.push_back(wpkg_control::control_file::field_builddependsindep_factory_t::canonicalized_name());
    depend_names.push_back(wpkg_control::control_file::field_builtusing_factory_t::canonicalized_name());

    // since we're locked we assume that the list and the status of packages
    // won't change under our feet
    typedef std::map<std::string, wpkgar_manager::package_status_t> package_statuses_t;
    package_statuses_t status;
    wpkgar_manager::package_list_t list;
    f_manager->list_installed_packages(list);
    for(wpkgar_manager::package_list_t::const_iterator it(list.begin());
                                                       it != list.end();
                                                       ++it)
    {
        f_manager->check_interrupt();
        status[*it] = f_manager->package_status(*it);
    }

    // we use a simple repeat flag to implement the recursivity; it's a bit
    // slow but it does the job well
    bool repeat(true);
    while(repeat && wpkg_output::get_output_error_count() == 0)
    {
        repeat = false;

        for(wpkgar_manager::package_list_t::const_iterator it(list.begin());
                                                           it != list.end();
                                                           ++it)
        {
            f_manager->check_interrupt();

            // don't ever attempt to remove self!
            if(f_manager->exists_as_self(*it))
            {
                continue;
            }

            // check whether we want to remove this package
            bool remove_package(false);
            switch(status[*it])
            {
            case wpkgar_manager::config_files:
                if(f_purging_packages)
                {
                    remove_package = true;
                }
                break;

            case wpkgar_manager::installed:
            case wpkgar_manager::unpacked:
                remove_package = true;

                // ignore the remove order if the package is essential
                if(f_manager->field_is_defined(*it, wpkg_control::control_file::field_essential_factory_t::canonicalized_name())
                && f_manager->get_field_boolean(*it, wpkg_control::control_file::field_essential_factory_t::canonicalized_name()))
                {
                    remove_package = false;
                }

                // prevent the removal if the package is required
                if(remove_package
                && f_manager->field_is_defined(*it, wpkg_control::control_file::field_priority_factory_t::canonicalized_name()))
                {
                    case_insensitive::case_insensitive_string priority(f_manager->get_field(*it, wpkg_control::control_file::field_priority_factory_t::canonicalized_name()));
                    if(priority == "required")
                    {
                        remove_package = false;
                    }
                }
                break;

            default:
                // at this point ignore all the other statuses
                break;

            }
            // if the status suggest we can remove the package, check the
            // selection to know whether it is "auto"
            if(remove_package)
            {
                remove_package = false;
                if(f_manager->field_is_defined(*it, wpkg_control::control_file::field_xselection_factory_t::canonicalized_name()))
                {
                    const wpkg_control::control_file::field_xselection_t::selection_t selection(wpkg_control::control_file::field_xselection_t::validate_selection(f_manager->get_field(*it, wpkg_control::control_file::field_xselection_factory_t::canonicalized_name())));
                    remove_package = selection == wpkg_control::control_file::field_xselection_t::selection_auto;
                }
                else if(f_manager->field_is_defined(*it, "X-Explicit"))
                {
                    // if X-Selection is not defined, fallback on the
                    // internal X-Explicit value which is set to No when
                    // a package was only ever installed implicitly
                    remove_package = !f_manager->get_field_boolean(*it, "X-Explicit");
                }
                // else -- in other cases, ignore that package
            }

            // still true? then we want to check whether it is still depended
            // on and if not actually remove this package; note that a package
            // with a status of config-files is never depended on!
            if(remove_package
            && status[*it] != wpkgar_manager::config_files)
            {
                for(wpkgar_manager::package_list_t::const_iterator jt(list.begin());
                                                                   remove_package && jt != list.end();
                                                                   ++jt)
                {
                    // skip ourselves
                    if(*it == *jt)
                    {
                        continue;
                    }
                    switch(status[*jt])
                    {
                    case wpkgar_manager::installed:
                    case wpkgar_manager::unpacked:
                    case wpkgar_manager::half_installed:
                    case wpkgar_manager::half_configured:
                        // half-installed and half-configured are not really
                        // installed but their dependencies should not be
                        // removed until they get repaired in some way
                        break;

                    default:
                        // packages that are not installed do not need any
                        // dependencies so we can ignore them
                        continue;

                    }

                    // check all the dependency fields
                    for(depend_field_names_t::const_iterator nt(depend_names.begin());
                                                             remove_package && nt != depend_names.end();
                                                             ++nt)
                    {
                        // field is not even defined?
                        if(!f_manager->field_is_defined(*jt, *nt))
                        {
                            continue;
                        }

                        // check the actual dependencies
                        wpkg_dependencies::dependencies depends(f_manager->get_field(*jt, *nt));
                        for(int k(0); k < depends.size(); ++k)
                        {
                            const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(k));
                            if(d.f_name == *it)
                            {
                                // someone still depends on us, we cannot be
                                // removed
                                remove_package = false;
                                break;
                            }
                        }
                    }
                }
            }

            // no package depend on this one, remove it!
            if(remove_package)
            {
                repeat = true;

                wpkg_output::log("auto-removing package %1")
                        .quoted_arg(*it)
                    .level(wpkg_output::level_info)
                    .module(wpkg_output::module_remove_package)
                    .package("wpkg")
                    .action("remove-delete");

                if(!dryrun)
                {
                    wpkgar_remove pkg_remove(f_manager);
                    if(f_purging_packages)
                    {
                        pkg_remove.set_purging();
                    }
                    pkg_remove.add_package(*it);
                    if(pkg_remove.validate())
                    {
                        int i(pkg_remove.remove());
                        if(i >= 0 && f_purging_packages)
                        {
                            pkg_remove.deconfigure(i);
                            // ignore errors here, but we'll stop very soon
                            // if one occured (the repeat is ignored in that
                            // case)
                        }
                    }
                }

                // completely ignore this package in the following rounds
                // if no errors occured (there won't be any other round
                // when errors were detected)
                if(wpkg_output::get_output_error_count() == 0)
                {
                    status[*it] = wpkgar_manager::not_installed;
                }
            }
        }
    }
}



bool wpkgar_remove::deconfigure_package(package_item_t *item)
{
    // determine what the function should really do
    bool unpacked(item->get_original_status() == wpkgar_manager::installed);

    // the difference between deconfigure and purge is that in case of
    // the purge the package files were already removed
    const std::string command(unpacked ? "deconfigure" : "purge");

    // we have to run the prerm scripts (not part of dpkg it seems)
    if(!prerm_scripts(item, command))
    {
        return false;
    }

    f_manager->set_field(item->get_name(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Half-Configured", true);
    f_manager->set_field(item->get_name(), "X-Deconfigure-Date", wpkg_util::rfc2822_date(), true);

    // get the list of configuration files
    std::vector<std::string> files;
    f_manager->conffiles(item->get_filename(), files);

    const wpkg_filename::uri_filename root(f_manager->get_inst_path());
    for(std::vector<std::string>::iterator it(files.begin());
                                           it != files.end();
                                           ++it)
    {
        const wpkg_filename::uri_filename confname(root.append_child(*it));
        if(unpacked)
        {
            // in this case we want to save the user file instead
            const wpkg_filename::uri_filename user(confname.append_path(".wpkg-user"));
            user.os_unlink();

            // check whether it exists because if not os_rename() fails
            // (and the user has had the right to delete that file!)
            if(confname.exists())
            {
                confname.os_rename(user, false);
            }
            else
            {
                wpkg_output::log("no configuration file %1, it probably was deleted.")
                        .quoted_arg(confname)
                    .module(wpkg_output::module_deconfigure_package)
                    .package(*it)
                    .action("remove-deconfigure");
            }
        }
        else
        {
            confname.os_unlink();
            confname.append_path(".wpkg-new").os_unlink();
            confname.append_path(".wpkg-old").os_unlink();
        }
    }

    // mark the package as unpacked or not installed!
    // (XXX what is the new status when starting from Half-Installed or Half-Configured)
    const std::string final_status(unpacked ? "Unpacked" : "Not-Installed");
    f_manager->set_field(item->get_name(), wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), final_status, true);

    // old-postrm deconfigure (still unpacked)  or  old-postrm purge (not installed)
    wpkgar_manager::script_parameters_t params;
    params.push_back(command);
    if(!f_manager->run_script(item->get_filename(), wpkgar_manager::wpkgar_script_postrm, params))
    {
        wpkg_output::log("the postrm script failed for package %1 while deconfiguring.")
                .quoted_arg(item->get_name())
            .level(wpkg_output::level_warning)
            .module(wpkg_output::module_remove_package)
            .action("remove-deconfigure");
    }

    // hooks-postrm purge|deconfigure <package-name> <version>
    wpkgar_manager::script_parameters_t hook_params;
    hook_params.push_back(command);
    hook_params.push_back(item->get_name());
    hook_params.push_back(item->get_version());
    if(!f_manager->run_script("core", wpkgar_manager::wpkgar_script_postrm, hook_params))
    {
        wpkg_output::log("a postrm global validation hook failed for package %1 while deconfiguring.")
                .quoted_arg(item->get_name())
            .level(wpkg_output::level_warning)
            .module(wpkg_output::module_remove_package)
            .action("remove-deconfigure");
    }

    return true;
}


/** \brief Deconfigure the specified package.
 *
 * This function deletes all the configuration files of the specified
 * package.
 *
 *   - Run the "prerm deconfigure" script
 *   - Delete the configuration files
 *
 * Note that only packages that were marked for explicit removal
 * or implicit removal are affected. Also the package must already
 * have been marked for removal.
 *
 * \param[in] idx  The index as returned by remove(), if you're not using
 * remove() then use an index from zero to count() - 1.
 */
bool wpkgar_remove::deconfigure(int idx)
{
    // the caller is responsible for locking the database
    if(!f_manager->was_locked())
    {
        throw std::logic_error("the manager must be locked before calling wpkgar_remove::deconfigure()");
    }

    if(idx < 0 || static_cast<wpkgar_package_list_t::size_type>(idx) >= f_packages.size())
    {
        throw std::range_error("index out of range in wpkgar_remove::deconfigure()");
    }

    switch(f_packages[idx].get_type())
    {
    case package_item_t::package_type_removing:
    case package_item_t::package_type_implicit:
        if(!f_deconfiguring_packages && !f_packages[idx].is_removed())
        {
            throw std::logic_error("somehow the wpkgar_remove::deconfigure() function was called on package \"" + f_packages[idx].get_name() + "\" that is not yet removed.");
        }
        break;

    case package_item_t::package_type_same:
        // this happens on --deconfigure, simply ignore those packages
        // (they are ignored in the remove() call so a standard --remove
        // or --purge will not call the deconfigure function when a
        // package has this type.)
        return true;

    case package_item_t::package_type_installed:
    case package_item_t::package_type_unpacked:
    case package_item_t::package_type_configured:
        // these are packages that were part of the implicit list in
        // case --recursive was used and these were dependencies of
        // the packages being removed or deconfigured
        return true;

    default:
        throw std::logic_error("the wpkgar_remove::deconfigure() function cannot be called with an index representing a package other than 'removing' or 'implicit'.");

    }

    std::string package_name(f_packages[idx].get_name());
    wpkg_output::log("deconfiguring %1")
            .quoted_arg(package_name)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_deconfigure_package)
        .package(f_packages[idx].get_name());

    if(f_deconfiguring_packages)
    {
        // if the user is just deconfiguring and thus we need
        // a re-configure command; otherwise the remove command
        // is "install" so we won't need this entry (which would
        // appear in the wrong order anyway!)
        f_manager->track("configure " + package_name, package_name);
    }

    return deconfigure_package(&f_packages[idx]);
}



}
// vim: ts=4 sw=4 et
