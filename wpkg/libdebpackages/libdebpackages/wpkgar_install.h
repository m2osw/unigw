/*    wpkgar_install.h -- install debian packages
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
 * \brief Declaration of the install function of the library.
 *
 * Packages can be installed in a target. These declarations are used to
 * define a class used to install packages on a target. Internally, this is
 * very complex as you can see by the large number of validation functions.
 * The validation functions actually all have "side effects" in that they
 * actually compute the final tree of packages to be installed or find out
 * that the specified packages cannot be installed.
 */
#ifndef WPKGAR_INSTALL_H
#define WPKGAR_INSTALL_H
#include    "libdebpackages/wpkgar.h"

namespace wpkg_backup
{
class wpkgar_backup;
}

namespace wpkgar
{

namespace details
{
class disk_list_t;
}

class DEBIAN_PACKAGE_EXPORT wpkgar_install
{
public:
    // returned by unpack()
    static const int    WPKGAR_ERROR = -1;
    static const int    WPKGAR_EOP = -2;

    enum parameter_t
    {
        wpkgar_install_force_architecture,      // allow installation whatever the architecture
        wpkgar_install_force_breaks,            // allow installation with breaks
        wpkgar_install_force_configure_any,     // allow auto-configuration of unpacked packages
        wpkgar_install_force_conflicts,         // allow installation with conflicts
        wpkgar_install_force_depends,           // allow installation with missing dependencies
        wpkgar_install_force_depends_version,   // allow installation with wrong versions
        wpkgar_install_force_distribution,      // allow installation of packages without a distribution field
        wpkgar_install_force_downgrade,         // allow updates of older versions of packages
        wpkgar_install_force_file_info,         // allow chmod()/chown() failures
        wpkgar_install_force_hold,              // allow upgrades/downgrades of held packages
        wpkgar_install_force_overwrite,         // allow new packages to overwrite existing files
        wpkgar_install_force_overwrite_dir,     // allow new packages to overwrite existing directories
        wpkgar_install_force_rollback,          // do a rollback on error
        wpkgar_install_force_upgrade_any_version,   // allow upgrading even if a Minimum-Upgradable-Version is defined
        wpkgar_install_force_vendor,            // allow installing of incompatible vendor names
        wpkgar_install_quiet_file_info,         // do not print chmod/chown warnings
        wpkgar_install_recursive,               // read sub-directories of repositories
        wpkgar_install_skip_same_version        // do not re-install over itself
    };

    class install_info_t
    {
    public:
        friend class wpkgar_install;

        enum install_type_t
        {
            install_type_undefined, // undefined
            install_type_explicit,  // explicitly asked for
            install_type_implicit   // necessary to satisfy dependencies
        };

        install_info_t();

        std::string    get_name()         const;
        std::string    get_version()      const;
        install_type_t get_install_type() const;
        bool           is_upgrade()       const;

    private:
        std::string                 f_name;
        std::string                 f_version;
        install_type_t              f_install_type;
        controlled_vars::fbool_t    f_is_upgrade;
    };

    wpkgar_install(wpkgar_manager *manager);

    typedef std::vector<install_info_t> install_info_list_t;
    install_info_list_t get_install_list();

    void set_parameter(parameter_t flag, int value);
    int get_parameter(parameter_t flag, int default_value) const;
    void set_installing();
    void set_configuring();
    void set_reconfiguring();
    void set_unpacking();
    void add_field_validation(const std::string& expression);
    void add_package(const std::string& package);
    const std::string& get_package_name( const int idx ) const;
    int count() const;

    bool validate();
    bool pre_configure();
    int unpack();
    bool configure(int idx);
    int reconfigure();

    // functions used internally
    bool find_essential_file(std::string filename, const size_t skip_idx);

private:
    friend class details::disk_list_t;

    class package_item_t
    {
    public:
        enum package_type_t
        {
            // command line defined
            package_type_explicit,          // requested by the administrator (command line)

            // repository defined
            package_type_implicit,          // necessary to satisfy dependencies
            package_type_available,         // not marked as necessary or invalid yet

