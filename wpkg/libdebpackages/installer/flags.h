/*    flags.h -- installation flags
 *    Copyright (C) 2016  Made to Order Software Corporation
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
#pragma once

#include <map>
#include <memory>

namespace wpkgar
{

namespace installer
{

class flags
    : public std::enable_shared_from_this<flags>
{
public:
    enum parameter_t
    {
        param_force_architecture,      		// allow installation whatever the architecture
        param_force_breaks,            		// allow installation with breaks
        param_force_configure_any,     		// allow auto-configuration of unpacked packages
        param_force_conflicts,         		// allow installation with conflicts
        param_force_depends,           		// allow installation with missing dependencies
        param_force_depends_version,   		// allow installation with wrong versions
        param_force_distribution,      		// allow installation of packages without a distribution field
        param_force_downgrade,         		// allow updates of older versions of packages
        param_force_file_info,         		// allow chmod()/chown() failures
        param_force_hold,              		// allow upgrades/downgrades of held packages
        param_force_overwrite,         		// allow new packages to overwrite existing files
        param_force_overwrite_dir,     		// allow new packages to overwrite existing directories
        param_force_rollback,          		// do a rollback on error
        param_force_upgrade_any_version,    // allow upgrading even if a Minimum-Upgradable-Version is defined
        param_force_vendor,            		// allow installing of incompatible vendor names
        param_quiet_file_info,         		// do not print chmod/chown warnings
        param_recursive,               		// read sub-directories of repositories
        param_skip_same_version        		// do not re-install over itself
    };

	typedef std::shared_ptr<flags> pointer_t;

	flags();
	virtual ~flags();

    int  get_parameter ( const parameter_t flag, const int default_value ) const;
	void set_parameter ( const parameter_t flag, const int value );

private:
    typedef std::map<parameter_t, int>	flags_map_t;
    flags_map_t                         f_flags_map;
};

}
// namespace installer

}
// namespace wpkgar
