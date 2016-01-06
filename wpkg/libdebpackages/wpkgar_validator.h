/*    wpkgar_validator.h -- install debian packages
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
 *    Doug Barbieri  doug@dooglio.net
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
#pragma once

#include    "libdebpackages/wpkgar.h"
#include    "libdebpackages/wpkgar_repository.h"
#include    "libdebpackages/wpkg_dependencies.h"
#include    "libdebpackages/installer/install_info.h"
#include    "libdebpackages/installer/package_item.h"
#include    "libdebpackages/installer/tree_generator.h"
#include    "controlled_vars/controlled_vars_auto_enum_init.h"

#include <functional>
#include <stack>

namespace wpkgar
{

#if !defined(MO_DARWIN) && !defined(MO_SUNOS) && !defined(MO_FREEBSD)
namespace details
{
class DEBIAN_PACKAGE_EXPORT disk_list_t;
}
#endif

class DEBIAN_PACKAGE_EXPORT validator
    : public std::enable_shared_from_this<validator>
{
public:
    // returned by unpack()
    static const int WPKGAR_ERROR = -1;
    static const int WPKGAR_EOP   = -2;

    enum parameter_t
    {
        validator_force_architecture,      // allow installation whatever the architecture
        validator_force_breaks,            // allow installation with breaks
        validator_force_configure_any,     // allow auto-configuration of unpacked packages
        validator_force_conflicts,         // allow installation with conflicts
        validator_force_depends,           // allow installation with missing dependencies
        validator_force_depends_version,   // allow installation with wrong versions
        validator_force_distribution,      // allow installation of packages without a distribution field
        validator_force_downgrade,         // allow updates of older versions of packages
        validator_force_file_info,         // allow chmod()/chown() failures
        validator_force_hold,              // allow upgrades/downgrades of held packages
        validator_force_overwrite,         // allow new packages to overwrite existing files
        validator_force_overwrite_dir,     // allow new packages to overwrite existing directories
        validator_force_rollback,          // do a rollback on error
        validator_force_upgrade_any_version,   // allow upgrading even if a Minimum-Upgradable-Version is defined
        validator_force_vendor,            // allow installing of incompatible vendor names
        validator_quiet_file_info,         // do not print chmod/chown warnings
        validator_recursive,               // read sub-directories of repositories
        validator_skip_same_version        // do not re-install over itself
    };

    enum task_t
    {
        task_installing_packages,
        task_configuring_packages,
        task_reconfiguring_packages,
        task_unpacking_packages
    };

    typedef std::shared_ptr<validator> pointer_t;

    validator( wpkgar_manager::pointer_t manager );

    void set_parameter(parameter_t flag, int value);
    int get_parameter(parameter_t flag, int default_value) const;

    bool get_task() const;
    void set_task( task_t task );

    void add_field_validation(const std::string& expression);
    void add_package( const std::string& package, const std::string& version = std::string(), const bool force_reinstall = false );
    void add_package( wpkgar_repository::package_item_t entry, const bool force_reinstall = false );
    const std::string& get_package_name( const int idx ) const;
    int count() const;

    // functions used internally
    bool find_essential_file(std::string filename, const size_t skip_idx);

    wpkg_output::progress_record_t get_current_progress() const;

#if !defined(MO_DARWIN) && !defined(MO_SUNOS) && !defined(MO_FREEBSD)
    friend class details::disk_list_t;
#endif

    typedef std::map<parameter_t, int>                                    wpkgar_flags_t;
    typedef installer::package_item_t::list_t                             wpkgar_package_list_t;
    typedef std::vector<installer::package_item_t *>                      wpkgar_package_ptrs_t;
    typedef std::vector<wpkg_dependencies::dependencies::dependency_t>    wpkgar_dependency_list_t;
    typedef std::map<std::string, bool>                                   wpkgar_package_listed_t;
    typedef std::vector<std::string>                                      wpkgar_list_of_strings_t;

    enum validation_return_t
    {
        validation_return_success,
        validation_return_error,
        validation_return_missing,
        validation_return_held,
        validation_return_unpacked
    };

    // disallow copying
    validator(const validator& rhs);
    validator& operator = (const validator& rhs);

    wpkgar_package_list_t::const_iterator find_package_item(const wpkg_filename::uri_filename& filename) const;
    wpkgar_package_list_t::iterator       find_package_item_by_name(const std::string& name);

    // validation sub-functions
    void validate_directory( installer::package_item_t package );
    bool validate_packages_to_install();
    bool validate_directories();
    void validate_package_name( installer::package_item_t& pkg );
    void validate_package_names();
    void installing_source();
    void validate_installed_package( const std::string& pkg );
    void validate_installed_packages();
    void validate_distribution_package( const installer::package_item_t& package );
    void validate_distribution();
    void validate_architecture_package( installer::package_item_t& pkg );
    void validate_architecture();
    int match_dependency_version(const wpkg_dependencies::dependencies::dependency_t& d, const installer::package_item_t& name);
    bool find_installed_predependency_package( installer::package_item_t& pkg, const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d);
    bool find_installed_predependency(const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d);
    void validate_predependencies();
    validation_return_t find_explicit_dependency(wpkgar_package_list_t::size_type index, const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d, const std::string& field_name);
    validation_return_t find_installed_dependency(wpkgar_package_list_t::size_type index, const wpkg_filename::uri_filename& package_name, const wpkg_dependencies::dependencies::dependency_t& d, const std::string& field_name);
    bool read_repository_index( const wpkg_filename::uri_filename& repo_filename, memfile::memory_file& index_file );
    void read_repositories();
    void trim_conflicts
        ( const bool check_available
        , const bool only_explicit
        , wpkg_filename::uri_filename filename
        , installer::package_item_t::package_type_t idx_type
        , installer::package_item_t& parent_package
        , installer::package_item_t& depends_package
        , const wpkg_dependencies::dependencies::dependency_t& dependency
        );
    void trim_breaks
        ( const bool check_available
        , const bool only_explicit
        , wpkg_filename::uri_filename filename
        , installer::package_item_t::package_type_t idx_type
        , installer::package_item_t& parent_package
        , installer::package_item_t& depends_package
        , const wpkg_dependencies::dependencies::dependency_t& dependency
        );
    void trim_conflicts( wpkgar_package_list_t& tree, wpkgar_package_list_t::size_type idx, bool only_explicit );
    bool trim_dependency
        ( installer::package_item_t& item
        , wpkgar_package_ptrs_t& parents
        , const wpkg_dependencies::dependencies::dependency_t& dependency
        , const std::string& field_name
        );
    void trim_available(installer::package_item_t& item, wpkgar_package_ptrs_t& parents);
    void trim_available_packages();
    validation_return_t validate_installed_depends_field(const wpkgar_package_list_t::size_type idx, const std::string& field_name);
    validation_return_t validate_installed_dependencies();
    void find_best_dependency(const std::string& package_name, const wpkg_dependencies::dependencies::dependency_t& d);
    bool check_implicit_for_upgrade(wpkgar_package_list_t& tree, const wpkgar_package_list_t::size_type idx);
    void find_dependencies( wpkgar_package_list_t& tree, const wpkgar_package_list_t::size_type idx, wpkgar_dependency_list_t& missing, wpkgar_dependency_list_t& held );
    bool verify_tree( wpkgar_package_list_t& tree, wpkgar_dependency_list_t& missing, wpkgar_dependency_list_t& held );
    bool trees_are_practically_identical(const wpkgar_package_list_t& left, const wpkgar_package_list_t& right) const;
    int compare_trees(const wpkgar_package_list_t& left, const wpkgar_package_list_t& right) const;
    void output_tree(int count, const wpkgar_package_list_t& tree, const std::string& sub_title);
    void validate_dependencies();
    void validate_packager_version();
    void validate_installed_size_and_overwrite();
    void validate_fields();
    void validate_scripts();
    void sort_package_dependencies(const std::string& name, wpkgar_package_listed_t& listed);
    void sort_packages();

    wpkg_control::control_file::field_xselection_t::selection_t get_xselection( const wpkg_filename::uri_filename& filename ) const;
    wpkg_control::control_file::field_xselection_t::selection_t get_xselection( const std::string& filename ) const;

    // configuration sub-functions
    bool configure_package(installer::package_item_t *item);

private:
    wpkgar_manager::pointer_t                 f_manager;
    wpkgar_manager::package_list_t            f_list_installed_packages;
    wpkgar_flags_t                            f_flags;
    std::string                               f_architecture;
    wpkgar_manager::package_status_t          f_original_status;
    wpkgar_package_list_t                     f_packages;
    installer::tree_generator::package_idxs_t f_sorted_packages;
    task_t                                    f_task;
    controlled_vars::fbool_t                  f_repository_packages_loaded;
    controlled_vars::fbool_t                  f_install_includes_choices;
    controlled_vars::zuint32_t                f_tree_max_depth;
    wpkgar_list_of_strings_t                  f_essential_files;
    wpkgar_list_of_strings_t                  f_field_validations;
    wpkgar_list_of_strings_t                  f_field_names;
    controlled_vars::fbool_t                  f_read_essentials;
    controlled_vars::fbool_t                  f_install_source;

    class progress_scope
    {
    public:
        progress_scope( validator* inst
                      , const std::string& what
                      , const uint64_t max );
        ~progress_scope();

    private:
        validator* f_installer;
    };

    friend class progress_scope;

    void add_progess_record( const std::string& what, const uint64_t max );
    void pop_progess_record();
    void increment_progress();

    typedef std::stack<wpkg_output::progress_record_t> progress_stack_t;
    progress_stack_t                    f_progress_stack;
};

}   // namespace wpkgar

// vim: ts=4 sw=4 et
