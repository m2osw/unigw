/*    wpkg_output.h -- declaration of the wpkg output class
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

/** \file
 * \brief Handle output messages.
 *
 * The library generates output, but instead of directly printing it in a
 * console or a file, it processes it through an interface. The applications
 * that make use of the library can therefore decide where the output should
 * go. For example, wpkg prints the string in stdout or stderr, and when
 * someone requests the messages to be sent to the log with --output-log
 * option, then it writes in both places: the console and the log file.
 *
 * The library understands normal messages and debug messages. It supports
 * multiple levels and can reproduce the behavior of the --verbose and
 * the --quiet options.
 */
#ifndef WPKG_OUTPUT_H
#define WPKG_OUTPUT_H

#include    "libdebpackages/wpkg_filename.h"

#include    "controlled_vars/controlled_vars_need_init.h"
#include    "controlled_vars/controlled_vars_ptr_auto_init.h"

#include    <string>


namespace wpkg_output
{

class wpkg_output_exception : public std::runtime_error
{
public:
    wpkg_output_exception(const std::string& what_msg) : runtime_error(what_msg) {}
};

class wpkg_output_exception_parameter : public wpkg_output_exception
{
public:
    wpkg_output_exception_parameter(const std::string& what_msg) : wpkg_output_exception(what_msg) {}
};

class wpkg_output_exception_format : public wpkg_output_exception
{
public:
    wpkg_output_exception_format(const std::string& what_msg) : wpkg_output_exception(what_msg) {}
};


enum level_t
{
    // WARNING: levels MUST be in order for compare_levels() to work right
    level_debug,
    level_info,
    level_warning,
    level_error,
    level_fatal
};

typedef controlled_vars::auto_init<level_t, level_info> safe_level_t;

DEBIAN_PACKAGE_EXPORT const char * level_to_string(const level_t level);
DEBIAN_PACKAGE_EXPORT int compare_levels(const level_t l1, const level_t l2);


enum module_t
{
    // start the module at a larger number so if we mix the module
    // and the level we detect the error immediately (assuming
    // we generate the message, hence full coverage requirements)
    module_attached = 100,
    module_detached,
    module_build_info,
    module_build_package,
    module_validate_installation,
    module_unpack_package,
    module_configure_package,
    module_validate_removal,
    module_remove_package,
    module_deconfigure_package,
    module_run_script,
    module_repository,
    module_control,
    module_changelog,
    module_copyright,
    module_field,
    module_tool,
    module_track
};

typedef controlled_vars::auto_init<module_t, module_tool> safe_module_t;

DEBIAN_PACKAGE_EXPORT const char *module_to_string(const module_t module);

DEBIAN_PACKAGE_EXPORT std::string generate_timestamp();
DEBIAN_PACKAGE_EXPORT std::string make_raw_message_parsable();


class DEBIAN_PACKAGE_EXPORT debug_flags
{
public:
    typedef unsigned int debug_t;

    static const debug_t    debug_none             = 000000;
    static const debug_t    debug_basics           = 000001; // progress information
    static const debug_t    debug_scripts          = 000002; // invocation and status of maintainer scripts
    static const debug_t    debug_depends_graph    = 000004; // create graph files (.dot)
    static const debug_t    debug_files            = 000010; // output for each file processed
    static const debug_t    debug_config           = 000020; // output for each configuration file
    static const debug_t    debug_conflicts        = 000040; // dependencies and conflicts
    static const debug_t    debug_detail_files     = 000100; // detailed output for each file processed
    static const debug_t    debug_detail_config    = 000200; // detailed output for each configuration file
    static const debug_t    debug_detail_conflicts = 000400; // detailed output about dependencies and conflicts
    static const debug_t    debug_database         = 001000; // database details
    static const debug_t    debug_full             = 002000; // full details
    static const debug_t    debug_progress         = 004000; // tell the user what we're working on next
    static const debug_t    debug_trigger          = 010000; // trigger activation and processing
    static const debug_t    debug_detail_trigger   = 020000; // detailed output for each trigger
    static const debug_t    debug_full_trigger     = 040000; // all output about each trigger
    static const debug_t    debug_all              = 077777;

    typedef controlled_vars::auto_init<debug_t, debug_none> safe_debug_t;

    // TODO:
    // add functions to transform strings to debug flags
    // (i.e. debug flags by name) and other fun things
};

class DEBIAN_PACKAGE_EXPORT message_t
{
public:
    message_t();

