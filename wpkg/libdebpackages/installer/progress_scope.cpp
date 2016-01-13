/*    wpkgar_validator.cpp -- install a debian package
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

#include "libdebpackages/installer/progress_scope.h"
#include "libdebpackages/wpkg_output.h"

namespace wpkgar
{

namespace installer
{

progress_stack::progress_stack()
	//: f_progress_stack() -- auto init
{
}

progress_stack::~progress_stack()
{
}

void progress_stack::add_progess_record( const std::string& what, const uint64_t max )
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


void progress_stack::increment_progress()
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


void progress_stack::pop_progess_record()
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
