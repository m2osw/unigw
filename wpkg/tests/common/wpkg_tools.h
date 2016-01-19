/*    wpkg_tools.h
 *    Copyright (C) 2013-2016  Made to Order Software Corporation
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

#include "libdebpackages/wpkg_control.h"

#include <memory>
#include <string>
#include <vector>

namespace test_common
{


class obj_setenv
{
public:
    obj_setenv(const std::string& var);
    ~obj_setenv();

private:
	char *		f_copy;
	std::string	f_name;
};


class wpkg_tools
{
public:
    wpkg_tools();
    virtual ~wpkg_tools();

    static std::string get_tmp_dir  () { return f_tmp_dir; }
    static std::string get_wpkg_tool() { return f_wpkg_tool; }
    //
    static void set_tmp_dir  ( const std::string& val ) { f_tmp_dir   = val; }
    static void set_wpkg_tool( const std::string& val ) { f_wpkg_tool = val; }

protected:
    typedef std::vector<std::string>                    string_list_t;
    typedef std::shared_ptr<wpkg_control::control_file> control_file_pointer_t;

    static wpkg_filename::uri_filename get_root();
    static wpkg_filename::uri_filename get_target_path();
    static wpkg_filename::uri_filename get_database_path();
    static wpkg_filename::uri_filename get_repository();

    std::string             escape_string( const std::string& orig_field );
    control_file_pointer_t  get_new_control_file(const std::string& test_name);
    std::string 			get_package_file_name( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl );

    void	init_database( control_file_pointer_t ctrl = control_file_pointer_t() );

    void    create_file     ( wpkg_control::file_list_t& files, wpkg_control::file_list_t::size_type idx, wpkg_filename::uri_filename path );
    void    create_package  ( const std::string& name, control_file_pointer_t ctrl, bool reset_wpkg_dir = true);
    void    create_package  ( const std::string& name, control_file_pointer_t ctrl, int expected_return_value, bool reset_wpkg_dir = true);
    void    install_package ( const std::string& name, control_file_pointer_t ctrl );
    void    install_package ( const std::string& name, control_file_pointer_t ctrl, int expected_return_value );
    void    remove_package  ( const std::string& name, control_file_pointer_t ctrl );
    void    remove_package  ( const std::string& name, control_file_pointer_t ctrl, int expected_return_value );
    void    purge_package   ( const std::string& name, control_file_pointer_t ctrl );
    void    purge_package   ( const std::string& name, control_file_pointer_t ctrl, int expected_return_value );

    int     execute_cmd( const std::string& cmd );

private:
    static std::string   f_tmp_dir;
    static std::string   f_wpkg_tool;
};

}
// namespace test_common

// vim: ts=4 sw=4 et
