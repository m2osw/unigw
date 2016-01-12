/*    package_list.cpp
 *    Copyright (C) 2006-2016  Made to Order Software Corporation
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

#include "libdebpackages/installer/package_list.h"

#include <sstream>

namespace wpkgar
{

namespace installer
{

package_list::package_list( wpkgar_manager::pointer_t manager )
    : f_manager(manager)
    //, f_list_installed_packages() -- auto-init
    //, f_packages() -- auto-init
{
    f_manager->list_installed_packages(f_list_installed_packages);
}


package_list::list_t::const_iterator package_list::find_package_item(const wpkg_filename::uri_filename& filename) const
{
    auto iter = std::find_if( f_packages.begin(), f_packages.end(), [&filename]( const package_item_t& package )
    {
        return package.get_filename().full_path() == filename.full_path();
    });

    return iter;
}


package_list::list_t::iterator package_list::find_package_item_by_name(const std::string& name)
{
    auto iter = std::find_if( f_packages.begin(), f_packages.end(), [&name]( const package_item_t& package )
    {
        return package.get_name() == name;
    });

    return iter;
}


/** \brief Add a package directly from the wpkar_repository object, instead of by string.
 */
void package_list::add_package( wpkgar_repository::package_item_t entry, const bool force_reinstall )
{
    if( entry.get_status() == wpkgar_repository::package_item_t::invalid )
    {
        std::stringstream ss;
        ss << "Cannot install package '" << entry.get_name() << "' since it is invalid!";
        throw std::runtime_error(ss.str());
    }

    bool install_it = false;
    if( force_reinstall )
    {
        install_it = true;
    }
    else if( (entry.get_status() == wpkgar_repository::package_item_t::not_installed)
          || (entry.get_status() == wpkgar_repository::package_item_t::need_upgrade)
          )
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


/** \brief Add a package by package name, and option version
 */
void package_list::add_package( const std::string& package, const std::string& version, const bool force_reinstall )
{
    const wpkg_filename::uri_filename pck(package);
    list_t::const_iterator item(find_package_item(pck));
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

            typedef std::map<std::string,wpkgar_repository::package_item_t> version_package_map_t;
            version_package_map_t version_map;

            for( const auto& entry : repository.upgrade_list() )
            {
                if( entry.get_name() == package )
                {
                    version_map[entry.get_version()] = entry;
                }
            }

            if( version_map.empty() )
            {
                std::stringstream ss;
                ss << "Cannot install package '" << package << "' because it doesn't exist in the repository!";
                throw std::runtime_error(ss.str());
            }

            if( version.empty() )
            {
                auto greatest_version( version_map.rbegin()->first );
                if( version_map.size() > 1 )
                {
                    wpkg_output::log("package '%1' has multiple versions available in the selected repositories. Selected the greatest version '%2'.")
                            .quoted_arg(package)
                            .quoted_arg(greatest_version)
                        .level(wpkg_output::level_warning)
                        .module(wpkg_output::module_validate_installation)
                        .package(package)
                        .action("install-validation");
                }

                // Select the greatest version
                //
                add_package( version_map.rbegin()->second, force_reinstall );
            }
            else
            {
                add_package( version_map[version], force_reinstall );
            }
        }
    }
}


bool package_list::find_essential_file(std::string filename, const size_t skip_idx)
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
        for( wpkgar_package_list_t::size_type idx = 0; idx < f_packages.size(); ++idx )
        {
            auto& pkg( f_packages[idx] );
            if(static_cast<wpkgar_package_list_t::size_type>(skip_idx) == idx)
            {
                // this is the package we're working on and obviously
                // filename will exist in this package
                continue;
            }
            // any package that is already installed or unpacked
            // or that is about to be installed is checked
            switch(pkg.get_type())
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
            if(!pkg.field_is_defined(wpkg_control::control_file::field_essential_factory_t::canonicalized_name())
            || !pkg.get_boolean_field(wpkg_control::control_file::field_essential_factory_t::canonicalized_name()))
            {
                // the default value of the Essential field is "No" (false)
                continue;
            }

            // TODO: change this load and use the Files field instead
            // make sure the package is loaded
            f_manager->load_package(pkg.get_filename());

            // check all the files defined in the data archive
            memfile::memory_file *wpkgar_file;
            f_manager->get_wpkgar_file(pkg.get_filename(), wpkgar_file);

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


const package_list::list_t& package_list::get_package_list() const
{
   return f_packages; 
}


package_list::list_t& package_list::get_package_list()
{
   return f_packages; 
}


const wpkgar_manager::package_list_t& package_list::get_installed_package_list() const
{
    return f_installed_packages;
}


wpkgar_manager::package_list_t& package_list::get_installed_package_list()
{
    return f_installed_packages;
}


}
// namespace wpkgar


}
// namespace installer


// vim: ts=4 sw=4 et
