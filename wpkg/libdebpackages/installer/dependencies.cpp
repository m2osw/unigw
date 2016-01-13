/*    dependencies.cpp -- install a debian package
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
#include    "libdebpackages/installer/dependencies.h"
#include    "libdebpackages/debian_version.h"
#include    "libdebpackages/debian_packages.h"
#include    "libdebpackages/wpkg_util.h"
#include    "libdebpackages/wpkgar_repository.h"
//#include    <algorithm>
//#include    <set>
//#include    <fstream>
//#include    <iostream>
#include    <sstream>
//#include    <stdarg.h>
//#include    <errno.h>
//#include    <time.h>
//#include    <cstdlib>
#if defined(MO_CYGWIN)
#   include    <Windows.h>
#endif


namespace wpkgar
{


namespace installer
{


/** \class dependencies
 * \brief The package validation manager.
 *
 * This class defines the functions necessary to validate packages
 * for installation. You may add one or more packages to the list of
 * packages to be marked for "explicit" installation.
 *
 * In most cases, you want to create a wpkgar_install object, then add one
 * or more packages to be installed. The dependencies object is used internally
 * by wpkgar_install.
 *
 * The purpose of this object is to allow unit testing of each function;
 * not only that, but wpkgar_install has gotten very big, so this refactor
 * helps reduce the size of the parent object, plus, as I've said above, allow
 * exposure of previously internal functions for unit testing.
 */


dependency_error::dependency_error( const std::string& what_msg )
    : runtime_error(what_msg)
{
}


dependencies::dependencies
    ( wpkgar_manager::pointer_t manager
    , package_list::pointer_t   list
    , flags::pointer_t          flags
    )
        : f_manager(manager)
        , f_package_list(list)
        , f_flags(flags)
        //, f_repository_packages_loaded(false) -- auto-init
        //, f_install_includes_choices(false)   -- auto-init
        //, f_tree_max_depth(0)                 -- auto-init
        //, f_field_names()                     -- auto-init
{
}



bool dependencies::get_install_includes_choices() const
{
    return f_install_includes_choices;
}


// if invalid, return -1
// if valid but not in range, return 0
// if valid and in range, return 1
int dependencies::match_dependency_version(const wpkg_dependencies::dependencies::dependency_t& d, const package_item_t& item)
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


namespace
{
    class find_exception : public std::runtime_error
    {
        public:
            find_exception(const std::string& what_msg) : runtime_error(what_msg) {}
    };
}


bool dependencies::find_installed_predependency_package( package_item_t& pkg, const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d)
{
    const wpkg_filename::uri_filename filename(pkg.get_filename());
    switch(pkg.get_type())
    {
        case package_item_t::package_type_installed:
        case package_item_t::package_type_unpacked:
            // the version check is required for both installed
            // and unpacked packages
            if(match_dependency_version(d, pkg) != 1)
            {
                if(!f_flags->get_parameter(flags::param_force_depends_version, false))
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
                    throw find_exception("incompatible version for pre-dependency");
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
            if(pkg.get_type() == package_item_t::package_type_installed)
            {
                return true;
            }

            // handle the package_item_t::package_type_unpacked case which
            // requires some additional tests
            if(f_flags->get_parameter(flags::param_force_configure_any, false))
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
                pkg.set_type(package_item_t::package_type_configure);
                return true;
            }
            if(f_flags->get_parameter(flags::param_force_depends, false))
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
            throw find_exception("pre-dependency still needs to be configured");

        default:
            wpkg_output::log("file %1 has a pre-dependency (%2) which is not in a valid state to continue our installation (it was removed or purged?)")
                .quoted_arg(filename)
                .quoted_arg(d.f_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_validate_installation)
                .package(filename)
                .action("install-validation");
            throw find_exception("pre-dependency not in valid state");
    }

    return false;
}


