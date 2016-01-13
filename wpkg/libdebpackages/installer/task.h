/*    task.h -- set the task for the installer
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
#pragma once

#include "libdebpackages/debian_export.h"

#include <memory>

namespace wpkgar
{

namespace installer
{

class DEBIAN_PACKAGE_EXPORT task
{
public:
    enum task_t
    {
        task_installing_packages,
        task_configuring_packages,
        task_reconfiguring_packages,
        task_unpacking_packages
    };

    typedef std::shared_ptr<task> pointer_t;

    task( task_t init_task );
    virtual ~task();

    task_t get_task() const;
    void set_task( task_t val );

    bool operator ==( task_t t ) const;
    bool operator !=( task_t t ) const;

private:
	task_t	f_task;
};

}   // namespace installer

}   // namespace wpkgar

// vim: ts=4 sw=4 et
