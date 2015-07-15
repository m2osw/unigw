/*    wpkg.cpp -- manage debian archives
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
 * \brief The implementation of the wpkg tool.
 *
 * This file is the implementation of the wpkg tool with already over 7,000
 * lines of code and comments. The file includes all the commands that are
 * supported by wpkg. However, most of the implementation is found in the
 * libdebpackages library.
 */
#include    "libdebpackages/wpkgar_build.h"
#include    "libdebpackages/wpkgar_install.h"
#include    "libdebpackages/wpkgar_remove.h"
#include    "libdebpackages/wpkgar_repository.h"
#include    "libdebpackages/wpkgar_tracker.h"
#include    "libdebpackages/wpkg_util.h"
#include    "libdebpackages/wpkg_copyright.h"
#include    "libdebpackages/wpkg_stream.h"
#include    "libdebpackages/advgetopt.h"
#include    "libdebpackages/debian_packages.h"
#include    "libdebpackages/debian_version.h"
#include    "license.h"
#include    "zlib.h"
#include    "bzlib.h"
#include    "libtld/tld.h"
#include    "controlled_vars/controlled_vars_version.h"
#include    <assert.h>
#include    <errno.h>
#include    <signal.h>
#include    <algorithm>
#ifdef MO_WINDOWS
#   include    <time.h>
#else
#   include    <unistd.h>
#endif
#include    <iostream>
#include    <sstream>

#ifdef _MSC_VER
// "unknown pragma"
#pragma warning(disable: 4068)
#endif


namespace
{

char **g_argv;


/** \brief Whether Ctrl-C was pressed.
 *
 * The wpkg tool protects itself from being interrupted by Ctrl-C. If that
 * happens, the g_interrupted variable is set to true and the process
 * continues up to the next interruptible point. This allows for a much
 * safer processing of the wpkg database when one is being accessed.
 *
 * Also this way the Ctrl-C interruption makes use of an exception meaning
 * that the stack gets unwinded as expected before wpkg exists.
 */
bool g_interrupted(false);


/** \brief Implementation of the wpkgar_interrupt.
 *
 * This class is an implementation of the wpkgar_interrupt interface so
 * we can safely interrupt a process while the library is running. This
 * means packages being installed or removed will continue to be working
 * as expected.
 *
 * This implementation makes use of the g_interrupted variable which is
 * set to true whenever the user presses Ctrl-C.
 */
class wpkg_interrupt : public wpkgar::wpkgar_interrupt
{
public:
    virtual bool stop_now()
    {
        return g_interrupted;
    }
};

// we never release this object, doesn't make any difference at this point
wpkg_interrupt interrupt;



/** \brief Handling of the library output.
 *
 * This class is the output handler of the wpkg tool.
 *
 * It outputs the data to the console and when --output-log is used, it
 * also copies the data in a log file in a computer readable format
 * (i.e. quotes inside strings are backslashed, etc.)
 *
 * The class is also responsible for keeping track of the wpkg exit code.
 * Namely, if an error is detected (as indicated by the level of a message)
 * then the exit code becomes 1 instead of 0.
 */
class tool_output : public wpkg_output::output
{
public:
    tool_output();
    ~tool_output();

    void close_output();
    void set_output_file(const std::string& filename);
    const std::string& get_output_file() const;
    void set_level(wpkg_output::level_t level);

    int exit_code() const;

protected:
    virtual void log_message( const wpkg_output::message_t& msg ) const;
    virtual void output_message( const wpkg_output::message_t& msg ) const;

private:
    std::string                             f_output_filename;
    wpkg_stream::fstream                    f_output;
    wpkg_output::level_t                    f_log_level;
    mutable wpkg_output::level_t            f_highest_level;
};

tool_output::tool_output()
    //: f_output_filename("") -- auto-init
    //: f_output() -- auto-init
    : f_log_level(wpkg_output::level_warning)
    , f_highest_level(wpkg_output::level_debug)
{
}

tool_output::~tool_output()
{
    close_output();
}

void tool_output::close_output()
{
    f_output.close();
    f_output_filename.clear();
}

void tool_output::set_output_file(const std::string& filename)
{
    close_output();
    f_output_filename = filename;
    f_output.append(filename);
    if(!f_output.good())
    {
        fprintf(stderr, "wpkg:error: could not open the log file for writing.\n");
    }
    else
    {
        wpkg_output::log("wpkg started")
            .debug(wpkg_output::debug_flags::debug_progress)
            .level(wpkg_output::level_info) // restore level
            .action("log");
    }
}

const std::string& tool_output::get_output_file() const
{
    return f_output_filename;
}

void tool_output::log_message(const wpkg_output::message_t& msg) const
{
    if(f_output.good())
    {
        std::string message(msg.get_full_message(false));
        f_output.write(message.c_str(), message.length());
        if(message.length() > 0 && message[message.length() - 1] != '\n')
        {
            f_output.write("\n", 1);
        }
    }
}

void tool_output::output_message( const wpkg_output::message_t& msg ) const
{
    const wpkg_output::level_t level = msg.get_level();
    //
    if(level > f_highest_level)
    {
        f_highest_level = level;
    }
    if(wpkg_output::compare_levels(level, f_log_level) >= 0 || (msg.get_debug_flags() & wpkg_output::debug_flags::debug_progress) != 0)
    {
        fprintf(stderr, "%s\n", msg.get_full_message(true).c_str());
    }
}

void tool_output::set_level(wpkg_output::level_t level)
{
    f_log_level = level;
}

int tool_output::exit_code() const
{
    return f_highest_level >= wpkg_output::level_error ? 1 : 0;
}


// we never release this object, doesn't make any difference at this point
tool_output g_output;


/** \brief Wrapper class to handle the wpkg command line arguments.
 *
 * This class is a wrapper that helps in handling the extremely large
 * number of parameters that the wpkg tool accepts on its command line.
 * This includes converting the command (or mix of commands) in a
 * command_t value.
 */
class command_line
{
public:
    enum command_t
    {
        command_unknown,
        command_add_hooks,
        command_add_sources,
        command_architecture,
        command_atleast_version,
        command_atleast_wpkg_version,
        command_audit,
        command_autoremove,
        command_build,
        command_build_and_install,
        command_canonicalize_version,
        command_cflags,
        command_check_install,
        command_compare_versions,
        command_compress,
        command_configure,
        command_contents,
        command_control,
        command_copyright,
        command_create_admindir,
        command_create_database_lock,
        command_create_index,
        command_database_is_locked,
        command_decompress,
        command_deconfigure,
        command_directory_size,
        command_exact_version,
        command_extract,
        command_field,
        command_fsys_tarfile,
        command_help,
        command_increment_build_number,
        command_info,
        command_install,
        command_install_size,
        command_is_installed,
        command_libs,
        command_license,
        command_list,
        command_list_all,
        command_listfiles,
        command_list_hooks,
        command_list_index_packages,
        command_list_sources,
        command_max_version,
        command_md5sums,
        command_md5sums_check,
        command_modversion,
        command_os,
        command_print_architecture,
        command_print_build_number,
        command_print_variables,
        command_processor,
        command_purge,
        command_reconfigure,
        command_remove,
        command_remove_database_lock,
        command_remove_hooks,
        command_remove_sources,
        command_rollback,
        command_search,
        command_set_selection,
        command_show,
        command_package_status,
        command_triplet,
        command_unpack,
        command_update,
        command_update_status,
        command_upgrade,
        command_upgrade_info,
        command_variable,
        command_verify_control,
        command_verify_project,
        command_vendor,
        command_version
    };

    command_line(int argc, char *argv[], std::vector<std::string> configuration_files);

    const advgetopt::getopt& opt() const;
    advgetopt::getopt& opt();
    command_t command() const;
    int size() const; // # of filenames, fields, package names...
    wpkg_filename::uri_filename filename(int idx) const;
    std::string argument(int idx) const;
    std::string get_string(const std::string& name, int idx = 0) const;
    bool quiet() const;
    bool verbose() const;
    bool dry_run(bool msg = true) const;
    int zlevel() const;
    memfile::memory_file::file_format_t compressor() const;

    void add_filename(const std::string& option, const std::string& repository_filename);

private:
    void set_command(command_t c);

    typedef std::vector<std::string> filename_vector_t;
    typedef controlled_vars::limited_auto_init<char, 1, 9, 9> zlevel_t;
    typedef controlled_vars::limited_auto_init<command_t, command_unknown, command_version, command_unknown> zcommand_t;
    typedef controlled_vars::limited_auto_init<memfile::memory_file::file_format_t, memfile::memory_file::file_format_undefined, memfile::memory_file::file_format_other, memfile::memory_file::file_format_best> zcompressor_t;

    advgetopt::getopt                       f_opt;
    zcommand_t                              f_command;
    controlled_vars::flbool_t               f_quiet;
    controlled_vars::flbool_t               f_verbose;
    controlled_vars::flbool_t               f_dry_run;
    zlevel_t                                f_zlevel;
    wpkg_output::debug_flags::safe_debug_t  f_debug_flags;
    memfile::memory_file::file_format_t     f_compressor;
    std::string                             f_option;
    filename_vector_t                       f_filenames;        // if not empty, use this list instead of opt.get_string("filename", idx)
};

const advgetopt::getopt::option wpkg_options[] =
{
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        NULL,
        NULL,
        "Usage: wpkg -<command> [-<opt>] <filename> | <package-name> | <field> ...",
        advgetopt::getopt::help_argument
    },
    // COMMANDS
    {
        '\0',
        0,
        NULL,
        NULL,
        "commands:",
        advgetopt::getopt::help_argument
    },
    {
        '\0',
        0,
        "add-hooks",
        NULL,
        "add one or more global hooks to your wpkg system",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "add-sources",
        NULL,
        "add one or more sources to your core/sources.list file found in your administration directory",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "architecture",
        NULL,
        "architecture wpkg was compiled with (and expected to be the target of your packages)",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ALIAS,
        "atleast-pkgconfig-version", // note though that the version is "wrong" compared to the pkg-config tool itself
        NULL,
        "atleast-wpkg-version",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "atleast-version",
        NULL,
        "check whether the version of a package is at least what is specified right after this command",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "atleast-wpkg-version",
        NULL,
        "check that wpkg is at least a certain version, if so exit with 0, otherwise exit with 1",
        advgetopt::getopt::required_argument
    },
    {
        'C',
        0,
        "audit",
        NULL,
        "audit a wpkg installation target",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "autoremove",
        NULL,
        "automatically remove all implicitly installed packages that are not depended on anymore; may be used with --purge to completely remove those packages",
        advgetopt::getopt::no_argument
    },
    {
        'b',
        0,
        "build",
        NULL,
        "build a wpkg package",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "build-and-install",
        NULL,
        "build and install a wpkg package",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "canonalize-version",
        NULL,
        NULL, // keep my blunder hidden
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "canonicalize-version",
        NULL,
        "canonicalize a version by removing unnecessary values",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "cflags",
        NULL,
        "print out the C/C++ command line options necessary to compile against the specified packages",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "check-install",
        NULL,
        "check that a set of packages can be installed",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        0,
        "compare-versions",
        NULL,
        "compare two versions against each others using the specified operator (v1 op v2)",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        0,
        "compress",
        NULL,
        "compress files; specify the filename with the compression extension",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "configure",
        NULL,
        "configure a package that was unpacked earlier",
        advgetopt::getopt::required_multiple_argument
    },
    {
        'c',
        0,
        "contents",
        NULL,
        "list of the files available in this package",
        advgetopt::getopt::required_argument
    },
    {
        'e',
        0,
        "control",
        NULL,
        "extract the entire control archive",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "copyright",
        NULL,
        "prints out the copyright file of a package",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "create-admindir",
        NULL,
        "prepare the administration directory (database)",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "create-database-lock",
        NULL,
        "create the database lock file (so an external tool can safely work on the wpkg database)",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "create-index",
        NULL,
        "create an index file from a list of Debian packages",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "database-is-locked",
        NULL,
        "check whether the database is currently locked",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "decompress",
        NULL,
        "decompress files; specify the filename with the compression extension, it will be removed",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "deconfigure",
        NULL,
        "deconfigure the packages specified on the command line",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        0,
        "directory-size",
        NULL,
        "compute the size of a directory and print the result in stdout",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "exact-version",
        NULL,
        "check whether the version of a package is exactly what is specified right after this command",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ALIAS,
        "exists",
        NULL,
        "is-installed",
        advgetopt::getopt::required_argument
    },
    {
        'x',
        0,
        "extract",
        NULL,
        "extract files from a wpkg package",
        advgetopt::getopt::no_argument
    },
    {
        'f',
        0,
        "field",
        NULL,
        "show the value of the specified fields",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "fsys-tarfile",
        NULL,
        "print the decompressed data.tar.gz file to stdout so it can directly be piped to tar",
        advgetopt::getopt::required_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        NULL,
        "print the help message about all the wpkg commands and options; for more information, try: wpkg --help help",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help-nobr",
        NULL,
        NULL, // hide from --help screen
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "increment-build-number",
        NULL,
        "increment the build number defined in a build number file; by default the file is wpkg/build-number; you may specify another file too",
        advgetopt::getopt::no_argument
    },
    {
        'I',
        0,
        "info",
        NULL,
        "show detailed information about this package",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "force-reinstall",
        NULL,
        "if package is already installed, force a re-installation of a package; useful for packages in a source list",
        advgetopt::getopt::no_argument
    },
    {
        'i',
        0,
        "install",
        NULL,
        "install a wpkg compatible package",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        0,
        "install-size",
        NULL,
        "retrieve the Installed-Size field from the specified packages and return the total sum",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        0,
        "is-installed",
        NULL,
        "check whether a package is currently installed",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "libs",
        NULL,
        "retrieve the list of libraries to link against to make use of this package",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "license",
        NULL,
        "displays the license of this tool",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "licence", // French spelling
        NULL,
        NULL, // hidden argument in --help screen
        advgetopt::getopt::no_argument
    },
    {
        'l',
        0,
        "list",
        NULL,
        "displays the list of installed packages, optionally add a shell pattern to limit the list",
        advgetopt::getopt::optional_argument
    },
    {
        '\0',
        0,
        "list-all",
        NULL,
        "display all the installed packages a la pkg-config (package name, description)",
        advgetopt::getopt::no_argument
    },
    {
        'L',
        0,
        "listfiles",
        NULL,
        "displays the list of files installed by the named packages",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        0,
        "list-hooks",
        NULL,
        "list the currently installed global and package hooks",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "list-index-packages",
        NULL,
        "displays the list of packages from a package index (see --create-index)",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        0,
        "list-sources",
        NULL,
        "displays the list of sources from a sources.list file",
        advgetopt::getopt::optional_multiple_argument
    },
    {
        '\0',
        0,
        "max-version",
        NULL,
        "check whether the version of a package is at most what is specified right after this command",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "md5sums",
        NULL,
        "create an md5sums file from the files defined on the command line",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "md5sums-check",
        NULL,
        "check files against their md5sums; the name of the md5sums file, as created by the --md5sums, is expected right after this command, followed by the files to check",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "modversion",
        NULL,
        "retrieve the version of a package from its pkgconfig file",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "os",
        NULL,
        "print to stdout the name of the os wpkg was compiled with",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "package-status",
        NULL,
        "display the current status of a package (processed X-Status and other fields)",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        0,
        "print-architecture",
        NULL,
        "print installation architecture",
        advgetopt::getopt::no_argument
    },
    {
        'p',
        0,
        "print-avail",
        NULL,
        "print installed package control file",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "print-build-number",
        NULL,
        "read the build number file and print its contents",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "print-variables",
        NULL,
        "print the list of variables defined in a .pc file",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "processor",
        NULL,
        "print processor wpkg was compiled with; use --verbose to print out the running machine",
        advgetopt::getopt::no_argument
    },
    {
        'P',
        0,
        "purge",
        NULL,
        "purge the packages specified on the command line",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        0,
        "reconfigure",
        NULL,
        "reconfigure a package by deconfiguring it, reinstalling the initial configuration files, and configuring it with those initial files",
        advgetopt::getopt::required_multiple_argument
    },
    {
        'r',
        0,
        "remove",
        NULL,
        "remove the packages specified on the command line",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        0,
        "remove-database-lock",
        NULL,
        "delete the database lock file (if the packager crashed without cleaning up properly)",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "remove-hooks",
        NULL,
        "remove one or more global hooks from your wpkg system",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "remove-sources",
        NULL,
        "remove one or more sources to your core/sources.list file found in your administration directory",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "rollback",
        NULL,
        "run a rollback script to restore the status of a target system",
        advgetopt::getopt::required_argument
    },
    {
        'S',
        0,
        "search",
        NULL,
        "search installed packages for the specified file",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "set-selection",
        NULL,
        "set the selection mode of a package (auto, manual, normal, hold)",
        advgetopt::getopt::required_argument
    },
    {
        'W',
        0,
        "show",
        NULL,
        "show basic information about packages",
        advgetopt::getopt::required_argument
    },
    {
        's',
        advgetopt::getopt::GETOPT_FLAG_ALIAS,
        "status",
        NULL,
        "field",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "triplet",
        NULL,
        "print to stdout the architecture triplet as it would appear in the Architecture field",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "unpack",
        NULL,
        "unpack the files of the specified packages, this is similar to --install without --configure",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        0,
        "update",
        NULL,
        "update the index files from the different sources",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "update-status",
        NULL,
        "print out the current status of the last update",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "upgrade",
        NULL,
        "upgrade your current target with all newer available packages",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "upgrade-info",
        NULL,
        "list the packages that can be upgraded with --upgrade",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "upgrade-urgent",
        NULL,
        "upgrade your current target with packages that have an urgency of high or greater",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "variable",
        NULL,
        "print the content of the named variable found in the .pc file of the listed packages",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "vendor",
        NULL,
        "print to stdout the name of the vendor of this version of wpkg",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "verify",
        NULL,
        "check the archive validity",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "verify-control",
        NULL,
        "validate control or control info files",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "verify-project",
        NULL,
        "validate a project to prepare it for the wpkg environment; you must be in the root directory of the project to start this command",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        NULL,
        "show the version of wpkg",
        advgetopt::getopt::no_argument
    },
    {
        'X',
        0,
        "vextract",
        NULL,
        "extract and list files from a wpkg package",
        advgetopt::getopt::no_argument
    },
    // OPTIONS
    {
        '\0',
        0,
        NULL,
        NULL,
        "options:",
        advgetopt::getopt::help_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "admindir",
        "var/lib/wpkg",
        "define the administration directory (i.e. wpkg database folder), default is /var/lib/wpkg",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "build-number-filename",
        NULL,
        "define the name of the build number file; the default is \"wpkg/build_number\"",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "clear-exceptions",
        NULL,
        "remove all the exceptions and thus include all the files found in the control folder to the output .deb archive",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "cmake-generator",
        NULL,
        "define the generator to use with cmake; in most cases \"Unix Makefiles\" under Unix systems and \"NMake Makefiles\" using the MS-Windows Visual Studio development system",
        advgetopt::getopt::required_argument
    },
    {
        'Z',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "compressor",
        "gzip",
        "type of compression to use (gzip, bzip2, lzma, xz, none); default is best available",
        advgetopt::getopt::required_argument
    },
    {
        'D',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "debug",
        NULL,
        "define a set of debug to be printed out while wpkg works",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ALIAS,
        "define-variable",
        NULL,
        "field-variables",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "dry-run",
        NULL,
        "run all the validations of the command, do not actually run the process",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "enforce-path-length-limit",
        NULL,
        "while building a package, generate an error if a paths length is over this length (range 64 to 65536); to get warnings instead of errors use --path-length-limit instead",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "exception",
        NULL,
        "add one exception to the list of files not to add in a data.tar.gz file (i.e. \".svn\" or \"*.bak\")",
        advgetopt::getopt::required_multiple_argument
    },
    {
        'V',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "field-variables",
        NULL,
        "define field variables (list of <name>=<value> entries)",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-all",
        NULL,
        "turn on all the --force-... options",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-architecture",
        NULL,
        "force the installation of a package even if it has an incompatible architecture",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-breaks",
        NULL,
        "force the installation of a package even if it breaks another",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-configure-any",
        NULL,
        "force the configuration of packages that are dependencies not yet configured",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-conflicts",
        NULL,
        "force the installation of a package even if it is in conflicts",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-depends",
        NULL,
        "force the installation or removal of a package even if dependencies are not all properly satisfied",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-depends-version",
        NULL,
        "force the installation of a package even if the versions do not match",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-distribution",
        NULL,
        "allow the installation of packages from any distribution",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-downgrade",
        NULL,
        "allow downgrading packages (i.e. install packages with a smaller version of those already installed)",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-file-info",
        NULL,
        "allow file information (chmod/chown) to fail on installation of packages",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-hold",
        NULL,
        "force an upgrade or downgrade even if the package is marked as being on hold",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-overwrite",
        NULL,
        "force the installation of a package even if it means some files get overwritten",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-overwrite-dir",
        NULL,
        "force the installation of a package even if it means some directory get overwritten by a file",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-remove-essential",
        NULL,
        "allow the removal of essential packages, this is forbidden by default",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-rollback",
        NULL,
        "rollback all installation processes if any one of them fails",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-upgrade-any-version",
        NULL,
        "force the installation of a package even if the already installed package has a version smaller than the new version of the package minimum upgradable version field",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "force-vendor",
        NULL,
        "force the installation of a package even if the vendor of the package does not match the target vendor string",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "ignore-empty-package",
        NULL,
        "silently exit with 0 status when there are no files to package in a build process",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "install-prefix",
        NULL,
        "define the installation prefix to build binary packages from a source package",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "instdir",
        "",
        "specify the installation directory, where files get unpacked, by default the root is used",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "interactive",
        "no-interactions",
        "let wpkg know that it is interactive",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "log-output",
        NULL,
        "specify an output filename where logs get saved; the log level is ignored with this one, all logs get saved in this file",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "make-tool",
        NULL,
        "define the name of the make tool to use to build things after cmake generated files; usually make or nmake",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-act",
        NULL,
        "run all the validations of the command, do not actually run the process",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-all",
        NULL,
        "turn off all the --force-... options",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-architecture",
        NULL,
        "prevent the action of the --force-architecture option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-breaks",
        NULL,
        "prevent the action of the --force-breaks option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-configure-any",
        NULL,
        "prevent the action of the --force-configure-any option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-conflicts",
        NULL,
        "prevent the action of the --force-conflicts option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-depends",
        NULL,
        "prevent the action of the --force-depends option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-depends-version",
        NULL,
        "prevent the action of the --force-depends-version option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-distribution",
        NULL,
        "prevent the action of the --force-distribution option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-downgrade",
        NULL,
        "prevent the action of the --force-downgrade option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-file-info",
        NULL,
        "make sure to fail if file information (chmod/chown) fails on installation of packages",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-hold",
        NULL,
        "prevent upgrades and downgrades of packages marked as being on hold",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-overwrite",
        NULL,
        "prevent the action of the --force-overwrite option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-overwrite-dir",
        NULL,
        "prevent the action of the --force-overwrite-dir option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-remove-essential",
        NULL,
        "prevent the action of the --force-remove-essential option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-rollback",
        NULL,
        "prevent the action of the --force-rollback option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-upgrade-any-version",
        NULL,
        "prevent the action of the --force-upgrade-any-version option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "no-force-vendor",
        NULL,
        "prevent the action of the --force-vendor option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "numbers",
        NULL,
        "show numbers instead of user/group names and mode flags",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "output-dir",
        NULL,
        "save the package being built in the specified directory",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "output-filename",
        NULL,
        "force the output filename of a package being built",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "output-repository-dir",
        NULL,
        "define the repository directory where source and binary packages shall be saved",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "path-length-limit",
        NULL,
        "while building a package, warn about paths that are over this length (range 64 to 65536); to get an error instead of a warning use --enforce-path-length-limit instead",
        advgetopt::getopt::required_argument
    },
    {
        'q',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "quiet",
        NULL,
        "prevent printing informational and warning messages; in some cases, avoid some lesser errors from being printed too",
        advgetopt::getopt::no_argument
    },
    {
        'R',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "recursive",
        NULL,
        "install: enable recursivity of repository directories (i.e. sub-directories are also scanned); remove: automatically allow removal of dependencies",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-all",
        NULL,
        "turn off all the --force-... options",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-architecture",
        NULL,
        "prevent the action of the --force-architecture option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-breaks",
        NULL,
        "prevent the action of the --force-breaks option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-configure-any",
        NULL,
        "prevent the action of the --force-configure-any option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-conflicts",
        NULL,
        "prevent the action of the --force-conflicts option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-depends",
        NULL,
        "prevent the action of the --force-depends option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-depends-version",
        NULL,
        "prevent the action of the --force-depends-version option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-distribution",
        NULL,
        "prevent the action of the --force-distribution option",
        advgetopt::getopt::no_argument
    },
    {
        'G',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-downgrade",
        NULL,
        "prevent the action of the --force-downgrade option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-file-info",
        NULL,
        "make sure to fail if file information (chmod/chown) fails on installation of packages",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-hold",
        NULL,
        "prevent upgrades and downgrades of packages marked as being on hold",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-overwrite",
        NULL,
        "prevent the action of the --force-overwrite option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-overwrite-dir",
        NULL,
        "prevent the action of the --force-overwrite-dir option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-remove-essential",
        NULL,
        "prevent the action of the --force-remove-essential option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-rollback",
        NULL,
        "prevent the action of the --force-rollback option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-upgrade-any-version",
        NULL,
        "prevent the action of the --force-upgrade-any-version option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "refuse-vendor",
        NULL,
        "prevent the action of the --force-vendor option",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "repository",
        NULL,
        "define the path to a directory filled with packages, and automatically install dependencies if such are missing",
        advgetopt::getopt::required_multiple_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "root",
        "/",
        "define the root directory (i.e. where everything is installed), default is /",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "run-unit-tests",
        NULL,
        "run the unit tests of a package right after building a package from its source package and before creating its binary packages",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "showformat",
        NULL,
        "format used with the --show command; variables can be referenced as ${field:[-]width}",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "simulate",
        NULL,
        "run all the validations of the command, do not actually run the process",
        advgetopt::getopt::no_argument
    },
    {
        'E',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "skip-same-version",
        NULL,
        "skip installing packages that are already installed (i.e. version is the same)",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "tmpdir",
        NULL,
        "define the temporary directory (i.e. /tmp under a Unix system), the default is dynamically determined",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "tracking-journal",
        NULL,
        "explicitly specify the filename of the tracking journal; in which case the journal does not get deleted",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE | advgetopt::getopt::GETOPT_FLAG_ALIAS,
        "validate-fields",
        NULL,
        "verify-fields",
        advgetopt::getopt::required_multiple_argument
    },
    {
        'v',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "verbose",
        NULL,
        "print additional information as available",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "verify-fields",
        NULL,
        "validate control file fields (used along --verify or --install)",
        advgetopt::getopt::required_multiple_argument
    },
    {
        'z',
        advgetopt::getopt::GETOPT_FLAG_ENVIRONMENT_VARIABLE | advgetopt::getopt::GETOPT_FLAG_CONFIGURATION_FILE,
        "zlevel",
        "9",
        "compression level when building (1-9), default is 9",
        advgetopt::getopt::required_argument
    },
    {
        '\0',
        0,
        "running-copy",
        NULL,
        NULL, // hidden argument in --help screen
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "filename",
        NULL,
        NULL, // hidden argument in --help screen
        advgetopt::getopt::default_multiple_argument
    },
    {
        '\0',
        0,
        NULL,
        NULL,
        NULL,
        advgetopt::getopt::end_of_options
    }
};