            // installed status
            package_type_not_installed,     // package is not currently installed
            package_type_installed,         // package is installed
            package_type_unpacked,          // package is unpacked but not configured
            package_type_configure,         // package is going to be configured
            package_type_upgrade,           // package is going to be upgraded
            package_type_upgrade_implicit,  // package is implicitly upgraded to satisfy dependencies
            package_type_downgrade,         // package is going to be downgraded

            // different "invalid" states
            package_type_invalid,           // clearly determined as invalid (bad architecture, version, etc.)
            package_type_same,              // ignored because it is already installed
            package_type_older,             // removed because the version is smaller (package is older)
            package_type_directory          // this is a directory, read it once when check dependencies and then ignore
        };

        package_item_t(wpkgar_manager *manager, const wpkg_filename::uri_filename& filename, package_type_t type = package_type_explicit);
        package_item_t(wpkgar_manager *manager, const wpkg_filename::uri_filename& filename, package_type_t type, const memfile::memory_file& ctrl);

        const wpkg_filename::uri_filename& get_filename() const;
        const std::string& get_name() const;
        const std::string& get_architecture() const;
        const std::string& get_version() const;
        wpkgar_manager::package_status_t get_original_status() const;
        bool field_is_defined(const std::string& name) const;
        std::string get_field(const std::string& name) const;
        bool get_boolean_field(const std::string& name) const;
        bool validate_fields(const std::string& expression) const;
        bool is_conffile(const std::string& path) const;
        void set_type(const package_type_t type);
        package_type_t get_type() const;
        void set_upgrade(int32_t upgrade);
        int32_t get_upgrade() const;
        void mark_unpacked();
        bool is_unpacked() const;
        void copy_package_in_database();

        void load(bool ctrl);

    private:
        enum loaded_state_t
        {
            load_state_not_loaded,
            load_state_control_file,
            load_state_full
        };
        typedef controlled_vars::limited_auto_init<loaded_state_t, load_state_not_loaded, load_state_control_file, load_state_not_loaded>  safe_loaded_state_t;

        wpkgar_manager *            f_manager;
        wpkg_filename::uri_filename f_filename;
        package_type_t              f_type;
        std::shared_ptr<memfile::memory_file>          f_ctrl;
        std::shared_ptr<wpkg_control::control_file>    f_fields;
        safe_loaded_state_t         f_loaded;
        controlled_vars::fbool_t    f_depends_done;
        controlled_vars::fbool_t    f_unpacked;
        std::string                 f_name;
        std::string                 f_architecture;
        std::string                 f_version;
        wpkgar_manager::package_status_t f_original_status;
        controlled_vars::mint32_t   f_upgrade;
    };

    typedef std::map<parameter_t, int>                      wpkgar_flags_t;
    typedef std::vector<package_item_t>                     wpkgar_package_list_t;
    typedef std::vector<package_item_t *>                   wpkgar_package_ptrs_t;
    typedef std::vector<wpkgar_package_list_t::size_type>   wpkgar_package_idxs_t;
    typedef std::vector<const wpkg_dependencies::dependencies::dependency_t *>   wkgar_dependency_list_t;
    typedef std::map<std::string, bool>                     wpkgar_package_listed_t;
    typedef std::vector<std::string>                        wpkgar_list_of_strings_t;

    enum validation_return_t
    {
        validation_return_success,
        validation_return_error,
        validation_return_missing,
        validation_return_unpacked
    };

    // disallow copying
    wpkgar_install(const wpkgar_install& rhs);
    wpkgar_install& operator = (const wpkgar_install& rhs);

    wpkgar_package_list_t::const_iterator find_package_item(const wpkg_filename::uri_filename& filename) const;
    wpkgar_package_list_t::iterator find_package_item_by_name(const std::string& name);