    void                    set_level(level_t level);
    void                    set_module(module_t module);
    void                    set_program_name(const std::string& program_name);
    void                    set_package_name(const char *package_name);
    void                    set_package_name(const std::string& package_name);
    void                    set_package_name(const wpkg_filename::uri_filename& package_name);
    void                    set_time_stamp(const std::string& time_stamp);
    void                    set_action(const std::string& action);
    void                    set_debug_flags(const debug_flags::debug_t dbg_flags);
    void                    set_raw_message(const std::string& raw_message);

    std::string             get_full_message(bool raw_message = false) const;
    level_t                 get_level() const;
    module_t                get_module() const;
    std::string             get_program_name() const;
    std::string             get_package_name() const;
    std::string             get_time_stamp() const;
    std::string             get_action() const;
    debug_flags::debug_t    get_debug_flags() const;
    std::string             get_raw_message() const;

private:
    safe_level_t                f_level;
    safe_module_t               f_module;
    std::string                 f_program_name;
    std::string                 f_package_name;
    std::string                 f_time_stamp;
    std::string                 f_action;
    debug_flags::safe_debug_t   f_debug_flags;
    std::string                 f_raw_message;
};


class output;

class DEBIAN_PACKAGE_EXPORT log
{
public:
    log(const char *format);
    log(const wchar_t *format);
    log(const std::string& format);
    log(const std::wstring& format);
    log(std::string& output_message, const char *format);
    log(std::string& output_message, const wchar_t *format);
    log(std::string& output_message, const std::string& format);
    log(std::string& output_message, const std::wstring& format);
    ~log();

    log& debug(debug_flags::debug_t debug_flags);

    log& level(level_t level);

    log& module(module_t module);

    log& package(const char *package_name);
    log& package(const wchar_t *package_name);
    log& package(const std::string& package_name);
    log& package(const std::wstring& package_name);
    log& package(const wpkg_filename::uri_filename& package_name);

    log& action(const char *action_name);
    log& action(const wchar_t *action_name);
    log& action(const std::string& action_name);
    log& action(const std::wstring& action_name);

    log& arg(const char *s);
    log& arg(const wchar_t *s);
    log& arg(const std::string& s);
    log& arg(const std::wstring& s);
    log& arg(const wpkg_filename::uri_filename& filename);
    log& quoted_arg(const char *s);
    log& quoted_arg(const wchar_t *s);
    log& quoted_arg(const std::string& s);
    log& quoted_arg(const std::wstring& s);
    log& quoted_arg(const wpkg_filename::uri_filename& filename);
    log& arg(const char v);
    log& arg(const unsigned char v);
    log& arg(const signed char v);
    log& arg(const signed short v);
    log& arg(const unsigned short v);
    log& arg(const signed int v);
    log& arg(const unsigned int v);
    log& arg(const signed long v);
    log& arg(const unsigned long v);
#if defined(MO_WINDOWS) && defined(_WIN64) && !defined(MO_MINGW32)
    log& arg(const size_t v);
#endif
    log& arg(const float v);
    log& arg(const double v);

private:
    std::string replace_arguments();

    std::string                                     f_format;
    std::vector<std::string>                        f_args;
    message_t                                       f_message;
    controlled_vars::ptr_auto_init<std::string>     f_output_message;
};


class DEBIAN_PACKAGE_EXPORT output
{
public:
    output();
    virtual ~output() {}

    void addref();
    void release();

    void set_program_name(const std::string& program_name);
    const std::string& get_program_name() const;
    void set_debug(debug_flags::debug_t debug_flags);
    debug_flags::debug_t get_debug_flags() const;
    uint32_t error_count() const;
    void reset_error_count();

    void log(const message_t& message) const;

protected:
    virtual void log_message( const message_t& msg_obj ) const;
    virtual void output_message( const message_t& msg_obj ) const;

private:
    controlled_vars::muint32_t          f_refcount;
    std::string                         f_program_name;
    debug_flags::safe_debug_t           f_debug_flags;
    mutable controlled_vars::zuint32_t  f_error_count;
};



DEBIAN_PACKAGE_EXPORT void set_output(output *out);
DEBIAN_PACKAGE_EXPORT output *get_output();
DEBIAN_PACKAGE_EXPORT debug_flags::debug_t get_output_debug_flags();
DEBIAN_PACKAGE_EXPORT uint32_t get_output_error_count();


}       // namespace wpkg_output
// vim: ts=4 sw=4 et
#endif //#ifndef WPKGAR_OUTPUT_H