void version(command_line& cl)
{
    if(cl.size() != 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "The --version option does not take any other arguments");
        /*NOTREACHED*/
    }

    if(cl.verbose())
    {
        printf("wpkg %s (built on %s)\nbzip2 %s\nzlib %s\nlibtld %s\ncontrolled_vars %s\n",
            debian_packages_version_string(), debian_packages_build_time(),
            BZ2_bzlibVersion(),
            zlibVersion(),
            tld_version(),
            controlled_vars::controlled_vars_version()
        );
    }
    else
    {
        printf("%s\n", debian_packages_version_string());
    }

    exit(0);
}



struct help_t;

typedef void (*help_func_t)(command_line& cl, const help_t *h);

/** \brief Structure used to define help entries.
 *
 * This structure is used to define the help entries of the advanced
 * help in wpkg. It includes a name, a function when that specific
 * help entry is specified on the command line, and a description
 * of this entry.
 */
struct help_t
{
    const char *    f_name;
    help_func_t     f_func;
    const char *    f_help;
};

// pre-declaration for help_list()
extern const help_t advanced_help[];


void help_output_control_field(command_line& cl, wpkg_control::control_file::field_factory_map_t::const_iterator& f)
{
    if(cl.verbose())
    {
        printf("%s:", f->second->name());
        wpkg_field::field_file::field_factory_t::name_list_t e(f->second->equivalents());
        if(!e.empty())
        {
            size_t max(e.size());
            for(size_t i(0); i < max; ++i)
            {
                printf(" (or %s)", e[i].c_str());
            }
        }
        printf("\n  %s\n\n", f->second->help());
    }
    else if(cl.quiet())
    {
        printf("%s\n", f->second->name());
    }
    else
    {
        printf("%-32s: %.43s...\n", f->second->name(), f->second->help());
    }
}

void help_control_field(command_line& cl, const help_t *h)
{
    static_cast<void>(h);

    const wpkg_control::control_file::field_factory_map_t *fields(wpkg_control::control_file::field_factory_map());

    const size_t max(cl.size());
    if(max > 1)
    {
        // idx = 0 is the command: "field"
        for(size_t idx(1); idx < max; ++idx)
        {
            const case_insensitive::case_insensitive_string name(cl.get_string("filename", static_cast<int>(idx)));
            wpkg_control::control_file::field_factory_map_t::const_iterator f(fields->find(name));
            if(f == fields->end())
            {
                printf("wpkg:warning: unknown field \"%s\".\n", name.c_str());
            }
            else
            {
                help_output_control_field(cl, f);
            }
        }
    }
    else
    {
        for(wpkg_control::control_file::field_factory_map_t::const_iterator f(fields->begin()); f != fields->end(); ++f)
        {
            help_output_control_field(cl, f);
        }
    }
}



void help_output_copyright_field(command_line& cl, wpkg_copyright::copyright_file::field_factory_map_t::const_iterator& f)
{
    if(cl.verbose())
    {
        printf("%s:", f->second->name());
        wpkg_field::field_file::field_factory_t::name_list_t e(f->second->equivalents());
        if(!e.empty())
        {
            size_t max(e.size());
            for(size_t i(0); i < max; ++i)
            {
                printf(" (or %s)", e[i].c_str());
            }
        }
        printf("\n  %s\n\n", f->second->help());
    }
    else if(cl.quiet())
    {
        printf("%s\n", f->second->name());
    }
    else
    {
        printf("%-32s: %.43s...\n", f->second->name(), f->second->help());
    }
}

void help_copyright_field(command_line& cl, const help_t *h)
{
    static_cast<void>(h);

    const wpkg_copyright::copyright_file::field_factory_map_t *fields(wpkg_copyright::copyright_file::field_factory_map());

    const size_t max(cl.size());
    if(max > 1)
    {
        // idx = 0 is the command: "field"
        for(size_t idx(1); idx < max; ++idx)
        {
            const case_insensitive::case_insensitive_string name(cl.get_string("filename", static_cast<int>(idx)));
            wpkg_copyright::copyright_file::field_factory_map_t::const_iterator f(fields->find(name));
            if(f == fields->end())
            {
                printf("wpkg:warning: unknown copyright field \"%s\".\n", name.c_str());
            }
            else
            {
                help_output_copyright_field(cl, f);
            }
        }
    }
    else
    {
        for(wpkg_copyright::copyright_file::field_factory_map_t::const_iterator f(fields->begin()); f != fields->end(); ++f)
        {
            help_output_copyright_field(cl, f);
        }
    }
}



void help_list_of_terms(command_line& cl, const char *msg, const wpkg_control::control_file::list_of_terms_t *t)
{
    if(!cl.quiet())
    {
        printf("%s:\n", msg);
    }
    for(; t->f_term != NULL; ++t)
    {
        if(cl.verbose())
        {
            printf("%s:\n%s\n\n", t->f_term, t->f_help);
        }
        else
        {
            printf("%s\n", t->f_term);
        }
    }
}


void help_build_validations(command_line& cl, const help_t *h)
{
    static_cast<void>(h);

    help_list_of_terms(cl, "List of validations used against a project to build its source package", wpkgar::wpkgar_build::source_validation::list());
}

void help_priorities(command_line& cl, const help_t *h)
{
    static_cast<void>(h);

    help_list_of_terms(cl, "List of properties", wpkg_control::control_file::field_priority_t::list());
}

void help_sections(command_line& cl, const help_t *h)
{
    static_cast<void>(h);

    help_list_of_terms(cl, "List of sections", wpkg_control::control_file::field_section_t::list());
}

void help_urgencies(command_line& cl, const help_t *h)
{
    static_cast<void>(h);

    help_list_of_terms(cl, "List of urgency terms", wpkg_control::control_file::field_urgency_t::list());
}

void help_list(command_line& cl, const help_t *h)
{
    static_cast<void>(h);

    if(!cl.quiet())
    {
        printf("List of help commands:\n");
    }
    for(const help_t *all(advanced_help); all->f_name != NULL; ++all)
    {
        if(cl.verbose())
        {
            printf("%s:\n%s\n\n", all->f_name, all->f_help);
        }
        else
        {
            printf("%s\n", all->f_name);
        }
    }
}

void help_help(command_line& cl, const help_t *h)
{
    static_cast<void>(cl);

    // note that when f_name is NULL, f_help is still defined
    // (i.e. we have a default help in case the user did not enter a
    // name we know about)
    printf("%s\n", h->f_help);
}



const help_t advanced_help[] =
{
    {
        "build-validations",
        help_build_validations,
        "List the validations used to check a project source directory before "
        "creating a source package with wpkg with:\n"
        "   wpkg --build\n\n"
        "The packager will create the source package only after all those "
        "validations ran successfully. If you are interested in just testing "
        "whether your project is ready, then you can use:\n"
        "   wpkg --verify-project\n"
        "which will give you detailed information about each validation that "
        "fails."
    },
    {
        "copyright",
        help_copyright_field,
        "Print help about a copyright field. To have detailed help about "
        "one specific field enter its name after the help command:\n"
        "   wpkg --help copyright files\n"
        "Note that copyright field names are case insensitive."
    },
    {
        "debug",
        help_help,
        "The --debug option can be used to turn on various debug built in "
        "wpkg. These are defined as flags at this point and they need to "
        "be specified as such. A later version will support using names and"
        "automatically convert the names to flags.\n"
        "   000001    Progress information.\n"
        "   000002    Invocation and status of maintainer scripts.\n"
        "   000004    Create graph files (.dot files) that can later be converted to images.\n"
        "   000010    Output for each file processed.\n"
        "   000020    Output for each configuration file.\n"
        "   000040    Details about dependencies and conflicts.\n"
        "   000100    Detailed output for each file being processed.\n"
        "   000200    Detailed output for each configuration file being processed.\n"
        "   000400    Detailed output about dependencies and conflicts.\n"
        "   001000    Database details.\n"
        "   002000    Full details of everything.\n"
        "   004000    Progress information, especially for installations and removals.\n"
        "   010000    Trigger activation and processing (not implemented in wpkg).\n"
        "   020000    Detailed output for each trigger (not implemented in wpkg).\n"
        "   040000    All output about each trigger (not implemented in wpkg).\n"
        "All those flags can be merged (added together)."
    },
    {
        "field",
        help_control_field,
        "Print help about a control file field. To have detailed help about "
        "a specific field enter its name after the help command:\n"
        "   wpkg --help field architecture\n"
        "Note that field names are case insensitive."
    },
    {
        "help",
        help_help,
        "The advanced help system gives you additional help directly from "
        "your command line. Note that if you have Internet access the "
        "website help is certainly a lot more practical as it gives you "
        "links between all the items, see http://windowspackager.org/ for "
        "details.\n\n"
        "To use this help system use the --help command followed by the "
        "name of the help you are interested in. For example:\n"
        "   wpkg --help help\n\n"
        "The list of commands can be found with:\n"
        "   wpkg --help help-list"
    },
    {
        "help-list",
        help_list,
        "List all the help commands that you can use with:\n"
        "   wpkg --help <command>\n"
    },
    {
        "priorities",
        help_priorities,
        "List of priority terms that can be used in the Priority field. "
        "This defines how important a package is in regard to an "
        "installation environment."
    },
    {
        "sections",
        help_sections,
        "List the name of valid Debian sections which can be used with the "
        "Section field. Note that only those sections are valid."
    },
    {
        "urgencies",
        help_urgencies,
        "List of valid urgency terms that can be used with the Urgency "
        "field. Note that Urgency levels have very specific meaning."
    },
    {
        NULL,
        help_help,
        "This help function was not found, to get a list "
        "of valid help functions try:\n"
        "   wpkg --help help-list"
    }
};




void help(command_line& cl)
{
    const case_insensitive::case_insensitive_string cmd(cl.get_string("filename", 0));

    for(const help_t *h(advanced_help);; ++h)
    {
        if(h->f_name == NULL || cmd == h->f_name)
        {
            h->f_func(cl, h);
            break;
        }
    }

    // we do not return
    exit(1);
}


command_line::command_line(int argc, char *argv[], std::vector<std::string> configuration_files)
    : f_opt(argc, argv, wpkg_options, configuration_files, "WPKG_OPTIONS")
    , f_command(static_cast<int>(command_unknown)) // TODO: cast because of enum handling in controlled_vars...
    //, f_quiet(false) -- auto-init
    //, f_verbose(false) -- auto-init
    //, f_dry_run(false) -- auto-init
    , f_zlevel(9)
    //, f_debug_flags(debug_none)
    , f_compressor(memfile::memory_file::file_format_best)
    , f_option("filename")
    //, f_filenames() -- auto-init
{
// these two flags may be tweaked by commands
    f_quiet = f_opt.is_defined("quiet");
    f_verbose = f_opt.is_defined("verbose");
    f_dry_run = f_opt.is_defined("dry-run") || f_opt.is_defined("no-act") || f_opt.is_defined("simulate");

// define the interactive mode between wpkg and the administrator
// (default is no-interactions); needs to be done early on
    std::string interactive(f_opt.get_string("interactive"));
    if(interactive == "no-interactions")
    {
        wpkg_filename::uri_filename::set_interactive(wpkg_filename::uri_filename::wpkgar_interactive_mode_no_interactions);
    }
    else if(interactive == "console")
    {
        wpkg_filename::uri_filename::set_interactive(wpkg_filename::uri_filename::wpkgar_interactive_mode_console);
    }
    else if(interactive == "gui")
    {
        wpkg_filename::uri_filename::set_interactive(wpkg_filename::uri_filename::wpkgar_interactive_mode_gui);
    }
    else
    {
        f_opt.usage(advgetopt::getopt::error, "the --interactive option only accepts \"no-interactions\", \"console\", or \"gui\"");
        /*NOTREACHED*/
    }

// determine command
    // first we check all the commands and determine which
    // command the user used, if the user specified multiple
    // commands then it is an error except in a few cases where
    // two commands get "merged" (i.e. --autoremove + --purge,
    // --build + --install, --build --create-index, etc.)

    //
    // The following accepts:
    //    --autoremove
    //    --purge
    //    --autoremove --purge
    //
    if(f_opt.is_defined("autoremove"))
    {
        set_command(command_autoremove);
    }
    else if(f_opt.is_defined("purge"))
    {
        set_command(command_purge);
    }

    //
    // The following can be merged as following
    //    --build
    //    --build --install   <=>   --build-and-install
    //    --build --create-index
    //    --build --install --create-index
    //
    // Other combos generate an error, for example:
    //    --build --build-and-install
    //    --install --build-and-install
    //    --install --create-index
    //
    if(f_opt.is_defined("build"))
    {
        if(f_opt.is_defined("build-and-install"))
        {
            // generate an error on purpose
            set_command(command_build);
            set_command(command_build_and_install);
        }
        else if(f_opt.is_defined("install"))
        {
            set_command(command_build_and_install);
        }
        else
        {
            set_command(command_build);
        }
    }
    else if(f_opt.is_defined("build-and-install"))
    {
        set_command(command_build_and_install);
        if(f_opt.is_defined("install"))
        {
            // generate an error on purpose
            set_command(command_install);
        }
    }
    else if(f_opt.is_defined("create-index"))
    {
        set_command(command_create_index);
        if(f_opt.is_defined("install"))
        {
            // generate an error on purpose
            set_command(command_install);
        }
    }
    else if(f_opt.is_defined("install"))
    {
        set_command(command_install);
    }

    if(f_opt.is_defined("add-hooks"))
    {
        set_command(command_add_hooks);
    }
    if(f_opt.is_defined("add-sources"))
    {
        set_command(command_add_sources);
    }
    if(f_opt.is_defined("architecture"))
    {
        set_command(command_architecture);
    }
    if(f_opt.is_defined("atleast-version"))
    {
        set_command(command_atleast_version);
    }
    if(f_opt.is_defined("atleast-wpkg-version"))
    {
        set_command(command_atleast_wpkg_version);
    }
    if(f_opt.is_defined("audit"))
    {
        set_command(command_audit);
    }

    if(f_opt.is_defined("canonicalize-version")
    || f_opt.is_defined("canonalize-version"))
    {
        set_command(command_canonicalize_version);
    }
    if(f_opt.is_defined("cflags"))
    {
        set_command(command_cflags);
    }
    if(f_opt.is_defined("check-install"))
    {
        set_command(command_check_install);
    }
    if(f_opt.is_defined("compare-versions"))
    {
        set_command(command_compare_versions);
    }
    if(f_opt.is_defined("compress"))
    {
        set_command(command_compress);
    }
    if(f_opt.is_defined("configure"))
    {
        set_command(command_configure);
    }
    if(f_opt.is_defined("contents"))
    {
        set_command(command_contents);
    }
    if(f_opt.is_defined("control"))
    {
        set_command(command_control);
    }
    if(f_opt.is_defined("copyright"))
    {
        set_command(command_copyright);
    }
    if(f_opt.is_defined("create-admindir"))
    {
        set_command(command_create_admindir);
    }
    if(f_opt.is_defined("create-database-lock"))
    {
        set_command(command_create_database_lock);
    }
    if(f_opt.is_defined("database-is-locked"))
    {
        set_command(command_database_is_locked);
    }
    if(f_opt.is_defined("decompress"))
    {
        set_command(command_decompress);
    }
    if(f_opt.is_defined("deconfigure"))
    {
        set_command(command_deconfigure);
    }
    if(f_opt.is_defined("directory-size"))
    {
        set_command(command_directory_size);
    }
    if(f_opt.is_defined("exact-version"))
    {
        set_command(command_exact_version);
    }
    if(f_opt.is_defined("extract"))
    {
        set_command(command_extract);
    }
    if(f_opt.is_defined("field"))
    {
        set_command(command_field);
    }
    if(f_opt.is_defined("fsys-tarfile"))
    {
        set_command(command_fsys_tarfile);
    }
    if(f_opt.is_defined("help"))
    {
        set_command(command_help);
    }
    if(f_opt.is_defined("help-nobr"))
    {
        set_command(command_help);
    }
    if(f_opt.is_defined("increment-build-number"))
    {
        set_command(command_increment_build_number);
    }
    if(f_opt.is_defined("info"))
    {
        set_command(command_info);
    }
    if(f_opt.is_defined("install-size"))
    {
        set_command(command_install_size);
    }
    if(f_opt.is_defined("is-installed"))
    {
        set_command(command_is_installed);
    }
    if(f_opt.is_defined("libs"))
    {
        set_command(command_libs);
    }
    if(f_opt.is_defined("license") || f_opt.is_defined("licence"))
    {
        set_command(command_license);
    }
    if(f_opt.is_defined("list"))
    {
        set_command(command_list);
    }
    if(f_opt.is_defined("list-all"))
    {
        set_command(command_list_all);
    }
    if(f_opt.is_defined("listfiles"))
    {
        set_command(command_listfiles);
    }
    if(f_opt.is_defined("list-hooks"))
    {
        set_command(command_list_hooks);
    }
    if(f_opt.is_defined("list-index-packages"))
    {
        set_command(command_list_index_packages);
    }
    if(f_opt.is_defined("list-sources"))
    {
        set_command(command_list_sources);
    }
    if(f_opt.is_defined("max-version"))
    {
        set_command(command_max_version);
    }
    if(f_opt.is_defined("md5sums"))
    {
        set_command(command_md5sums);
    }
    if(f_opt.is_defined("md5sums-check"))
    {
        set_command(command_md5sums_check);
    }
    if(f_opt.is_defined("modversion"))
    {
        set_command(command_modversion);
    }
    if(f_opt.is_defined("os"))
    {
        set_command(command_os);
    }
    if(f_opt.is_defined("package_status"))
    {
        set_command(command_package_status);
    }
    if(f_opt.is_defined("print-architecture"))
    {
        set_command(command_print_architecture);
    }
    if(f_opt.is_defined("print-avail"))
    {
        set_command(command_info);
    }
    if(f_opt.is_defined("print-build-number"))
    {
        set_command(command_print_build_number);
    }
    if(f_opt.is_defined("print-variables"))
    {
        set_command(command_print_variables);
    }
    if(f_opt.is_defined("processor"))
    {
        set_command(command_processor);
    }
    if(f_opt.is_defined("reconfigure"))
    {
        set_command(command_reconfigure);
    }
    if(f_opt.is_defined("remove"))
    {
        set_command(command_remove);
    }
    if(f_opt.is_defined("remove-database-lock"))
    {
        set_command(command_remove_database_lock);
    }
    if(f_opt.is_defined("remove-hooks"))
    {
        set_command(command_remove_hooks);
    }
    if(f_opt.is_defined("remove-sources"))
    {
        set_command(command_remove_sources);
    }
    if(f_opt.is_defined("rollback"))
    {
        set_command(command_rollback);
    }
    if(f_opt.is_defined("search"))
    {
        set_command(command_search);
    }
    if(f_opt.is_defined("set-selection"))
    {
        set_command(command_set_selection);
    }
    if(f_opt.is_defined("show"))
    {
        set_command(command_show);
    }
    if(f_opt.is_defined("status"))
    {
        set_command(command_field);
    }
    if(f_opt.is_defined("triplet"))
    {
        set_command(command_triplet);
    }
    if(f_opt.is_defined("unpack"))
    {
        set_command(command_unpack);
    }
    if(f_opt.is_defined("update"))
    {
        set_command(command_update);
    }
    if(f_opt.is_defined("update-status"))
    {
        set_command(command_update_status);
    }
    if(f_opt.is_defined("upgrade"))
    {
        set_command(command_upgrade);
    }
    if(f_opt.is_defined("upgrade-info"))
    {
        set_command(command_upgrade_info);
    }
    if(f_opt.is_defined("upgrade-urgent"))
    {
        set_command(command_upgrade);
    }
    if(f_opt.is_defined("variable"))
    {
        set_command(command_variable);
    }
    if(f_opt.is_defined("verify-control"))
    {
        set_command(command_verify_control);
    }
    if(f_opt.is_defined("verify-project"))
    {
        set_command(command_verify_project);
    }
    if(f_opt.is_defined("vendor"))
    {
        set_command(command_vendor);
    }
    if(f_opt.is_defined("vextract"))
    {
        set_command(command_extract);
        f_verbose = true;
    }
    if(f_opt.is_defined("verify"))
    {
        // we always verify when we load a package, just make it quiet
        set_command(command_info);
        f_quiet = true;
    }
    if(f_opt.is_defined("version"))
    {
        set_command(command_version);
    }

// parse options

    // compression level (1-9)
    f_zlevel = f_opt.get_long("zlevel", 0, 1, 9);

    // compressor name (none, best, gzip, bzip2, xz, lzma)
    if(f_opt.is_defined("compressor"))
    {
        std::string name(f_opt.get_string("compressor"));
        if(name != "best" && name != "default")
        {
            if(name == "gz" || name == "gzip")
            {
                f_compressor = memfile::memory_file::file_format_gz;
            }
            else if(name == "bz2" || name == "bzip2")
            {
                f_compressor = memfile::memory_file::file_format_bz2;
            }
            else if(name == "xz" || name == "7z")
            {
                f_compressor = memfile::memory_file::file_format_xz;
            }
            else if(name == "lzma")
            {
                f_compressor = memfile::memory_file::file_format_lzma;
            }
            else if(name == "none")
            {
                f_compressor = memfile::memory_file::file_format_other;
            }
            else
            {
                f_opt.usage(advgetopt::getopt::error, "supported compressors: gzip, bzip2, lzma, xz, none");
                /*NOTREACHED*/
            }
        }
    }

    // output for log info
    g_output.set_program_name(f_opt.get_program_name());
    if(g_output.get_output_file().empty() && f_opt.is_defined("log-output"))
    {
        g_output.set_output_file(f_opt.get_string("log-output"));
    }
    if(f_verbose)
    {
        f_debug_flags |= wpkg_output::debug_flags::debug_progress;
        g_output.set_level(wpkg_output::level_info);
    }
    else if(f_quiet)
    {
        g_output.set_level(wpkg_output::level_error);
    }

    // check for debug flags
    if(f_opt.is_defined("debug"))
    {
        std::string debug(f_opt.get_string("debug"));
        if(debug.find_first_not_of(debug[0] == '0' ? "01234567" : "0123456789") != std::string::npos)
        {
            f_opt.usage(advgetopt::getopt::error, "the --debug option (-D) only accepts valid decimal or octal numbers");
            /*NOTREACHED*/
        }
        f_debug_flags |= strtol(debug.c_str(), NULL, 0);
        g_output.set_level(wpkg_output::level_debug);
    }
    g_output.set_debug_flags(f_debug_flags);

    // if the default files is turn on, keep the temporary files
    if(f_debug_flags & wpkg_output::debug_flags::debug_detail_files)
    {
        wpkg_filename::temporary_uri_filename::keep_files();
    }

    // check for a user defined temporary directory
    if(f_opt.is_defined("tmpdir"))
    {
        wpkg_filename::temporary_uri_filename::set_tmpdir(f_opt.get_string("tmpdir"));
    }

    // execute the immediate commands
    switch(f_command)
    {
    case command_unknown:
        f_opt.usage(advgetopt::getopt::error, "At least one of the command line options must be a command");
        /*NOTREACHED*/
        break;

    case command_help:
        if(f_opt.size("filename") >= 1)
        {
            help(*this);
            /*NOTREACHED*/
        }
        f_opt.usage(f_opt.is_defined("help-nobr") ? advgetopt::getopt::no_error_nobr : advgetopt::getopt::no_error,
                "Usage: wpkg -<command> [-<opt>] <filename> | <package-name> | <field> ...\nFor detailed help try: wpkg --help help");
        /*NOTREACHED*/
        break;

    case command_version:
        version(*this);
        /*NOTREACHED*/
        break;

    case command_license:
        license::license();
        exit(1);
        /*NOTREACHED*/
        break;

    default:
        // other commands are dealt with later
        break;

    }
}

