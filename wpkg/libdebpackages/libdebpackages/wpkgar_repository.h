/*    wpkgar_repository.h -- manage a repository of debian packages
 *    Copyright (C) 2013-2015  Made to Order Software Corporation
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
 * \brief Declarations for repository handling.
 *
 * The repository class is used to compute what needs to be upgraded from
 * a list of sources that it also handles.
 *
 * This class is used by the different --update and --upgrade commands
 * of wpkg.
 */
#ifndef WPKGAR_REPOSITORY_H
#define WPKGAR_REPOSITORY_H
#include    "libdebpackages/wpkgar.h"
#include    "controlled_vars/controlled_vars_auto_enum_init.h"

namespace wpkgar
{

class DEBIAN_PACKAGE_EXPORT wpkgar_repository
{
public:
    enum parameter_t
    {
        wpkgar_repository_recursive              // read sub-directories of repositories
    };

    class DEBIAN_PACKAGE_EXPORT index_entry
    {
    public:
        memfile::memory_file::file_info         f_info;
        std::shared_ptr<memfile::memory_file>   f_control;
    };
    typedef std::vector<index_entry> entry_vector_t;

    class DEBIAN_PACKAGE_EXPORT source
    {
    public:
        typedef std::map<std::string, std::string> parameter_map_t;

        std::string get_type() const;
        std::string get_parameter(const std::string& name, const std::string& def_value = "") const;
        parameter_map_t get_parameters() const;
        std::string get_uri() const;
        std::string get_distribution() const;
        int get_component_size() const;
        std::string get_component(int index) const;

        void set_type(const std::string& type);
        void add_parameter(const std::string& name, const std::string& value);
        void set_uri(const std::string& uri);
        void set_distribution(const std::string& distribution);
        void add_component(const std::string& component);

    private:
        typedef std::vector<std::string> component_vector_t;

        std::string             f_type;
        parameter_map_t         f_parameters;
        std::string             f_uri;
        std::string             f_distribution;
        component_vector_t      f_components;
    };
    typedef std::vector<source> source_vector_t;

    class DEBIAN_PACKAGE_EXPORT update_entry_t
    {
    public:
        enum update_entry_status_t
        {
            status_unknown,         // not yet checked
            status_ok,              // last update worked
            status_failed           // last update failed
        };
        enum update_entry_time_t
        {
            first_try,
            first_success,
            last_success,
            last_failure,

            time_max
        };

        int32_t get_index() const;
        update_entry_status_t get_status() const;
        std::string get_uri() const;
        time_t get_time(update_entry_time_t t) const;

        void set_index(int index);
        void set_status(update_entry_status_t status);
        void set_uri(const std::string& uri);
        void update_time(time_t t);

        void from_string(const std::string& line);
        std::string to_string() const;

    private:
        typedef controlled_vars::auto_init<time_t> ztime_t;
        typedef controlled_vars::limited_auto_enum_init<update_entry_status_t, status_unknown, status_failed, status_unknown> zstatus_t;

        controlled_vars::zint32_t       f_index; // local file number, starting at 1, if 0, not initialized yet
        zstatus_t                       f_status;
        std::string                     f_uri;
        ztime_t                         f_times[time_max];
    };
    typedef std::vector<update_entry_t>      update_entry_vector_t;

    class DEBIAN_PACKAGE_EXPORT package_item_t
    {
    public:
        enum package_item_status_t
        {
            not_installed,
            need_upgrade,
            blocked_upgrade,  // package is on hold
            installed,
            invalid
        };

        package_item_t(wpkgar_manager *manager, const memfile::memory_file::file_info& info, const memfile::memory_file& data);

        void check_installed_package(bool exists);

        package_item_status_t get_status() const;
        const memfile::memory_file::file_info& get_info() const;
        std::string get_name() const;
        std::string get_architecture() const;
        std::string get_version() const;
        std::string get_field(const std::string& name) const;
        bool field_is_defined(const std::string& name) const;
        std::string get_cause_for_rejection() const;

    private:
        typedef controlled_vars::limited_auto_enum_init<package_item_status_t, not_installed, invalid, invalid> safe_package_item_status_t;

        wpkgar_manager *                            f_manager;
        safe_package_item_status_t                  f_status;
        memfile::memory_file::file_info             f_info;
        std::shared_ptr<wpkg_control::control_file> f_control;
        std::string                                 f_cause_for_rejection;
    };
    typedef std::vector<package_item_t>         wpkgar_package_list_t;

    wpkgar_repository(wpkgar_manager *manager);

    void set_parameter(parameter_t flag, int value);
    int get_parameter(parameter_t flag, int default_value) const;

    void create_index(memfile::memory_file& index_file);
    static void load_index(const memfile::memory_file& file, entry_vector_t& entries);

    void read_sources(const memfile::memory_file& filename, source_vector_t& sources);
    void write_sources(memfile::memory_file& file, const source_vector_t& sources);

    void update();
    const update_entry_vector_t *load_index_list();
    const wpkgar_package_list_t& upgrade_list();

private:
    typedef std::map<parameter_t, int>                      wpkgar_flags_t;

    // disallow copying
    wpkgar_repository(const wpkgar_repository& rhs);
    wpkgar_repository& operator = (const wpkgar_repository& rhs);

    bool next_source(source& src) const;
    void update_index(const wpkg_filename::uri_filename& uri);
    void save_index_list() const;
    void upgrade_index(size_t i, memfile::memory_file& index_file);
    bool is_installed_package(const std::string& name) const;

    wpkgar_manager *                    f_manager;
    wpkgar_flags_t                      f_flags;
    wpkgar_package_list_t               f_packages;
    controlled_vars::fbool_t            f_repository_packages_loaded;
    update_entry_vector_t               f_update_index;
    wpkgar_manager::package_list_t      f_installed_packages;
};

}   // namespace wpkgar
#endif
//#ifndef WPKGAR_REPOSITORY_H
// vim: ts=4 sw=4 et
