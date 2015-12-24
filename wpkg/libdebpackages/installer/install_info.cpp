/*    install_info.cpp -- install info type for debian packages slated for install
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
 *    Doug Barbieri	 doug@m2osw.com
 */ 

#include "libdebpackages/installer/install_info.h"

namespace wpkgar
{

namespace installer
{

/** \class install_info_t
 * \brief One item in the list of packages to be installed.
 *
 * After running the validation process, the list of packages that is to be
 * installed is known by the wpkgar_install object. This list can be
 * retrieved with the use of the get_install_list() function.
 * That function returns a vector of install_info_t objects which give you
 * the name and version of the package, whether it is installed explicitly or
 * implicitly, and if the installation is a new installation or an
 * upgrade (i.e. see the is_upgrade() function.)
 *
 * Note that the function used to retrieve this list does not return the
 * other packages (i.e. packages that are available in a repository, already
 * installed, etc. are not returned in this list.)
 */

install_info_t::install_info_t()
    //: f_name("") -- auto-init
    //, f_version("") -- auto-init
    : f_install_type(install_type_undefined)
    //, f_is_upgrade(false) -- auto-init
{
}


std::string   install_info_t::get_name() const
{
    return f_name;
}


std::string   install_info_t::get_version() const
{
    return f_version;
}


install_info_t::install_type_t install_info_t::get_install_type() const
{
    return f_install_type;
}


bool install_info_t::is_upgrade() const
{
    return f_is_upgrade;
}


}
// namespace installer

}
// namespace wpkgar

// vim: ts=4 sw=4 et