void command_line::set_command(command_t c)
{
    if(c == command_unknown)
    {
        return;
    }
    if(command_unknown != f_command)
    {
        f_opt.usage(advgetopt::getopt::error, "only one command can be specified in your list of arguments");
        /*NOTREACHED*/
    }
    // TODO: static_cast<> is  to avoid a warning, but the controlled_vars
    //       should allow better handling of enumerations
    f_command = static_cast<int>(c);
}

const advgetopt::getopt& command_line::opt() const
{
    return f_opt;
}

advgetopt::getopt& command_line::opt()
{
    return f_opt;
}

command_line::command_t command_line::command() const
{
    return f_command;
}

int command_line::size() const
{
    if(f_option == "filename" || f_filenames.empty())
    {
        return f_opt.size("filename");
    }
    return static_cast<int>(f_filenames.size());
}

wpkg_filename::uri_filename command_line::filename(int idx) const
{
    if(f_option == "filename" || f_filenames.empty())
    {
        return f_opt.get_string("filename", idx);
    }
    return f_filenames.at(idx);
}

std::string command_line::argument(int idx) const
{
    // do NOT canonicalize an argument
    if(f_option == "filename" || f_filenames.empty())
    {
        return f_opt.get_string("filename", idx);
    }
    return f_filenames.at(idx);
}

std::string command_line::get_string(const std::string& name, int idx) const
{
    if(f_option != name || f_filenames.empty())
    {
        return f_opt.get_string(name, idx);
    }
    return f_filenames.at(idx);
}

bool command_line::quiet() const
{
    return f_quiet;
}

bool command_line::verbose() const
{
    return f_verbose;
}

bool command_line::dry_run(bool msg) const
{
    if(msg && f_dry_run)
    {
        wpkg_output::log("the --dry-run option was used; stopping process now")
            .action("wpkg-dryrun");
    }
    return f_dry_run;
}

int command_line::zlevel() const
{
    return f_zlevel;
}

memfile::memory_file::file_format_t command_line::compressor() const
{
    return f_compressor;
}

void command_line::add_filename(const std::string& option, const std::string& repository_filename)
{
    f_option = option;
    f_filenames.push_back(repository_filename);
}


} // no name namespace




void define_wpkg_running_and_copy(const command_line& cl, wpkg_filename::uri_filename& wpkg_running, wpkg_filename::uri_filename& wpkg_copy)
{
    wpkg_running.set_filename(cl.opt().get_program_fullname());
    wpkg_filename::uri_filename wpkg_dir(wpkg_running.dirname());
    wpkg_filename::uri_filename program_name(cl.opt().get_program_name());
    if(program_name.segment_size() == 0)
    {
        throw std::runtime_error("the program name is an empty string");
    }
    std::string filename(program_name.segment(program_name.segment_size() - 1));
    case_insensitive::case_insensitive_string pn(filename.substr(0, 8));
    if(pn != "copy-of-")
    {
        program_name = program_name.dirname();
        program_name = program_name.append_child("copy-of-" + filename);
    }
    if(program_name.is_absolute())
    {
        wpkg_copy = program_name.path_only();
    }
    else
    {
        wpkg_copy = wpkg_dir.append_child(program_name.path_only());
    }
}

void init_manager(command_line& cl, wpkgar::wpkgar_manager& manager, const std::string& option)
{
    // add self so we can deal with the case when we're upgrading ourself
    //
    // if you write an application that links against the libdebpackages
    // library and you want to allow for auto-upgrades, then you will need
    // to add all your dependencies to your instance of the manager. This
    // includes your package, its direct dependencies, the dependencies
    // of those dependencies, etc. However, you probably do not want to
    // implement a copy because you'd need to copy your application plus
    // all the DLLs it needs to run. If you implement an upgrade for your
    // application, you may want to run wpkg using execvp() as shown below.
    // Although at this point there is no way for you to restart your
    // application afterward with wpkg itself, you could write a small
    // shell script or batch file and run that with the right information
    // and the last command line would be used to restart your app.
    manager.add_self("wpkg");
#ifdef MO_MINGW32
    manager.add_self("wpkg-mingw32");
#endif
    {
        // if wpkg upgraded itself then it created a copy of itself; these
        // few lines of code check for the existence of that copy and
        // if it exists deletes it
        wpkg_filename::uri_filename wpkg_running;
        wpkg_filename::uri_filename wpkg_copy;
        define_wpkg_running_and_copy(cl, wpkg_running, wpkg_copy);
        if(!same_file(wpkg_running.os_filename().get_utf8(), wpkg_copy.os_filename().get_utf8())
        && wpkg_copy.exists())
        {
            wpkg_copy.os_unlink();
        }
    }

    manager.set_interrupt_handler(&interrupt);

    // all these directories have a default if not specified on the command line
    manager.set_root_path(cl.opt().get_string("root"));
    manager.set_inst_path(cl.opt().get_string("instdir"));
    manager.set_database_path(cl.opt().get_string("admindir"));

    std::shared_ptr<wpkgar::wpkgar_tracker> tracker;
    if(cl.opt().is_defined("tracking-journal"))
    {
        const std::string journal(cl.opt().get_string("tracking-journal"));
        tracker.reset(new wpkgar::wpkgar_tracker(&manager, journal));
        tracker->keep_file(true);
        manager.set_tracker(tracker);
        manager.track("# tracking " + option + " on " + wpkg_output::generate_timestamp());
        wpkg_output::log("tracking journal: %1")
                .quoted_arg(journal)
            .level(wpkg_output::level_info)
            .action("log");
    }

    if(cl.opt().is_defined("repository"))
    {
        std::string repositories("repositories ");
        int max_repository(cl.opt().size("repository"));
        for(int i(0); i < max_repository; ++i)
        {
            // no need to protect " inside f_repository names
            // since that character is not legal (because it
            // is not legal under MS-Windows.)
            repositories += " \"" + cl.opt().get_string("repository", i) + "\"";

            manager.add_repository(cl.opt().get_string("repository", i));
        }
        if(tracker)
        {
            manager.track(repositories);
        }
    }
}


