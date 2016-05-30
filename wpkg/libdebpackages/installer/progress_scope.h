/*    progress_scope.h -- Keep track of progress records
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
 *    R. Douglas Barbieri Wilke   doug@m2osw.com
 */

#pragma once

#include "libdebpackages/debian_export.h"
#include "libdebpackages/wpkg_output.h"

#include <string>
#include <stack>

namespace wpkgar
{

namespace installer
{


class DEBIAN_PACKAGE_EXPORT progress_stack
{
public:
    progress_stack();
    virtual ~progress_stack();

    void add_progess_record( const std::string& what, const uint64_t max );
    void pop_progess_record();
    void increment_progress();

private:
    typedef std::stack<wpkg_output::progress_record_t> stack_t;
    stack_t     f_progress_stack;
};


template <class T, class P>
class DEBIAN_PACKAGE_EXPORT progress_scope_t
{
public:
	progress_scope_t( T* inst
                    , const std::string& what
                    , const P max
                    )
        : f_inst(inst)
    {
        f_inst->add_progess_record( what, max );
    }


    ~progress_scope_t()
    {
        f_inst->pop_progess_record();
    }

private:
	T*	f_inst;
};

} // installer

} // wpkgar


// vim: ts=4 sw=4 et