void dependencies::find_installed_predependency(const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d)
{
    // search for package d.f_name in the list of installed packages
    for( auto& pkg : f_package_list->get_package_list() )
    {
        if(d.f_name == pkg.get_name())
        {
            try
            {
                if( find_installed_predependency_package( pkg, package_name, d ) )
                {
                    return;
                }
            }
            catch( find_exception x )
            {
                throw dependency_error( x.what() );
            }
        }
    }

    // the file doesn't exist (is missing) but user may not care
    if(f_flags->get_parameter(flags::param_force_depends, false))
    {
        // user accepts broken dependencies...
        wpkg_output::log("package %1 has pre-dependency %2 which is not installed.")
                .quoted_arg(package_name)
                .quoted_arg(d.to_string())
            .level(wpkg_output::level_warning)
            .module(wpkg_output::module_validate_installation)
            .package(package_name)
            .action("install-validation");
        return;
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
    throw dependency_error( "uninstalled predependency!" );
}


void dependencies::validate_predependencies()
{
    progress_scope s( this, "validate_predependencies", f_package_list->get_package_list().size() );

    // note: at this point we have not read repositories yet

    // already installed packages must have already been loaded for this
    // validation function to work
    for( auto& pkg : f_package_list->get_package_list() )
    {
        f_manager->check_interrupt();
        increment_progress();

        if(pkg.get_type() == package_item_t::package_type_explicit)
        {
            // full path to package
            const wpkg_filename::uri_filename filename(pkg.get_filename());

            // get list of pre-dependencies if any
            if(pkg.field_is_defined(wpkg_control::control_file::field_predepends_factory_t::canonicalized_name()))
            {
                wpkg_dependencies::dependencies pre_depends(pkg.get_field(wpkg_control::control_file::field_predepends_factory_t::canonicalized_name()));
                for(int i(0); i < pre_depends.size(); ++i)
                {
                    const wpkg_dependencies::dependencies::dependency_t& d(pre_depends.get_dependency(i));
                    find_installed_predependency(filename, d);
                }
            }
        }
    }
}

bool dependencies::read_repository_index( const wpkg_filename::uri_filename& repo_filename, memfile::memory_file& index_file )
{
    // repository must include an index, if not and the repository
    // is a direct filename then we attempt to create the index now
    wpkg_filename::uri_filename index_filename(repo_filename.append_child("index.tar.gz"));
    memfile::memory_file compressed;
    if(index_filename.is_direct())
    {
        if(!index_filename.exists())
        {
            wpkg_output::log("Creating index file, since it does not exist in repository '%1'.")
                .quoted_arg(repo_filename)
                .debug(wpkg_output::debug_flags::debug_detail_config)
                .module(wpkg_output::module_validate_installation)
                .package(index_filename);

            // that's a direct filename but the index is missing,
            // create it on the spot
            wpkgar_repository repository(f_manager);
            // If the user wants a recursive repository index he will have to do it manually because --recursive is already
            // used for another purpose along the --install and I do not think that it is wise to do this here anyway
            //repository.set_parameter(wpkgar::wpkgar_repository::wpkgar_repository_recursive, f_flags->get_parameter(flags::param_recursive, false));
            repository.create_index(index_file);
            index_file.compress(compressed, memfile::memory_file::file_format_gz);
            compressed.write_file(index_filename);
        }
        else
        {
            wpkg_output::log("Reading index file from repository '%1'.")
                .quoted_arg(repo_filename)
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
                .quoted_arg(repo_filename)
                .debug(wpkg_output::debug_flags::debug_detail_config)
                .module(wpkg_output::module_validate_installation)
                .package(index_filename);

            compressed.read_file(index_filename);
            compressed.decompress(index_file);
        }
        catch(const memfile::memfile_exception&)
        {
            wpkg_output::log("skip remote repository %1 as it does not seem to include an index.tar.gz file.")
                .quoted_arg(repo_filename)
                .debug(wpkg_output::debug_flags::debug_detail_config)
                .module(wpkg_output::module_validate_installation)
                .package(index_filename);
            return false;
        }
    }

    return true;
}

void dependencies::read_repositories()
{
    // load the files once
    if(f_repository_packages_loaded)
    {
        return;
    }

    f_repository_packages_loaded = true;
    const auto& repositories(f_manager->get_repositories());
    progress_scope s( this, "repositories", repositories.size() );
    for( auto& repo_filename : repositories )
    {
        f_manager->check_interrupt();
        increment_progress();

        memfile::memory_file index_file;
        if( !read_repository_index( repo_filename, index_file ) )
        {
            continue;
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
            package_item_t package(f_manager, repo_filename.append_child(filename), package_item_t::package_type_available, ctrl);

            // verify package architecture
            const std::string arch(package.get_architecture());
            if(arch != "all" && !wpkg_dependencies::dependencies::match_architectures
                (
                arch
                , f_architecture
                , f_flags->get_parameter(flags::param_force_vendor, false) != 0)
                )
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

            f_package_list->get_package_list().push_back(package);
        }
    }
}


void dependencies::trim_conflicts
    ( const bool check_available
    , const bool only_explicit
    , wpkg_filename::uri_filename filename
    , installer::package_item_t::package_type_t idx_type
    , package_item_t& parent_package
    , package_item_t& depends_package
    , const wpkg_dependencies::dependencies::dependency_t& dependency
    )
{
    if( !only_explicit || depends_package.get_type() == package_item_t::package_type_explicit )
    {
        switch(depends_package.get_type())
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
                if(dependency.f_name == depends_package.get_name()
                        && match_dependency_version(dependency, depends_package) == 1)
                {
                    // ouch! found a match, mark that package as invalid
                    int err(2);
                    switch(depends_package.get_type())
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
                            depends_package.set_type(package_item_t::package_type_invalid);
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
                            parent_package.set_type(package_item_t::package_type_invalid);
                            break;

                        default:
                            // caller is at fault
                            throw std::logic_error("trim_conflicts() called with an unexpected package type [Conflicts]");

                    }
                    if(err == 2)
                    {
                        // we do not mark explicit/installed packages
                        // as invalid; output an error instead
                        if(f_flags->get_parameter(flags::param_force_conflicts, false))
                        {
                            // user accepts conflicts, use a warning
                            err = 0;
                        }
                        wpkg_output::log("package %1 is in conflict with %2.")
                            .quoted_arg(filename)
                            .quoted_arg(depends_package.get_filename())
                            .level(err == 0 ? wpkg_output::level_warning : wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(filename)
                            .action("install-validation");

                        if( err )
                        {
                            throw dependency_error( "package conflict" );
                        }
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


void dependencies::trim_breaks
    ( const bool check_available
    , const bool only_explicit
    , wpkg_filename::uri_filename filename
    , installer::package_item_t::package_type_t idx_type
    , package_item_t& parent_package
    , package_item_t& depends_package
    , const wpkg_dependencies::dependencies::dependency_t& dependency
    )
{
    if( !only_explicit || depends_package.get_type() == package_item_t::package_type_explicit )
    {
        switch(depends_package.get_type())
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
                if(dependency.f_name == depends_package.get_name()
                        && match_dependency_version(dependency, depends_package) == 1)
                {
                    // ouch! found a match, mark that package as invalid
                    int err(2);
                    switch(depends_package.get_type())
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
                            depends_package.set_type(package_item_t::package_type_invalid);
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
                            parent_package.set_type(package_item_t::package_type_invalid);
                            break;

                        default:
                            // caller is at fault
                            throw std::logic_error("trim_conflicts() called with an unexpected package type [Breaks]");

                    }
                    if(err == 2)
                    {
                        // we do not mark explicit/installed packages
                        // as invalid; generate an error instead
                        if(f_flags->get_parameter(flags::param_force_breaks, false))
                        {
                            // user accepts Breaks, use a warning
                            err = 0;
                        }
                        wpkg_output::log("package %1 breaks %2.")
                            .quoted_arg(filename)
                            .quoted_arg(depends_package.get_filename())
                            .level(err == 0 ? wpkg_output::level_warning : wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(filename)
                            .action("install-validation");

                        if( err )
                        {
                            throw dependency_error( "package conflict" );
                        }
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


/** \brief Check whether a package is in conflict with another.
 *
 * This function checks whether the specified package (tree[idx]) is in
 * conflict with any others.
 *
 * The specified tree maybe f_package_list->get_package_list() or a copy that we're working on.
 *
 * The only-explicit flag is used to know whether we're only checking
 * explicit packages as conflict destinations. This is useful to trim
 * the f_package_list->get_package_list() tree before building all the trees.
 *
 * The function checks the Conflicts field and then the Breaks field.
 * The Breaks fields are ignored if the packager is just unpacking
 * packages specified on the command line.
 *
 * \param[in,out] tree  The tree to check for conflicts.
 * \param[in] idx  The index of the item being checked.
 * \param[in] only_explicit  Whether only explicit packages are checked.
 */
void dependencies::trim_conflicts(package_list::list_t& tree, const package_list::list_t::size_type idx, const bool only_explicit)
{
    auto& parent_package( tree[idx] );
    wpkg_filename::uri_filename filename(parent_package.get_filename());
    package_item_t::package_type_t idx_type(parent_package.get_type());
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
    if(parent_package.field_is_defined(wpkg_control::control_file::field_conflicts_factory_t::canonicalized_name()))
    {
        wpkg_dependencies::dependencies depends(parent_package.get_field(wpkg_control::control_file::field_conflicts_factory_t::canonicalized_name()));
        for(int i(0); i < depends.size(); ++i)
        {
            const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(i));
            for(package_list::list_t::size_type j(0); j < tree.size(); ++j)
            {
                f_manager->check_interrupt();

                if( j == idx ) continue;

                trim_conflicts( check_available, only_explicit, filename, idx_type, parent_package, tree[j], d );
            }
        }
    }

    // breaks don't apply if we're just unpacking
    if(f_task == task_unpacking_packages)
    {
        return;
    }

    // got a Breaks field?
    if(parent_package.field_is_defined(wpkg_control::control_file::field_breaks_factory_t::canonicalized_name()))
    {
        wpkg_dependencies::dependencies depends(parent_package.get_field(wpkg_control::control_file::field_breaks_factory_t::canonicalized_name()));
        for(int i(0); i < depends.size(); ++i)
        {
            const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(i));
            for(package_list::list_t::size_type j(0); j < tree.size(); ++j)
            {
                f_manager->check_interrupt();

                if( j == idx ) continue;

                trim_breaks( check_available, only_explicit, filename, idx_type, parent_package, tree[j], d );
            }
        }
    }
}


bool dependencies::trim_dependency
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
    for( auto& pkg : f_package_list->get_package_list() )
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
                if(!f_flags->get_parameter(flags::param_force_depends_version, false))
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
                    throw dependency_error( "incompatible version in dependency" );
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
        for( auto& pkg : f_package_list->get_package_list() )
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
    for( auto& pkg : f_package_list->get_package_list() )
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
    for( auto& pkg : f_package_list->get_package_list() )
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
                    if(!f_flags->get_parameter(flags::param_force_depends, false))
                    {
                        wpkg_output::log("package %1 depends on %2 which is an installed package with an incompatible version constraint (%3).")
                                .quoted_arg(filename)
                                .quoted_arg(last_package->get_filename())
                                .arg(dependency.to_string())
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_validate_installation)
                                .package(filename)
                                .action("install-validation");
                        throw dependency_error( "incompatible version constraint in dependency" );
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
                if(!f_flags->get_parameter(flags::param_force_depends, false))
                {
                    wpkg_output::log("no explicit or implicit package satisfy dependency %1 of package %2.")
                        .quoted_arg(dependency.to_string())
                        .quoted_arg(item.get_name())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(filename)
                        .action("install-validation");
                    throw dependency_error( "satisify dependency" );
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


void dependencies::trim_available( package_item_t& item, wpkgar_package_ptrs_t& parents )
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
            throw dependency_error( "circular dependency" );
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


void dependencies::trim_available_packages()
{
    progress_scope s( this, "trim_available_packages", f_package_list->get_package_list().size() );

    // start by removing all the available packages that are in conflict with
    // the explicit packages because we'll never be able to use them
    for( package_list::list_t::size_type idx(0); idx < f_package_list->get_package_list().size(); ++idx)
    {
        increment_progress();

        auto& pkg( f_package_list->get_package_list()[idx] );
        // start from the top level (i.e. only check explicit dependencies)
        switch(pkg.get_type())
        {
        case package_item_t::package_type_explicit:
            if(f_task != task_reconfiguring_packages)
            {
                trim_conflicts(f_package_list->get_package_list(), idx, false);
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
            trim_conflicts(f_package_list->get_package_list(), idx, true);
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

    if(f_task != task_reconfiguring_packages)
    {
        wpkgar_package_ptrs_t parents;
        for( auto& pkg : f_package_list->get_package_list() )
        {
            // start from the top level (i.e. only check explicit dependencies)
            if(pkg.get_type() == package_item_t::package_type_explicit)
            {
                trim_available(pkg, parents);
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
dependencies::validation_return_t dependencies::find_explicit_dependency(package_list::list_t::size_type index, const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d, const std::string& field_name)
{
    // check whether it is part of the list of packages the user
    // specified on the command line (explicit)
    package_list::list_t::size_type found(static_cast<package_list::list_t::size_type>(-1));
    for( package_list::list_t::size_type idx(0); idx < f_package_list->get_package_list().size(); ++idx)
    {
        auto& pkg( f_package_list->get_package_list()[idx] );
        if(index != idx // skip myself
        && pkg.get_type() == package_item_t::package_type_explicit
        && d.f_name == pkg.get_name())
        {
            if(found != static_cast<package_list::list_t::size_type>(-1))
            {
                // found more than one!
                if(!same_file(package_name.os_filename().get_utf8(), pkg.get_filename().os_filename().get_utf8()))
                {
                    // and they both come from two different files so that's an error!
                    wpkg_output::log("files %1 and %2 define the same package (their Package field match) but are distinct files.")
                            .quoted_arg(pkg.get_filename())
                            .quoted_arg(package_name)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(package_name)
                        .action("install-validation");
                    throw dependency_error( "Package field overlap" );
                }
                else
                {
                    // this package is as valid as the other since they both are
                    // the exact same, but we don't want to do the work twice so
                    // to ignore it set as invalid
                    pkg.set_type(package_item_t::package_type_invalid);
                }
            }
            else
            {
                const std::string& architecture(pkg.get_architecture());
                if((architecture == "src" || architecture == "source")
                && field_name != wpkg_control::control_file::field_builtusing_factory_t::canonicalized_name())
                {
                    // the only place were a source package can depend on
                    // another source package is in the Built-Using field;
                    // anywhere else and it is an error because nothing shall
                    // otherwise depend on a source package
                    wpkg_output::log("package %1 is a source package and it cannot be part of the list of dependencies defined in %2.")
                            .quoted_arg(pkg.get_filename())
                            .arg(field_name)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(package_name)
                        .action("install-validation");
                    throw dependency_error( "Package field overlap" );
                }

                // keep the first we find
                found = idx;
            }
        }
    }

    if(found != static_cast<package_list::list_t::size_type>(-1))
    {
        const wpkg_filename::uri_filename filename(f_package_list->get_package_list()[found].get_filename());
        if(match_dependency_version(d, f_package_list->get_package_list()[found]) == 1)
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
        throw dependency_error( "file has incompatible version for dependency" );
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
dependencies::validation_return_t dependencies::find_installed_dependency(package_list::list_t::size_type index, const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d, const std::string& field_name)
{
    // check whether it is part of the list of packages the user
    // specified on the command line (explicit)
    package_list::list_t::size_type found(static_cast<package_list::list_t::size_type>(-1));
    for( package_list::list_t::size_type idx(0); idx < f_package_list->get_package_list().size(); ++idx)
    {
        auto& pkg( f_package_list->get_package_list()[idx] );
        if(index != idx // skip myself
        && pkg.get_type() == package_item_t::package_type_installed
        && d.f_name == pkg.get_name())
        {
            if(found != static_cast<package_list::list_t::size_type>(-1))
            {
                // found more than one!?
                // this should never happen since you cannot install two
                // distinct packages on a target with the exact same name!
                wpkg_output::log("found two distinct installed packages, %1 and %2, with the same name?!")
                        .quoted_arg(pkg.get_filename())
                        .quoted_arg(f_package_list->get_package_list()[found].get_filename())
                        .quoted_arg(package_name)
                    .level(wpkg_output::level_fatal)
                    .module(wpkg_output::module_validate_installation)
                    .package(package_name)
                    .action("install-validation");
                throw dependency_error( "two packages with same name" );
            }

            const std::string& architecture(pkg.get_architecture());
            if((architecture == "src" || architecture == "source")
            && field_name != wpkg_control::control_file::field_builtusing_factory_t::canonicalized_name())
            {
                // the only place were a source package can depend on
                // another source package is in the Built-Using field;
                // anywhere else and it is an error because nothing shall
                // otherwise depend on a source package
                wpkg_output::log("package %1 is a source package and it cannot be part of the list of dependencies defined in %2.")
                        .quoted_arg(pkg.get_filename())
                        .arg(field_name)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(package_name)
                    .action("install-validation");
                throw dependency_error( "source package cannot be a dependency" );
            }

            // keep the first we find
            found = idx;
        }
    }

    if(found != static_cast<package_list::list_t::size_type>(-1))
    {
        const wpkg_filename::uri_filename name(f_package_list->get_package_list()[found].get_name());
        if(match_dependency_version(d, f_package_list->get_package_list()[found]) == 1)
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


dependencies::validation_return_t dependencies::validate_installed_depends_field(const package_list::list_t::size_type idx, const std::string& field_name)
{
    // full path to package
    const wpkg_filename::uri_filename filename(f_package_list->get_package_list()[idx].get_filename());
    validation_return_t result(validation_return_success);

    // we already checked that the field existed in the previous function
    wpkg_dependencies::dependencies depends(f_package_list->get_package_list()[idx].get_field(field_name));
    progress_scope s( this, "validate_installed_depends_field", depends.size() );
    for(int i(0); i < depends.size(); ++i)
    {
        f_manager->check_interrupt();
        increment_progress();

        const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(i));
        validation_return_t r(find_explicit_dependency(idx, filename, d, field_name));
        if(r == validation_return_error) // not used, since throwing now
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


dependencies::validation_return_t dependencies::validate_installed_dependencies()
{
    // first check whether all the dependencies are self contained (i.e. the
    // package being installed only needs already installed packages or has
    // no dependencies in the first place.)
    //
    // if so we avoid the whole algorithm trying to auto-install missing
    // dependencies using packages defined in repositories

    // result is success by default
    validation_return_t result(validation_return_success);

    progress_scope s( this, "validate_installed_dependencies", f_package_list->get_package_list().size() );

    for( package_list::list_t::size_type idx(0); idx < f_package_list->get_package_list().size(); ++idx )
    {
        auto& pkg( f_package_list->get_package_list()[idx] );
        increment_progress();
        if(pkg.get_type() == package_item_t::package_type_explicit)
        {
            // full path to package
            const wpkg_filename::uri_filename filename(pkg.get_filename());
            const std::string& architecture(pkg.get_architecture());
            const bool is_source(architecture == "src" || architecture == "source");

            // get list of dependencies if any
            for( const auto& field_name : f_field_names )
            {
                if(pkg.field_is_defined(field_name))
                {
                    // kind of a hackish test here... if not Depends field
                    // and it is a binary package, that's an error
                    if(!is_source
                        && (field_name != wpkg_control::control_file::field_depends_factory_t::canonicalized_name())
                        )
                    {
                        wpkg_output::log("%1 is a binary package and yet it includes build dependencies.")
                                .quoted_arg(filename)
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_validate_installation)
                            .package(pkg.get_name())
                            .action("install-validation");
                        throw dependency_error( "binary package contains build dependencies" );
                    }

                    validation_return_t r(validate_installed_depends_field(idx, field_name));
                    if(r == validation_return_error) // not used, since throwing now
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


bool dependencies::check_implicit_for_upgrade(package_list::list_t& tree, const package_list::list_t::size_type idx)
{
    // check whether this implicit package is upgrading an existing package
    // because if so we have to mark the already installed package as being
    // upgraded and we have to link both packages together; also we do not
    // allow implicit downgrade, we prevent auto-upgrade of package which
    // selection is "Hold", and a few other things too...

    // TBD: if not installing I do not think we should end up here...
    //      but just in case I test
    if(f_task != task_installing_packages)
    {
        return true;
    }

    // no problem if the package is not already installed
    // (we first test whether it's listed because that's really fast)
    const auto& tree_pkg( tree[idx] );
    const std::string name(tree_pkg.get_name());
    const auto& installed_package_list( f_package_list->get_installed_package_list() );
    if( std::find( installed_package_list.begin(), installed_package_list.end(), name ) == installed_package_list.end() )
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
        if(!f_task == task_unpacking_packages)
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
    std::string vo(tree_pkg.get_version());
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
    for(package_list::list_t::size_type i(0); i < tree.size(); ++i)
    {
        if(tree[i].get_type() == type && tree[i].get_name() == name)
        {
            tree[i].set_type(package_item_t::package_type_upgrade);
            return true;
        }
    }

    // we've got an error here; the installed package must already exists
    // since it was loaded when validating said installed packages
    throw std::logic_error("an implicit target cannot upgrade an existing package if that package does not exist in the f_package_list->get_package_list() vector; this is an internal error and the code needs to be fixed if it ever happens"); // LCOV_EXCL_LINE
}


/** \brief Return XSelection if defined in the package status.
 *
 * This method checks if XSelection is defined in the package status. If so, then it returns the selection.
 *
 * \return selection, or "selection_normal" if not defined.
 *
 */
wpkg_control::control_file::field_xselection_t::selection_t dependencies::get_xselection( const wpkg_filename::uri_filename& filename ) const
{
    return get_xselection( filename.os_filename().get_utf8() );
}

wpkg_control::control_file::field_xselection_t::selection_t dependencies::get_xselection( const std::string& filename ) const
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
void dependencies::find_dependencies( package_list::list_t& tree, const package_list::list_t::size_type idx, wpkgar_dependency_list_t& missing, wpkgar_dependency_list_t& held )
{
    const auto& tree_pkg( tree[idx] );
    const wpkg_filename::uri_filename filename(tree_pkg.get_filename());

    trim_conflicts(tree, idx, false);

    const std::string& architecture(tree_pkg.get_architecture());
    const bool is_source(architecture == "src" || architecture == "source");

    for( const auto& field_name : f_field_names )
    {
        // any dependencies in this entry?
        if(!tree_pkg.field_is_defined(field_name))
        {
            // no dependencies means "satisfied"
            continue;
        }

        // check the dependencies
        wpkg_dependencies::dependencies depends(tree_pkg.get_field(field_name));
        for(int i(0); i < depends.size(); ++i)
        {
            const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(i));

            package_list::list_t::size_type unpacked_idx(0);
            validation_return_t found(validation_return_missing);
            for(package_list::list_t::size_type tree_idx(0);
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
                                            selection( dependencies::get_xselection( tree_item.get_filename() ) );

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
                if(f_flags->get_parameter(flags::param_force_configure_any, false))
                {
                    f_package_list->get_package_list()[unpacked_idx].set_type(package_item_t::package_type_configure);
                    found = validation_return_success;
                }
            }

            // kind of a hackish test here... if not Depends field
            // and it is a binary package, that's an error
            if( !is_source
                && (field_name != wpkg_control::control_file::field_depends_factory_t::canonicalized_name())
                && (found == validation_return_success)
                )
            {
                // similar to having a missing dependency error wise
                found = validation_return_missing;

                // this is an error no matter what, we may end up printing it
                // many times though...
                wpkg_output::log("%1 is a binary package and yet it includes build dependencies.")
                        .quoted_arg(filename)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_validate_installation)
                    .package(tree_pkg.get_name())
                    .action("install-validation");
                throw dependency_error( "binary package contains build dependencies" );
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


bool dependencies::verify_tree( package_list::list_t& tree, wpkgar_dependency_list_t& missing, wpkgar_dependency_list_t& held )
{
    // if reconfiguring we have a good tree (i.e. the existing installation
    // tree is supposed to be proper)
    if(f_task == task_reconfiguring_packages)
    {
        return true;
    }

    progress_scope s( this, "verify_tree", tree.size() );

    // save so we know whether any dependencies are missing
    wpkgar_dependency_list_t::size_type missing_count(missing.size());
    wpkgar_dependency_list_t::size_type held_count(held.size());

    // verifying means checking that all dependencies are satisfied
    // also, in this case "available" dependencies that are required
    // get the new type "implicit" so we know we have to install them
    // and we can save the correct status in the package once installed
    for( package_list::list_t::size_type idx(0); idx < tree.size(); ++idx )
    {
        increment_progress();

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
bool dependencies::trees_are_practically_identical(
    const package_list::list_t& left,
    const package_list::list_t& right) const
{
    // A functor for testing the equivalence of package versions. This is
    // hoisted out here to make the comparison loop below easier to read.
    // Ideally this would be floating outside the function altogether,
    // but there's not much point while the types it references are only
    // defined inside dependencies.
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
    for( const auto& left_pkg : left )
    {
        if(left_pkg.is_marked_for_install())
        {
            package_list::list_t::const_iterator rhs = find_if(right.begin(),
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


int dependencies::compare_trees(const package_list::list_t& left, const package_list::list_t& right) const
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
    for( const auto& left_pkg : left )
    {
        f_manager->check_interrupt();

        switch(left_pkg.get_type())
        {
        case package_item_t::package_type_explicit:
        case package_item_t::package_type_implicit:
        case package_item_t::package_type_configure:
        case package_item_t::package_type_upgrade:
        case package_item_t::package_type_upgrade_implicit:
        case package_item_t::package_type_downgrade:
            {
                std::string name(left_pkg.get_name());
                for( const auto& right_pkg : right )
                {
                    switch(right_pkg.get_type())
                    {
                    case package_item_t::package_type_explicit:
                    case package_item_t::package_type_implicit:
                    case package_item_t::package_type_configure:
                    case package_item_t::package_type_upgrade:
                    case package_item_t::package_type_upgrade_implicit:
                    case package_item_t::package_type_downgrade:
                        if(name == right_pkg.get_name())
                        {
                            // found similar packages, check versions
                            int r(wpkg_util::versioncmp(left_pkg.get_version(), right_pkg.get_version()));
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


void dependencies::output_tree(int file_count, const package_list::list_t& tree, const std::string& sub_title)
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

    for( package_list::list_t::size_type idx(0); idx < tree.size(); ++idx )
    {
        const auto& tree_pkg( tree[idx] );
        f_manager->check_interrupt();

        const char *name(tree_pkg.get_name().c_str());
        const char *version(tree_pkg.get_version().c_str());
        switch(tree_pkg.get_type())
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
        for( const auto& field_name : f_field_names )
        {
            if(!tree_pkg.field_is_defined(field_name))
            {
                continue;
            }

            // check the dependencies
            wpkg_dependencies::dependencies depends(tree_pkg.get_field(field_name));
            for(int i(0); i < depends.size(); ++i)
            {
                const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(i));

                for(package_list::list_t::size_type j(0);
                                                     j < tree.size();
                                                     ++j)
                {
                    auto& pkg_j(tree[j]);
                    if(d.f_name == pkg_j.get_name())
                    {
                        if(match_dependency_version(d, pkg_j) == 1)
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
void dependencies::validate_dependencies()
{
    // self-contained with explicit and installed dependencies?
    if(validate_installed_dependencies() != validation_return_missing)
    {
        if((wpkg_output::get_output_debug_flags() & wpkg_output::debug_flags::debug_depends_graph) != 0)
        {
            // output the verified tree
            output_tree(1, f_package_list->get_package_list(), "no implied packages");
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
        output_tree(0, f_package_list->get_package_list(), "tree with repositories");
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
    // packages, then our current tree is enough and we can simply use
    // it for the next step and if it fails, we're done...
    if(!f_install_includes_choices)
    {
        wpkgar_dependency_list_t missing;
        wpkgar_dependency_list_t held;
        if(!verify_tree(f_package_list->get_package_list(), missing, held))
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
            throw dependency_error( ss.str().c_str() );
        }
        if((wpkg_output::get_output_debug_flags() & wpkg_output::debug_flags::debug_depends_graph) != 0)
        {
            // output the verified tree
            output_tree(1, f_package_list->get_package_list(), "no choices");
        }
        return;
    }

    // note that here when we want to add dependencies we add them to the
    // implicit list of packages and from now on we have to check both
    // lists to be complete... (explicit + implicit); other lists are
    // ignored except the available while we search for dependencies

    progress_scope s( this, "validate_dependencies", f_package_list->get_package_list().size() );
    package_list::list_t best;
    for(tree_generator tree_gen(f_package_list->get_package_list());;)
    {
        increment_progress();
        package_list::list_t tree(tree_gen.next());
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
                    throw dependency_error( "two trees are similar" );
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
        throw dependency_error( "could not create a complete tree" );
    }

    // just keep the best, all the other trees we can discard
    f_package_list->get_package_list() = best;
}


void dependencies::add_progess_record( const std::string& what, const uint64_t max )
{
    wpkg_output::progress_record_t record;
    record.set_progress_what ( what );
    record.set_progress_max  ( max  );
    f_progress_stack.push( record );

    wpkg_output::log("progress")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation)
        .progress( record );
}


void dependencies::increment_progress()
{
    if( f_progress_stack.empty() )
    {
        return;
    }

    f_progress_stack.top().increment_current_progress();

    wpkg_output::log("increment progress")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation)
        .progress( f_progress_stack.top() );
}


void dependencies::pop_progess_record()
{
    if( f_progress_stack.empty() )
    {
        return;
    }

    wpkg_output::progress_record_t record( f_progress_stack.top() );
    f_progress_stack.pop();

    wpkg_output::log("pop progress")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation)
        .progress( record );
}

}   // installer namespace

}   // wpkgar namespace

// vim: ts=4 sw=4 et