void init_installer
    ( command_line& cl
    , wpkgar::wpkgar_manager& manager
    , wpkgar::wpkgar_install& pkg_install
    , const std::string& option
    , const wpkg_filename::uri_filename& package_name = wpkg_filename::uri_filename()
    )
{
    init_manager(cl, manager, option);

    const int max(cl.opt().size(option));
    if(max == 0)
    {
        throw std::runtime_error("--" + option + " requires at least one parameter");
    }

    // add the force, no-force/refuse parameters
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_architecture,
        (cl.opt().is_defined("force-architecture") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-architecture")
                            && !cl.opt().is_defined("refuse-architecture")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_breaks,
        (cl.opt().is_defined("force-breaks") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-breaks")
                            && !cl.opt().is_defined("refuse-breaks")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_configure_any,
        (cl.opt().is_defined("force-configure-any") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-configure-any")
                            && !cl.opt().is_defined("refuse-configure-any")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_conflicts,
        (cl.opt().is_defined("force-conflicts") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-conflicts")
                            && !cl.opt().is_defined("refuse-conflicts")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_depends,
        (cl.opt().is_defined("force-depends") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-depends")
                            && !cl.opt().is_defined("refuse-depends")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_depends_version,
        (cl.opt().is_defined("force-depends-version") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-depends-version")
                            && !cl.opt().is_defined("refuse-depends-version")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_distribution,
        (cl.opt().is_defined("force-distribution") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-distribution")
                            && !cl.opt().is_defined("refuse-distribution")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_downgrade,
        (cl.opt().is_defined("force-downgrade") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-downgrade")
                            && !cl.opt().is_defined("refuse-downgrade")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_file_info,
        (cl.opt().is_defined("force-file-info") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-file-info")
                            && !cl.opt().is_defined("refuse-file-info")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_hold,
        (cl.opt().is_defined("force-hold") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-hold")
                            && !cl.opt().is_defined("refuse-hold")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_upgrade_any_version,
        (cl.opt().is_defined("force-upgrade-any-version") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-upgrade-any-version")
                            && !cl.opt().is_defined("refuse-upgrade-any-version")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_overwrite,
        (cl.opt().is_defined("force-overwrite") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-overwrite")
                            && !cl.opt().is_defined("refuse-overwrite")
                            && !cl.opt().is_defined("refuse-all"));
    // overwriting directories is just way too ugly to include in --force-all
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_overwrite_dir,
        (cl.opt().is_defined("force-overwrite-dir") /*|| cl.opt().is_defined("force-all")*/)
                            && !cl.opt().is_defined("no-force-overwrite-dir")
                            && !cl.opt().is_defined("refuse-overwrite-dir")
                            && !cl.opt().is_defined("refuse-all"));
    // the rollback is kind of a positive thing so we don't use it on --force-all
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_rollback,
        (cl.opt().is_defined("force-rollback") /*|| cl.opt().is_defined("force-all")*/)
                            && !cl.opt().is_defined("no-force-rollback")
                            && !cl.opt().is_defined("refuse-rollback")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_vendor,
        (cl.opt().is_defined("force-vendor") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-vendor")
                            && !cl.opt().is_defined("refuse-vendor")
                            && !cl.opt().is_defined("refuse-all"));

    // some additional parameters
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_skip_same_version, cl.opt().is_defined("skip-same-version"));
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_recursive, cl.opt().is_defined("recursive"));

    // add the list of verify-fields expressions if any
    if(cl.opt().is_defined("verify-fields"))
    {
        const int fields_max(cl.opt().size("verify-fields"));
        for(int i(0); i < fields_max; ++i)
        {
            pkg_install.add_field_validation(cl.opt().get_string("verify-fields", i));
        }
    }

    // add the list of package names
    if(package_name.empty())
    {
        for(int i(0); i < max; ++i)
        {
            const std::string& name( cl.get_string( option, i ) );
            pkg_install.add_package( name, cl.opt().is_defined( "force-reinstall" ) );
        }

        if( pkg_install.count() == 0 )
        {
            wpkg_output::log("You are attempting to install one or more packages that are already installed. Nothing done! Use '--force-reinstall' to force a reinstallation.")
                    .level(wpkg_output::level_warning)
                    .module(wpkg_output::module_configure_package)
                    .action("install-validation");
            exit(0);
        }
        //
        if( pkg_install.count() != max )
        {
            wpkg_output::log("One or more packages you specified for installation are already installed. See the '--force-reinstall' option.")
                    .level(wpkg_output::level_warning)
                    .module(wpkg_output::module_configure_package)
                    .action("install-validation");
        }
    }
    else
    {
        pkg_install.add_package(package_name.full_path());
    }
}


void init_field_variables
    ( command_line& cl
    , wpkgar::wpkgar_manager& manager
    , wpkg_field::field_file *field
    )
{
    const int max = cl.opt().size("field-variables");
    for(int i(0); i < max; ++i)
    {
        std::string fv(cl.opt().get_string("field-variables", i));
        std::string::size_type p(fv.find('='));
        if(p == std::string::npos)
        {
            cl.opt().usage(advgetopt::getopt::error, "--field-variables (-V) only accepts variable definitions that include an equal sign");
            /*NOTREACHED*/
        }
        if(p == 0)
        {
            cl.opt().usage(advgetopt::getopt::error, "the name of a variable in --field-variables (-V) cannot be empty (name expected before the equal sign)");
            /*NOTREACHED*/
        }
        std::string name(fv.substr(0, p));
        std::string value(fv.substr(p + 1));
        if(value.length() > 1 && value[0] == '"' && *value.rbegin() == '"')
        {
            value = value.substr(1, value.length() - 2);
        }
        else if(value.length() > 1 && value[0] == '\'' && *value.rbegin() == '\'')
        {
            value = value.substr(1, value.length() - 2);
        }
        if(field) // directly to a control file list of variables
        {
            field->set_variable(name, value);
        }
        else
        {
            manager.set_field_variable(name, value);
        }
    }
}

void check_install(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    wpkgar::wpkgar_install pkg_install(&manager);
    init_installer(cl, manager, pkg_install, "check-install");
    pkg_install.set_installing();

    bool result;
    {
        wpkgar::wpkgar_lock lock_wpkg(&manager, "Verifying");
        result = pkg_install.validate();
    }
    exit(result ? 0 : 1);
}

void install(command_line& cl, const wpkg_filename::uri_filename package_name = wpkg_filename::uri_filename(), const std::string& option = "install")
{
    wpkgar::wpkgar_manager manager;
    wpkgar::wpkgar_install pkg_install(&manager);
    init_installer(cl, manager, pkg_install, option, package_name);
    pkg_install.set_installing();

    wpkgar::wpkgar_lock lock_wpkg(&manager, "Installing");

    if( pkg_install.validate() && !cl.dry_run())
    {
        {
            wpkgar::wpkgar_install::install_info_list_t install_list = pkg_install.get_install_list();
            std::stringstream explicit_packages;
            std::stringstream implicit_packages;
            std::for_each( install_list.begin(), install_list.end(),
                           [&](const wpkgar::wpkgar_install::install_info_t& info )
            {
                switch( info.get_install_type() )
                {
                case wpkgar::wpkgar_install::install_info_t::install_type_explicit:
                    if( !explicit_packages.str().empty() )
                    {
                        explicit_packages << ", ";
                    }
                    explicit_packages << info.get_name();
                    break;
                case wpkgar::wpkgar_install::install_info_t::install_type_implicit:
                    if( !implicit_packages.str().empty() )
                    {
                        implicit_packages << ", ";
                    }
                    implicit_packages << info.get_name();
                    break;
                default:
                    assert(0);
                }
            }
            );

            if( !implicit_packages.str().empty() )
            {
                std::stringstream ss;
                ss  << "the following new packages are required dependencies, and will be installed indirectly: "
                    << implicit_packages.str();
                wpkg_output::log(ss.str().c_str())
                    .level(wpkg_output::level_info)
                    .module(wpkg_output::module_validate_installation)
                    .package("wpkg")
                    .action("upgrade-initialization");
            }

            if( !explicit_packages.str().empty() )
            {
                std::stringstream ss;
                ss  << "the following new packages will be installed directly: "
                    << explicit_packages.str();
                wpkg_output::log(ss.str().c_str())
                    .level(wpkg_output::level_info)
                    .module(wpkg_output::module_validate_installation)
                    .package("wpkg")
                    .action("upgrade-initialization");
            }
        }

        if(manager.is_self() && !cl.opt().is_defined("running-copy"))
        {
            // in this case we drop the lock; our copy will re-create a lock as required
            lock_wpkg.unlock();

            wpkg_output::log("wpkg is trying to upgrade itself; wpkg is starting a copy of itself to ensure proper functionality")
                .level(wpkg_output::level_warning)
                .module(wpkg_output::module_validate_installation)
                .package("wpkg")
                .action("upgrade-initialization");

            // we test as "is self" and not "running copy" to determine
            // that we're doing an upgrade of self; we do not attempt to
            // check whether the executable itself would be replaced
            wpkg_filename::uri_filename wpkg_running;
            wpkg_filename::uri_filename wpkg_copy;
            define_wpkg_running_and_copy(cl, wpkg_running, wpkg_copy);
            if(same_file(wpkg_running.os_filename().get_utf8(), wpkg_copy.os_filename().get_utf8()))
            {
                // this should not happen, but we cannot really prevent it from happening
                wpkg_output::log("it looks like wpkg was inadvertently started from a copy of itself while attempting to upgrade itself")
                    .level(wpkg_output::level_fatal)
                    .module(wpkg_output::module_validate_installation)
                    .package("wpkg")
                    .action("upgrade-initialization");
                return;
            }
            if(wpkg_copy.exists())
            {
                wpkg_output::log("somehow the file %1 already exists so wpkg cannot upgrade itself at this time")
                        .quoted_arg(wpkg_copy)
                    .level(wpkg_output::level_fatal)
                    .module(wpkg_output::module_validate_installation)
                    .package("wpkg")
                    .action("upgrade-initialization");
                return;
            }
            // copy the file
            memfile::memory_file wpkg_binary;
            wpkg_binary.read_file(wpkg_running);
            wpkg_binary.write_file(wpkg_copy);
            // start the copy with the same arguments + "--running-copy"
#ifdef MO_WINDOWS
            // to be Unicode compliant under MS-Windows we have to convert
            // all the parameters to wide character strings
            std::vector<std::wstring> wargs;
            for(char **v = g_argv; *v != NULL; ++v)
            {
                // Yeah! Windows is weird, we pass the exact arguments, but we still
                // need to ourselves quote them...
                std::wstring warg(libutf8::mbstowcs(wpkg_util::make_safe_console_string(*v)));
                wargs.push_back(warg);
            }
            //
            // *** WARNING ***
            // DO NOT MERGE THESE TWO LOOPS!!!
            // *** WARNING ***
            //
            std::vector<wchar_t *> wargv;
            for(std::vector<std::wstring>::const_iterator it(wargs.begin()); it != wargs.end(); ++it)
            {
                wargv.push_back(const_cast<wchar_t *>(it->c_str()));
            }
            std::wstring cmdline(libutf8::mbstowcs(wpkg_copy.path_only()));
            wargv[0] = const_cast<wchar_t *>(cmdline.c_str());
            wargv.push_back(const_cast<wchar_t *>(L"--running-copy"));
            wargv.push_back(NULL);
            _wexecvp(wargv[0], &wargv[0]);
#else
            if(chmod(wpkg_copy.os_filename().get_utf8().c_str(), 600) != 0)
            {
                wpkg_output::log("we could not set the execution permission on the wpkg copy: %1")
                        .quoted_arg(wpkg_copy)
                    .level(wpkg_output::level_fatal)
                    .module(wpkg_output::module_validate_installation)
                    .package("wpkg")
                    .action("upgrade-initialization");
                return;
            }
            std::vector<char *> argv;
            for(char **v(g_argv); *v != NULL; ++v)
            {
                argv.push_back(*v);
            }
            std::string cmdline(wpkg_copy.path_only());
            argv[0] = const_cast<char *>(cmdline.c_str());
            argv.push_back(const_cast<char *>("--running-copy"));
            argv.push_back(NULL);
            execvp(argv[0], &argv[0]);
#endif
            perror("execvp to run a wpkg copy failed");
            wpkg_copy.os_unlink();
            wpkg_output::log("execution of the wpkg copy executable somehow failed; original executable: %1")
                    .quoted_arg(wpkg_running)
                .level(wpkg_output::level_fatal)
                .module(wpkg_output::module_validate_installation)
                .package("wpkg")
                .action("upgrade-initialization");
            return;
        }
        if(pkg_install.pre_configure())
        {
            for(;;)
            {
                manager.check_interrupt();

                const int i(pkg_install.unpack());
                if(i < 0)
                {
                    break;
                }
                if(!pkg_install.configure(i))
                {
                    break;
                }
            }
        }
    }
}


void install_size(command_line& cl)
{
    int max(cl.opt().size("install-size"));
    if(max == 0)
    {
        throw std::runtime_error("--install-size requires at least one package name");
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "contents");

    unsigned long total(0);
    for(int i(0); i < max; ++i)
    {
        const std::string name(cl.opt().get_string("install-size", i));
        if(name.find_first_of("_/") == std::string::npos)
        {
            // TODO: support already installed packages?
            cl.opt().usage(advgetopt::getopt::error, "--install-size does not work with already installed packages");
            /*NOTREACHED*/
        }
        // make sure the package is loaded
        manager.load_package(name);
        if(manager.field_is_defined(name, "Installed-Size"))
        {
            total += manager.get_field_integer(name, "Installed-Size");
        }
    }

    printf("%lu\n", total);
}

void unpack(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    wpkgar::wpkgar_install pkg_install(&manager);
    init_installer(cl, manager, pkg_install, "unpack");
    pkg_install.set_unpacking();

    wpkgar::wpkgar_lock lock_wpkg(&manager, "Installing");
    if(pkg_install.validate() && !cl.dry_run())
    {
        for(;;)
        {
            manager.check_interrupt();

            if(pkg_install.unpack() < 0)
            {
                break;
            }
        }
    }
}

void update_status(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "update-status");
    wpkgar::wpkgar_repository repository(&manager);

    wpkgar::wpkgar_lock lock_wpkg(&manager, "Updating");
    const wpkgar::wpkgar_repository::update_entry_vector_t *index_entries(repository.load_index_list());
    if(index_entries == NULL)
    {
        printf("The --update command line option was never used.\n");
    }
    else if(index_entries->size() == 0)
    {
        printf("The sources.list file is empty and no repository index was loaded.\n");
    }
    else
    {
        size_t max(index_entries->size());
        for(size_t i(0); i < max; ++i)
        {
            const wpkgar::wpkgar_repository::update_entry_t& e(index_entries->at(i));
            printf("%3d. %s\n", e.get_index(), e.get_uri().c_str());
            const char *status;
            switch(e.get_status())
            {
            case wpkgar::wpkgar_repository::update_entry_t::status_unknown:
                status = "unknown";
                break;

            case wpkgar::wpkgar_repository::update_entry_t::status_ok:
                status = "ok";
                break;

            case wpkgar::wpkgar_repository::update_entry_t::status_failed:
                status = "failed";
                break;

            default:
                status = "?undefined?";
                break;

            }
            printf("     Last Status: %s\n", status);
            printf("     First Try On: %s\n", wpkg_util::rfc2822_date(e.get_time(wpkgar::wpkgar_repository::update_entry_t::first_try)).c_str());
            if(e.get_time(wpkgar::wpkgar_repository::update_entry_t::first_success) == 0)
            {
                printf("     Never Succeeded.\n");
            }
            else
            {
                printf("     First Success On: %s\n", wpkg_util::rfc2822_date(e.get_time(wpkgar::wpkgar_repository::update_entry_t::first_success)).c_str());
                printf("     Last Success On: %s\n", wpkg_util::rfc2822_date(e.get_time(wpkgar::wpkgar_repository::update_entry_t::last_success)).c_str());
            }
            if(e.get_time(wpkgar::wpkgar_repository::update_entry_t::last_failure) == 0)
            {
                printf("     Never Failed.\n");
            }
            else
            {
                printf("     Last Failure On: %s\n", wpkg_util::rfc2822_date(e.get_time(wpkgar::wpkgar_repository::update_entry_t::last_failure)).c_str());
            }
        }
    }
}

void update(command_line& cl)
{
    // TODO -- we want to run repository.update(cl.dry_run()) instead... (once implemented)
    if(cl.dry_run())
    {
        update_status(cl);
    }
    else
    {
        wpkgar::wpkgar_manager manager;
        init_manager(cl, manager, "update");
        wpkgar::wpkgar_repository repository(&manager);

        wpkgar::wpkgar_lock lock_wpkg(&manager, "Updating");
        repository.update();
    }
}

void upgrade_info(command_line& cl)
{
    if(cl.size() != 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--upgrade-info cannot be used with any filenames");
        /*NOTREACHED*/
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "upgrade-info");
    wpkgar::wpkgar_repository repository(&manager);

    wpkgar::wpkgar_lock lock_wpkg(&manager, "Upgrading");
    const wpkgar::wpkgar_repository::wpkgar_package_list_t& list(repository.upgrade_list());
    size_t max(list.size());
    for(size_t i(0); i < max; ++i)
    {
        const wpkgar::wpkgar_repository::package_item_t& pkg(list[i]);
        const std::string package_name(pkg.get_name());

        switch(pkg.get_status())
        {
        case wpkgar::wpkgar_repository::package_item_t::not_installed:
            if(cl.verbose())
            {
                printf("package \"%s\" version %s is available for installation.\n", package_name.c_str(), pkg.get_version().c_str());
            }
            break;

        case wpkgar::wpkgar_repository::package_item_t::need_upgrade:
            {
                case_insensitive::case_insensitive_string urgency(pkg.field_is_defined("Urgency") ? pkg.get_field("Urgency") : "low");
                bool urgent(urgency == "high" || urgency == "emergency" || urgency == "critical");
                printf("package \"%s\" will be upgraded to version %s the next time you run with --upgrade%s\n",
                    package_name.c_str(), pkg.get_version().c_str(),
                    urgent ? " or --upgrade-urgent" : "");
                if(cl.verbose())
                {
                    printf("   full URI is \"%s\"\n", pkg.get_info().get_uri().full_path().c_str());
                }
            }
            break;

        case wpkgar::wpkgar_repository::package_item_t::blocked_upgrade:
            printf("package \"%s\" will NOT be upgraded because auto-upgrades are currently blocked\n", package_name.c_str());
            break;

        case wpkgar::wpkgar_repository::package_item_t::installed:
            if(cl.verbose())
            {
                printf("package \"%s\" is installed from the newest available version.\n", package_name.c_str());
            }
            break;

        case wpkgar::wpkgar_repository::package_item_t::invalid:
            if(cl.verbose())
            {
                printf("package \"%s\" is considered invalid: %s\n", package_name.c_str(), pkg.get_cause_for_rejection().c_str());
            }
            break;

        }
    }
}

void upgrade(command_line& cl)
{
    bool urgent_only(cl.opt().is_defined("upgrade-urgent"));
    const char *cmd(urgent_only ? "upgrade-urgent" : "upgrade");

    if(cl.size() != 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--%s cannot be used with any filenames", cmd);
        /*NOTREACHED*/
    }
    if(cl.opt().is_defined("force-hold"))
    {
        cl.opt().usage(advgetopt::getopt::error, "--%s cannot be used with --force-hold", cmd);
        /*NOTREACHED*/
    }

    if(cl.dry_run())
    {
        upgrade_info(cl);
        return;
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, cmd);
    wpkgar::wpkgar_repository repository(&manager);

    {
        // the install() call creates its own lock, we need to have this
        // one removed before we can move on
        wpkgar::wpkgar_lock lock_wpkg(&manager, "Upgrading");
        const wpkgar::wpkgar_repository::wpkgar_package_list_t& list(repository.upgrade_list());
        const size_t max(list.size());
        for(size_t i(0); i < max; ++i)
        {
            if(list[i].get_status() == wpkgar::wpkgar_repository::package_item_t::need_upgrade)
            {
                bool skip(urgent_only);
                if(skip)
                {
                    case_insensitive::case_insensitive_string urgency(list[i].field_is_defined("Urgency") ? list[i].get_field("Urgency") : "low");
                    skip = urgency != "high" && urgency != "emergency" && urgency != "critical";
                }
                if(!skip)
                {
                    const memfile::memory_file::file_info& info(list[i].get_info());
                    const wpkg_filename::uri_filename filename(info.get_uri());
                    cl.add_filename(cmd, filename.full_path());

                    // show info if --verbose specified
                    wpkg_output::log("package %1 marked for upgrade to version %2")
                            .quoted_arg(filename.full_path())
                            .arg(list[i].get_version())
                        .module(wpkg_output::module_repository)
                        .package(filename)
                        .action("upgrade-initialization");
                }
            }
        }
    }

    if(cl.size() == 0)
    {
        wpkg_output::log("no packages to upgrade at this time")
            .level(wpkg_output::level_warning)
            .module(wpkg_output::module_repository)
            .action("upgrade");
        return;
    }

    // the install function will use a new manager (defined inside the
    // install() function itself) to make sure we do not leak unwanted
    // data from the upgrade list computation
    install(cl, wpkg_filename::uri_filename(), cmd);
}

void vendor(command_line& cl)
{
    if(cl.verbose())
    {
        // if you create a fork, change the [original] entry with information
        // about your own version, for example: [fork of 0.7.3]
        printf("%s (%s) [original]\n", debian_packages_vendor(), debian_packages_version_string());
    }
    else
    {
        printf("%s\n", debian_packages_vendor());
    }
}

void configure(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    wpkgar::wpkgar_install pkg_install(&manager);
    init_installer(cl, manager, pkg_install, "configure");
    pkg_install.set_configuring();

    // if pending is set then we want to read the database and configure any
    // half-installed packages

    wpkgar::wpkgar_lock lock_wpkg(&manager, "Installing");
    if(pkg_install.validate() && !cl.dry_run())
    {
        // TODO: test that all the specified packages are indeed unpacked
        //       before attempting to configure them
        const int max(pkg_install.count());
        for(int i(0); i < max; ++i)
        {
            manager.check_interrupt();

            // TODO: the order is wrong because we do need to first
            //       configure packages that don't have unpacked
            //       dependencies
            if(!pkg_install.configure(i))
            {
                break;
            }
        }
    }
}

void reconfigure(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    wpkgar::wpkgar_install pkg_install(&manager);
    init_installer(cl, manager, pkg_install, "reconfigure");
    pkg_install.set_reconfiguring();

    wpkgar::wpkgar_lock lock_wpkg(&manager, "Installing");
    if(pkg_install.validate() && !cl.dry_run())
    {
        for(;;)
        {
            manager.check_interrupt();

            int i(pkg_install.reconfigure());
            if(i < 0)
            {
                break;
            }
            if(!pkg_install.configure(i))
            {
                break;
            }
        }
    }
}

void is_installed(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "is-installed");
    std::string name(cl.get_string("is-installed"));
    if(manager.safe_package_status(name) == wpkgar::wpkgar_manager::installed)
    {
        // true, it is installed
        if(cl.verbose())
        {
            printf("true\n");
        }
        exit(0);
    }
    // "error," it is not installed
    if(cl.verbose())
    {
        printf("false\n");
    }
    exit(1);
}

void add_hooks(command_line& cl)
{
    const int max(cl.size());
    if(max == 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--add-hooks expects at least one global hook script filename");
        /*NOTREACHED*/
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "add-hooks");
    for(int i(0); i < max; ++i)
    {
        manager.add_global_hook(cl.argument(i).c_str());
    }
}

void remove_hooks(command_line& cl)
{
    const int max(cl.size());
    if(max == 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--remove-hooks expects at least one global hook script name");
        /*NOTREACHED*/
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "remove-hooks");
    for(int i(0); i < max; ++i)
    {
        if(!manager.remove_global_hook(cl.argument(i).c_str()))
        {
            wpkg_output::log("global hook %1 could not be removed because it was not installed.")
                    .quoted_arg(cl.argument(i))
                .level(wpkg_output::level_warning)
                .action("pkg-config");
        }
    }
}

void list_hooks(command_line& cl)
{
    if(cl.size() != 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--list-hooks does not expects any parameter");
        /*NOTREACHED*/
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "list-hooks");
    wpkgar::wpkgar_manager::hooks_t hooks(manager.list_hooks());
    const wpkgar::wpkgar_manager::hooks_t::size_type max(hooks.size());
    bool first(true);
    for(wpkgar::wpkgar_manager::hooks_t::size_type i(0); i < max; ++i)
    {
        const std::string name(hooks[i]);
        if(name.substr(0, 5) == "core_")
        {
            if(first)
            {
                first = false;
                printf("Global Hooks:\n");
            }
            printf("  %s\n", name.substr(5).c_str());
        }
    }
    first = true;
    for(wpkgar::wpkgar_manager::hooks_t::size_type i(0); i < max; ++i)
    {
        const std::string name(hooks[i]);
        if(name.substr(0, 5) != "core_")
        {
            if(first)
            {
                first = false;
                printf("Package Hooks:\n");
            }
            printf("  %s\n", name.c_str());
        }
    }
}

void add_sources(command_line& cl)
{
    int max(cl.size());
    if(max == 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--add-sources expects at least one entry");
        /*NOTREACHED*/
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "add-sources");
    wpkg_filename::uri_filename name(manager.get_database_path());
    name = name.append_child("core/sources.list");
    wpkgar::wpkgar_repository repository(&manager);
    wpkgar::wpkgar_repository::source_vector_t sources;
    memfile::memory_file sources_file;
    if(name.exists())
    {
        sources_file.read_file(name);
        sources_file.printf("\n");
    }
    else
    {
        sources_file.create(memfile::memory_file::file_format_other);
    }
    for(int i(0); i < max; ++i)
    {
        sources_file.printf("%s\n", cl.argument(i).c_str());
    }
    repository.read_sources(sources_file, sources);
    sources_file.create(memfile::memory_file::file_format_other);
    repository.write_sources(sources_file, sources);
    sources_file.write_file(name);
}

void architecture(command_line& cl)
{
    if(cl.size() != 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--architecture does not take any parameters.");
        /*NOTREACHED*/
    }

    if(cl.verbose())
    {
        printf("%s (%s)\n", debian_packages_architecture(), debian_packages_machine());
    }
    else
    {
        printf("%s\n", debian_packages_architecture());
    }
}

void atleast_version(command_line& cl)
{
    if(cl.size() != 1)
    {
        cl.opt().usage(advgetopt::getopt::error, "--atleast-version expects exactly two parameters: wpkg --atleast-version <version> <package name>.");
        /*NOTREACHED*/
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "atleast-version");
    const std::string package_name(cl.argument(0));
    manager.load_package(package_name);
    const std::string version(manager.get_field(package_name, wpkg_control::control_file::field_version_factory_t::canonicalized_name()));

    if(wpkg_util::versioncmp(version, cl.opt().get_string("atleast-version")) < 0)
    {
        // version is too small
        exit(1);
    }
}

void atleast_wpkg_version(command_line& cl)
{
    if(cl.size() != 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--atleast-wpkg-version takes exactly one parameter.");
        /*NOTREACHED*/
    }

    if(wpkg_util::versioncmp(debian_packages_version_string(), cl.opt().get_string("atleast-wpkg-version")) < 0)
    {
        // version is too small
        exit(1);
    }
}

void exact_version(command_line& cl)
{
    if(cl.size() != 1)
    {
        cl.opt().usage(advgetopt::getopt::error, "--exact-version expects exactly two parameters: wpkg --exact-version <version> <package name>.");
        /*NOTREACHED*/
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "exact-version");
    const std::string package_name(cl.argument(0));
    manager.load_package(package_name);
    const std::string version(manager.get_field(package_name, wpkg_control::control_file::field_version_factory_t::canonicalized_name()));

    if(wpkg_util::versioncmp(version, cl.opt().get_string("exact-version")) != 0)
    {
        // version is not equal
        exit(1);
    }
}

void max_version(command_line& cl)
{
    if(cl.size() != 1)
    {
        cl.opt().usage(advgetopt::getopt::error, "--max-version expects exactly two parameters: wpkg --max-version <version> <package name>.");
        /*NOTREACHED*/
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "max-version");
    const std::string package_name(cl.argument(0));
    manager.load_package(package_name);
    const std::string version(manager.get_field(package_name, wpkg_control::control_file::field_version_factory_t::canonicalized_name()));

    if(wpkg_util::versioncmp(version, cl.opt().get_string("max-version")) > 0)
    {
        // version is not equal
        exit(1);
    }
}

void processor(command_line& cl)
{
    if(cl.size() != 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--processor does not take any parameters.");
        /*NOTREACHED*/
    }

    if(cl.verbose())
    {
        printf("%s (%s)\n", debian_packages_processor(), debian_packages_machine());
    }
    else
    {
        printf("%s\n", debian_packages_processor());
    }
}

void audit(command_line& cl)
{
    int max(cl.size());
    if(max != 0)
    {
        printf("error:%s: --audit does not take any parameters.\n",
            cl.opt().get_program_name().c_str());
        exit(1);
    }

    // TODO: check that directories exist as expected by the module
    // TODO: check file attributes (under Win32 it would be the read-only flag)
    // TODO: offer help to fix the problems when -v is used
    int err(0);
    {
        // we must have the manager within a sub-block to make sure that the
        // database lock gets removed before we call exit()
        wpkgar::wpkgar_manager manager;
        init_manager(cl, manager, "audit");
        // auditing is very similar to listing so at this point we use that status
        wpkgar::wpkgar_lock lock_wpkg(&manager, "Listing");
        wpkgar::wpkgar_manager::package_list_t list;
        manager.list_installed_packages(list);

        const wpkg_filename::uri_filename package_path(manager.get_inst_path());
        for(wpkgar::wpkgar_manager::package_list_t::const_iterator it(list.begin());
                it != list.end(); ++it)
        {
            try
            {
                if(cl.verbose())
                {
                    printf("working on %s\n", it->c_str());
                }
                wpkgar::wpkgar_manager::package_status_t status(manager.package_status(*it));
                bool check_md5sums(false);
                switch(status)
                {
                case wpkgar::wpkgar_manager::not_installed:
                case wpkgar::wpkgar_manager::config_files:
                case wpkgar::wpkgar_manager::installing:
                case wpkgar::wpkgar_manager::upgrading:
                case wpkgar::wpkgar_manager::removing:
                case wpkgar::wpkgar_manager::purging:
                    break;

                case wpkgar::wpkgar_manager::unpacked:
                case wpkgar::wpkgar_manager::installed:
                    check_md5sums = true;
                    break;

                case wpkgar::wpkgar_manager::no_package:
                    printf("%s: package is missing\n", it->c_str());
                    ++err;
                    break;

                case wpkgar::wpkgar_manager::unknown:
                    printf("%s: package could not be loaded\n", it->c_str());
                    ++err;
                    break;

                case wpkgar::wpkgar_manager::half_installed:
                    printf("%s: package is half installed\n", it->c_str());
                    ++err;
                    break;

                case wpkgar::wpkgar_manager::half_configured:
                    printf("%s: package is half configured\n", it->c_str());
                    ++err;
                    // although in a bad state, all the files are expected to
                    // be properly unpacked for this one
                    check_md5sums = true;
                    break;

                // unexpected status for a package
                case wpkgar::wpkgar_manager::listing:
                case wpkgar::wpkgar_manager::verifying:
                case wpkgar::wpkgar_manager::ready:
                    printf("%s: package has an invalid status\n", it->c_str());
                    ++err;
                    break;

                }
                if(check_md5sums)
                {
                    // the package is already loaded, just get the md5sums file
                    // and the wpkgar file
                    memfile::memory_file md5sums_file;
                    wpkg_util::md5sums_map_t md5sums;
                    if(manager.has_control_file(*it, "md5sums"))
                    {
                        // if the file is not present there should be no regular
                        // or continuous files in the package... (just dirs?!)
                        std::string md5filename("md5sums");
                        manager.get_control_file(md5sums_file, *it, md5filename, false);
                        wpkg_util::parse_md5sums(md5sums, md5sums_file);
                    }
                    memfile::memory_file *wpkgar_file;
                    manager.get_wpkgar_file(*it, wpkgar_file);
                    wpkgar_file->set_package_path(package_path);
                    wpkgar_file->dir_rewind();
                    for(;;)
                    {
                        memfile::memory_file::file_info info;
                        memfile::memory_file data;
                        if(!wpkgar_file->dir_next(info, &data))
                        {
                            break;
                        }
                        std::string filename(info.get_filename());
                        if(filename[0] == '/')
                        {
                            switch(info.get_file_type())
                            {
                            case memfile::memory_file::file_info::regular_file:
                            case memfile::memory_file::file_info::continuous:
                                {
                                    const wpkg_filename::uri_filename fullname(package_path.append_child(filename));
                                    //printf("%s: %s\n", it->c_str(), fullname.c_str());
                                    filename.erase(0, 1);
                                    if(md5sums.find(filename) != md5sums.end())
                                    {
                                        std::string sum(data.md5sum());
                                        if(md5sums[filename] != sum)
                                        {
                                            if(!manager.is_conffile(*it, filename))
                                            {
                                                printf("%s: file \"%s\" md5sum differs\n",
                                                        it->c_str(),
                                                        fullname.original_filename().c_str());
                                                ++err;
                                            }
                                            else if(cl.verbose())
                                            {
                                                printf("%s: configuration file \"%s\" was modified\n", it->c_str(), fullname.original_filename().c_str());
                                            }
                                        }
                                        // remove the entry so we can err in case some
                                        // md5sums were not used up (why are they defined?)
                                        md5sums.erase(md5sums.find(filename));
                                    }
                                    else
                                    {
                                        printf("%s: file \"%s\" is not defined in the list of md5sums\n",
                                                    it->c_str(),
                                                    fullname.original_filename().c_str());
                                        ++err;
                                    }
                                }
                                break;

                            default:
                                // other types are not checked they don't have
                                // an md5sum anyway
                                break;

                            }
                        }
                    }
                    if(!md5sums.empty())
                    {
                        for(wpkg_util::md5sums_map_t::const_iterator m5(md5sums.begin());
                                m5 != md5sums.end(); ++m5)
                        {
                            const wpkg_filename::uri_filename fullname(package_path.append_child(m5->first));
                            printf("%s: package has file \"%s\" in its md5sums file but not in its wpkgar index\n",
                                it->c_str(), fullname.original_filename().c_str());
                            ++err;
                        }
                    }
                }
            }
            catch(const std::exception&)
            {
                printf("%s: package could not be loaded\n", it->c_str());
                ++err;
            }
        }
    }
    if(cl.verbose() && err > 0)
    {
        printf("%d error%s found while auditing\n", err, (err != 1 ? "s" : ""));
    }

    // exit with an error if anything failed while auditing
    exit(err == 0 ? 0 : 1);
}


void create_index(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    wpkgar::wpkgar_repository pkg_repository(&manager);
    init_manager(cl, manager, "create-index");

    pkg_repository.set_parameter(wpkgar::wpkgar_repository::wpkgar_repository_recursive, cl.opt().is_defined("recursive"));

    // check for a set of repository names
    if(manager.get_repositories().empty())
    {
        cl.opt().usage(advgetopt::getopt::error, "--create-index requires at least one --repository name");
        /*NOTREACHED*/
    }

    // check that the extension matches as expected
    std::string archive(cl.get_string("create-index"));
    memfile::memory_file::file_format_t ar_format(memfile::memory_file::filename_extension_to_format(archive, true));
    switch(ar_format)
    {
    case memfile::memory_file::file_format_tar:
        break;

    case memfile::memory_file::file_format_ar:
    case memfile::memory_file::file_format_zip:
    case memfile::memory_file::file_format_7z:
    case memfile::memory_file::file_format_wpkg:
        cl.opt().usage(advgetopt::getopt::error, "unsupported archive file extension (we only support .tar for a repository index)");
        /*NOTREACHED*/
        break;

    default:
        cl.opt().usage(advgetopt::getopt::error, "unsupported archive file extension (we support .deb, .a, .tar)");
        /*NOTREACHED*/
        break;

    }

    // create the output
    memfile::memory_file index;
    pkg_repository.create_index(index);

    if(index.size() == 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "the resulting index is empty; please specify the right repository(ies) and the --recursive option if necessary");
        /*NOTREACHED*/
    }

    // check whether a compression is defined
    memfile::memory_file::file_format_t format(memfile::memory_file::filename_extension_to_format(archive));
    switch(format)
    {
    case memfile::memory_file::file_format_gz:
    case memfile::memory_file::file_format_bz2:
    case memfile::memory_file::file_format_lzma:
    case memfile::memory_file::file_format_xz:
        {
            memfile::memory_file compressed;
            index.compress(compressed, format);
            compressed.write_file(archive);
        }
        break;

    default: // we don't prevent any extension here
        index.write_file(archive);
        break;

    }
}


void build(command_line& cl, wpkg_filename::uri_filename& package_name, const std::string& option = "build")
{
    const bool do_create_index(cl.opt().is_defined("create-index"));
    if(do_create_index && !cl.opt().is_defined("output-repository-dir"))
    {
        cl.opt().usage(advgetopt::getopt::error, "when --build is used with --create-index, then --output-repository-dir must be defined.");
        /*NOTREACHED*/
    }

    bool need_lock(false);
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, option);
    std::shared_ptr<wpkgar::wpkgar_build> pkg_build;
    if(cl.size() == 0)
    {
        // build a source package; `pwd` is the root of the project
        pkg_build.reset(new wpkgar::wpkgar_build(&manager, ""));
    }
    else
    {
        // build a binary package
        pkg_build.reset(new wpkgar::wpkgar_build(&manager, cl.get_string("filename", 0)));
        if(cl.size() == 2)
        {
            // binary package is created from a control file
            pkg_build->set_extra_path(cl.filename(1));
        }
        else if(cl.size() != 1)
        {
            cl.opt().usage(advgetopt::getopt::error, "--build accepts zero, one, or two file parameters.");
            /*NOTREACHED*/
        }
        else
        {
            wpkg_filename::uri_filename filename(cl.filename(0));
#ifdef WINDOWS
            case_insensitive::case_insensitive_string ext(filename.extension());
#else
            std::string ext(filename.extension());
#endif
            if(ext == "deb")
            {
                // if compiling a source package, check a few more options
                pkg_build->set_parameter(wpkgar::wpkgar_build::wpkgar_build_recursive, cl.opt().is_defined("recursive"));
                pkg_build->set_parameter(wpkgar::wpkgar_build::wpkgar_build_run_unit_tests, cl.opt().is_defined("run-unit-tests"));
                pkg_build->set_parameter(wpkgar::wpkgar_build::wpkgar_build_force_file_info, cl.opt().is_defined("force-file-info"));
                if(cl.opt().is_defined("install-prefix"))
                {
                    pkg_build->set_install_prefix(cl.opt().get_string("install-prefix"));
                }
                need_lock = true;
            }
            else if(filename.is_dir())
            {
                // if compiling a source repository
                if(cl.opt().is_defined("install-prefix"))
                {
                    pkg_build->set_install_prefix(cl.opt().get_string("install-prefix"));
                }
            }
        }
    }

    pkg_build->set_zlevel(cl.zlevel());
    pkg_build->set_compressor(cl.compressor());
    if(cl.opt().is_defined("enforce-path-length-limit"))
    {
        if(cl.opt().is_defined("path-length-limit"))
        {
            cl.opt().usage(advgetopt::getopt::error, "--enforce-path-length-limit and --path-length-limit cannot be used together.");
            /*NOTREACHED*/
        }
        pkg_build->set_path_length_limit(-cl.opt().get_long("enforce-path-length-limit"));
    }
    else if(cl.opt().is_defined("path-length-limit"))
    {
        pkg_build->set_path_length_limit(cl.opt().get_long("path-length-limit"));
    }
    if(cl.opt().is_defined("output-filename"))
    {
        pkg_build->set_filename(cl.opt().get_string("output-filename", 0));
    }
    if(cl.opt().is_defined("output-dir"))
    {
        pkg_build->set_output_dir(cl.opt().get_string("output-dir", 0));
    }
    if(cl.opt().is_defined("output-repository-dir"))
    {
        pkg_build->set_output_repository_dir(cl.opt().get_string("output-repository-dir", 0));
    }
    if(cl.opt().is_defined("cmake-generator"))
    {
        pkg_build->set_cmake_generator(cl.opt().get_string("cmake-generator"));
    }
    if(cl.opt().is_defined("make-tool"))
    {
        pkg_build->set_make_tool(cl.opt().get_string("make-tool"));
    }
    if(cl.opt().is_defined("build-number-filename"))
    {
        pkg_build->set_build_number_filename(cl.opt().get_string("build-number-filename"));
    }
    pkg_build->set_parameter(wpkgar::wpkgar_build::wpkgar_build_ignore_empty_packages, cl.opt().is_defined("ignore-empty-packages"));
    if(cl.opt().is_defined("clear-exceptions"))
    {
        pkg_build->add_exception("");
    }
    const int max(cl.opt().size("exception"));
    for(int i(0); i < max; ++i)
    {
        std::string e(cl.opt().get_string("exception", i));
        pkg_build->add_exception(e);
    }
    pkg_build->set_program_fullname(cl.opt().get_program_fullname());
    init_field_variables(cl, manager, NULL);

    {
        std::shared_ptr<wpkgar::wpkgar_lock> lock_wpkg;
        if(need_lock)
        {
            lock_wpkg.reset(new wpkgar::wpkgar_lock(&manager, "Building"));
        }
        pkg_build->build();

        // we have to reset the tracker now or the rollback happens at
        // the wrong time (i.e. after the manager gets unlocked)
        const std::shared_ptr<wpkgar::wpkgar_tracker> null_tracker;
        manager.set_tracker(null_tracker);
    }

    if(do_create_index)
    {
        // TBD: do we really want that? shall we share the list from the
        //      main() function?
        std::vector<std::string> configuration_files;
        configuration_files.push_back("/etc/wpkg/wpkg.conf");
        configuration_files.push_back("~/.config/wpkg/wpkg.conf");

        const std::string repository_directory(cl.opt().get_string("output-repository-dir"));
        wpkg_filename::uri_filename output_dir(repository_directory);
        output_dir = output_dir.append_child(cl.opt().get_string("create-index"));
        const std::string create_index_param(output_dir.full_path());
        const char *argv[] =
        {
            "wpkg",
            "--create-index",
            create_index_param.c_str(),
            "--recursive",
            "--repository",
            repository_directory.c_str(),
            NULL
        };
        printf("wpkg --create-index %s --recursive --repository %s\n", create_index_param.c_str(), repository_directory.c_str());
        command_line sub_cl(sizeof(argv) / sizeof(argv[0]) - 1, const_cast<char **>(argv), configuration_files);
        create_index(sub_cl);
    }

    // for the build_and_install() function
    package_name = pkg_build->get_package_name();
}

void build_and_install(command_line& cl)
{
    wpkg_filename::uri_filename package_name;
    build(cl, package_name, "build-and-install");
    if(!package_name.empty())
    {
        // we install only if the build did not find out that the package
        // was empty (if empty then the name remains undefined)
        install(cl, package_name, "build-and-install");
    }
}

void verify_control(command_line& cl)
{
    const int max(cl.size());
    if(max == 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--verify-control must be used with at least one control filename.");
        /*NOTREACHED*/
    }

    for(int i(0); i < max; ++i)
    {
        memfile::memory_file ctrl_input;
        ctrl_input.read_file(cl.filename(i));
        wpkg_control::binary_control_file ctrl(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::control_file_state_t));
        ctrl.set_input_file(&ctrl_input);
        ctrl.read();
        ctrl.set_input_file(NULL);
        printf("Verified %s\n", ctrl.get_field(wpkg_control::control_file::field_package_factory_t::canonicalized_name()).c_str());
    }
}

void verify_project(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "verify-project");
    std::shared_ptr<wpkgar::wpkgar_build> pkg_build;
    if(cl.size() != 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--verify-project does not accept any arguments.");
        /*NOTREACHED*/
    }
    // validate a project environment; `pwd` is the root of the project
    pkg_build.reset(new wpkgar::wpkgar_build(&manager, ""));

    if(cl.opt().is_defined("clear-exceptions"))
    {
        pkg_build->add_exception("");
    }
    const int max(cl.opt().size("exception"));
    for(int i(0); i < max; ++i)
    {
        std::string e(cl.opt().get_string("exception", i));
        pkg_build->add_exception(e);
    }
    init_field_variables(cl, manager, NULL);

    // now run the validation of the source
    wpkgar::wpkgar_build::source_validation sv;
    wpkg_control::source_control_file controlinfo_fields;
    if(!pkg_build->validate_source(sv, controlinfo_fields))
    {
        const wpkgar::wpkgar_build::source_validation::source_properties_t& p(sv.get_properties());
        for(wpkgar::wpkgar_build::source_validation::source_properties_t::const_iterator i(p.begin()); i != p.end(); ++i)
        {
            // if unknown it was not yet worked on, avoid the error message for now
            if(i->second.get_status() != wpkgar::wpkgar_build::source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID
            && i->second.get_status() != wpkgar::wpkgar_build::source_validation::source_property::SOURCE_VALIDATION_STATUS_UNKNOWN)
            {
                printf("\n%s is not valid:\n  %s\n", i->second.get_name(), i->second.get_help());
            }
        }
        return;
    }

    if(cl.verbose())
    {
        printf("Your project is valid. You can build a source package with: wpkg --build\n");
    }
}


bool is_letter(char c)
{
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z');
}

void canonicalize_version(command_line& cl)
{
    if(cl.opt().size("canonicalize-version") != 1)
    {
        fprintf(stderr, "error:%s: --canonicalize-version expects exactly 1 parameter.\n",
            cl.opt().get_program_name().c_str());
        exit(1);
    }
    char err[256];
    std::string org(cl.opt().get_string("canonicalize-version", 0));
    debian_version_handle_t v(string_to_debian_version(org.c_str(), err, sizeof(err)));
    if(v == NULL)
    {
        fprintf(stderr, "error:%s: version \"%s\" is not a valid Debian version: %s.\n",
                cl.opt().get_program_name().c_str(), org.c_str(), err);
        exit(1);
    }
    char version[256];
    int r(debian_version_to_string(v, version, sizeof(version)));
    if(r == -1)
    {
        fprintf(stderr, "error:%s: version \"%s\" could not be canonicalized (too long? %d).\n",
                cl.opt().get_program_name().c_str(), org.c_str(), errno);
        exit(1);
    }
    printf("%s\n", version);
}

void compare_versions(command_line& cl)
{
    if(cl.opt().size("compare-versions") != 3)
    {
        fprintf(stderr, "error:%s: --compare-versions expects exactly 3 parameters.\n",
            cl.opt().get_program_name().c_str());
        exit(255);
    }

    char err[256];
    std::string v1(cl.opt().get_string("compare-versions", 0));
    std::string v2(cl.opt().get_string("compare-versions", 2));
    int c(0);
    if(!v1.empty() && !v2.empty())
    {
        debian_version_handle_t a(string_to_debian_version(v1.c_str(), err, sizeof(err)));
        if(a == NULL)
        {
            fprintf(stderr, "error:%s: version \"%s\" is not a valid Debian version: %s.\n",
                    cl.opt().get_program_name().c_str(), v1.c_str(), err);
            exit(255);
        }
        debian_version_handle_t b(string_to_debian_version(v2.c_str(), err, sizeof(err)));
        if(b == NULL)
        {
            fprintf(stderr, "error:%s: version \"%s\" is not a valid Debian version: %s.\n",
                    cl.opt().get_program_name().c_str(), v2.c_str(), err);
            exit(255);
        }

        c = debian_versions_compare(a, b);
    }
    else
    {
        if(v1.empty())
        {
            c = v2.empty() ? 0 : -1;
        }
        else /*if(v2.empty()) -- this must be true here */
        {
            c = 1;
        }
    }

    std::string op(cl.opt().get_string("compare-versions", 1));
    int r(0);
    if(op == "=" || op == "==" || op == "eq")
    {
        r = c == 0;
    }
    else if(op == "!=" || op == "<>" || op == "ne")
    {
        // op != and <> are not Debian compatible
        r = c != 0;
    }
    else if(op == "<=" || op == "le")
    {
        r = c <= 0;
    }
    else if(op == "<" || op == "<<" || op == "lt")
    {
        r = c < 0;
    }
    else if(op == ">=" || op == "ge")
    {
        r = c >= 0;
    }
    else if(op == ">" || op == ">>" || op == "gt")
    {
        r = c > 0;
    }
    else if(op == "lt-nl")
    {
        if(v1.empty() || v2.empty())
        {
            r = c > 0;
        }
        else
        {
            r = c < 0;
        }
    }
    else if(op == "le-nl")
    {
        if(v1.empty() || v2.empty())
        {
            r = c >= 0;
        }
        else
        {
            r = c <= 0;
        }
    }
    else if(op == "gt-nl")
    {
        if(v1.empty() || v2.empty())
        {
            r = c < 0;
        }
        else
        {
            r = c > 0;
        }
    }
    else if(op == "ge-nl")
    {
        if(v1.empty() || v2.empty())
        {
            r = c <= 0;
        }
        else
        {
            r = c >= 0;
        }
    }

    if(cl.verbose())
    {
        printf("%s\n", r ? "true" : "false");
    }

    exit(r ? 0 : 1);
}

void compress(command_line& cl)
{
    int max(cl.size());
    if(max == 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--compress expects at least one parameter on the command line");
        /*NOTREACHED*/
    }
    const bool force_hold((cl.opt().is_defined("force-hold") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-hold")
                            && !cl.opt().is_defined("refuse-hold")
                            && !cl.opt().is_defined("refuse-all"));
    const bool force_overwrite((cl.opt().is_defined("force-overwrite") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-overwrite")
                            && !cl.opt().is_defined("refuse-overwrite")
                            && !cl.opt().is_defined("refuse-all"));
    std::string output;
    if(cl.opt().is_defined("output-filename"))
    {
        if(max == 1)
        {
            output = cl.opt().get_string("output-filename");
        }
        else
        {
            cl.opt().usage(advgetopt::getopt::error, "--output-filename can only be used if --compress is used with a single filename");
            /*NOTREACHED*/
        }
    }
    for(int i(0); i < max; ++i)
    {
        wpkg_filename::uri_filename filename(cl.get_string("filename"));
        memfile::memory_file::file_format_t format(cl.compressor());
        if(format == memfile::memory_file::file_format_best)
        {
            if(output.empty())
            {
                format = memfile::memory_file::filename_extension_to_format(filename);
            }
            else
            {
                format = memfile::memory_file::filename_extension_to_format(output);
            }
        }
        switch(format)
        {
        case memfile::memory_file::file_format_gz:
        case memfile::memory_file::file_format_bz2:
        case memfile::memory_file::file_format_lzma:
        case memfile::memory_file::file_format_xz:
            {
                wpkg_filename::uri_filename old_filename;
                wpkg_filename::uri_filename new_filename;
                if(cl.compressor() == memfile::memory_file::file_format_best
                && (output.empty() || format == memfile::memory_file::file_format_best))
                {
                    if(!output.empty())
                    {
                        cl.opt().usage(advgetopt::getopt::error, "--output-filename can only be used if --compress is used with --compressor when no known extension is used");
                        /*NOTREACHED*/
                    }
                    wpkg_filename::uri_filename dir(filename.dirname());
                    old_filename = dir.append_child(filename.basename(true));
                    new_filename = filename;
                }
                else
                {
                    old_filename = filename;
                    if(output.empty())
                    {
                        switch(cl.compressor())
                        {
                        case memfile::memory_file::file_format_gz:
                            new_filename.set_filename(filename.full_path() + ".gz");
                            break;

                        case memfile::memory_file::file_format_bz2:
                            new_filename.set_filename(filename.full_path() + ".bz2");
                            break;

                        case memfile::memory_file::file_format_lzma:
                            new_filename.set_filename(filename.full_path() + ".lzma");
                            break;

                        case memfile::memory_file::file_format_xz:
                            new_filename.set_filename(filename.full_path() + ".xz");
                            break;

                        default:
                            throw std::logic_error("the file format from --compressor is not supported");

                        }
                    }
                    else
                    {
                        new_filename = output;
                    }
                }

                // TODO: add warning and/or support for --force-overwrite?
                if(!new_filename.exists() || force_overwrite)
                {
                    if(new_filename.exists())
                    {
                        printf("wpkg:warning: overwriting \"%s\" with compressed version.\n", new_filename.full_path().c_str());
                    }
                    memfile::memory_file decompressed;
                    if(cl.verbose())
                    {
                        printf("wpkg: compress \"%s\" to \"%s\".\n", old_filename.full_path().c_str(), new_filename.full_path().c_str());
                    }
                    decompressed.read_file(old_filename);
                    memfile::memory_file compressed;
                    decompressed.compress(compressed, format, cl.zlevel());
                    compressed.write_file(new_filename);
                    if(!force_hold)
                    {
                        old_filename.os_unlink();
                    }
                }
                else if(cl.verbose())
                {
                    printf("wpkg: file \"%s\" already exists, no compression performed.\n", filename.full_path().c_str());
                }
            }
            break;

        default: // silently ignore others
            if(cl.verbose())
            {
                printf("wpkg: unknown compression extension, ignoring \"%s\".\n", filename.full_path().c_str());
            }
            break;

        }
    }
}

void decompress(command_line& cl)
{
    int max(cl.size());
    if(max == 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--decompress expects at least one parameter on the command line");
        /*NOTREACHED*/
    }
    const bool force_hold((cl.opt().is_defined("force-hold") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-hold")
                            && !cl.opt().is_defined("refuse-hold")
                            && !cl.opt().is_defined("refuse-all"));
    const bool force_overwrite((cl.opt().is_defined("force-overwrite") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-overwrite")
                            && !cl.opt().is_defined("refuse-overwrite")
                            && !cl.opt().is_defined("refuse-all"));
    std::string output;
    if(cl.opt().is_defined("output-filename"))
    {
        if(max == 1)
        {
            output = cl.opt().get_string("output-filename");
        }
        else
        {
            cl.opt().usage(advgetopt::getopt::error, "--output-filename can only be used if --decompress is used with a single filename");
            /*NOTREACHED*/
        }
    }
    for(int i(0); i < max; ++i)
    {
        wpkg_filename::uri_filename filename(cl.get_string("filename"));
        memfile::memory_file::file_format_t format(memfile::memory_file::filename_extension_to_format(filename));
        switch(format)
        {
        case memfile::memory_file::file_format_gz:
        case memfile::memory_file::file_format_bz2:
        case memfile::memory_file::file_format_lzma:
        case memfile::memory_file::file_format_xz:
            {
                wpkg_filename::uri_filename dir(filename.dirname());
                wpkg_filename::uri_filename new_filename(output.empty() ? dir.append_child(filename.basename(true)) : output);
                if(!new_filename.exists() || force_overwrite)
                {
                    if(new_filename.exists())
                    {
                        printf("wpkg:warning: overwriting \"%s\" from compressed version.\n", new_filename.full_path().c_str());
                    }
                    if(cl.verbose())
                    {
                        printf("wpkg: decompress \"%s\" to \"%s\".\n", filename.full_path().c_str(), new_filename.full_path().c_str());
                    }
                    memfile::memory_file compressed;
                    compressed.read_file(filename);
                    memfile::memory_file decompressed;
                    compressed.decompress(decompressed);
                    decompressed.write_file(new_filename);
                    if(!force_hold)
                    {
                        filename.os_unlink();
                    }
                }
                else if(cl.verbose())
                {
                    printf("wpkg: file \"%s\" already exists, no decompression performed.\n", filename.full_path().c_str());
                }
            }
            break;

        default: // silently ignore others
            if(cl.verbose())
            {
                printf("wpkg: unknown compression extension or already uncompressed file, ignoring \"%s\".\n", filename.full_path().c_str());
            }
            break;

        }
    }
}

void contents(command_line& cl)
{
    // TODO: support listing contents of more than one package?
    if(cl.size() != 0)
    {
        fprintf(stderr, "error:%s: too many parameters on the command line for --contents.\n",
            cl.opt().get_program_name().c_str());
        exit(1);
    }
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "contents");
    manager.set_control_file_state(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::contents_control_file_state_t));
    const wpkg_filename::uri_filename name(cl.get_string("contents"));
    if(name.is_deb())
    {
        cl.opt().usage(advgetopt::getopt::error, "you cannot extract the files of the data.tar.gz file from an installed package");
        /*NOTREACHED*/
    }
    bool numbers(cl.opt().is_defined("numbers"));
    manager.load_package(name);
    memfile::memory_file p;
    std::string data_filename("data.tar");
    manager.get_control_file(p, name, data_filename, false);
    bool use_drive_letter(false);
    if(manager.field_is_defined(name, "X-Drive-Letter"))
    {
        use_drive_letter = manager.get_field_boolean(name, "X-Drive-Letter");
    }
    p.dir_rewind();
    for(;;)
    {
        memfile::memory_file::file_info info;
        memfile::memory_file data;
        if(!p.dir_next(info, &data))
        {
            break;
        }
        std::string filename(info.get_filename());
        if(filename.length() >= 2 && filename[0] == '.' && filename[1] == '/')
        {
            filename.erase(0, 1);
        }
        if(use_drive_letter && filename.length() >= 3 && filename[0] == '/' && is_letter(filename[1]) && filename[2] == '/')
        {
            filename[0] = filename[1] & 0x5F; // capital letter for drives
            filename[1] = ':';
        }
        if(!cl.quiet())
        {
            if(numbers)
            {
                printf("%3o ", info.get_mode());
            }
            else
            {
                printf("%s ", info.get_mode_flags().c_str());
            }
            std::string user(info.get_user());
            std::string group(info.get_group());
            if(numbers || user.empty() || group.empty())
            {
                printf("%4d/%-4d", info.get_uid(), info.get_gid());
            }
            else
            {
                printf("%8.8s/%-8.8s", user.c_str(), group.c_str());
            }
            if(info.get_file_type() == memfile::memory_file::file_info::character_special
            || info.get_file_type() == memfile::memory_file::file_info::block_special)
            {
                printf(" %3d,%3d", info.get_dev_major(), info.get_dev_minor());
            }
            else
            {
                printf(" %7d", info.get_size());
            }
            printf("  %s %c%s",
                info.get_date().c_str(),
                manager.is_conffile(name, filename) ? '*' : ' ',
                filename.c_str());
            if(info.get_file_type() == memfile::memory_file::file_info::symbolic_link)
            {
                printf(" -> %s", info.get_link().c_str());
            }
            printf("\n");
        }
        else
        {
            printf("%s\n", filename.c_str());
        }
    }
}

void control(command_line& cl)
{
    if(cl.size() > 1)
    {
        printf("error:%s: too many parameters on the command line for --control.\n",
            cl.opt().get_program_name().c_str());
        exit(1);
    }
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "control");
    manager.set_control_file_state(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::contents_control_file_state_t));
    std::string name(cl.get_string("control"));
    manager.load_package(name);
    memfile::memory_file p;
    std::string control_filename("control.tar");
    manager.get_control_file(p, name, control_filename);
    if(cl.size() == 1)
    {
        // if an output folder is specified, extract the files there
        const wpkg_filename::uri_filename output_path(cl.filename(0));
        // reset the filename
        control_filename = "control.tar";
        manager.get_control_file(p, name, control_filename, false);
        p.dir_rewind();
        for(;;)
        {
            memfile::memory_file::file_info info;
            memfile::memory_file data;
            if(!p.dir_next(info, &data))
            {
                break;
            }
            const wpkg_filename::uri_filename out(output_path.append_safe_child(info.get_filename()));
            if(cl.verbose())
            {
                printf("%s\n", out.original_filename().c_str());
            }
            switch(info.get_file_type())
            {
            case memfile::memory_file::file_info::regular_file:
            case memfile::memory_file::file_info::continuous:
                data.write_file(out, true);
                break;

            case memfile::memory_file::file_info::symbolic_link:
                {
                    const wpkg_filename::uri_filename link(info.get_link());
                    link.os_symlink(out);
                }
                break;

            default:
                // other file types are ignored here
                break;

            }
        }
    }
    else
    {
        // NOTE: instead dpkg creates a directory named DEBIAN by default
        p.write_file(control_filename);
    }
}

void copyright(command_line& cl)
{
    if(cl.size() != 0)
    {
        printf("error:%s: --copyright expects exactly one package name.\n",
            cl.opt().get_program_name().c_str());
        exit(1);
    }
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "copyright");
    manager.set_control_file_state(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::contents_control_file_state_t));
    const wpkg_filename::uri_filename name(cl.get_string("copyright"));
    if(name.is_deb())
    {
        // already installed package, check in the installation tree
        wpkg_filename::uri_filename copyright_filename(manager.get_root_path());
        copyright_filename = copyright_filename.append_child("usr/share/doc");
        copyright_filename = copyright_filename.append_child(name.path_only());
        copyright_filename = copyright_filename.append_child("copyright");
        if(copyright_filename.exists())
        {
            memfile::memory_file data;
            data.read_file(copyright_filename);
            int offset(0);
            std::string line;
            while(data.read_line(offset, line))
            {
                printf("%s\n", line.c_str());
            }
            return;
        }
    }
    else
    {
        manager.load_package(name);
        memfile::memory_file p;
        std::string data_filename("data.tar");
        manager.get_control_file(p, name, data_filename, false);
        p.dir_rewind();
        // TODO: do we need to check the package name validity or was it done
        //       in the control file already?
        std::string package(manager.get_field(name, "Package"));
        case_insensitive::case_insensitive_string copyright_filename("usr/share/doc/" + package + "/copyright");
        for(;;)
        {
            memfile::memory_file::file_info info;
            memfile::memory_file data;
            if(!p.dir_next(info, &data))
            {
                break;
            }
            std::string filename(info.get_filename());
            if(filename[0] == '.' && filename[1] == '/')
            {
                filename = filename.substr(2);
            }
            if(copyright_filename == filename.c_str())
            {
                // found the file, print it in stdout
                int offset(0);
                std::string line;
                while(data.read_line(offset, line))
                {
                    printf("%s\n", line.c_str());
                }
                return;
            }
        }
    }
    // this is not exactly an error under MS-Windows
    fprintf(stderr, "error: the copyright file was not found (it is not mandatory because MS-Windows is not as restricted as Linux.)\n");
    exit(1);
    /*NOTREACHED*/
}

