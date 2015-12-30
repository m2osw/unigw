/*    wpkgar_package.h -- declaration of the wpkg package class
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

#include "libdebpackages/memfile.h"
#include "libdebpackages/wpkg_control.h"
#include "libdebpackages/wpkg_filename.h"

#include <memory>
#include <string>


namespace wpkgar
{

/** \brief The archive package holder.
 *
 * The package manager reads packages and saves them in a wpkgar_package
 * so as to have access to them quickly when referenced again.
 *
 * This class handles one package and the wpkgar_manager handles all the
 * packages that you ever loaded in a session.
 *
 * This class is private as the callers are only expected to directly
 * deal with the package files and not this class. This is just to have
 * a list of packages the manager can handle.
 */
class wpkgar_package
{
public:
    wpkgar_package(
        const wpkg_filename::uri_filename& fullname,
        std::shared_ptr<wpkg_control::control_file::control_file_state_t> control_file_state);

    void get_wpkgar_file(memfile::memory_file *& wpkgar_file_out);
    void set_package_path(const wpkg_filename::uri_filename& path);
    const wpkg_filename::uri_filename& get_package_path() const;
    const wpkg_filename::uri_filename& get_fullname() const;
    void check_contents();
    void set_field_variable(const std::string& name, const std::string& value);

    void read_archive(memfile::memory_file& p);
    void read_package();
    bool has_control_file(const std::string& filename);
    void read_control_file(memfile::memory_file& p, std::string& filename, bool compress);
    bool validate_fields(const std::string& expression);
    bool load_conffiles();
    void conffiles(std::vector<std::string>& conf_files);
    bool is_conffile(const std::string& filename);

    // the control file is read-only from the outside!
    // (the package system will add fields as required when creating packages)
    const wpkg_control::control_file& get_control_file_info() const;
    wpkg_control::control_file& get_status_file_info();

private:
    // avoid copies
    wpkgar_package(const wpkgar_package& rhs);
    wpkgar_package& operator = (const wpkgar_package& rhs);

    void read_control(memfile::memory_file& p);
    void read_data(memfile::memory_file& p);

    /** \brief Class used to memorize the location of files in an archive.
     *
     * This file is used to memorize the location where a file is in an
     * archive so later we can read that file with one instant seek instead
     * of searching the whole archive directory.
     */
    class wpkgar_file
    {
    public:
        wpkgar_file(int offset, const memfile::memory_file::file_info& info);

        int get_offset() const;

        void set_data_dir_pos(int pos);
        int get_data_dir_pos() const;

    private:
        // avoid copies
        wpkgar_file(const wpkgar_file& rhs);
        wpkgar_file& operator = (const wpkgar_file& rhs);

        controlled_vars::zbool_t        f_modified;
        controlled_vars::mint32_t       f_offset;
        controlled_vars::zint32_t       f_data_dir_pos;
        memfile::memory_file::file_info f_info;
    };

    typedef std::map<std::string, std::shared_ptr<wpkgar_file> >    file_t;
    typedef std::map<std::string, int>                              conffiles_t;

    wpkg_filename::uri_filename       f_package_path;
    wpkg_filename::uri_filename       f_fullname;
    controlled_vars::zbool_t          f_modified;          // if one or more files were modified
    controlled_vars::zbool_t          f_conffiles_defined;
    conffiles_t                       f_conffiles;
    file_t                            f_files;
    memfile::memory_file              f_wpkgar_file;
    wpkg_control::binary_control_file f_control_file;      // control fields
    wpkg_control::status_control_file f_status_file;       // control fields in the status file
};

}
// namespace wpkgar

// vim: ts=4 sw=4 et
