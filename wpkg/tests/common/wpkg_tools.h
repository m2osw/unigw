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

namespace test_common
{


class obj_setenv
{
public:
	obj_setenv(const std::string& var)
		: f_copy(strdup(var.c_str()))
	{
		putenv(f_copy);
		std::string::size_type p(var.find_first_of('='));
		f_name = var.substr(0, p);
	}
	~obj_setenv()
	{
		putenv(strdup((f_name + "=").c_str()));
		free(f_copy);
	}

private:
	char *		f_copy;
	std::string	f_name;
};


class wpkg_tools
{
public:
    wpkg_tools();
    virtual ~wpkg_tools();

    static void set_tmp_dir  ( const std::string& val ) { f_tmp_dir   = val; }
    static void set_wpkg_tool( const std::string& val ) { f_wpkg_tool = val; }

protected:
    typedef std::vector<std::string>                    string_list_t;
    typedef std::shared_ptr<wpkg_control::control_file> control_file_pointer_t;

    static std::string   f_tmp_dir;
    static std::string   f_wpkg_tool;

    std::string             escape_string( const std::string& orig_field );
    control_file_pointer_t  get_new_control_file(const std::string& test_name);

    void    create_file     ( wpkg_control::file_list_t& files, wpkg_control::file_list_t::size_type idx, wpkg_filename::uri_filename path );
    void    create_package  ( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, int expected_return_value, bool reset_wpkg_dir = true);
    void    install_package ( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl );
    void    install_package ( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, int expected_return_value );
    void    remove_package  ( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl );
    void    remove_package  ( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, int expected_return_value );
    void    purge_package   ( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl );
    void    purge_package   ( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, int expected_return_value );

    int     execute_cmd( const std::string& cmd );
};

}
// namespace test_common

// vim: ts=4 sw=4 et
