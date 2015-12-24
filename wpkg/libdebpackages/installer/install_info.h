/*    install_info.h -- install info type for debian packages slated for install
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
#pragma once

#include    "controlled_vars/controlled_vars.h"
#include    "libdebpackages/debian_export.h"

#include    <vector>

namespace wpkgar
{

class wpkgar_install;

namespace installer
{

class DEBIAN_PACKAGE_EXPORT install_info_t
{
	public:
		friend class wpkgar::wpkgar_install;

		enum install_type_t
		{
			install_type_undefined, // undefined
			install_type_explicit,  // explicitly asked for
			install_type_implicit   // necessary to satisfy dependencies
		};

		install_info_t();

		std::string    get_name()         const;
		std::string    get_version()      const;
		install_type_t get_install_type() const;
		bool           is_upgrade()       const;

	private:
		std::string                 f_name;
		std::string                 f_version;
		install_type_t              f_install_type;
		controlled_vars::fbool_t    f_is_upgrade;
};

typedef std::vector<install_info_t> install_info_list_t;

}
// namespace installer

}
// namespace wpkgar

// vim: ts=4 sw=4 et