void create_admindir(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "create-admindir");
    manager.create_database(cl.get_string("create-admindir"));
}

void create_database_lock(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "create-database-lock");
    try {
        // in this case we keep the status set as "Ready"
        manager.lock("Ready");
        if(cl.verbose())
        {
            printf("database lock was created.\n");
        }
        exit(0);
    }
    catch(...)
    {
        fprintf(stderr, "error: that database could not be locked, maybe it is already locked.\n");
        exit(1);
    }
}

void database_is_locked(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "database-is-locked");
    if(manager.is_locked())
    {
        if(cl.verbose())
        {
            printf("true\n");
        }
        exit(0);
    }
    else
    {
        if(cl.verbose())
        {
            printf("false\n");
        }
        exit(1);
    }
}

void directory_size(command_line& cl)
{
    // this function is mainly for debug purposes in case we wanted to fix the
    // version used by the build so it works more like what we need for our
    // packages (wpkg --directory-size <dirname>)
    memfile::memory_file in;
    long total_size(0);
    std::string dir_name(cl.get_string("directory-size"));
    in.dir_rewind(dir_name, true);
    for(;;)
    {
        memfile::memory_file::file_info info;
        if(!in.dir_next(info, NULL))
        {
            break;
        }
        memfile::memory_file::file_info::file_type_t type(info.get_file_type());
        if(type == memfile::memory_file::file_info::regular_file
        || type == memfile::memory_file::file_info::continuous)
        {
            // round up the size to the next block
            // TODO: let users define the block size
            long size((info.get_size() + 511) & -512);
            if(cl.verbose())
            {
                printf("%9ld %s\n", size, info.get_filename().c_str());
            }
            total_size += size;
        }
    }
    if(cl.verbose())
    {
        printf("%9ld\n", total_size);
    }
    else
    {
        printf("%ld\n", total_size);
    }
}

