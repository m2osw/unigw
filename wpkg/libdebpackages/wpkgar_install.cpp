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
#include    "libdebpackages/installer/details/disk.h"
#include    <algorithm>
#include    <set>
#include    <fstream>
#include    <iostream>
#include    <sstream>
#include    <stdarg.h>
#include    <errno.h>
#include    <time.h>
#include    <cstdlib>
#if defined(MO_CYGWIN)
#   include    <Windows.h>
#endif


namespace wpkgar
{

using namespace installer;


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



wpkgar_install::wpkgar_install( wpkgar_manager::pointer_t manager )
    : f_manager(manager)
    //, f_flags()                -- auto-init
    //, f_package_list()         -- auto-init
    //, f_dependencies()         -- auto-init
    //, f_architecture("")       -- auto-init
    //, f_original_status()      -- auto-init
    //, f_sorted_packages()      -- auto-init
    //, f_task()                 -- auto-init
    //, f_tree_max_depth(0)      -- auto-init
    //, f_install_source(false)  -- auto-init
    //, f_field_validations()    -- auto-init
{
    f_flags.reset( new flags );
    f_package_list.reset( new package_list( f_manager ) );
    f_task.reset( new task( task::task_installing_packages ) );
    f_dependencies.reset( new dependencies( f_manager, f_package_list, f_flags, f_task ) );
}


void wpkgar_install::set_installing()
{
    f_task->set_task( task::task_installing_packages );
}


void wpkgar_install::set_configuring()
{
    f_task->set_task( task::task_configuring_packages );
}


void wpkgar_install::set_reconfiguring()
{
    f_task->set_task( task::task_reconfiguring_packages );
}


void wpkgar_install::set_unpacking()
{
    f_task->set_task( task::task_unpacking_packages );
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


void wpkgar_install::validate_directory( package_item_t package )
{
    // if we cannot access that file, it's probably a direct package
    // name in which case we're done here (another error should occur
    // for those since it's illegal)
    const wpkg_filename::uri_filename filename(package.get_filename());
    if(filename.is_dir())
    {
        // this is a directory, so mark it as such
        package.set_type(package_item_t::package_type_directory);

        // read the directory *.deb files
        memfile::memory_file r;
        r.dir_rewind(filename, f_flags->get_parameter(flags::param_recursive, false) != 0);
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
            f_package_list->add_package(package_filename);
        }
    }
}


bool wpkgar_install::validate_packages_to_install()
{
    // this can happen if the user specify an empty directory as input
    int size(0);
    auto& packages( f_package_list->get_package_list() );
    for( const auto& pkg : packages )
    {
        if(pkg.get_type() == package_item_t::package_type_explicit
        || pkg.get_type() == package_item_t::package_type_implicit)
        {
            // we don't need to know how many total, just that there is at
            // least one so we break immediately
            ++size;
            break;
        }
    }
    if( size == 0 )
    {
        wpkg_output::log("the directories you specified do not include any valid *.deb files, did you forget --recursive?")
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_validate_installation)
            .action("install-validation");
        return false;
    }
    return true;
}


// transform the directories in a list of .deb packages
bool wpkgar_install::validate_directories()
{
    // if not installing (--configure, --reconfigure) then there is nothing to test here
    if(*f_task != task::task_installing_packages)
    {
        // in this case all the package names must match installed packages
        return true;
    }

    auto& packages( f_package_list->get_package_list() );

    progress_scope s( &f_progress_stack, "validate_directories", packages.size() * 2 );

    std::for_each( packages.begin(), packages.end(),
            [this]( package_item_t pkg )
            {
                f_manager->check_interrupt();
                f_progress_stack.increment_progress();
                this->validate_directory(pkg);
            }
        );

    return validate_packages_to_install();;
}


