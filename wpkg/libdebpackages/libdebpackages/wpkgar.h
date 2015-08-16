/*    wpkgar.h -- declaration of the wpkg archive
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
 * \brief wpkg archive manager.
 *
 * This file declares the wpkg archive manager which is used to build,
 * unpack, install, configure, upgrade, deconfigure, remove, purge
 * packages.
 *
 * The class is very handy to handle any number of packages in a fairly
 * transparent manner as it will give you direct access to control files
 * and their fields, package data, repositories, etc.
 */
#ifndef WPKGAR_H
#define WPKGAR_H
#include    "wpkg_control.h"
#include    "controlled_vars/controlled_vars_auto_enum_init.h"


namespace wpkgar
{

class wpkgar_exception : public std::runtime_error
{
public:
    wpkgar_exception(const std::string& what_msg) : runtime_error(what_msg) {}
};

class wpkgar_exception_parameter : public wpkgar_exception
{
public:
    wpkgar_exception_parameter(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_invalid : public wpkgar_exception
{
public:
    wpkgar_exception_invalid(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_invalid_emptydir : public wpkgar_exception_invalid
{
public:
    wpkgar_exception_invalid_emptydir(const std::string& what_msg) : wpkgar_exception_invalid(what_msg) {}
};

class wpkgar_exception_compatibility : public wpkgar_exception
{
public:
    wpkgar_exception_compatibility(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_undefined : public wpkgar_exception
{
public:
    wpkgar_exception_undefined(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_io : public wpkgar_exception
{
public:
    wpkgar_exception_io(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_defined_twice : public wpkgar_exception
{
public:
    wpkgar_exception_defined_twice(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_locked : public wpkgar_exception
{
public:
    wpkgar_exception_locked(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class wpkgar_exception_stop : public wpkgar_exception
{
public:
    wpkgar_exception_stop(const std::string& what_msg) : wpkgar_exception(what_msg) {}
};

class DEBIAN_PACKAGE_EXPORT wpkgar_interrupt
{
public:
    virtual ~wpkgar_interrupt();
    virtual bool stop_now();
};


// internal class to handle packages individually
class DEBIAN_PACKAGE_EXPORT wpkgar_package;


// tracker is set in the manager, but tracking is done with another object
class DEBIAN_PACKAGE_EXPORT wpkgar_tracker_interface
{
public:
    virtual         ~wpkgar_tracker_interface();

    virtual void    track(const std::string& command, const std::string& package_name);
};


// the manager handles many archives in one place
class DEBIAN_PACKAGE_EXPORT wpkgar_manager
{
public:
    enum package_status_t
    {
        no_package,  // package cannot be found
        unknown,     // package exists, status is not known
        not_installed,
        config_files,
        installing,
        upgrading,
        half_installed,
        unpacked,
        half_configured,
        installed,
        removing,
        purging,
        listing,
        verifying,
        ready
    };

    enum script_t
    {
        wpkgar_script_validate,
        wpkgar_script_preinst,
        wpkgar_script_postinst,
        wpkgar_script_prerm,
        wpkgar_script_postrm
    };

    typedef std::vector<std::string>        package_list_t;
    typedef std::vector<std::string>        script_parameters_t;
    typedef std::vector<std::string>        hooks_t;
    typedef std::vector<std::string>        conffiles_t;

                                            wpkgar_manager();
                                            ~wpkgar_manager();
    void                                    create_database(const wpkg_filename::uri_filename& ctrl_filename);

    const wpkg_filename::uri_filename&      get_root_path() const;
    void                                    set_root_path(const wpkg_filename::uri_filename& root_path);
    const wpkg_filename::uri_filename       get_inst_path() const;
    void                                    set_inst_path(const wpkg_filename::uri_filename& inst_path);
    const wpkg_filename::uri_filename       get_database_path() const;
    void                                    set_database_path(const wpkg_filename::uri_filename& database_path);

    bool                                    has_package(const wpkg_filename::uri_filename& package_name) const;
    void                                    load_package(const wpkg_filename::uri_filename& name, bool force_reload = false);
    wpkg_filename::uri_filename             get_package_path(const wpkg_filename::uri_filename& package_name) const;
    void                                    get_wpkgar_file(const wpkg_filename::uri_filename& package_name, memfile::memory_file *& wpkgar_file);
    package_status_t                        package_status(const wpkg_filename::uri_filename& package_name);
    package_status_t                        safe_package_status(const wpkg_filename::uri_filename& name);
    void                                    add_self(const std::string& package);
    bool                                    include_self(const std::string& package);
    bool                                    exists_as_self(const std::string& package);
    bool                                    is_self() const;

    void                                    list_installed_packages(package_list_t& list);
    void                                    add_repository(const wpkg_filename::uri_filename& repository);
    void                                    set_repositories(const wpkg_filename::filename_list_t& repositories);
    const wpkg_filename::filename_list_t&   get_repositories() const;
    void                                    add_sources_list();

    void                                    set_tracker(std::shared_ptr<wpkgar_tracker_interface> tracker);
    std::shared_ptr<wpkgar_tracker_interface> get_tracker() const;
    void                                    track(const std::string& command, const std::string& package_name = "");

    void                                    add_global_hook(const wpkg_filename::uri_filename& script_name);
    bool                                    remove_global_hook(const wpkg_filename::uri_filename& script_name);
    hooks_t                                 list_hooks() const;
    void                                    install_hooks(const std::string& package_name);
    void                                    remove_hooks(const std::string& package_name);
    bool                                    run_script(const wpkg_filename::uri_filename& package_name, script_t script, script_parameters_t params);

    void                                    lock(const std::string& status);
    void                                    unlock();
    bool                                    was_locked() const;
    bool                                    is_locked() const;
    bool                                    remove_lock();

    // control file
    void                                    set_control_file_state(std::shared_ptr<wpkg_control::control_file::control_file_state_t> state);
    void                                    set_field_variable(const std::string& name, const std::string& value);
    void                                    set_control_variables(wpkg_control::control_file& control);
    void                                    set_package_selection_to_reject(const std::string& package_name);
    bool                                    has_control_file(const wpkg_filename::uri_filename& package_name, const std::string& control_filename) const;
    void                                    get_control_file(memfile::memory_file& p, const wpkg_filename::uri_filename& package_name, std::string& control_filename, bool compress = true);
    bool                                    validate_fields(const wpkg_filename::uri_filename& package_name, const std::string& expression);
    void                                    conffiles(const wpkg_filename::uri_filename& package_name, conffiles_t& conf_files) const;
    bool                                    is_conffile(const wpkg_filename::uri_filename& package_name, const std::string& filename) const;
    bool                                    field_is_defined(const wpkg_filename::uri_filename& package_name, const std::string& name) const;
    void                                    set_field(const wpkg_filename::uri_filename& package_name, const std::string& name, const std::string& value, bool save = false);
    void                                    set_field(const wpkg_filename::uri_filename& package_name, const std::string& name, long value, bool save = false);
    std::string                             get_field(const wpkg_filename::uri_filename& package_name, const std::string& name) const;
    std::string                             get_description(const wpkg_filename::uri_filename& package_name, const std::string& name, std::string& long_description) const;
    wpkg_dependencies::dependencies         get_dependencies(const wpkg_filename::uri_filename& package_name, const std::string& name) const;
    const wpkg_control::control_file::field_file::list_t get_field_list(const wpkg_filename::uri_filename& package_name, const std::string& name) const;
    std::string                             get_field_first_line(const wpkg_filename::uri_filename& package_name, const std::string& name) const;
    std::string                             get_field_long_value(const wpkg_filename::uri_filename& package_name, const std::string& name) const;
    bool                                    get_field_boolean(const wpkg_filename::uri_filename& package_name, const std::string& name) const;
    long                                    get_field_integer(const wpkg_filename::uri_filename& package_name, const std::string& name) const;
    int                                     number_of_fields(const wpkg_filename::uri_filename& package_name) const;
    const std::string&                      get_field_name(const wpkg_filename::uri_filename& package_name, int idx) const;

    // handle interrupts (i.e. so users can break wpkg processing)
    void                                    set_interrupt_handler(wpkgar_interrupt *handler);
    void                                    check_interrupt() const;

private:
                                            // avoid copies
                                            wpkgar_manager(const wpkgar_manager& rhs);
                                            wpkgar_manager& operator = (const wpkgar_manager& rhs);

    const std::shared_ptr<wpkgar_package>   get_package(const wpkg_filename::uri_filename& package_name) const;
    void                                    load_temporary_package(const wpkg_filename::uri_filename& filename);
    bool                                    run_one_script(const wpkg_filename::uri_filename& package_name, const std::string& interpreter, const wpkg_filename::uri_filename& script_name, const std::string& parameters);

    typedef std::map<std::string, std::shared_ptr<wpkgar_package> >         packages_t;
    typedef std::map<std::string, std::string>                              field_variables_t;
    typedef std::map<std::string, int>                                      self_packages_t;
    typedef controlled_vars::auto_init<int, -1>                             lock_fd_t;

    std::shared_ptr<wpkg_control::control_file::control_file_state_t> f_control_file_state;
    controlled_vars::fbool_t                            f_root_path_is_defined;
    wpkg_filename::uri_filename                         f_root_path;
    wpkg_filename::uri_filename                         f_inst_path;
    wpkg_filename::uri_filename                         f_database_path;
    packages_t                                          f_packages;
    wpkg_filename::filename_list_t                      f_repository;
    field_variables_t                                   f_field_variables;
    wpkg_filename::uri_filename                         f_lock_filename;
    lock_fd_t                                           f_lock_fd;
    controlled_vars::zint32_t                           f_lock_count;
    controlled_vars::ptr_auto_init<wpkgar_interrupt>    f_interrupt_handler;
    self_packages_t                                     f_selves;
    controlled_vars::fbool_t                            f_include_selves;
    std::shared_ptr<wpkgar_tracker_interface>           f_tracker;
};


class DEBIAN_PACKAGE_EXPORT wpkgar_lock
{
public:
    wpkgar_lock(wpkgar_manager *manager, const std::string& status);
    ~wpkgar_lock();
    void unlock();

private:
    wpkgar_manager *            f_manager;
};


class DEBIAN_PACKAGE_EXPORT wpkgar_rollback
{
public:
                                    wpkgar_rollback(wpkgar_manager *manager, const wpkg_filename::uri_filename& tracking_filename);
                                    ~wpkgar_rollback();

    void                            rollback();
    void                            commit();

    wpkg_filename::uri_filename     get_tracking_filename() const;

private:
    wpkgar_manager *            f_manager;
    wpkg_filename::uri_filename f_tracking_filename;
};


}       // namespace wpkgar

#endif
//#ifndef WPKGAR_H
// vim: ts=4 sw=4 et