void os(command_line& cl)
{
    if(cl.verbose())
    {
        printf("%s by %s [%s]\n", debian_packages_os(), debian_packages_vendor(), debian_packages_processor());
    }
    else
    {
        printf("%s\n", debian_packages_os());
    }
}

void triplet(command_line& /*cl*/)
{
    // triplet as you'd want to have it in the Architecture field
    printf("%s-%s-%s\n", debian_packages_os(), debian_packages_vendor(), debian_packages_processor());
}

void print_field(const std::string& field_name, const std::string& value)
{
    if(!field_name.empty())
    {
        printf("%s: ", field_name.c_str());
    }
    for(const char *s(value.c_str()); *s != '\0'; ++s)
    {
        printf("%c", *s);
        if(*s == '\n')
        {
            printf(" ");
        }
    }
    printf("\n");
}

void field(command_line& cl)
{
    // TBD: The fields are printed whatever the current status of
    //      the package; should we warn/err if the package is not
    //      properly unpacked, installed, and a few other states?
    //      The user can use --is-installed first though.
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "field");
    manager.set_control_file_state(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::contents_control_file_state_t));
    std::string name(cl.get_string("field"));
    manager.load_package(name);
    int max(cl.size());
    if(max == 0)
    {
        // if no field specified, show them all
        max = manager.number_of_fields(name);
        for(int i(0); i < max; ++i)
        {
            std::string field_name(manager.get_field_name(name, i));
            std::string value(manager.get_field(name, field_name));
            if(field_name != "X-Status" || value != "unknown")
            {
                print_field(field_name, value);
            }
        }
    }
    else if(max == 1)
    {
        std::string field_name(cl.argument(0));
        std::string value(manager.get_field(name, field_name));
        print_field("", value);
    }
    else
    {
        // only read those fields (throw if not available)
        for(int i(0); i < max; ++i)
        {
            std::string field_name(cl.argument(i));
            std::string value(manager.get_field(name, field_name));
            print_field(field_name, value);
        }
    }
}

void display_pkgconfig(command_line& cl, const std::string& field_name, const std::string& option)
{
    int max(cl.size());
    if(max == 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "%s expects at least one package name", option.c_str());
        /*NOTREACHED*/
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, option);

    std::string paths(wpkg_util::utf8_getenv("PKG_CONFIG_PATH", ""));
    std::vector<std::string> path_list;
#if defined(MO_WINDOWS) || defined(MO_MINGW32)
    const char sep(';');
#else
    const char sep(':');
#endif
    if(!paths.empty())
    {
        paths += sep;
    }
    // TODO: how do we NOT add the /usr part? under MS-Windows we are not
    //       expected to have a /usr
    paths += "/usr/lib/pkgconfig";
    paths += sep;
    paths += "/usr/share/pkgconfig"; // default always defined (but put last)
    const char *n;
    for(const char *start(paths.c_str()); *start != '\0'; start = n)
    {
        for(n = start; *n != sep && *n != '\0'; ++n);
        path_list.push_back(std::string(start, n - start));
        for(; *n == sep; ++n);
    }

    class pkgconfig_state_t : public wpkg_field::field_file::field_file_state_t
    {
    public:
        virtual bool allow_transformations() const
        {
            return true;
        }

        virtual bool accept_sub_packages() const
        {
            return false;
        }
    };

    // check all the named packages
    bool first(true);
    const wpkg_filename::uri_filename instdir(manager.get_inst_path());
    for(int i(0); i < max; ++i)
    {
        bool found(false);
        std::string package_name(cl.argument(i));

        std::shared_ptr<wpkg_field::field_file::field_file_state_t> state(std::shared_ptr<wpkg_field::field_file::field_file_state_t>(new pkgconfig_state_t));
        for(std::vector<std::string>::const_iterator p(path_list.begin()); p != path_list.end(); ++p)
        {
            const wpkg_filename::uri_filename path(*p);
            wpkg_filename::uri_filename pcfile;
            if(path.is_absolute())
            {
                pcfile = path.append_child(package_name + ".pc");
            }
            else
            {
                pcfile = instdir.append_child(*p).append_child(package_name + ".pc");
            }
            if(pcfile.exists())
            {
                wpkg_field::field_file field(state);
                init_field_variables(cl, manager, &field);

                memfile::memory_file pkgconfig;
                pkgconfig.read_file(pcfile);
                field.set_input_file(&pkgconfig);
                // we loop over because it is legal in .pc files to have
                // empty lines
                do
                {
                    field.read();
                }
                while(!field.eof());
                field.set_input_file(NULL);

                std::string source_project_name(package_name);
                if(field.field_is_defined("Source-Project"))
                {
                    source_project_name = field.get_field("Source-Project");
                }

                if(manager.safe_package_status(source_project_name) == wpkgar::wpkgar_manager::installed)
                {
                    manager.load_package(source_project_name);
                    field.auto_transform_variables();
                    field.set_variable("rootdir", manager.get_root_path().full_path());
                    field.set_variable("instdir", instdir.full_path());
                    field.set_variable("admindir", manager.get_database_path().full_path());
                    field.set_variable("name", manager.get_field(source_project_name, wpkg_control::control_file::field_package_factory_t::canonicalized_name()));
                    field.set_variable("version", manager.get_field(source_project_name, wpkg_control::control_file::field_version_factory_t::canonicalized_name()));
                    field.set_variable("description", manager.get_field_first_line(source_project_name, wpkg_control::control_file::field_description_factory_t::canonicalized_name()));
                    if(manager.field_is_defined(source_project_name, wpkg_control::control_file::field_homepage_factory_t::canonicalized_name()))
                    {
                        field.set_variable("homepage", manager.get_field(source_project_name, wpkg_control::control_file::field_homepage_factory_t::canonicalized_name()));
                    }
                    std::string install_prefix;
                    if(manager.field_is_defined(source_project_name, wpkg_control::control_file::field_installprefix_factory_t::canonicalized_name()))
                    {
                        install_prefix = manager.get_field(source_project_name, wpkg_control::control_file::field_installprefix_factory_t::canonicalized_name());
                        if(!install_prefix.empty() && install_prefix[0] != '/')
                        {
                            install_prefix = "/" + install_prefix;
                        }
                    }
                    field.set_variable("install_prefix", install_prefix);

                    if(cl.opt().is_defined("print-variables"))
                    {
                        // print the list of variables
                        if(!first)
                        {
                            // package separator
                            printf("\n");
                        }
                        int max_variables(field.number_of_variables());
                        for(int j(0); j < max_variables; ++j)
                        {
                            // variables defined in the .pc file
                            const std::string& name(field.get_variable_name(j));
                            printf("%s\n", name.c_str());
                        }
                    }
                    else if(cl.opt().is_defined("variable"))
                    {
                        if(!first)
                        {
                            printf(" ");
                        }
                        const std::string variable_name(cl.opt().get_string("variable"));
                        std::string value(field.get_variable(variable_name, true));
                        field.transform_dynamic_variables(field.get_variable_info(variable_name).get(), value);
                        printf("%s", value.c_str());
                    }
                    else if(field.field_is_defined(field_name))
                    {
                        std::string value(field.get_field(field_name));
                        printf("%s\n", value.c_str());
                    }
                    else
                    {
                        // field is not defined, print an empty line
                        printf("\n");
                    }
                    found = true;
                    break;
                }
            }
        }

        if(!found)
        {
            wpkg_output::log("no .pc file found for package %1; please check that the package --is-installed or that you defined PKG_CONFIG_PATH to the correct directory.")
                    .quoted_arg(package_name)
                .level(wpkg_output::level_warning)
                .action("pkg-config");
        }

        first = false;
    }

    if(cl.opt().is_defined("variable"))
    {
        printf("\n");
    }
}

void fsys_tarfile(command_line& cl)
{
    if(cl.size() != 0)
    {
        printf("error:%s: too many parameters on the command line for --fsys-tarfile.\n",
            cl.opt().get_program_name().c_str());
        exit(1);
    }
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "fsys-tarfile");
    manager.set_control_file_state(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::contents_control_file_state_t));
    const wpkg_filename::uri_filename name(cl.get_string("fsys-tarfile"));
    if(name.is_deb())
    {
        cl.opt().usage(advgetopt::getopt::error, "you cannot extract the data.tar.gz file from an installed package");
        /*NOTREACHED*/
    }
    manager.load_package(name);
    memfile::memory_file p;
    std::string data_filename("data.tar");
    // get the uncompressed data in p
    manager.get_control_file(p, name, data_filename, false);

    // print this in stdout so one can pipe it through tar
    // (we send the decompressed version)
    char buf[memfile::memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE];
    for(int sz(p.size()), offset(0), r(0); sz > 0; sz -= r, offset += r)
    {
        int size(sz > memfile::memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE
            ? memfile::memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE
            : sz);
        r = p.read(buf, offset, size);
        fwrite(buf, r, 1, stdout);
    }
}

void info(command_line& cl)
{
    int max(cl.size());
    bool print_avail(false);

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, cl.opt().is_defined("info") ? "info"
                            : cl.opt().is_defined("verify") ? "verify"
                            : "print-avail");
    manager.set_control_file_state(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::contents_control_file_state_t));
    std::string name;
    if(cl.opt().is_defined("info"))
    {
        name = cl.get_string("info");
    }
    else if(cl.opt().is_defined("verify"))
    {
        // Note: while reading the package we check the debian-binary file
        //       (if not "2.0\n" then we throw) and we extract the control
        //       and data tarballs so we do not need to get them to make
        //       sure they exist; also decompressing the whole data
        //       file is done for verification (see last if() block)
        name = cl.get_string("verify");
        if(max != 0)
        {
            fprintf(stderr, "error:%s: too many parameters on the command line for --verify.\n",
                cl.opt().get_program_name().c_str());
            exit(1);
        }
    }
    else if(cl.opt().is_defined("print-avail"))
    {
        name = cl.get_string("print-avail");
        if(max != 0)
        {
            // actually in case of --print-avail we should support multiple
            // package name on the command line
            fprintf(stderr, "error:%s: too many parameters on the command line for --print-avail.\n",
                cl.opt().get_program_name().c_str());
            exit(1);
        }
        print_avail = true;
    }
    else
    {
        // this should not be reached
        throw std::logic_error("unknown command line option used to reach info()");
    }
    int size(-1);
    try {
        memfile::memory_file::file_info deb_info;
        memfile::memory_file::disk_file_to_info(name, deb_info);
        size = deb_info.get_size();
    }
    catch(const memfile::memfile_exception_io& /*e*/)
    {
        // size could not be determined
        // (maybe the user is checking an installed package)
    }
    manager.load_package(name);
    memfile::memory_file p;
    std::string control_filename("control.tar");
    manager.get_control_file(p, name, control_filename);
    memfile::memory_file ctrl;
    control_filename = "control.tar";
    manager.get_control_file(ctrl, name, control_filename, false);

    // we only support the new format so we can print that as is at this point
    if(!cl.quiet() && !print_avail)
    {
        if(size == -1)
        {
            printf(" installed package\n");
        }
        else if(max == 0)
        {
            printf(" new debian package, version 2.0\n");
            printf(" size %d bytes: control archive= %d bytes (%d uncompressed).\n", size, p.size(), ctrl.size());
        }
    }

    bool has_control(false), has_md5sums(false);
    memfile::memory_file control_info_file, md5sums_file;
    std::vector<bool> found_files;
    found_files.resize(max);
    ctrl.dir_rewind();
    for(;;)
    {
        // TODO: it should in alphabetical order...
        //       we could use our wpkg index to get the files?
        memfile::memory_file::file_info info;
        memfile::memory_file data;
        if(!ctrl.dir_next(info, &data))
        {
            break;
        }
        switch(info.get_file_type())
        {
        case memfile::memory_file::file_info::regular_file:
        case memfile::memory_file::file_info::continuous:
            {
                std::string filename(info.get_filename());
                if(filename.length() > 2 && filename[0] == '.' && filename[1] == '/')
                {
                    filename.erase(0, 2);
                }
                if(max > 0)
                {
                    // mark whether we find the control and md5sums files
                    if(filename == "control")
                    {
                        if(has_control)
                        {
                            cl.opt().usage(advgetopt::getopt::error, "\"control\" file found twice in the control archive");
                            /*NOTREACHED*/
                        }
                        has_control = true;
                    }
                    else if(filename == "md5sums")
                    {
                        if(has_md5sums)
                        {
                            cl.opt().usage(advgetopt::getopt::error, "\"md5sums\" file found twice in the control archive");
                            /*NOTREACHED*/
                        }
                        has_md5sums = true;
                    }
                    // TODO: dpkg writes the files in the order specified on the
                    //       command line instead of the archive order
                    for(int i(0); i < max; ++i)
                    {
                        if(filename == cl.argument(i))
                        {
                            // just print the file in the output
                            int offset(0);
                            std::string line;
                            while(data.read_line(offset, line))
                            {
                                printf("%s\n", line.c_str());
                            }
                            found_files[i] = true;
                            break;
                        }
                    }
                }
                else
                {
                    // we assume that each is a text file, count lines
                    int count(0);
                    std::string cmd;
                    std::string line;
                    int offset(0);
                    if(data.read_line(offset, line))
                    {
                        ++count;
                    }
                    char type = ' ';
                    if(line.length() > 2 && line[0] == '#' && line[1] == '!')
                    {
                        type = '*';
                        cmd = line;
                    }
                    for(; data.read_line(offset, line); ++count);
                    if(filename == "control")
                    {
                        // keep a copy so we can print it afterward
                        if(has_control)
                        {
                            cl.opt().usage(advgetopt::getopt::error, "\"control\" file found twice in the control archive");
                            /*NOTREACHED*/
                        }
                        data.copy(control_info_file);
                        has_control = true;
                    }
                    else if(filename == "md5sums")
                    {
                        // keep a copy so we can check against file md5sum
                        // when --verify-ing
                        if(has_md5sums)
                        {
                            cl.opt().usage(advgetopt::getopt::error, "\"md5sums\" file found twice in the control archive");
                            /*NOTREACHED*/
                        }
                        data.copy(md5sums_file);
                        has_md5sums = true;
                    }

                    // print result
                    if(!cl.quiet() && !print_avail)
                    {
                        printf(" %7d bytes, %5d lines   %c  %-21s%s\n",
                                data.size(), count, type, filename.c_str(), cmd.c_str());
                    }
                }
            }
            break;

        default:
            // all the info is in regular files
            break;

        }
    }

    for(int i(0); i < max; ++i)
    {
        if(!found_files[i])
        {
            cl.opt().usage(advgetopt::getopt::warning, "\"%s\" not found in the control tarball of this package", cl.argument(i).c_str());
            /*NOTREACHED*/
        }
    }

    int err(0);
    if(has_control)
    {
        if(max == 0)
        {
            int offset(0);
            std::string line;
            while(control_info_file.read_line(offset, line))
            {
                if(!cl.quiet())
                {
                    printf(" %s\n", line.c_str());
                }
            }
            if(cl.opt().is_defined("verify") && cl.opt().is_defined("verify-fields"))
            {
                // user wanted some extra field validation
                wpkg_control::binary_control_file validate_ctrl(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::control_file_state_t));
                validate_ctrl.set_input_file(&control_info_file);
                validate_ctrl.read();
                validate_ctrl.set_input_file(NULL);
                int fields_max(cl.opt().size("verify-fields"));
                for(int i(0); i < fields_max; ++i)
                {
                    std::string v(cl.opt().get_string("verify-fields", i));
                    if(!validate_ctrl.validate_fields(v))
                    {
                        fprintf(stderr, "error:%s: field validation failed with: \"%s\".\n",
                                    cl.opt().get_program_name().c_str(), v.c_str());
                        ++err;
                    }
                }
            }
        }
    }
    else
    {
        fprintf(stderr, "error:%s: no control file found in the control archive.\n",
                    cl.opt().get_program_name().c_str());
        ++err;
    }

    if(!has_md5sums)
    {
        fprintf(stderr, "error:%s: no md5sums file found in the control archive.\n",
                cl.opt().get_program_name().c_str());
        ++err;
    }

    // verification includes reading the entire data.tar file
    // which validates the tarball!
    if(cl.opt().is_defined("verify"))
    {
        memfile::memory_file data;
        std::string data_filename("data.tar");
        manager.get_control_file(data, name, data_filename, false);

        wpkg_util::md5sums_map_t md5sums;
        if(has_md5sums)
        {
            // transform the md5sums file in a map
            wpkg_util::parse_md5sums(md5sums, md5sums_file);
        }
#ifdef WIN32
        std::map<case_insensitive::case_insensitive_string, bool> md5sums_found;
#else
        std::map<std::string, bool> md5sums_found;
#endif

        data.dir_rewind();
        for(;;)
        {
            memfile::memory_file::file_info info;
            memfile::memory_file input_data;
            if(!data.dir_next(info, &input_data))
            {
                break;
            }
            // TODO: we should canonicalize the filename for this test
            std::string filename(info.get_filename());
            if(md5sums_found.find(filename) != md5sums_found.end())
            {
                fprintf(stderr, "error:%s: file \"%s\" is defined multiple times in the data archive\n",
                        cl.opt().get_program_name().c_str(),
                        filename.c_str());
                ++err;
            }
            else
            {
                md5sums_found[filename] = true;
            }
            // directory, special files, etc. do not have an md5sum
            switch(info.get_file_type())
            {
            case memfile::memory_file::file_info::regular_file:
            case memfile::memory_file::file_info::continuous:
                if(has_md5sums)
                {
                    if(md5sums.find(filename) != md5sums.end())
                    {
                        std::string sum(input_data.md5sum());
                        if(md5sums[filename] != sum)
                        {
                            fprintf(stderr, "error:%s: file \"%s\" md5sum is not valid\n",
                                    cl.opt().get_program_name().c_str(),
                                    filename.c_str());
                            ++err;
                        }
                        // remove the entry so we can err in case some
                        // md5sums were not used up (why are they defined?)
                        md5sums.erase(md5sums.find(filename));
                    }
                    else
                    {
                        fprintf(stderr, "error:%s: file \"%s\" is not defined in the list of md5sums\n",
                                    cl.opt().get_program_name().c_str(),
                                    filename.c_str());
                        ++err;
                    }
                }
                break;

            default:
                // md5sums are in regular files only
                break;

            }
        }
        if(!md5sums.empty())
        {
            fprintf(stderr, "error:%s: more md5sums defined than there are files in the data archive.",
                        cl.opt().get_program_name().c_str());
            ++err;
        }
    }
    if(err != 0)
    {
        exit(1);
    }
}

