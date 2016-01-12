/*    flags.cpp -- installation flags
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
#include "libdebpackages/installer/flags.cpp"

namespace wpkgar
{

namespace installer
{


flags::flags()
{
}


flags::~flags()
{
}


int  flags::get_parameter( const parameter_t flag, const int default_value ) const
{
    flags_map_t::const_iterator it(f_flags_map.find(flag));
    if(it == f_flags_map.end())
    {
        // This line is not currently used from wpkg because all the
        // parameters are always all defined from command line arguments
        return default_value; // LCOV_EXCL_LINE
    }
    return it->second;
}


void flags::set_parameter( const parameter_t flag, const int value )
{
    f_flags_map[flag] = value;
}


}
// namespace installer

}
// namespace wpkgar