void wpkgar_install::validate_package_name( package_item_t& pkg )
{
    if(!pkg.get_filename().is_deb())
    {
        // this is a full package name (a .deb file)
        if(*f_task != task::task_installing_packages)
        {
            wpkg_output::log("package %1 cannot be used with --configure or --reconfigure.")
                .quoted_arg(pkg.get_filename())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(pkg.get_filename())
                .action("install-validation");
        }
        return;
    }

    // this is an install name
    switch( f_task->get_task() )
    {
        case task::task_installing_packages:
            {
                wpkg_output::log("package %1 cannot be used with --install, --unpack, or --check-install.")
                    .quoted_arg(pkg.get_filename())
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(pkg.get_filename())
                    .action("install-validation");
            }
            break;

        case task::task_reconfiguring_packages:
            {
                pkg.load(false);
                switch(pkg.get_original_status())
                {
                    case wpkgar_manager::not_installed:
                        wpkg_output::log("package %1 cannot be reconfigured since pkg. not currently installed.")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::config_files:
                        wpkg_output::log("package %1 was removed. Its configuration files are still available but the package cannot be reconfigured.")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::installed:
                        // perfect -- the type remains explicit
                        {
                            auto selection( f_dependencies->get_xselection( pkg.get_filename() ) );
                            if(selection == wpkg_control::control_file::field_xselection_t::selection_hold)
                            {
                                if(f_flags->get_parameter(flags::param_force_hold, false))
                                {
                                    wpkg_output::log("package %1 is on hold, yet it will be reconfigured.")
                                        .quoted_arg(pkg.get_filename())
                                        .level(wpkg_output::level_warning)
                                        .module(wpkg_output::module_validate_installation)
                                        .package(pkg.get_filename())
                                        .action("install-validation");
                                }
                                else
                                {
                                    wpkg_output::log("package %1 is on hold, it cannot be reconfigured.")
                                        .quoted_arg(pkg.get_filename())
                                        .level(wpkg_output::level_error)
                                        .module(wpkg_output::module_validate_installation)
                                        .package(pkg.get_filename())
                                        .action("install-validation");
                                }
                            }
                        }
                        break;

                    case wpkgar_manager::unpacked:
                        wpkg_output::log("package %1 is not configured yet, it cannot be reconfigured.")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::no_package:
                        wpkg_output::log("package %1 cannot be configured in its current state.")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::unknown:
                        wpkg_output::log("package %1 has an unexpected status of \"unknown\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::half_installed:
                        wpkg_output::log("package %1 has an unexpected status of \"half-installed\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::installing:
                        wpkg_output::log("package %1 has an unexpected status of \"installing\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::upgrading:
                        wpkg_output::log("package %1 has an unexpected status of \"upgrading\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::half_configured:
                        wpkg_output::log("package %1 has an unexpected status of \"half-configured\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::removing:
                        wpkg_output::log("package %1 has an unexpected status of \"removing\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::purging:
                        wpkg_output::log("package %1 has an unexpected status of \"purging\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::listing:
                        wpkg_output::log("package %1 has an unexpected status of \"listing\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::verifying:
                        wpkg_output::log("package %1 has an unexpected status of \"verifying\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::ready:
                        wpkg_output::log("package %1 has an unexpected status of \"ready\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;
                }
            }
            break;

        case task::task_configuring_packages:
            {
                pkg.load(false);
                switch(pkg.get_original_status())
                {
                    case wpkgar_manager::not_installed:
                        wpkg_output::log("package %1 cannot be configured since it is not currently unpacked.")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::config_files:
                        wpkg_output::log("package %1 was removed. Its configuration files are still available but the package cannot be configured.")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::installed:
                        // accepted although there is nothing to do against already installed packages
                        wpkg_output::log("package %1 is already installed --configure will have no effect.")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_warning)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        pkg.set_type(package_item_t::package_type_same);
                        break;

                    case wpkgar_manager::unpacked:
                        {
                            auto selection( f_dependencies->get_xselection( pkg.get_filename() ) );
                            if(selection == wpkg_control::control_file::field_xselection_t::selection_hold)
                            {
                                if(f_flags->get_parameter(flags::param_force_hold, false))
                                {
                                    wpkg_output::log("package %1 is on hold, yet it will be configured.")
                                        .quoted_arg(pkg.get_filename())
                                        .level(wpkg_output::level_warning)
                                        .module(wpkg_output::module_validate_installation)
                                        .package(pkg.get_filename())
                                        .action("install-validation");
                                }
                                else
                                {
                                    wpkg_output::log("package %1 is on hold, it cannot be configured.")
                                        .quoted_arg(pkg.get_filename())
                                        .level(wpkg_output::level_error)
                                        .module(wpkg_output::module_validate_installation)
                                        .package(pkg.get_filename())
                                        .action("install-validation");
                                }
                            }
                        }
                        pkg.set_type(package_item_t::package_type_unpacked);
                        break;

                    case wpkgar_manager::no_package:
                        wpkg_output::log("package %1 cannot be configured in its current state.")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::unknown:
                        wpkg_output::log("package %1 has an unexpected status of \"unknown\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::half_installed:
                        wpkg_output::log("package %1 has an unexpected status of \"half-installed\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::installing:
                        wpkg_output::log("package %1 has an unexpected status of \"installing\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::upgrading:
                        wpkg_output::log("package %1 has an unexpected status of \"upgrading\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::half_configured:
                        wpkg_output::log("package %1 has an unexpected status of \"half-configured\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::removing:
                        wpkg_output::log("package %1 has an unexpected status of \"removing\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::purging:
                        wpkg_output::log("package %1 has an unexpected status of \"purging\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::listing:
                        wpkg_output::log("package %1 has an unexpected status of \"listing\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::verifying:
                        wpkg_output::log("package %1 has an unexpected status of \"verifying\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                    case wpkgar_manager::ready:
                        wpkg_output::log("package %1 has an unexpected status of \"ready\".")
                            .quoted_arg(pkg.get_filename())
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_filename())
                            .action("install-validation");
                        break;

                }
            }
            break;

        case task::task_unpacking_packages:
            // Nothing to do
            break;
    }
}


// configuring: only already installed package names
// installing, unpacking, checking an install: only new package names
void wpkgar_install::validate_package_names()
{
    auto& packages( f_package_list->get_package_list() );

    progress_scope s( &f_progress_stack, "validate_package_names", packages.size() * 2 );

    for( auto& pkg : packages )
    {
        f_manager->check_interrupt();
        f_progress_stack.increment_progress();

        validate_package_name( pkg );
    }
}


void wpkgar_install::installing_source()
{
    auto& packages( f_package_list->get_package_list() );
    progress_scope s( &f_progress_stack, "installing_source", packages.size() );

    f_install_source = false;

    // if not installing (--configure, --reconfigure) then there is nothing to test here
    if(*f_task == task::task_installing_packages)
    {
        for( auto& pkg : packages )
        {
            f_manager->check_interrupt();
            f_progress_stack.increment_progress();

            const std::string architecture(pkg.get_architecture());
            if(architecture == "src" || architecture == "source")
            {
                f_install_source = true;
                break;
            }
        }
    }
}


void wpkgar_install::validate_installed_package( const std::string& pkg_name )
{
    auto& packages( f_package_list->get_package_list() );

    // this package is an installed package so we cannot
    // just load a control file from an index file; plus
    // at this point we do not know whether it will end
    // up in the packages vector
    f_manager->load_package(pkg_name);
    package_item_t::package_type_t type(package_item_t::package_type_invalid);
    switch(f_manager->package_status(pkg_name))
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
                .quoted_arg(pkg_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(pkg_name)
                .action("install-validation");
            break;

        case wpkgar_manager::unknown:
            wpkg_output::log("package %1 has an unexpected status of \"unknown\".")
                .quoted_arg(pkg_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(pkg_name)
                .action("install-validation");
            break;

        case wpkgar_manager::half_installed:
            wpkg_output::log("package %1 has an unexpected status of \"half-installed\".")
                .quoted_arg(pkg_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(pkg_name)
                .action("install-validation");
            break;

        case wpkgar_manager::installing:
            wpkg_output::log("package %1 has an unexpected status of \"installing\".")
                .quoted_arg(pkg_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(pkg_name)
                .action("install-validation");
            break;

        case wpkgar_manager::upgrading:
            wpkg_output::log("package %1 has an unexpected status of \"upgrading\".")
                .quoted_arg(pkg_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(pkg_name)
                .action("install-validation");
            break;

        case wpkgar_manager::half_configured:
            wpkg_output::log("package %1 has an unexpected status of \"half-configured\".")
                .quoted_arg(pkg_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(pkg_name)
                .action("install-validation");
            break;

        case wpkgar_manager::removing:
            wpkg_output::log("package %1 has an unexpected status of \"removing\".")
                .quoted_arg(pkg_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(pkg_name)
                .action("install-validation");
            break;

        case wpkgar_manager::purging:
            wpkg_output::log("package %1 has an unexpected status of \"purging\".")
                .quoted_arg(pkg_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(pkg_name)
                .action("install-validation");
            break;

        case wpkgar_manager::listing:
            wpkg_output::log("package %1 has an unexpected status of \"listing\".")
                .quoted_arg(pkg_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(pkg_name)
                .action("install-validation");
            break;

        case wpkgar_manager::verifying:
            wpkg_output::log("package %1 has an unexpected status of \"verifying\".")
                .quoted_arg(pkg_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(pkg_name)
                .action("install-validation");
            break;

        case wpkgar_manager::ready:
            wpkg_output::log("package %1 has an unexpected status of \"ready\".")
                .quoted_arg(pkg_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(pkg_name)
                .action("install-validation");
            break;

    }

    // note: *f_task == task::task_installing_packages is true if you are installing or
    //       unpacking
    if(*f_task == task::task_installing_packages)
    {
        if(type == package_item_t::package_type_not_installed)
        {
            // user may be attempting to install this package, make
            // sure it is not marked as a "Reject" (X-Selection)
            if(f_manager->field_is_defined(pkg_name, wpkg_control::control_file::field_xselection_factory_t::canonicalized_name()))
            {
                auto selection( f_dependencies->get_xselection( pkg_name ) );
                auto item(f_package_list->find_package_item_by_name(pkg_name));
                if(item != packages.end()
                        && item->get_type() == package_item_t::package_type_explicit
                        && selection == wpkg_control::control_file::field_xselection_t::selection_reject)
                {
                    wpkg_output::log("package %1 is marked as rejected; use --set-selection to change its status first.")
                        .quoted_arg(pkg_name)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(pkg_name)
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

            // IMPORTANT: note that pkg_name is a name (Package field), not a path, in this case
            auto item(f_package_list->find_package_item_by_name(pkg_name));
            if(item != packages.end())
            {
                // the user is doing an update, an overwrite, or a downgrade
                // it must be from an explicit package; note that implicit
                // packages are not yet defined here
                if(item->get_type() != package_item_t::package_type_explicit)
                {
                    // at this point the existing items MUST be explicit or
                    // something is really wrong
                    wpkg_output::log("package %1 found twice in the existing installation.")
                        .quoted_arg(pkg_name)
                        .level(wpkg_output::level_fatal)
                        .module(wpkg_output::module_validate_installation)
                        .package(pkg_name)
                        .action("install-validation");
                }
                if(*f_task != task::task_unpacking_packages)
                {
                    // with --install we cannot upgrade a package that was just unpacked.
                    if(type == package_item_t::package_type_unpacked)
                    {
                        // you cannot update/upgrade an unpacked package with --install, it needs configuration
                        if(f_flags->get_parameter(flags::param_force_configure_any, false))
                        {
                            wpkg_output::log("package %1 is unpacked, it will be configured before getting upgraded.")
                                .quoted_arg(pkg_name)
                                .level(wpkg_output::level_warning)
                                .module(wpkg_output::module_validate_installation)
                                .package(pkg_name)
                                .action("install-validation");
                            item->set_type(package_item_t::package_type_configure);
                            // we do not change the package 'type' on purpose
                            // it will be checked again in the if() below
                        }
                        else
                        {
                            wpkg_output::log("package %1 is unpacked, it cannot be updated with --install. Try --configure first, or use --unpack.")
                                .quoted_arg(pkg_name)
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_validate_installation)
                                .package(pkg_name)
                                .action("install-validation");
                        }
                    }
                }
                if(item->get_type() == package_item_t::package_type_explicit)
                {
                    // Note: using f_manager directly since the package is not
                    //       yet in the packages vector
                    auto selection( f_dependencies->get_xselection( pkg_name ) );
                    std::string vi(f_manager->get_field(pkg_name, wpkg_control::control_file::field_version_factory_t::canonicalized_name()));
                    std::string vo(item->get_version());
                    const int c(wpkg_util::versioncmp(vi, vo));
                    if(c == 0)
                    {
                        if(f_flags->get_parameter(flags::param_skip_same_version, false))
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
                            if(f_flags->get_parameter(flags::param_force_hold, false))
                            {
                                wpkg_output::log("package %1 is being upgraded even though it is on hold.")
                                    .quoted_arg(pkg_name)
                                    .level(wpkg_output::level_warning)
                                    .module(wpkg_output::module_validate_installation)
                                    .package(pkg_name)
                                    .action("install-validation");
                            }
                            else
                            {
                                wpkg_output::log("package %1 is not getting upgraded because it is on hold.")
                                    .quoted_arg(pkg_name)
                                    .level(wpkg_output::level_error)
                                    .module(wpkg_output::module_validate_installation)
                                    .package(pkg_name)
                                    .action("install-validation");
                            }
                        }

                        if(item->field_is_defined(wpkg_control::control_file::field_minimumupgradableversion_factory_t::canonicalized_name()))
                        {
                            const std::string minimum_version(item->get_field(wpkg_control::control_file::field_minimumupgradableversion_factory_t::canonicalized_name()));
                            const int m(wpkg_util::versioncmp(vi, minimum_version));
                            if(m < 0)
                            {
                                if(f_flags->get_parameter(flags::param_force_upgrade_any_version, false))
                                {
                                    wpkg_output::log("package %1 version %2 is being upgraded even though the Minimum-Upgradable-Version says it won't work right since it was not upgraded to at least version %3 first.")
                                        .quoted_arg(pkg_name)
                                        .arg(vi)
                                        .arg(minimum_version)
                                        .level(wpkg_output::level_warning)
                                        .module(wpkg_output::module_validate_installation)
                                        .package(pkg_name)
                                        .action("install-validation");
                                }
                                else
                                {
                                    wpkg_output::log("package %1 version %2 is not getting upgraded because the Minimum-Upgradable-Version says it won't work right without first upgrading it to at least version %3.")
                                        .quoted_arg(pkg_name)
                                        .arg(vi)
                                        .arg(minimum_version)
                                        .level(wpkg_output::level_error)
                                        .module(wpkg_output::module_validate_installation)
                                        .package(pkg_name)
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
                        if(f_flags->get_parameter(flags::param_force_downgrade, false))
                        {
                            if(selection == wpkg_control::control_file::field_xselection_t::selection_hold)
                            {
                                if(f_flags->get_parameter(flags::param_force_hold, false))
                                {
                                    wpkg_output::log("package %1 is being downgraded even though it is on hold.")
                                        .quoted_arg(pkg_name)
                                        .level(wpkg_output::level_warning)
                                        .module(wpkg_output::module_validate_installation)
                                        .package(pkg_name)
                                        .action("install-validation");
                                }
                                else
                                {
                                    wpkg_output::log("package %1 is not getting downgraded because it is on hold.")
                                        .quoted_arg(pkg_name)
                                        .level(wpkg_output::level_error)
                                        .module(wpkg_output::module_validate_installation)
                                        .package(pkg_name)
                                        .action("install-validation");
                                }
                            }

                            // at this time it's just a warning but a dependency
                            // version may break because of this
                            wpkg_output::log("package %1 is being downgraded which may cause some dependency issues.")
                                .quoted_arg(pkg_name)
                                .level(wpkg_output::level_warning)
                                .module(wpkg_output::module_validate_installation)
                                .package(pkg_name)
                                .action("install-validation");
                            // unexpected downgrade
                            type = package_item_t::package_type_downgrade;
                        }
                        else
                        {
                            wpkg_output::log("package %1 cannot be downgraded.")
                                .quoted_arg(pkg_name)
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_validate_installation)
                                .package(pkg_name)
                                .action("install-validation");
                        }
                    }
                }
            }
            // add the result, but only if installing or unpacking
            // (i.e. in most cases this indicates an installed package)
            package_item_t package_item(f_manager, pkg_name, type);
            packages.push_back(package_item);
        }
    }
}


void wpkgar_install::validate_installed_packages()
{
    const auto& installed_packages( f_package_list->get_installed_package_list() );
    progress_scope s( &f_progress_stack, "validate_installed_packages", installed_packages.size() );

    // read the names of all the installed packages
    for( const auto& pkg_name : installed_packages )
    {
        f_manager->check_interrupt();
        f_progress_stack.increment_progress();

        try
        {
            validate_installed_package( pkg_name );
        }
        catch(const std::exception& e)
        {
            wpkg_output::log("installed package %1 could not be loaded (%2).")
                    .quoted_arg(pkg_name)
                    .arg(e.what())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(pkg_name)
                .action("install-validation");
        }
    }
}


void wpkgar_install::validate_distribution_package( const package_item_t& package )
{
    const std::string distribution(f_manager->get_field("core", "Distribution"));
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
                        if(f_flags->get_parameter(flags::param_force_distribution, false))
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
                        if(f_flags->get_parameter(flags::param_force_distribution, false))
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


/** \brief Validate the distribution field.
 *
 * This function checks whether a distribution field is defined in the
 * core package (i.e. the database control settings.) If so, then the
 * name of that field has to match all the packages that are about
 * to be installed implicitly or explicitly.
 */
void wpkgar_install::validate_distribution()
{
    auto& packages( f_package_list->get_package_list() );
    progress_scope s( &f_progress_stack, "validate_distribution", packages.size() );

    // if the Distribution field is not defined for that target
    // then we're done here
    if(!f_manager->field_is_defined("core", "Distribution"))
    {
        return;
    }

    for( const auto& package : packages )
    {
        f_manager->check_interrupt();
        f_progress_stack.increment_progress();
        validate_distribution_package( package );
    }
}


void wpkgar_install::validate_architecture_package( package_item_t& pkg )
{
    if(pkg.get_type() == package_item_t::package_type_explicit)
    {
        // match the architecture
        const std::string arch(pkg.get_architecture());
        // all and source architectures can always be installed
        if(arch != "all" && arch != "src" && arch != "source"
                && !wpkg_dependencies::dependencies::match_architectures(arch, f_architecture, f_flags->get_parameter(flags::param_force_vendor, false) != 0))
        {
            const wpkg_filename::uri_filename filename(pkg.get_filename());
            if(!f_flags->get_parameter(flags::param_force_architecture, false))
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


void wpkgar_install::validate_architecture()
{
    auto& packages( f_package_list->get_package_list() );
    progress_scope s( &f_progress_stack, "validate_architecture", packages.size() );

    for( auto& pkg : packages )
    {
        f_manager->check_interrupt();
        f_progress_stack.increment_progress();
        validate_architecture_package( pkg );
    }
}


void wpkgar_install::validate_packager_version()
{
    auto& packages( f_package_list->get_package_list() );
    progress_scope s( &f_progress_stack, "validate_packager_version", packages.size() );

    // note: at this point we have one valid tree to be installed

    // already installed packages are ignore here
    for( auto& pkg : packages )
    {
        f_progress_stack.increment_progress();

        switch(pkg.get_type())
        {
        case package_item_t::package_type_explicit:
        case package_item_t::package_type_implicit:
            {
                // full path to package
                const wpkg_filename::uri_filename filename(pkg.get_filename());

                // get list of dependencies if any
                if(pkg.field_is_defined(wpkg_control::control_file::field_packagerversion_factory_t::canonicalized_name()))
                {
                    const std::string build_version(pkg.get_field(wpkg_control::control_file::field_packagerversion_factory_t::canonicalized_name()));
                    int c(wpkg_util::versioncmp(debian_packages_version_string(), build_version));
                    // our version is expected to be larger or equal in which case
                    // we're good; if we're smaller, then we may not be 100%
                    // compatible (and in some cases, 0%... which will be caught
                    // here once we have such really bad cases.)
                    if(c < 0)
                    {
                        wpkg_output::log("package %1 was build with packager v%2 which may not be 100%% compatible with this packager v%3.")
                                .quoted_arg(pkg.get_name())
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
                            .quoted_arg(pkg.get_name())
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
    installer::details::disk_list_t     disks( f_manager, f_package_list, f_flags );
#endif

    auto& packages( f_package_list->get_package_list() );
    progress_scope s( &f_progress_stack, "validate_installed_size_and_overwrite", packages.size() );

    const wpkg_filename::uri_filename root(f_manager->get_inst_path());
    controlled_vars::zuint32_t total;
    int32_t idx = -1;
    for( auto& outer_pkg : packages )
    {
        f_progress_stack.increment_progress();

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
                for( auto& inner_pkg : packages )
                {
                    ++j;
                    switch(inner_pkg.get_type())
                    {
                    case package_item_t::package_type_upgrade:
                    case package_item_t::package_type_downgrade:
                        if(name == inner_pkg.get_name())
                        {
                            f_manager->check_interrupt();

                            wpkg_output::log("loading package \"" + name + "\" and determining if needs upgrading." )
                                .level(wpkg_output::level_info)
                                .debug(wpkg_output::debug_flags::debug_progress)
                                .module(wpkg_output::module_validate_installation);

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
        if(factor != 0 && (*f_task == task::task_installing_packages || *f_task == task::task_unpacking_packages))
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
    // for( auto& outer_pkg : packages )

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

    auto& packages( f_package_list->get_package_list() );
    for( auto& pkg : packages )
    {
        switch(pkg.get_type())
        {
        case package_item_t::package_type_explicit:
        case package_item_t::package_type_implicit:
            // we want the corresponding upgrade (downgrade) package
            // because we use that for our overwrite test
            for( const auto& fld : f_field_validations )
            {
                f_manager->check_interrupt();

                const std::string& name(pkg.get_name());
                if(!pkg.validate_fields(fld))
                {
                    wpkg_output::log("package %1 did not validate against %2.")
                            .quoted_arg(name)
                            .quoted_arg(fld)
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

    const auto& field_names( f_dependencies->get_field_names() );
    auto& packages( f_package_list->get_package_list() );
    for( wpkgar_package_list_t::size_type idx(0); idx < packages.size(); ++idx )
    {
        auto& pkg( packages[idx] );
        if(pkg.get_name() == name)
        {
            f_manager->check_interrupt();

            switch(pkg.get_type())
            {
            case package_item_t::package_type_explicit:
            case package_item_t::package_type_implicit:
                // check dependencies because they need to be added first
                for( const auto& field_name : field_names )
                {
                    if(pkg.field_is_defined(field_name))
                    {
                        wpkg_dependencies::dependencies depends(pkg.get_field(field_name));
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
    auto& packages( f_package_list->get_package_list() );
    progress_scope s( &f_progress_stack, "sort_packages", packages.size() );

    wpkgar_package_listed_t listed;

    for( auto& pkg : packages )
    {
        f_progress_stack.increment_progress();
        sort_package_dependencies(pkg.get_name(), listed);
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
    auto& packages( f_package_list->get_package_list() );
    progress_scope s( &f_progress_stack, "validate_scripts", packages.size() );

    // run the package validation script of the packages being installed
    // or upgraded and as we're at it generate the list of package names
    int errcnt(0);
    std::string package_names;
    for( auto& pkg : packages )
    {
        f_progress_stack.increment_progress();

        switch(pkg.get_type())
        {
        case package_item_t::package_type_explicit:
        case package_item_t::package_type_implicit:
            {
                package_names += pkg.get_filename().full_path() + " ";

                // new-validate install <new-version> [<old-version>]
                wpkgar_manager::script_parameters_t params;
                params.push_back("install");
                params.push_back(pkg.get_version());
                int32_t upgrade_idx(pkg.get_upgrade());
                if(upgrade_idx != -1)
                {
                    params.push_back(packages[upgrade_idx].get_version());
                }
                if(!f_manager->run_script(pkg.get_filename(), wpkgar_manager::wpkgar_script_validate, params))
                {
                    wpkg_output::log("the validate script of package %1 returned with an error, installation aborted.")
                            .quoted_arg(pkg.get_name())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(pkg.get_name())
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


int wpkgar_install::count() const
{
    return static_cast<int>( f_package_list->count() );
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
    progress_scope s( &f_progress_stack, "validate", 13 );

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
    f_dependencies->init_field_names();

    // installation architecture
    // (note that dpkg can be setup to support multiple architecture;
    // at this point we support just one.)
    //f_manager->load_package("core"); (already done when locked)
    f_architecture = f_manager->get_field("core", wpkg_control::control_file::field_architecture_factory_t::canonicalized_name());

    // some of the package names may be directory names, make sure we
    // know what's what and actually replace all the directory names
    // with their content so we don't have to know about those later
    // all of those are considered explicitly defined packages
    wpkg_output::log("validate directories")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    if(!validate_directories())
    {
        // the list of packages may end up being empty in which case we
        // just return since there is really nothing more we can do
        return false;
    }

    f_progress_stack.increment_progress();

//printf("list packages (nearly) at the start:\n");
//for(wpkgar_package_list_t::size_type idx(0); idx < packages.size(); ++idx)
//{
//printf("  %d - [%s]\n", (int)packages[idx].get_type(), packages[idx].get_filename().c_str());
//}

    // make sure package names correspond to the type of installation
    // (i.e. in --configure all the names must be installed packages, in
    // all other cases, it must not be.)
    wpkg_output::log("validate package name")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_package_names();
    f_progress_stack.increment_progress();

    // check whether some packages are source packages;
    wpkg_output::log("validate installation type (source/binary)")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    installing_source();
    f_progress_stack.increment_progress();
    if(f_install_source)
    {
        auto& field_names( f_dependencies->get_field_names() );
        // IMPORTANT NOTE:
        // I have a validation that checks whether binary fields include one
        // of those Build dependency fields; that validation is void when
        // none of the packages are source packages. So if that package is
        // never necessary to build any package source, it will never be
        // checked for such (but it is made valid by the --build command!)
        field_names.push_back(wpkg_control::control_file::field_builddepends_factory_t::canonicalized_name());
        field_names.push_back(wpkg_control::control_file::field_builddependsarch_factory_t::canonicalized_name());
        field_names.push_back(wpkg_control::control_file::field_builddependsindep_factory_t::canonicalized_name());
        field_names.push_back(wpkg_control::control_file::field_builtusing_factory_t::canonicalized_name());
    }

    // make sure that the currently installed packages are in the
    // right state for a new installation to occur
    wpkg_output::log("validate installed packages")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_installed_packages();
    f_progress_stack.increment_progress();

    // make sure that all the packages to be installed have the same
    // architecture as defined in the core package
    // (note: this is done before checking dependencies because it is
    // assumed that implicit packages are added only if their architecture
    // matches the core architecture, and of course already installed
    // packages have the right architecture.)
    wpkg_output::log("validate architecture")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_architecture();
    f_progress_stack.increment_progress();

    // if any Pre-Depends is not satisfied in the explicit packages then
    // the installation will fail (although we can go on with validations)
    wpkg_output::log("validate pre-dependencies")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    f_dependencies->validate_predependencies();
    f_progress_stack.increment_progress();

    // before we can check a complete list of what is going to be installed
    // we first need to make sure that this list is complete; this means we
    // need to determine whether all the dependencies are satisfied this
    // adds the dependencies to the list and at the end we have a long list
    // that includes all the packages we need to check further
    wpkg_output::log("validate dependencies")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    f_dependencies->validate_dependencies();
    f_progress_stack.increment_progress();

    // when marking a target with a specific distribution then only
    // packages with the same distribution informations should be
    // installed on that target; otherwise packages may not be 100%
    // compatible (i.e. incompatible compiler used to compile two
    // libraries running together...)
    wpkg_output::log("validate distribution name")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_distribution();
    f_progress_stack.increment_progress();

    // check that the packager used to create the explicit and implicit
    // packages was the same or an older version; if newer, we print out
    // a message in verbose mode; at this point this does not generate
    // error (it may later if incompatibilities are found)
    wpkg_output::log("validate packager version")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_packager_version();
    f_progress_stack.increment_progress();

    // check user defined C-like expressions against the control file
    // fields of all the packages being installed (implicitly or
    // explicitly)
    wpkg_output::log("validate fields")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);
    validate_fields();
    f_progress_stack.increment_progress();

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
            .level(wpkg_output::level_info)
            .debug(wpkg_output::debug_flags::debug_progress)
            .module(wpkg_output::module_validate_installation);
        validate_installed_size_and_overwrite();
        f_progress_stack.increment_progress();
    }

    if(wpkg_output::get_output_error_count() == 0)
    {
        // run user defined validation scripts found in the implicit and
        // explicit packages
        wpkg_output::log("validate hooks")
            .level(wpkg_output::level_info)
            .debug(wpkg_output::debug_flags::debug_progress)
            .module(wpkg_output::module_validate_installation);
        validate_scripts();
        f_progress_stack.increment_progress();
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
        f_progress_stack.increment_progress();
    }

//printf("list packages after deps:\n");
//for(wpkgar_package_list_t::size_type idx(0); idx < packages.size(); ++idx)
//{
//printf("  %d - [%s]\n", (int)packages[idx].get_type(), packages[idx].get_filename().c_str());
//}

    return wpkg_output::get_output_error_count() == 0;
}



/** \brief Get a shared pointer to the manager object.
 */
wpkgar_manager::pointer_t wpkgar_install::get_manager() const
{
    return f_manager;
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
install_info_list_t wpkgar_install::get_install_list() const
{
    install_info_list_t list;

    auto& packages( f_package_list->get_package_list() );
    for( const auto& pkg : packages )
    {
        switch(pkg.get_type())
        {
        case package_item_t::package_type_explicit:
        case package_item_t::package_type_implicit:
            {
                install_info_t info;
                info.f_name    = pkg.get_name();
                info.f_version = pkg.get_version();

                switch( pkg.get_type() )
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

                const int32_t upgrade_idx(pkg.get_upgrade());
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


flags::pointer_t wpkgar_install::get_flags() const
{
    return f_flags;
}


package_list::pointer_t wpkgar_install::get_package_list() const
{
    return f_package_list;
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
        auto& packages( f_package_list->get_package_list() );
        std::string old_version;
        for( auto& pkg : packages )
        {
            if(pkg.get_type() == package_item_t::package_type_not_installed
            && pkg.get_name() == item->get_name())
            {
                if(f_manager->package_status(pkg.get_filename()) == wpkgar_manager::config_files)
                {
                    // we found a package that is being re-installed
                    // (i.e. package configuration files exist,
                    // and it's not being upgraded)
                    old_version = pkg.get_version();
                    conf_install = &pkg;
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
    int file_info_err(f_flags->get_parameter(flags::param_force_file_info, false) ? memfile::memory_file::file_info_return_errors : memfile::memory_file::file_info_throw);

    // apply the file info
    memfile::memory_file::info_to_disk_file(destination, info, file_info_err);

    if(file_info_err & memfile::memory_file::file_info_permissions_error)
    {
        if(f_flags->get_parameter(flags::param_quiet_file_info, false) == 0)
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
        if(f_flags->get_parameter(flags::param_quiet_file_info, false) == 0)
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
    if(*f_task != task::task_reconfiguring_packages)
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
            if(*f_task == task::task_reconfiguring_packages)
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
                        if(is_config || *f_task != task::task_reconfiguring_packages)
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
                    if(*f_task != task::task_reconfiguring_packages)
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
                    if(*f_task != task::task_reconfiguring_packages)
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
                    if(*f_task != task::task_reconfiguring_packages)
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

    if( (*f_task != task::task_reconfiguring_packages) && (upgrade != NULL || conf_install != NULL))
    {
        item->copy_package_in_database();
    }

    set_status(item, upgrade, conf_install, "Unpacked");

    if(*f_task == task::task_reconfiguring_packages)
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
    auto& packages( f_package_list->get_package_list() );
    for( auto& pkg : packages )
    {
        if(pkg.get_type() == package_item_t::package_type_configure)
        {
            std::string package_name(pkg.get_name());
            wpkg_output::log("pre-configuring %1")
                        .quoted_arg(package_name)
                .level(wpkg_output::level_info)
                .debug(wpkg_output::debug_flags::debug_progress)
                .module(wpkg_output::module_validate_installation);

            f_manager->track("deconfigure " + package_name, package_name);
            if(!configure_package(&pkg))
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

    auto& packages( f_package_list->get_package_list() );

    for( auto idx : f_sorted_packages )
    {
        auto& package( packages[idx] );
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
                        .level(wpkg_output::level_info)
                        .debug(wpkg_output::debug_flags::debug_progress)
                        .module(wpkg_output::module_validate_installation);

                    package_item_t *upgrade(NULL);
                    const int32_t upgrade_idx(package.get_upgrade());
                    if(upgrade_idx != -1)
                    {
                        upgrade = &packages[upgrade_idx];

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
    for( auto& pkg : files )
    {
        const wpkg_filename::uri_filename confname(root.append_child(pkg));
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
            std::string confbasename(pkg.c_str());
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
                        .quoted_arg(pkg)
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
                            .quoted_arg(pkg)
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

    if(idx < 0 || static_cast<wpkgar_package_list_t::size_type>(idx) >= f_package_list->get_package_list().size())
    {
        throw std::range_error("index out of range in wpkgar_install::configure()");
    }

    auto& packages( f_package_list->get_package_list() );
    auto& pkg( packages[idx] );
    switch(pkg.get_type())
    {
    case package_item_t::package_type_explicit:
    case package_item_t::package_type_implicit:
        if(!pkg.is_unpacked())
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
            .quoted_arg(pkg.get_name())
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);

    return configure_package(&pkg);
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

    auto& packages( f_package_list->get_package_list() );

    int idx = 0;
    for( auto iter = packages.begin(); iter != packages.end(); ++iter, ++idx )
    {
        // the sort is probably not useful here
        wpkgar_package_list_t::size_type sorted_idx(f_sorted_packages[idx]);
        auto& pkg( *iter );
        if(!pkg.is_unpacked())
        {
            switch(pkg.get_type())
            {
            case package_item_t::package_type_explicit:
                wpkg_output::log("reconfiguring %1")
                        .quoted_arg(pkg.get_name())
                    .debug(wpkg_output::debug_flags::debug_progress)
                    .module(wpkg_output::module_validate_installation);

                if(!do_unpack(&pkg, &pkg))
                {
                    // an error occured, we cannot continue
                    return WPKGAR_ERROR;
                }

                return static_cast<int>(sorted_idx);

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