void increment_build_number(command_line& cl)
{
    int max(cl.size());
    if(max > 1)
    {
        printf("error:%s: too many parameters on the command line for --increment-build-number.\n",
            cl.opt().get_program_name().c_str());
        exit(1);
    }
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "verify-project");
    wpkgar::wpkgar_build pkg_build(&manager, "");
    if(cl.opt().is_defined("build-number-filename"))
    {
        if(max == 1)
        {
            printf("error:%s: the build number filename cannot be defined more than once; please remove the --build-number-filename parameter.\n",
                cl.opt().get_program_name().c_str());
            exit(1);
        }
        pkg_build.set_build_number_filename(cl.opt().get_string("build-number-filename"));
    }
    else if(max == 1)
    {
        // set user defined filename if defined
        pkg_build.set_build_number_filename(cl.opt().get_string("increment-build-number"));
    }
    pkg_build.increment_build_number();

    if(cl.verbose())
    {
        int build_number(0);
        if(pkg_build.load_build_number(build_number, false))
        {
            printf("%d\n", build_number);
        }
        else
        {
            printf("error:%s: could not read the build number back.\n",
                cl.opt().get_program_name().c_str());
            exit(1);
        }
    }
}

void list(command_line& cl)
{
    int max(cl.size());
    if(max > 1)
    {
        printf("error:%s: too many parameters on the command line for --list.\n",
            cl.opt().get_program_name().c_str());
        exit(1);
    }
    const std::string& pattern_str(cl.opt().get_string("list"));
    const char *pattern = pattern_str.c_str();

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "list");
    wpkgar::wpkgar_lock lock_wpkg(&manager, "Listing");
    wpkgar::wpkgar_manager::package_list_t list;
    manager.list_installed_packages(list);

    bool first(true);
    for(wpkgar::wpkgar_manager::package_list_t::const_iterator it(list.begin());
            it != list.end(); ++it)
    {
        if(*pattern != '\0')
        {
            const wpkg_filename::uri_filename filename(it->c_str());
            if(!filename.glob(pattern))
            {
                // it doesn't match, skip it
                continue;
            }
        }
        if(first)
        {
            first = false;
            printf("Desired=Unknown/Install/Remove/Purge/Hold/reJect\n"
                   "| Status=Not/Inst/Conf-files/Unpacked/halF-conf/Half-inst/Working\n"
                   "|/ Err?=(none)/Configure\n"
                   "||/ Name                                  Version                          Description\n"
                   "+++-=====================================-================================-======================================================================\n");
        }
        try {
            char flags[4];
            memcpy(flags, "i- \0", sizeof(flags));
            manager.load_package(*it);
            if(manager.field_is_defined(*it, wpkg_control::control_file::field_xselection_factory_t::canonicalized_name()))
            {
                const wpkg_control::control_file::field_xselection_t::selection_t selection(wpkg_control::control_file::field_xselection_t::validate_selection(manager.get_field(*it, wpkg_control::control_file::field_xselection_factory_t::canonicalized_name())));
                if(selection == wpkg_control::control_file::field_xselection_t::selection_hold)
                {
                    flags[0] = 'h';
                }
                else if(selection == wpkg_control::control_file::field_xselection_t::selection_reject)
                {
                    flags[0] = 'j';
                }
            }
            const wpkgar::wpkgar_manager::package_status_t status(manager.package_status(*it));
            switch(status)
            {
            case wpkgar::wpkgar_manager::not_installed:
                flags[0] = flags[0] == 'j' ? 'j' : 'p';
                flags[1] = 'n';
                break;

            case wpkgar::wpkgar_manager::config_files:
                flags[0] = flags[0] == 'j' ? 'j' : 'r';
                flags[1] = 'c';
                break;

            case wpkgar::wpkgar_manager::unpacked:
                flags[1] = 'U';
                break;

            case wpkgar::wpkgar_manager::installed:
                flags[1] = 'i';
                break;

            case wpkgar::wpkgar_manager::no_package:
                flags[0] = 'u';
                flags[1] = 'n';
                break;

            case wpkgar::wpkgar_manager::unknown:
                flags[0] = '?';
                flags[1] = '?';
                break;

            case wpkgar::wpkgar_manager::installing:
            case wpkgar::wpkgar_manager::upgrading: // could be downgrading
                flags[1] = 'w';
                break;

            case wpkgar::wpkgar_manager::half_installed:
                flags[1] = 'H';
                break;

            case wpkgar::wpkgar_manager::half_configured:
                flags[1] = 'F';
                flags[2] = 'c';
                break;

            case wpkgar::wpkgar_manager::removing:
                flags[0] = 'r';
                flags[1] = 'w';
                break;

            case wpkgar::wpkgar_manager::purging:
                flags[0] = 'p';
                flags[1] = 'w';
                break;

            // unexpected status for a package
            case wpkgar::wpkgar_manager::listing:
            case wpkgar::wpkgar_manager::verifying:
            case wpkgar::wpkgar_manager::ready:
                flags[0] = 'u';
                flags[1] = '*';
                break;

            }
            const std::string version(manager.get_field(*it, "Version"));
            std::string long_description;
            const std::string description(manager.get_description(*it, "Description", long_description));
            printf("%s %-37.37s %-32.32s %-70.70s\n", flags,
                    it->c_str(), version.c_str(), description.c_str());
        }
        catch(const std::exception& e)
        {
            fprintf(stderr, "error:%s: installed package \"%s\" could not be loaded (%s).\n",
                    cl.opt().get_program_name().c_str(), it->c_str(), e.what());
        }
    }
    if(first)
    {
        if(*pattern == '\0' || pattern_str == "*")
        {
            fprintf(stderr, "No package installed in this environment.\n");
        }
        else
        {
            fprintf(stderr, "No package found matching \"%s\".\n", pattern);
        }
    }
}

void list_all(command_line& cl)
{
    if(cl.size() != 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--list-all does not take any parameters.");
        /*NOTREACHED*/
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "list-all");
    wpkgar::wpkgar_lock lock_wpkg(&manager, "Listing");
    wpkgar::wpkgar_manager::package_list_t list;
    manager.list_installed_packages(list);

    for(wpkgar::wpkgar_manager::package_list_t::const_iterator it(list.begin());
            it != list.end(); ++it)
    {
        manager.load_package(*it);
        const wpkgar::wpkgar_manager::package_status_t status(manager.package_status(*it));
        switch(status)
        {
        case wpkgar::wpkgar_manager::installed:
            printf("%-31s %s\n", it->c_str(), manager.get_field_first_line(*it, wpkg_control::control_file::field_description_factory_t::canonicalized_name()).c_str());
            break;

        case wpkgar::wpkgar_manager::unpacked:
            printf("? %-29s %s\n", it->c_str(), manager.get_field_first_line(*it, wpkg_control::control_file::field_description_factory_t::canonicalized_name()).c_str());
            break;

        case wpkgar::wpkgar_manager::config_files:
            printf("! %-29s %s\n", it->c_str(), manager.get_field_first_line(*it, wpkg_control::control_file::field_description_factory_t::canonicalized_name()).c_str());
            break;

        case wpkgar::wpkgar_manager::not_installed:
        case wpkgar::wpkgar_manager::no_package:
        case wpkgar::wpkgar_manager::unknown:
        case wpkgar::wpkgar_manager::installing:
        case wpkgar::wpkgar_manager::upgrading:
        case wpkgar::wpkgar_manager::half_installed:
        case wpkgar::wpkgar_manager::half_configured:
        case wpkgar::wpkgar_manager::removing:
        case wpkgar::wpkgar_manager::purging:
        case wpkgar::wpkgar_manager::listing:
        case wpkgar::wpkgar_manager::verifying:
        case wpkgar::wpkgar_manager::ready:
            // not properly installed we ignore them completely here
            break;

        }
    }
}

void listfiles(command_line& cl)
{
    int max(cl.opt().size("listfiles"));
    if(max == 0)
    {
        printf("error:%s: --listfiles expects at least one installed package name.\n",
            cl.opt().get_program_name().c_str());
        exit(1);
    }
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "listfiles");
    wpkgar::wpkgar_lock lock_wpkg(&manager, "Listing");

    bool first(true);
    for(int i(0); i < max; ++i)
    {
        const std::string& name(cl.opt().get_string("listfiles", i));
        manager.load_package(name);
        memfile::memory_file *wpkgar_file;
        manager.get_wpkgar_file(name, wpkgar_file);
        if(!first)
        {
            printf("\n");
        }
        if(cl.verbose())
        {
            printf("%s:\n", name.c_str());
        }
        wpkgar_file->dir_rewind();
        for(;;)
        {
            memfile::memory_file::file_info info;
            if(!wpkgar_file->dir_next(info, NULL))
            {
                break;
            }
            std::string filename(info.get_filename());
            if(filename[0] == '/')
            {
                printf("%s\n", filename.c_str());
            }
        }
        first = false;
    }
}

void list_index_packages(command_line& cl)
{
    if(cl.size() != 0)
    {
        std::cerr << "error:"
            << cl.opt().get_program_name()
            << ": --list-index-packages does not take extra parameters."
            << std::endl;
        exit(1);
    }

    int max(cl.opt().size("list-index-packages"));
    if(max == 0)
    {
        std::cerr << "error:"
            << cl.opt().get_program_name()
            << ": --list-index-packages expects at least one package index name."
            << std::endl;
        exit(1);
    }
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "list-index-packages");
    wpkgar::wpkgar_lock lock_wpkg(&manager, "Listing");

    for(int i(0); i < max; ++i)
    {
        const std::string& name(cl.opt().get_string("list-index-packages", i));
        memfile::memory_file package_index;
        package_index.read_file(name);
        wpkgar::wpkgar_repository repository(&manager);
        wpkgar::wpkgar_repository::entry_vector_t entries;
        repository.load_index(package_index, entries);

        for(wpkgar::wpkgar_repository::entry_vector_t::const_iterator it(entries.begin()); it != entries.end(); ++it)
        {
            printf("%7d  %s  %s\n",
                 it->f_info.get_size(),
                 it->f_info.get_date().c_str(),
                 it->f_info.get_filename().c_str());
        }
    }
}

void list_sources(command_line& cl)
{
    if(cl.size() != 0)
    {
        printf("error:%s: --list-source does not take extra parameters.\n",
            cl.opt().get_program_name().c_str());
        exit(1);
    }

    int max(cl.opt().size("list-sources"));
    if(max <= 0)
    {
        printf("error:%s: --list-sources expects at least one package index name.\n",
            cl.opt().get_program_name().c_str());
        exit(1);
    }
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "list-sources");
    wpkgar::wpkgar_lock lock_wpkg(&manager, "Listing");

    for(int i(0); i < max; ++i)
    {
        wpkg_filename::uri_filename name(cl.opt().get_string("list-sources", i));
        if(name.empty())
        {
            name = manager.get_database_path();
            name = name.append_child("core/sources.list");
        }
        wpkgar::wpkgar_repository repository(&manager);
        wpkgar::wpkgar_repository::source_vector_t sources;
        memfile::memory_file sources_file;
        sources_file.read_file(name);
        repository.read_sources(sources_file, sources);

        if(cl.verbose())
        {
            printf("file: \"%s\"\n", name.original_filename().c_str());
        }

        int line(1);
        for(wpkgar::wpkgar_repository::source_vector_t::const_iterator it(sources.begin()); it != sources.end(); ++it, ++line)
        {
            if(cl.verbose())
            {
                printf("%3d. ", line);
            }
            printf("%s", it->get_type().c_str());
            wpkgar::wpkgar_repository::source::parameter_map_t params(it->get_parameters());
            if(!params.empty())
            {
                printf(" [ ");
                for(wpkgar::wpkgar_repository::source::parameter_map_t::const_iterator p(params.begin()); p != params.end(); ++p)
                {
                    printf("%s=%s ", p->first.c_str(), p->second.c_str());
                }
                printf("]");
            }
            printf(" %s %s", it->get_uri().c_str(), it->get_distribution().c_str());

            int cnt(it->get_component_size());
            for(int j(0); j < cnt; ++j)
            {
                printf(" %s", it->get_component(j).c_str());
            }

            printf("\n");
        }
    }
}

void md5sums(command_line& cl)
{
    int max(cl.size());
    if(max == 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--md5sums expects at least one filename");
        /*NOTREACHED*/
    }

    for(int i(0); i < max; ++i)
    {
        std::string filename(cl.opt().get_string("filename", i));
        memfile::memory_file file;
        file.read_file(filename);
        printf("%s %c%s\n", file.md5sum().c_str(), file.is_text() ? ' ' : '*', filename.c_str());
    }
}

void md5sums_check(command_line& cl)
{
    int max(cl.size());
    if(max == 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--md5sums-check expects at least two filenames: the md5sums file and a file to check");
        /*NOTREACHED*/
    }

    // read the md5sums; if invalid it will fail
    std::string md5sums_filename(cl.opt().get_string("md5sums-check"));
    memfile::memory_file md5sums_file;
    md5sums_file.read_file(md5sums_filename);
    wpkg_util::md5sums_map_t md5sums;
    wpkg_util::parse_md5sums(md5sums, md5sums_file);

    // now check the specified files
    for(int i(0); i < max; ++i)
    {
        std::string filename(cl.opt().get_string("filename", i));
        wpkg_util::md5sums_map_t::const_iterator it(md5sums.find(filename));
        if(it == md5sums.end())
        {
            // this file does not exist in the md5sums file
            wpkg_output::log("file %1 is not defined in your list of md5sums")
                    .quoted_arg(filename)
                .level(wpkg_output::level_warning)
                .action("audit-validation");
        }
        else
        {
            memfile::memory_file file;
            file.read_file(filename);
            std::string loaded_file_md5sum(file.md5sum());
            if(it->second == loaded_file_md5sum)
            {
                wpkg_output::log("%1 is valid")
                        .quoted_arg(filename)
                    .action("audit-validation");
            }
            else
            {
                wpkg_output::log("the md5sum (%1) of file %2 does not match the one found (%3) in your list of md5sums")
                        .arg(it->second)
                        .quoted_arg(filename)
                        .arg(loaded_file_md5sum)
                    .level(wpkg_output::level_error)
                    .action("audit-validation");
            }
        }
    }
}

void print_architecture(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "print-architecture");
    wpkgar::wpkgar_lock lock_wpkg(&manager, "Listing");
    manager.load_package("core");
    std::string architecture(manager.get_field("core", "Architecture"));
    printf("%s\n", architecture.c_str());
}

void print_build_number(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "print-build-number");
    wpkgar::wpkgar_build pkg_build(&manager, "");
    if(cl.opt().is_defined("build-number-filename"))
    {
        // set user defined filename if defined
        pkg_build.set_build_number_filename(cl.opt().get_string("build-number-filename"));
    }

    int build_number(0);
    if(pkg_build.load_build_number(build_number, false))
    {
        printf("%d\n", build_number);
    }
    else
    {
        printf("error:%s: could not read the build number back.\n",
            cl.opt().get_program_name().c_str());
        exit(1);
    }
}

