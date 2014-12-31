/*    wpkgar_remove.h -- remove debian packages
 *    Copyright (C) 2012-2014  Made to Order Software Corporation
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
 * \brief Declaration of the class used to handle package removals.
 *
 * This file declares the remove class which is used to deconfigure,
 * remove, and purge packages.
 */
#ifndef WPKGAR_REMOVE_H
#define WPKGAR_REMOVE_H
#include    "libdebpackages/wpkgar.h"

namespace wpkgar {

class DEBIAN_PACKAGE_EXPORT wpkgar_remove
{
public:
    // returned by remove()
    static const int    WPKGAR_ERROR = -1;
    static const int    WPKGAR_EOP = -2;

    enum parameter_t {
        wpkgar_remove_force_depends,            // allow removal even if it breaks dependencies
        wpkgar_remove_force_hold,               // allow removal of packages on hold
        wpkgar_remove_force_remove_essentials,  // allow removal of essential packages
        wpkgar_remove_recursive                 // allow auto-deletion of all dependencies
    };

    wpkgar_remove(wpkgar_manager *manager);

    void set_parameter(parameter_t flag, int value);
    int get_parameter(parameter_t flag, int default_value) const;
    void set_instdir(const std::string& instdir);
    void set_purging();
    bool get_purging() const;
    void set_deconfiguring();
    bool get_deconfiguring() const;
    void add_package(const std::string& package);
    std::string get_package_name( const int i ) const;
    int count() const;

    bool validate();
    int remove();
    void autoremove(bool dryrun);
    bool deconfigure(int idx);

private:
    class DEBIAN_PACKAGE_EXPORT package_item_t
    {
    public:
        enum package_type_t
        {
            // command line defined
            package_type_explicit,          // requested by the administrator (command line)
            package_type_removing,          // explicit that is valid

            // installed status
            package_type_not_installed,     // package is not currently installed
            package_type_installed,         // package is installed
            package_type_unpacked,          // package is unpacked but not configured
            package_type_configured,        // package is configured
            package_type_implicit,          // must be removed to satisfy dependencies
            package_type_need_repair,       // package needs repair (if being removed, we're good)

            // different "invalid" states
            package_type_invalid,           // clearly determined as invalid (bad architecture, version, etc.)
            package_type_same               // keep this as is (already removed, in most cases)
        };

        package_item_t(wpkgar_manager *manager, const std::string& filename, package_type_t type);

        const std::string& get_filename() const;
        const std::string& get_name() const;
        const std::string& get_architecture() const;
        const std::string& get_version() const;
        wpkgar_manager::package_status_t get_original_status() const;
        void reset_original_status();
        void set_type(const package_type_t type);
        package_type_t get_type() const;
        void restore_original_status();
        void set_upgrade(int32_t upgrade);
        int32_t get_upgrade() const;
        void mark_removed();
        bool is_removed() const;
        void mark_configured();
        bool is_configured() const;
        bool get_installed() const;
        void copy_package_in_database(const std::string& status);

    private:
        void load();

        wpkgar_manager *            f_manager;
        std::string                 f_filename;
        std::string                 f_new_filename;
        package_type_t              f_type;
        controlled_vars::fbool_t    f_depends_done;
        controlled_vars::fbool_t    f_loaded;
        controlled_vars::fbool_t    f_removed;
        controlled_vars::fbool_t    f_configured;
        controlled_vars::fbool_t    f_installed;
        std::string                 f_name;
        std::string                 f_architecture;
        std::string                 f_version;
        std::string                 f_status;
        wpkgar_manager::package_status_t f_original_status;
        controlled_vars::mint32_t   f_upgrade;
    };

    typedef std::map<parameter_t, int>                      wpkgar_flags_t;
    typedef std::vector<package_item_t>                     wpkgar_package_list_t;
    typedef std::vector<package_item_t *>                   wpkgar_package_ptrs_t;
    typedef std::vector<const wpkg_dependencies::dependencies::dependency_t *>   wkgar_dependency_list_t;

    enum validation_return_t {
        validation_return_success,
        validation_return_error,
        validation_return_missing,
        validation_return_unpacked
    };

    // disallow copying
    wpkgar_remove(const wpkgar_remove& rhs);
    wpkgar_remove& operator = (const wpkgar_remove& rhs);

    wpkgar_package_list_t::const_iterator find_package_item(const std::string& filename) const;
    wpkgar_package_list_t::iterator find_package_item_by_name(const std::string& name);

    // validation sub-functions
    void validate_package_names();
    void validate_explicit_packages();
    void validate_installed_packages();
    int match_dependency_version(const wpkg_dependencies::dependencies::dependency_t& d, const std::string& name);
    bool can_package_be_removed(const std::string& filename, bool cannot_force);
    void validate_removal();
    validation_return_t validate_installed_dependencies();
    void output_tree(int count, const wpkgar_package_list_t& tree, const std::string& sub_title);
    void validate_dependencies();
    void validate_scripts();

    // unpack sub-functions
    bool prerm_scripts(package_item_t *item, const std::string& command);
    bool do_remove(package_item_t *item);

    // deconfiguration sub-functions
    bool deconfigure_package(package_item_t *item);

    wpkgar_manager *                    f_manager;
    wpkgar_flags_t                      f_flags;
    std::string                         f_instdir;
    wpkgar_package_list_t               f_packages;
    controlled_vars::fbool_t            f_purging_packages;
    controlled_vars::fbool_t            f_deconfiguring_packages;
};

}   // namespace wpkgar

#endif
//#ifndef WPKGAR_REMOVE_H
// vim: ts=4 sw=4 et
