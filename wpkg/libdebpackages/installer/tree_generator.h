/*    installer/tree_generator.h
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
#pragma once

#include    "controlled_vars/controlled_vars.h"
#include    "libdebpackages/debian_export.h"
#include    "libdebpackages/installer/package_item.h"

namespace wpkgar
{

namespace installer
{

class DEBIAN_PACKAGE_EXPORT tree_generator
{
public:
    typedef package_item_t::list_t::size_type package_index_t;
    typedef std::vector<package_index_t>      package_idxs_t;

	tree_generator( const package_item_t::list_t& root_tree );

	package_item_t::list_t      next();
	uint64_t                    tree_number() const;

private:
    typedef package_idxs_t                    pkg_alternatives_t;
    typedef std::vector<pkg_alternatives_t>   pkg_alternatives_list_t;

	const package_item_t::list_t            f_master_tree;
	pkg_alternatives_list_t                 f_pkg_alternatives;
	std::vector<controlled_vars::zuint64_t> f_divisor;
	controlled_vars::zuint64_t              f_n; // the n'th permution
	controlled_vars::zuint64_t              f_end;
};

}
// namespace installer

}
// namespace wpkgar

// vim: ts=4 sw=4 et