void init_remover(command_line& cl, wpkgar::wpkgar_manager& manager, wpkgar::wpkgar_remove& pkg_remove, const std::string& option)
{
    init_manager(cl, manager, option);

    int max(cl.opt().size(option));
    if(max == 0)
    {
        throw std::runtime_error("--" + option + " requires at least one parameter");
    }

    // add the force, no-force/refuse parameters
    pkg_remove.set_parameter(wpkgar::wpkgar_remove::wpkgar_remove_force_depends,
        (cl.opt().is_defined("force-depends") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-depends")
                            && !cl.opt().is_defined("refuse-depends")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_remove.set_parameter(wpkgar::wpkgar_remove::wpkgar_remove_force_hold,
        (cl.opt().is_defined("force-hold") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-hold")
                            && !cl.opt().is_defined("refuse-hold")
                            && !cl.opt().is_defined("refuse-all"));
    pkg_remove.set_parameter(wpkgar::wpkgar_remove::wpkgar_remove_force_remove_essentials,
        (cl.opt().is_defined("force-remove-essential") || cl.opt().is_defined("force-all"))
                            && !cl.opt().is_defined("no-force-remove-essential")
                            && !cl.opt().is_defined("refuse-remove-essential")
                            && !cl.opt().is_defined("refuse-all"));

    // some additional parameters
    pkg_remove.set_parameter(wpkgar::wpkgar_remove::wpkgar_remove_recursive, cl.opt().is_defined("recursive"));

    // add the list of package names
    for(int i(0); i < max; ++i)
    {
        pkg_remove.add_package(cl.opt().get_string(option, i));
    }
}

void remove(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    wpkgar::wpkgar_remove pkg_remove(&manager);
    init_remover(cl, manager, pkg_remove, "remove");

    wpkgar::wpkgar_lock lock_wpkg(&manager, "Removing");
    if(pkg_remove.validate() && !cl.dry_run())
    {
        if(manager.is_self())
        {
            // trying to remove self?!
            wpkg_output::log("you cannot remove wpkg, even if it is not marked as required because under MS-Windows it is just not possible to delete a running executable")
                .level(wpkg_output::level_fatal)
                .module(wpkg_output::module_validate_removal)
                .package("wpkg")
                .action("remove-validation");
        }
        else
        {
            for(;;)
            {
                manager.check_interrupt();

                int i(pkg_remove.remove());
                if(i < 0)
                {
                    break;
                }
                // keep all configuration files
            }
        }
    }
}

void purge(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    wpkgar::wpkgar_remove pkg_remove(&manager);
    pkg_remove.set_purging();
    init_remover(cl, manager, pkg_remove, "purge");

    wpkgar::wpkgar_lock lock_wpkg(&manager, "Removing");
    if(pkg_remove.validate() && !cl.dry_run())
    {
        if(manager.is_self())
        {
            // trying to remove self?!
            wpkg_output::log("you cannot purge wpkg, even if it is not marked as required because under MS-Windows it is just not possible to delete a running executable")
                .level(wpkg_output::level_fatal)
                .module(wpkg_output::module_validate_removal)
                .package("wpkg")
                .action("remove-validation");
        }
        else
        {
            for(;;)
            {
                manager.check_interrupt();

                int i(pkg_remove.remove());
                if(i < 0)
                {
                    break;
                }
                if(!pkg_remove.deconfigure(i))
                {
                    break;
                }
            }
        }
    }
}

void deconfigure(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    wpkgar::wpkgar_remove pkg_remove(&manager);
    pkg_remove.set_deconfiguring();
    init_remover(cl, manager, pkg_remove, "deconfigure");

    wpkgar::wpkgar_lock lock_wpkg(&manager, "Removing");
    if(pkg_remove.validate() && !cl.dry_run())
    {
        if(manager.is_self())
        {
            // trying to remove self?!
            wpkg_output::log("you cannot deconfigure wpkg, even if it is not marked as required because under MS-Windows it is just not possible to delete a running executable")
                .level(wpkg_output::level_fatal)
                .module(wpkg_output::module_deconfigure_package)
                .package("wpkg")
                .action("deconfigure-validation");
        }
        else
        {
            // TODO: this is a bit weak although the validation
            //       should be enough I'm not 100% sure that the
            //       order in which to deconfigure is important
            //       or not (TBD.)
            int max(pkg_remove.count());
            for(int i(0); i < max; ++i)
            {
                manager.check_interrupt();

                if(!pkg_remove.deconfigure(i))
                {
                    break;
                }
            }
        }
    }
}

void autoremove(command_line& cl)
{
    if(cl.size() != 0)
    {
        throw std::runtime_error("--autoremove does not take any parameter");
    }

    wpkgar::wpkgar_manager manager;
    wpkgar::wpkgar_remove pkg_remove(&manager);
    if(cl.opt().is_defined("purge"))
    {
        pkg_remove.set_purging();
    }
    init_remover(cl, manager, pkg_remove, "autoremove");

    wpkgar::wpkgar_lock lock_wpkg(&manager, "Removing");
    pkg_remove.autoremove(cl.dry_run());
}

void remove_database_lock(command_line& cl)
{
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "remove-database-lock");
    if(manager.remove_lock())
    {
        if(cl.verbose())
        {
            printf("database lock was removed.\n");
        }
        exit(0);
    }
    else
    {
        fprintf(stderr, "error: that database was not locked.\n");
        exit(1);
    }
}

void remove_sources(command_line& cl)
{
    int max(cl.size());
    if(max == 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--remove-sources expects at least one number");
        /*NOTREACHED*/
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "remove-sources");
    wpkg_filename::uri_filename name(manager.get_database_path());
    name = name.append_child("core/sources.list");
    wpkgar::wpkgar_repository repository(&manager);
    wpkgar::wpkgar_repository::source_vector_t sources;
    memfile::memory_file sources_file;
    sources_file.read_file(name);
    sources_file.printf("\n");
    repository.read_sources(sources_file, sources);

    // we need to have all the line references in order so we can delete the
    // last one first; otherwise the position would be invalid
    std::vector<int> lines;
    for(int i(0); i < max; ++i)
    {
        lines.push_back(cl.opt().get_long("filename", i, 1, static_cast<int>(sources.size())));
    }
    std::sort(lines.begin(), lines.end());

    max = static_cast<int>(lines.size());
    for(int i(max - 1); i >= 0; --i)
    {
        sources.erase(sources.begin() + lines[i]);
    }

    sources_file.create(memfile::memory_file::file_format_other);
    repository.write_sources(sources_file, sources);
    sources_file.write_file(name);
}

void rollback(command_line& cl)
{
    int max(cl.opt().size("filename"));
    if(max != 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--rollback expects exactly one parameter.\n");
        /*NOTREACHED*/
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "rollback");
    wpkgar::wpkgar_lock lock_wpkg(&manager, "Removing");

    wpkgar::wpkgar_tracker tracker(&manager, cl.opt().get_string("rollback"));
    tracker.keep_file(true);
}

void search(command_line& cl)
{
    int max(cl.opt().size("search"));
    if(max == 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--search expects at least one pattern or filename.\n");
        /*NOTREACHED*/
    }
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "search");
    wpkgar::wpkgar_lock lock_wpkg(&manager, "Listing");
    wpkgar::wpkgar_manager::package_list_t list;
    manager.list_installed_packages(list);

    int count(0);
    for(wpkgar::wpkgar_manager::package_list_t::const_iterator it(list.begin());
            it != list.end(); ++it)
    {
        manager.load_package(*it);
        memfile::memory_file *wpkgar_file;
        manager.get_wpkgar_file(*it, wpkgar_file);

        bool first(true);
        wpkgar_file->dir_rewind();
        for(;;)
        {
            memfile::memory_file::file_info info;
            if(!wpkgar_file->dir_next(info, NULL))
            {
                break;
            }
            const wpkg_filename::uri_filename filename(info.get_filename());
            if(filename.is_absolute())
            {
                for(int i(0); i < max; ++i)
                {
                    const std::string& pattern(cl.opt().get_string("search", i));
                    if(filename.glob(pattern.c_str()))
                    {
                        if(cl.verbose())
                        {
                            if(first)
                            {
                                printf("%s:\n", it->c_str());
                            }
                            printf("%s\n", filename.original_filename().c_str());
                        }
                        else
                        {
                            printf("%s: %s\n", it->c_str(), filename.original_filename().c_str());
                        }
                        first = false;
                        ++count;
                    }
                }
            }
        }
    }

    if(cl.verbose())
    {
        printf("%d file%s found.\n", count, (count != 1 ? "s" : ""));
    }
}

void set_selection(command_line& cl)
{
    int max(cl.size());
    if(max == 0)
    {
        cl.opt().usage(advgetopt::getopt::error, "--set-selection expects at least one package name");
        /*NOTREACHED*/
    }

    std::string value(cl.get_string("set-selection"));
    wpkg_control::control_file::field_xselection_t::selection_t selection(wpkg_control::control_file::field_xselection_t::validate_selection(value));
    if(selection == wpkg_control::control_file::field_xselection_t::selection_unknown)
    {
        cl.opt().usage(advgetopt::getopt::error, "unexpected selection name, we currently support 'auto', 'manual', 'normal', 'hold', and 'reject'");
        /*NOTREACHED*/
    }

    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "set-selection");
    if(selection == wpkg_control::control_file::field_xselection_t::selection_reject)
    {
        // the Reject is a special case...
        for(int i(0); i < max; ++i)
        {
            std::string name(cl.argument(i));
            manager.set_package_selection_to_reject(name);
        }
    }
    else
    {
        for(int i(0); i < max; ++i)
        {
            std::string name(cl.argument(i));
            manager.load_package(name);
            manager.set_field(name, wpkg_control::control_file::field_xselection_factory_t::canonicalized_name(), value, true);
        }
    }
}

void show(command_line& cl)
{
    int max(cl.size());
    if(max != 0)
    {
        printf("error:%s: too many parameters on the command line for --show.\n",
            cl.opt().get_program_name().c_str());
        exit(1);
    }
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "show");
    manager.set_control_file_state(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::contents_control_file_state_t));
    std::string name(cl.get_string("show"));
    manager.load_package(name);
    if(cl.opt().is_defined("showformat"))
    {
        // the format is defined, retrieve the values
        const std::string& showformat(cl.opt().get_string("showformat"));
        const char *s(showformat.c_str());
        while(*s != '\0')
        {
            if(*s == '$' && s[1] == '{')
            {
                s += 2;
                const char *start(s);
                while(*s != '\0' && *s != ':' && *s != '}')
                {
                    ++s;
                }
                std::string field_name(start, s - start);
                int width(0);
                if(*s == ':')
                {
                    ++s;
                    int sign(1);
                    if(*s == '-')
                    {
                        sign = -1;
                        ++s;
                    }
                    while(*s >= '0' && *s <= '9')
                    {
                        width = width * 10 + *s - '0';
                        ++s;
                    }
                    if(width >= 1024)
                    {
                        throw std::overflow_error("width too large in format");
                    }
                    width *= sign;
                }
                if(*s != '}')
                {
                    throw std::overflow_error("invalid field in --showformat, } expected at the end");
                }
                ++s;
                std::string value("undefined");
                if(manager.field_is_defined(name, field_name))
                {
                    value = manager.get_field(name, field_name);
                }
                if(width != 0)
                {
                    char buf[32];
                    snprintf(buf, sizeof(buf), "%%%ds", width);
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-nonliteral"
#endif
                    // buf is under control
                    printf(buf, value.c_str());
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
                }
                else
                {
                    printf("%s", value.c_str());
                }
            }
            else if(*s == '\\')
            {
                ++s;
                switch(*s)
                {
                case '\\':
                    printf("\\");
                    ++s;
                    break;

                case 'n':
                    printf("\n");
                    ++s;
                    break;

                case 'r':
                    printf("\r");
                    ++s;
                    break;

                case 't':
                    printf("\t");
                    ++s;
                    break;

                case '"':
                    // because DOS batches are this bad
                    printf("\"");
                    ++s;
                    break;

                // others?
                }
            }
            else
            {
                printf("%c", *s);
                ++s;
            }
        }
    }
    else
    {
        // if no format specified, show Package + Version
        std::string package_name(manager.get_field(name, "Package"));
        std::string version(manager.get_field(name, "Version"));
        printf("%s\t%s\n", package_name.c_str(), version.c_str());
    }
}

void package_status(command_line& cl)
{
    int max(cl.opt().size("package-status"));
    if(max == 0)
    {
        throw std::runtime_error("--package-status requires at least one parameter");
    }
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, "package-status");

    for(int i(0); i < max; ++i)
    {
        std::string name(cl.opt().get_string("package-status", i));
        const char *status(NULL);
        switch(manager.package_status(name))
        {
        case wpkgar::wpkgar_manager::no_package:
            status = "error: package not found";
            break;

        case wpkgar::wpkgar_manager::unknown:
            status = "error: package is not known";
            break;

        case wpkgar::wpkgar_manager::not_installed:
            status = "not-installed";
            break;

        case wpkgar::wpkgar_manager::config_files:
            status = "config-files";
            break;

        case wpkgar::wpkgar_manager::installing:
            status = "installing";
            break;

        case wpkgar::wpkgar_manager::upgrading:
            status = "upgrading";
            break;

        case wpkgar::wpkgar_manager::half_installed:
            status = "half-installed";
            break;

        case wpkgar::wpkgar_manager::unpacked:
            status = "unpacked";
            break;

        case wpkgar::wpkgar_manager::half_configured:
            status = "half-configured";
            break;

        case wpkgar::wpkgar_manager::installed:
            status = "installed";
            break;

        case wpkgar::wpkgar_manager::removing:
            status = "removing";
            break;

        case wpkgar::wpkgar_manager::purging:
            status = "purging";
            break;

        case wpkgar::wpkgar_manager::listing:
            status = "listing";
            break;

        case wpkgar::wpkgar_manager::verifying:
            status = "verifying";
            break;

        case wpkgar::wpkgar_manager::ready:
            status = "ready";
            break;

        }
        if(status != NULL)
        {
            printf("status: %s: %s\n", name.c_str(), status);
        }
    }
}

void extract(command_line& cl)
{
    int max(cl.size());
    if(max != 2)
    {
        cl.opt().usage(advgetopt::getopt::error, "the extract command expects exactly two parameters: package name and a destination folder");
        /*NOTREACHED*/
    }
    wpkgar::wpkgar_manager manager;
    init_manager(cl, manager, cl.verbose() ? "vextract" : "extract");
    manager.set_control_file_state(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::contents_control_file_state_t));
    const wpkg_filename::uri_filename name(cl.filename(0));
    if(name.is_deb())
    {
        cl.opt().usage(advgetopt::getopt::error, "you cannot extract the files of the data.tar.gz file from an installed package");
        /*NOTREACHED*/
    }
    manager.load_package(name);
    memfile::memory_file p;
    std::string data_filename("data.tar");
    manager.get_control_file(p, name, data_filename, false);
    const wpkg_filename::uri_filename output_path(cl.filename(1));
    p.dir_rewind();
    for(;;)
    {
        memfile::memory_file::file_info info;
        memfile::memory_file data;
        if(!p.dir_next(info, &data))
        {
            break;
        }
        const wpkg_filename::uri_filename out(output_path.append_safe_child(info.get_filename()));
        if(cl.verbose())
        {
            printf("%s\n", out.original_filename().c_str());
        }
        switch(info.get_file_type())
        {
        case memfile::memory_file::file_info::regular_file:
        case memfile::memory_file::file_info::continuous:
            data.write_file(out, true);
            break;

        case memfile::memory_file::file_info::symbolic_link:
            {
                const wpkg_filename::uri_filename link(info.get_link());
                link.os_symlink(out);
            }
            break;

        default:
            // nothing to do with other file types because MS-Windows
            // does not support them
            break;

        }
    }
}

void wpkg_break()
{
    g_interrupted = true;
    fprintf(stderr, "\nwpkg:%d: *** User break\n", static_cast<int>(getpid()));

    // resume normal execution on return; if possible the normal
    // execution stream will stop pretty quickly
    //
    // note that we do not re-establish the signal callback so
    // if the user punches Ctrl-C again the process stops at once
}

void setup_interrupt()
{
#if defined(MO_WINDOWS)
    signal(SIGINT, reinterpret_cast<void (__cdecl *)(int)>(wpkg_break));
    signal(SIGTERM, reinterpret_cast<void (__cdecl *)(int)>(wpkg_break));
#elif defined(MO_LINUX) || defined(MO_DARWIN) || defined(MO_CYGWIN) || defined(MO_SUNOS) || defined(MO_FREEBSD)
    signal(SIGINT, reinterpret_cast<void (*)(int)>(wpkg_break));
    signal(SIGTERM, reinterpret_cast<void (*)(int)>(wpkg_break));
#else
    signal(SIGINT, wpkg_break);
    signal(SIGTERM, wpkg_break);
#endif
}



#ifdef MO_WINDOWS
int utf8_main(int argc, char *argv[])
#else
int main(int argc, char *argv[])
#endif
{
    bool log_ready(false);
    g_argv = argv;

    // by now the g_output is ready so save it in the log object
    wpkg_output::set_output(&g_output);

    // we have a top try/catch to ensure stack unwinding and thus
    // have true RAII at all levels whatever the compiler.
    try
    {
        setup_interrupt();

        std::vector<std::string> configuration_files;
        configuration_files.push_back("/etc/wpkg/wpkg.conf");
        // TODO: add wpkg location + "../etc/wpkg/wpkg.conf" under MS-Windows
        configuration_files.push_back("~/.config/wpkg/wpkg.conf");

        // support lone one character flags as in /h
        // because windows users are used to use those
        if(argc == 2 && argv[1][0] == '/' && argv[1][1] != '\0' && argv[1][2] == '\0')
        {
            argv[1][0] = '-';
        }
        command_line cl(argc, argv, configuration_files);
        log_ready = true;

        switch(cl.command())
        {
        case command_line::command_add_hooks:
            add_hooks(cl);
            break;

        case command_line::command_add_sources:
            add_sources(cl);
            break;

        case command_line::command_architecture:
            architecture(cl);
            break;

        case command_line::command_atleast_version:
            atleast_version(cl);
            break;

        case command_line::command_atleast_wpkg_version:
            atleast_wpkg_version(cl);
            break;

        case command_line::command_audit:
            audit(cl);
            break;

        case command_line::command_autoremove:
            autoremove(cl);
            break;

        case command_line::command_build:
            {
                wpkg_filename::uri_filename package_name;
                build(cl, package_name);
            }
            break;

        case command_line::command_build_and_install:
            build_and_install(cl);
            break;

        case command_line::command_canonicalize_version:
            canonicalize_version(cl);
            break;

        case command_line::command_cflags:
            display_pkgconfig(cl, "Cflags", "cflags");
            break;

        case command_line::command_check_install:
            check_install(cl);
            break;

        case command_line::command_compare_versions:
            compare_versions(cl);
            break;

        case command_line::command_compress:
            compress(cl);
            break;

        case command_line::command_configure:
            configure(cl);
            break;

        case command_line::command_contents:
            contents(cl);
            break;

        case command_line::command_control:
            control(cl);
            break;

        case command_line::command_copyright:
            copyright(cl);
            break;

        case command_line::command_create_admindir:
            create_admindir(cl);
            break;

        case command_line::command_create_database_lock:
            create_database_lock(cl);
            break;

        case command_line::command_create_index:
            create_index(cl);
            break;

        case command_line::command_database_is_locked:
            database_is_locked(cl);
            break;

        case command_line::command_decompress:
            decompress(cl);
            break;

        case command_line::command_deconfigure:
            deconfigure(cl);
            break;

        case command_line::command_directory_size:
            directory_size(cl);
            break;

        case command_line::command_exact_version:
            exact_version(cl);
            break;

        case command_line::command_extract:
            extract(cl);
            break;

        case command_line::command_field:
            field(cl);
            break;

        case command_line::command_fsys_tarfile:
            fsys_tarfile(cl);
            break;

        case command_line::command_increment_build_number:
            increment_build_number(cl);
            break;

        case command_line::command_info:
            info(cl);
            break;

        case command_line::command_install:
            install(cl);
            break;

        case command_line::command_install_size:
            install_size(cl);
            break;

        case command_line::command_is_installed:
            is_installed(cl);
            break;

        case command_line::command_libs:
            display_pkgconfig(cl, "Libs", "libs");
            break;

        case command_line::command_list:
            list(cl);
            break;

        case command_line::command_list_all:
            list_all(cl);
            break;

        case command_line::command_listfiles:
            listfiles(cl);
            break;

        case command_line::command_list_hooks:
            list_hooks(cl);
            break;

        case command_line::command_list_index_packages:
            list_index_packages(cl);
            break;

        case command_line::command_list_sources:
            list_sources(cl);
            break;

        case command_line::command_max_version:
            max_version(cl);
            break;

        case command_line::command_md5sums:
            md5sums(cl);
            break;

        case command_line::command_md5sums_check:
            md5sums_check(cl);
            break;

        case command_line::command_modversion:
            display_pkgconfig(cl, "Version", "modversion");
            break;

        case command_line::command_os:
            os(cl);
            break;

        case command_line::command_print_architecture:
            print_architecture(cl);
            break;

        case command_line::command_print_build_number:
            print_build_number(cl);
            break;

        case command_line::command_print_variables:
            display_pkgconfig(cl, "*variables*", "print-variables");
            break;

        case command_line::command_processor:
            processor(cl);
            break;

        case command_line::command_purge:
            purge(cl);
            break;

        case command_line::command_reconfigure:
            reconfigure(cl);
            break;

        case command_line::command_remove:
            remove(cl);
            break;

        case command_line::command_remove_database_lock:
            remove_database_lock(cl);
            break;

        case command_line::command_remove_hooks:
            remove_hooks(cl);
            break;

        case command_line::command_remove_sources:
            remove_sources(cl);
            break;

        case command_line::command_rollback:
            rollback(cl);
            break;

        case command_line::command_search:
            search(cl);
            break;

        case command_line::command_set_selection:
            set_selection(cl);
            break;

        case command_line::command_show:
            show(cl);
            break;

        case command_line::command_package_status:
            package_status(cl);
            break;

        case command_line::command_triplet:
            triplet(cl);
            break;

        case command_line::command_unpack:
            unpack(cl);
            break;

        case command_line::command_update:
            update(cl);
            break;

        case command_line::command_update_status:
            update_status(cl);
            break;

        case command_line::command_upgrade:
            upgrade(cl);
            break;

        case command_line::command_variable:
            display_pkgconfig(cl, "*variable*", "variable");
            break;

        case command_line::command_verify_control:
            verify_control(cl);
            break;

        case command_line::command_verify_project:
            verify_project(cl);
            break;

        case command_line::command_upgrade_info:
            upgrade_info(cl);
            break;

        case command_line::command_vendor:
            vendor(cl);
            break;

        default:
            throw std::logic_error("internal error: unhandled command line function");

        }
    }
    // under windows the default for an exception is to be silent
    catch(const std::exception& e)
    {
        if(log_ready)
        {
            wpkg_output::log("%1")
                    .arg(e.what())
                .level(wpkg_output::level_fatal)
                .action("exception");
        }
        else
        {
            fprintf(stderr, "wpkg:error: %s\n", e.what());
        }
        wpkg_output::set_output(NULL);
        exit(1);
    }
    catch(...)
    {
        // nothing to do here, we're just making sure we
        // get a full stack unwinding effect for RAII
        wpkg_output::set_output(NULL);
        throw;
    }

    wpkg_output::set_output(NULL);
    return g_output.exit_code();
}

#if defined(MO_WINDOWS) && !defined(MO_MINGW32)
int wmain(int argc, wchar_t *wargv[])
{
    // transform the wchar_t strings to UTF-8 strings
    std::vector<std::string> args;
    for(int i(0); i < argc; ++i)
    {
        std::string s(libutf8::wcstombs(wargv[i]));
        args.push_back(s);
    }
    //
    // *** WARNING ***
    // DO NOT MERGE THESE TWO LOOPS!!!
    // *** WARNING ***
    //
    // the reason for the separation is because the push_back() may reallocate
    // the buffer and as a result may move the string pointers; once the vector
    // is complete, however, the strings won't move anymore so we can go ahead
    // and create an array of char * from the std::string
    //
    std::vector<char *> argv;
    for(int i(0); i < argc; ++i)
    {
        // we use const char here because we do not want to
        // use char const * const * argv for utf8_main()
        // (because it makes things more complicated than necessary)
        argv.push_back(const_cast<char *>(args[i].c_str()));
    }
    argv.push_back(NULL);
    return utf8_main(argc, &argv[0]);
}
#endif

#if defined(MO_MINGW32) || defined(MO_MINGW64)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR lpCmdLine, int nCmdShow)
{
    int argc;
    LPWSTR *wargv(CommandLineToArgvW(GetCommandLineW(), &argc));

    // transform the wchar_t strings to UTF-8 strings
    std::vector<std::string> args;
    for(int i(0); i < argc; ++i)
    {
        std::string s(libutf8::wcstombs(wargv[i]));
        args.push_back(s);
    }
    //
    // *** WARNING ***
    // DO NOT MERGE THESE TWO LOOPS!!!
    // *** WARNING ***
    //
    // the reason for the separation is because the push_back() may reallocate
    // the buffer and as a result may move the string pointers; once the vector
    // is complete, however, the strings won't move anymore so we can go ahead
    // and create an array of char * from the std::string
    //
    std::vector<char *> argv;
    for(int i(0); i < argc; ++i)
    {
        // we use const char here because we do not want to
        // use char const * const * argv for utf8_main()
        // (because it makes things more complicated than necessary)
        argv.push_back(const_cast<char *>(args[i].c_str()));
    }
    argv.push_back(NULL);
    return utf8_main(argc, &argv[0]);
}

// This implementation comes from: https://github.com/coderforlife/mingw-unicode-main
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR _lpCmdLine, int nCmdShow)
{
    WCHAR *lpCmdLine = GetCommandLineW();
    if(__argc == 1)
    { // avoids GetCommandLineW bug that does not always quote the program name if no arguments
      do
      {
          ++lpCmdLine;
      }
      while(*lpCmdLine);
    }
    else
    {
        BOOL quoted = lpCmdLine[0] == L'"';
        ++lpCmdLine; // skips the " or the first letter (all paths are at least 1 letter)
        while(*lpCmdLine)
        {
            if(quoted && lpCmdLine[0] == L'"')
            {
                quoted = FALSE;
            } // found end quote
            else if(!quoted && lpCmdLine[0] == L' ')
            {
                // found an unquoted space, now skip all spaces
                do
                {
                    ++lpCmdLine;
                }
                while(lpCmdLine[0] == L' ');
                break;
            }
            ++lpCmdLine;
        }
    }
    return wWinMain(hInstance, hPrevInstance, lpCmdLine, nCmdShow);
}
#endif


// vim: ts=4 sw=4 et