    // validation sub-functions
    bool validate_directories();
    void validate_package_names();
    void installing_source();
    void validate_installed_packages();
    void validate_distribution();
    void validate_architecture();
    int match_dependency_version(const wpkg_dependencies::dependencies::dependency_t& d, const package_item_t& name);
    bool find_installed_predependency(const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d);
    void validate_predependencies();
    validation_return_t find_explicit_dependency(wpkgar_package_list_t::size_type index, const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d, const std::string& field_name);
    validation_return_t find_installed_dependency(wpkgar_package_list_t::size_type index, const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d, const std::string& field_name);
    void read_repositories();
    void trim_conflicts(wpkgar_package_list_t& tree, wpkgar_package_list_t::size_type idx, bool only_explicit);
    void trim_available(package_item_t& item, wpkgar_package_ptrs_t& parents);
    void trim_available_packages();
    validation_return_t validate_installed_depends_field(const wpkgar_package_list_t::size_type idx, const std::string& field_name);
    validation_return_t validate_installed_dependencies();
    bool prepare_tree(wpkgar_package_list_t& tree, int count);
    void find_best_dependency(const std::string& package_name, const wpkg_dependencies::dependencies::dependency_t& d);
    bool check_implicit_for_upgrade(wpkgar_package_list_t& tree, const wpkgar_package_list_t::size_type idx);
    void find_dependencies(wpkgar_package_list_t& tree, const wpkgar_package_list_t::size_type idx, wkgar_dependency_list_t& missing);
    bool verify_tree(wpkgar_package_list_t& tree, wkgar_dependency_list_t& missing);
    int compare_trees(const wpkgar_package_list_t& left, const wpkgar_package_list_t& right);
    void output_tree(int count, const wpkgar_package_list_t& tree, const std::string& sub_title);
    void validate_dependencies();
    void validate_packager_version();
    void validate_installed_size_and_overwrite();
    void validate_fields();
    void validate_scripts();
    void sort_package_dependencies(const std::string& name, wpkgar_package_listed_t& listed);
    void sort_packages();

    // unpack sub-functions
    bool preupgrade_scripts(package_item_t *item, package_item_t *upgrade);
    bool postupgrade_scripts(package_item_t *item, package_item_t *upgrade, wpkg_backup::wpkgar_backup& backup);
    void cancel_upgrade_scripts(package_item_t *item, package_item_t *upgrade, wpkg_backup::wpkgar_backup& backup);
    bool preinst_scripts(package_item_t *item, package_item_t *upgrade, package_item_t *& conf_install);
    void cancel_install_scripts(package_item_t *item, package_item_t *conf_install, wpkg_backup::wpkgar_backup& backup);
    void set_status(package_item_t *item, package_item_t *upgrade, package_item_t *conf_install, const std::string& status);
    bool do_unpack(package_item_t *item, package_item_t *upgrade);
    void unpack_file(package_item_t *item, const wpkg_filename::uri_filename& destination, const memfile::memory_file::file_info& info);

    // configuration sub-functions
    bool configure_package(package_item_t *item);

    wpkgar_manager *                    f_manager;
    wpkgar_manager::package_list_t      f_list_installed_packages;
    wpkgar_flags_t                      f_flags;
    std::string                         f_architecture;
    wpkgar_manager::package_status_t    f_original_status;
    wpkgar_package_list_t               f_packages;
    wpkgar_package_idxs_t               f_sorted_packages;
    controlled_vars::tbool_t            f_installing_packages;
    controlled_vars::fbool_t            f_unpacking_packages;
    controlled_vars::fbool_t            f_reconfiguring_packages;
    controlled_vars::fbool_t            f_repository_packages_loaded;
    controlled_vars::fbool_t            f_install_includes_choices;
    controlled_vars::zuint32_t          f_install_choices;
    controlled_vars::zuint32_t          f_tree_max_depth;
    wpkgar_list_of_strings_t            f_essential_files;
    wpkgar_list_of_strings_t            f_field_validations;
    wpkgar_list_of_strings_t            f_field_names;
    controlled_vars::fbool_t            f_read_essentials;
    controlled_vars::fbool_t            f_install_source;
};

}   // namespace wpkgar

#endif
//#ifndef WPKGAR_INSTALL_H
// vim: ts=4 sw=4 et
