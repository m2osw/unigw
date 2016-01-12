/*    package_list.h -- Maintain installer package list.
 *    Copyright (C) 2012-2016  Made to Order Software Corporation
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
 * \brief TODO add good explanation
 *
 */
#pragma once

#include    "libdebpackages/wpkgar.h"
#include    "libdebpackages/wpkgar_repository.h"
#include    "libdebpackages/installer/package_item.h"

namespace wpkgar
{

namespace installer
{

class DEBIAN_PACKAGE_EXPORT package_list
    : public std::enable_shared_from_this<package_list>
{
public:
    typedef std::shared_ptr<package_list> pointer_t;
    typedef std::vector<std::string>      string_list_t;

    package_list( wpkgar_manager::pointer_t manager );

    void add_package( const std::string& package, const std::string& version = std::string(), const bool force_reinstall = false );
    void add_package( wpkgar_repository::package_item_t entry, const bool force_reinstall = false );
    const std::string& get_package_name( const int idx ) const;
    int count() const;

    // functions used internally
    bool find_essential_file( std::string filename, const size_t skip_idx );

    typedef package_item_t::list_t list_t;
    const list_t& get_package_list() const;
    list_t&       get_package_list();

    const wpkgar_manager::package_list_t& get_installed_package_list() const;
    wpkgar_manager::package_list_t&       get_installed_package_list();

private:
    list_t::const_iterator find_package_item(const wpkg_filename::uri_filename& filename) const;
    list_t::iterator       find_package_item_by_name(const std::string& name);

    wpkgar_manager::pointer_t        f_manager;
    list_t                           f_packages;
    string_list_t                    f_essential_files;
    wpkgar_manager::package_list_t   f_list_installed_packages;
};

}
// namespace installer

}   // namespace wpkgar

// vim: ts=4 sw=4 et
