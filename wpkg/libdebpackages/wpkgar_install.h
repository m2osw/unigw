/*    wpkgar_install.h -- install debian packages
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
#include    "libdebpackages/installer/dependencies.h"
#include    "libdebpackages/installer/flags.h"
#include    "libdebpackages/installer/install_info.h"
#include    "libdebpackages/installer/package_item.h"
#include    "libdebpackages/installer/package_list.h"
#include    "libdebpackages/installer/progress_scope.h"
#include    "libdebpackages/installer/task.h"
#include    "libdebpackages/installer/tree_generator.h"
#include    "controlled_vars/controlled_vars_auto_enum_init.h"

#include <functional>
#include <stack>

namespace wpkg_backup
{
class DEBIAN_PACKAGE_EXPORT wpkgar_backup;
}

namespace wpkgar
{

#if !defined(MO_DARWIN) && !defined(MO_SUNOS) && !defined(MO_FREEBSD)
namespace details
{
class DEBIAN_PACKAGE_EXPORT disk_list_t;
}
#endif

class DEBIAN_PACKAGE_EXPORT wpkgar_install
    : public std::enable_shared_from_this<wpkgar_install>
{
public:
    // returned by unpack()
    static const int WPKGAR_ERROR;
    static const int WPKGAR_EOP;

    typedef std::shared_ptr<wpkgar_install> pointer_t;

    wpkgar_install( wpkgar_manager::pointer_t manager );

    wpkgar_manager::pointer_t          get_manager()      const;
    installer::install_info_list_t     get_install_list() const;
    installer::flags::pointer_t        get_flags()        const;
    installer::package_list::pointer_t get_package_list() const;

    void set_installing();
    void set_configuring();
    void set_reconfiguring();
    void set_unpacking();

    void add_field_validation(const std::string& expression);

    int  count() const;
    bool validate();
    bool pre_configure();
    int  unpack();
    bool configure(int idx);
    int  reconfigure();

private:
#if !defined(MO_DARWIN) && !defined(MO_SUNOS) && !defined(MO_FREEBSD)
    friend class details::disk_list_t;
#endif

    //typedef std::map<parameter_t, int>                                    wpkgar_flags_t;
    typedef installer::package_item_t::list_t                             wpkgar_package_list_t;
    typedef std::vector<installer::package_item_t *>                      wpkgar_package_ptrs_t;
    typedef std::vector<wpkg_dependencies::dependencies::dependency_t>    wpkgar_dependency_list_t;
    typedef std::map<std::string, bool>                                   wpkgar_package_listed_t;

    // disallow copying
    wpkgar_install(const wpkgar_install& rhs);
    wpkgar_install& operator = (const wpkgar_install& rhs);

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
    bool check_implicit_for_upgrade(wpkgar_package_list_t& tree, const wpkgar_package_list_t::size_type idx);

    void validate_packager_version();
    void validate_installed_size_and_overwrite();
    void validate_fields();
    void validate_scripts();
    void sort_package_dependencies(const std::string& name, wpkgar_package_listed_t& listed);
    void sort_packages();

    // unpack sub-functions
    bool preupgrade_scripts(installer::package_item_t *item, installer::package_item_t *upgrade);
    bool postupgrade_scripts(installer::package_item_t *item, installer::package_item_t *upgrade, wpkg_backup::wpkgar_backup& backup);
    void cancel_upgrade_scripts(installer::package_item_t *item, installer::package_item_t *upgrade, wpkg_backup::wpkgar_backup& backup);
    bool preinst_scripts(installer::package_item_t *item, installer::package_item_t *upgrade, installer::package_item_t *& conf_install);
    void cancel_install_scripts(installer::package_item_t *item, installer::package_item_t *conf_install, wpkg_backup::wpkgar_backup& backup);
    void set_status(installer::package_item_t *item, installer::package_item_t *upgrade, installer::package_item_t *conf_install, const std::string& status);
    bool do_unpack(installer::package_item_t *item, installer::package_item_t *upgrade);
    void unpack_file(installer::package_item_t *item, const wpkg_filename::uri_filename& destination, const memfile::memory_file::file_info& info);

    // configuration sub-functions
    bool configure_package(installer::package_item_t *item);

    wpkgar_manager::pointer_t                 f_manager;
    installer::flags::pointer_t               f_flags;
    installer::package_list::pointer_t        f_package_list;
    installer::dependencies::pointer_t        f_dependencies;
    //wpkgar_manager::package_list_t            f_list_installed_packages;
    //wpkgar_flags_t                            f_flags;
    std::string                               f_architecture;
    wpkgar_manager::package_status_t          f_original_status;
    //wpkgar_package_list_t                     f_packages;
    installer::tree_generator::package_idxs_t f_sorted_packages;
    installer::task::pointer_t                f_task;
    //controlled_vars::fbool_t                  f_repository_packages_loaded;
    //controlled_vars::fbool_t                  f_install_includes_choices;
    controlled_vars::zuint32_t                f_tree_max_depth;
    //wpkgar_list_of_strings_t                  f_essential_files;
    //wpkgar_list_of_strings_t                  f_field_names;
    //controlled_vars::fbool_t                  f_read_essentials;
    controlled_vars::fbool_t                  f_install_source;

    typedef std::vector<std::string> string_list_t;
    string_list_t                             f_field_validations;

    typedef installer::progress_scope_t<installer::progress_stack,uint64_t> progress_scope;
    installer::progress_stack                 f_progress_stack;
};

}   // namespace wpkgar

// vim: ts=4 sw=4 et
