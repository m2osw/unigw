/*    wpkgar_build.h -- create debian packages
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
 */

/** \file
 * \brief Declaration of the build classes.
 *
 * This file declares the classes and types used to define the necessary
 * functions to create Debian like packages with the wpkg tool (or your
 * own tool if you link against this library, libdebpackages.)
 */
#ifndef WPKGAR_BUILD_H
#define WPKGAR_BUILD_H
#include    "libdebpackages/wpkgar.h"
#include    "controlled_vars/controlled_vars_auto_enum_init.h"
#include    "controlled_vars/controlled_vars_limited_auto_init.h"
#include    "controlled_vars/controlled_vars_limited_auto_enum_init.h"

#include    <memory>

namespace wpkgar
{


class DEBIAN_PACKAGE_EXPORT wpkgar_build
{
public:
    enum parameter_t
    {
        wpkgar_build_force_file_info,       // ignore errors of chown/chmod
        wpkgar_build_ignore_empty_packages, // do not generate an error on empty packages
        wpkgar_build_recursive,             // read sub-directories of repositories
        wpkgar_build_run_unit_tests         // run unit tests before creating packages
    };

    class DEBIAN_PACKAGE_EXPORT source_validation
    {
    public:
        class DEBIAN_PACKAGE_EXPORT source_property
        {
        public:
            enum status_t
            {
                SOURCE_VALIDATION_STATUS_UNKNOWN,
                SOURCE_VALIDATION_STATUS_VALID,
                SOURCE_VALIDATION_STATUS_INCOMPLETE,
                SOURCE_VALIDATION_STATUS_INVALID,
                SOURCE_VALIDATION_STATUS_MISSING
            };
            typedef controlled_vars::limited_auto_enum_init<status_t, SOURCE_VALIDATION_STATUS_UNKNOWN, SOURCE_VALIDATION_STATUS_MISSING, SOURCE_VALIDATION_STATUS_UNKNOWN>  safe_status_t;

                            source_property(); // for std::map<> support
                            source_property(const char *name, const char *help);

            const char *    get_name() const;
            const char *    get_help() const;
            void            set_status(status_t status);
            status_t        get_status() const;

            void            set_value(const std::string& value);
            bool            value_is_set() const;
            std::string     get_value() const;

        private:
            const char *                f_name;
            const char *                f_help;
            safe_status_t               f_status;
            controlled_vars::fbool_t    f_value_is_set;
            std::string                 f_value;
        };

        typedef std::map<std::string, source_property> source_properties_t;

                                    source_validation();

        void                        done(const char *name, source_property::status_t status);
        void                        set_value(const char *name, const std::string& value);
        source_property::status_t   get_status(const char *name) const;
        std::string                 get_value(const char *name) const;
        const source_properties_t&  get_properties() const;

        // for the --help and similar functionality in GUI apps
        static const wpkg_control::control_file::list_of_terms_t *list();

    private:
        source_properties_t         f_properties;
    };

    wpkgar_build( wpkgar_manager::pointer_t manager, const std::string& build_directory);

    void set_parameter(parameter_t flag, int value);
    int get_parameter(parameter_t flag, int default_value) const;
    void set_zlevel(int zlevel);
    void set_compressor(memfile::memory_file::file_format_t compressor);
    void set_path_length_limit(int limit);
    void set_extra_path(const wpkg_filename::uri_filename& extra_path);
    void set_build_number_filename(const wpkg_filename::uri_filename& filename);
    bool increment_build_number();
    bool load_build_number(int& build_number, bool quiet) const;
    void set_output_dir(const wpkg_filename::uri_filename& output_directory);
    void set_output_repository_dir(const wpkg_filename::uri_filename& output_directory);
    void set_filename(const wpkg_filename::uri_filename& filename);
    void set_install_prefix(const wpkg_filename::uri_filename& install_prefix);
    void set_cmake_generator(const std::string& generator);
    void set_make_tool(const std::string& make);
    void set_program_fullname(const std::string& program_name);
    void add_exception(const wpkg_filename::uri_filename& pattern);
    bool is_exception(const wpkg_filename::uri_filename& filename) const;
    wpkg_filename::uri_filename get_package_name() const;

    void build();

    bool validate_source(source_validation& validation_status, wpkg_control::control_file& controlinfo_fields);

private:
    typedef controlled_vars::limited_auto_init<int, 1, 9, 9> zlevel_t;
    typedef controlled_vars::limited_auto_init<int, -65536, 65536, 1024> path_limit_t;
    typedef std::vector<wpkg_filename::uri_filename> exception_vector_t;
    typedef std::map<parameter_t, int> wpkgar_flags_t;

    // disallow copying
    wpkgar_build(const wpkgar_build& rhs);
    wpkgar_build& operator = (const wpkgar_build& rhs);

    void append_file(memfile::memory_file& archive, memfile::memory_file::file_info& info, memfile::memory_file& file);
    void save_package(memfile::memory_file& debian_ar, const wpkg_control::control_file& fields);
    void prepare_cmd(std::string& cmd, const wpkg_filename::uri_filename& dir);
    bool run_cmake(const std::string& package_name, const wpkg_filename::uri_filename& build_tmpdir, const wpkg_filename::uri_filename& cwd);
    void build_source();
    void install_source_package();
    wpkg_filename::uri_filename find_source_file(const char **filenames, controlled_vars::fbool_t& rename);
    void build_project();
    void run_project_unit_tests();
    void build_project_packages();
    void build_packages();
    void build_repository();
    void build_info();
    void build_deb(const wpkg_filename::uri_filename& dir_name);

    wpkgar_manager::pointer_t           f_manager;
    zlevel_t                            f_zlevel;
    path_limit_t                        f_path_length_limit;
    controlled_vars::fbool_t            f_ignore_empty_packages;
    controlled_vars::fbool_t            f_run_tests;            // run unit tests when building a package
    controlled_vars::fbool_t            f_rename_changelog;
    controlled_vars::fbool_t            f_rename_copyright;
    controlled_vars::fbool_t            f_rename_controlinfo;
    wpkg_filename::uri_filename         f_changelog_filename;
    wpkg_filename::uri_filename         f_copyright_filename;
    wpkg_filename::uri_filename         f_controlinfo_filename;
    wpkg_filename::uri_filename         f_package_source_path;
    wpkg_filename::uri_filename         f_install_prefix;
    memfile::memory_file::file_format_t f_compressor;
    const wpkg_filename::uri_filename   f_build_directory;      // info file or directory
    wpkg_filename::uri_filename         f_output_dir;           // directory where output file go
    wpkg_filename::uri_filename         f_output_repository_dir;// directory where output file go, also using the Distribution & Component fields
    wpkg_filename::uri_filename         f_filename;             // base filename (if empty, generated name is used)
    wpkg_filename::uri_filename         f_package_name;         // package file name that gets written to
    wpkg_filename::uri_filename         f_extra_path;           // input directory (info) or output directory (directory)
    wpkg_filename::uri_filename         f_build_number_filename;// file with a number used as the build number
    exception_vector_t                  f_exceptions;           // files to never include in tarballs
    wpkgar_flags_t                      f_flags;
    std::string                         f_cmake_generator;
    std::string                         f_make_tool;
    std::string                         f_program_fullname;
};

}   // namespace wpkgar

#endif
//#ifndef WPKGAR_BUILD_H
// vim: ts=4 sw=4 et
