/*    wpkgar_tracker.cpp -- implementation of the wpkg tracker to rollback changes
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
 * \brief Track what is being done so we can eventually undo it.
 *
 * When running complex commands, such as a full upgrade of a target system,
 * it may be of interest to the administrator to be able to restore the
 * entire system to the way it was before starting the upgrade. The tracker
 * allows for such process.
 *
 * \note
 * The tracker was actually created in order to allow for the use of a
 * simple target in building packages. This means... we want to be able
 * to install all the necessary packages to create project A. Once project
 * A is built, we wanted to return the target to the state it was before
 * we started. The tracker allows us to restore that state.
 */
#include    "libdebpackages/wpkgar_tracker.h"
#include    "libdebpackages/wpkgar_install.h"
#include    "libdebpackages/wpkgar_remove.h"
#include    "libdebpackages/wpkg_stream.h"

namespace wpkgar
{


/** \class wpkgar_tracker
 * \brief The implementation of the tracker for the archive manager.
 *
 * This class is the tracker implementation that can be used with the
 * archive manager (wpkgar_manager) to track steps to be executed to
 * rollback what is being done to the database such as removing installed
 * software or configuring packages that get deconfigured.
 *
 * The tracker saves all the steps in a file. Note that it opens and
 * closes the file each time to make sure that the data is saved on
 * disk and not just in a memory buffer. This makes the tracker a little
 * slower than it could be, but a lot safer as it will be able to revert
 * the work even if a crash occurs.
 */


/** \brief Initialize an auto-rollback tracker object.
 *
 * This function creates an auto-rollback tracker object used to attempt
 * to rollback all the work done so far in case an error occurs. This can
 * be used with all the installation and removal functions (--install,
 * --remove, --configure, --unpack, --deconfigure, etc.)
 *
 * To prevent the automatic rollback, call the commit() function before the
 * tracker gets destroyed.
 *
 * By default the file gets deleted when your application exits. To create
 * a file that doesn't get deleted on exit, make sure to call the
 * keep_file() function once.
 *
 * \param[in] manager  The manager attached to the build, install, or remove
 *                     process to be rolled back.
 * \param[in] filename  Path to the journal to be used to rollback
 *                      the work. It cannot be an empty string.
 */
wpkgar_tracker::wpkgar_tracker(wpkgar_manager *manager, const wpkg_filename::uri_filename& filename)
    : f_manager(manager)
    , f_filename(filename)
{
    if(f_filename.empty())
    {
        throw wpkgar_exception_parameter("the filename of a tracker object cannot be the empty string");
    }
}


/** \brief Destruct the tracker object.
 *
 * This destructor rolls back the work as found in the journal file.
 * The filename of the journal file was specified on construction.
 *
 * The rollback process can be avoided by calling the commit() function
 * at least once before the destruction of the tracker object.
 *
 * If an exception occurs, then it is caught and an error message printed
 * before the function exists. This means the status of the target
 * system is unknown since part of the rollback were not executed.
 */
wpkgar_tracker::~wpkgar_tracker()
{
    try
    {
        rollback();
    }
    catch(const std::exception& e)
    {
        wpkg_output::log("exception while rolling back -- %1")
                .arg(e.what())
            .level(wpkg_output::level_fatal)
            .action("exception");
    }
}


/** \brief Commit the process.
 *
 * This function commits the process that just occured: unpack, installation,
 * removal, purge, configuration, deconfiguration.
 *
 * Once this function was called, the rollback() function has no more effect
 * which means the destructor also has no effect. Note, however, that the
 * rollback() function deletes the journal file on exit. That still happens.
 */
void wpkgar_tracker::commit()
{
    f_committed = true;
}


/** \brief Retrieve the filename of the journal.
 *
 * It may at times be useful to retrieve the journal filename. This function
 * can be used for that purpose. For example, you may first setup a tracker
 * object and then use this function to pass the journal filename to the
 * install or remove objects used along this object.
 *
 * In some circumstances, the tracking system is used internally. For
 * example, the --force-rollback command implies that the system tracks
 * everything it does in order to be able to restore it later. In that
 * situation, the system may generate a temporary filename for the journal
 * under the temporary directory. It will mark the file as \p keep (see
 * set_tracking() for info about the keep flag) so if you use the necessary
 * debug flag, that journal will be accessible.
 *
 * \return The filename of the journal.
 */
wpkg_filename::uri_filename wpkgar_tracker::get_filename() const
{
    return f_filename;
}


/** \brief Whether the journal file should be kept.
 *
 * By default the rollback() function deletes the journal filename, which
 * allows you to reuse the same file over and over again. (Note that you
 * cannot safely use the same tracker again, only the same file.)
 *
 * This function can be used to request for the system to not delete the
 * file.
 *
 * \param[in] keep  Whether the file should be kept on destruction of the manager.
 */
void wpkgar_tracker::keep_file(bool keep)
{
    f_keep_file = keep;
}


/** \brief Append one instruction to the tracking file.
 *
 * This function appends (writes at the end) one line that represents one
 * instruction that can later be executed to undo what the current
 * instruction is about to do.
 *
 * For example, the pre_configure() function is configuring packages that
 * have to be configured before certain new packages get installed. This
 * function tracks its work with the "deconfigure <package name>"
 * instruction which when executed will under the work of the pre_configure()
 * function.
 *
 * \important
 * The track() function must be called before applying a function so
 * that way we can make sure that it is available in the journal in the
 * event an error occurs. The last instruction in the journal may not get
 * executed when rolling back since the package may already be in that
 * state.
 *
 * \exception wpkgar_exception_io
 * The I/O exception is thrown if something goes wrong while handling the
 * file. Note that we re-open, write to, and then close the file each time.
 * We do that to get what is called a \em journal.
 *
 * \param[in] command  The tracking instruction and parameters.
 * \param[in] package_name  The name of the package concerned if available.
 */
void wpkgar_tracker::track(const std::string& command, const std::string& package_name)
{
    wpkgar_tracker_interface::track(command, package_name);

    // Note: We reopen the file each time on purpose to make sure that the
    //       information gets saved BEFORE we apply the (opposite) action.
    //       This is important to ensure that the journal is up to date.
    wpkg_stream::fstream file;
    file.append(f_filename);
    if(!file.good())
    {
        throw wpkgar_exception_io("opening the tracking file \"" + f_filename.original_filename() + "\" failed");
    }

    // write the command
    file.write(command.c_str(), command.length());

    // add a new line if there was none in command
    if(command.length() > 0 && command[command.length() - 1] != '\n')
    {
        file.write("\n", 1);
    }

    if(!file.good())
    {
        throw wpkgar_exception_io("writing to the tracking file \"" + f_filename.original_filename() + "\" failed");
    }
}


namespace
{

/** \brief Helper class used to handle commands.
 *
 * This class is used to store all the commands before executing them. This
 * is important as we rollback calling the functions backward.
 */
class wpkgar_command
{
public:
    /** \brief List of parameters.
     *
     * At this time all commands take exactly one parameter. However, we have
     * the capability to accept additional parameters.
     */
    typedef std::vector<std::string> param_list_t;

    explicit                wpkgar_command();
                            wpkgar_command(wpkgar_manager *manager,
                                           const wpkg_filename::uri_filename& filename,
                                           int line,
                                           const std::string& command,
                                           param_list_t params);
                            wpkgar_command(const wpkgar_command& rhs);
    wpkgar_command&         operator = (const wpkgar_command& rhs);

    void                    run();

private:
    void                    run_configure();
    void                    run_deconfigure();
    void                    run_downgrade();
    void                    run_install();
    void                    run_purge();
    void                    run_unpack();

    wpkgar_manager *        f_manager;
    wpkg_filename::uri_filename       f_filename;
    int                     f_line;
    std::string             f_command;
    param_list_t            f_params;
};

/** \brief List of commands.
 *
 * This vector is used to record all the commands found in a journal and
 * then execute them.
 */
typedef std::vector<wpkgar_command> command_list_t;


/** \brief Default initializer for the std::vector<> usage.
 *
 * Unfortunately we need to have a constructor with no parameters so we can
 * create vectors of wpkgar_command-s.
 *
 * DO NOT USE THIS CONSTRUCTOR OTHERWISE.
 */
wpkgar_command::wpkgar_command()
    : f_manager(NULL)
    //, f_filename() -- auto-init
    , f_line(0)
    //, f_command() -- auto-init
    //, f_params() -- auto-init
{
}


/** \brief Initialize the command object.
 *
 * The command objects expects a name (\p command) and a list of parameters
 * (\p params.)
 *
 * \param[in] manager  A pointer to the manager to use to run commands.
 * \param[in] filename  The name of the file from which we read the journal.
 * \param[in] line  The line from which the command was read in the journal.
 * \param[in] command  The name of the command.
 * \param[in] params  The list of parameters received for this command.
 */
wpkgar_command::wpkgar_command(wpkgar_manager *manager, const wpkg_filename::uri_filename& filename, int line, const std::string& command, param_list_t params)
    : f_manager(manager)
    , f_filename(filename)
    , f_line(line)
    , f_command(command)
    , f_params(params)
{
}


/** \brief Copy the command.
 *
 * This function makes a copy of a command.
 *
 * \param[in] rhs  The command to be copied.
 */
wpkgar_command::wpkgar_command(const wpkgar_command& rhs)
    : f_manager(rhs.f_manager)
    , f_filename(rhs.f_filename)
    , f_line(rhs.f_line)
    , f_command(rhs.f_command)
    , f_params(rhs.f_params)
{
}


wpkgar_command& wpkgar_command::operator = (const wpkgar_command& rhs)
{
    if(this != &rhs)
    {
        f_manager = rhs.f_manager;
        f_filename = rhs.f_filename;
        f_line = rhs.f_line;
        f_command = rhs.f_command;
        f_params = rhs.f_params;
    }

    return *this;
}


/** \brief Run this command.
 *
 * This function is called whenever we're ready to run all the commands. It
 * first determines the command and then calls a more appropriate
 * (specialized) command.
 */
void wpkgar_command::run()
{
    f_manager->check_interrupt();

    if(f_command == "configure")
    {
        run_configure();
    }
    else if(f_command == "deconfigure")
    {
        run_deconfigure();
    }
    else if(f_command == "downgrade")
    {
        run_downgrade();
    }
    else if(f_command == "install")
    {
        run_install();
    }
    else if(f_command == "purge")
    {
        run_purge();
    }
    else if(f_command == "unpack")
    {
        run_unpack();
    }
    else
    {
        std::string msg;
        wpkg_output::log(msg, "tracker:%1:%2: unknown command %3")
                .arg(f_filename)
                .arg(f_line)
                .quoted_arg(f_command)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_run_script)
            .action("rollback");
        throw wpkgar_exception_compatibility(msg);
    }
}


/** \brief Run the "configure" command.
 *
 * This function runs a configure on the specified package. This means
 * renaming configuration files and then running the postrm script as
 * required by that package.
 */
void wpkgar_command::run_configure()
{
    wpkgar::wpkgar_install pkg_install(f_manager);
    pkg_install.set_configuring();

    pkg_install.add_package(f_params[0]);

    if(pkg_install.validate())
    {
        // TODO: test that all the specified packages are indeed unpacked
        //       before attempting to configure them
        const int max(pkg_install.count());
        for(int i(0); i < max; ++i)
        {
            if(!pkg_install.configure(i))
            {
                break;
            }
        }
    }
}


/** \brief Run the "deconfigure" command.
 *
 * This function runs a deconfigure on the specified package. This means
 * running the prerm script and renaming the configuration files as
 * required by that package.
 */
void wpkgar_command::run_deconfigure()
{
    wpkgar::wpkgar_remove pkg_remove(f_manager);
    pkg_remove.set_deconfiguring();

    // always force the removal of essential packages
    // (unfortunately, we won't be able to remove required packages here!)
    pkg_remove.set_parameter(wpkgar::wpkgar_remove::wpkgar_remove_force_remove_essentials, true);

    pkg_remove.add_package(f_params[0]);

    if(pkg_remove.validate())
    {
        // TODO: this is a bit weak although the validation
        //       should be enough I'm not 100% sure that the
        //       order in which to deconfigure is important
        //       or not (TBD.)
        int max(pkg_remove.count());
        for(int i(0); i < max; ++i)
        {
            if(!pkg_remove.deconfigure(i))
            {
                break;
            }
        }
    }
}


/** \brief Run the "downgrade" command.
 *
 * This function runs an install which we expect will downgrade an
 * installed package. This means we force the downgrade.
 */
void wpkgar_command::run_downgrade()
{
    wpkgar::wpkgar_install pkg_install(f_manager);
    pkg_install.set_installing();

    // parameters
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_downgrade, true);
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_file_info, true);
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_quiet_file_info, true);
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_recursive, true);

    pkg_install.add_package(f_params[0]);

    if(pkg_install.validate())
    {
        if(pkg_install.pre_configure())
        {
            const int i(pkg_install.unpack());
            if(i < 0)
            {
                return;
            }
            pkg_install.configure(i);
        }
    }
}


/** \brief Run the "downgrade" command.
 *
 * This function runs an install which we expect will re-install a
 * removed package.
 */
void wpkgar_command::run_install()
{
    wpkgar::wpkgar_install pkg_install(f_manager);
    pkg_install.set_installing();

    // parameters
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_file_info, true);
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_quiet_file_info, true);
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_recursive, true);

    pkg_install.add_package(f_params[0]);

    if(pkg_install.validate())
    {
        if(pkg_install.pre_configure())
        {
            const int i(pkg_install.unpack());
            if(i < 0)
            {
                return;
            }
            pkg_install.configure(i);
        }
    }
}


/** \brief Run the "purge" command.
 *
 * This function runs a purge on the specified package. This deletes all
 * the files and deconfigure the package.
 */
void wpkgar_command::run_purge()
{
    wpkgar::wpkgar_remove pkg_remove(f_manager);
    pkg_remove.set_purging();

    // always force the removal of essential packages
    // (unfortunately, we won't be able to remove required packages here!)
    pkg_remove.set_parameter(wpkgar::wpkgar_remove::wpkgar_remove_force_remove_essentials, true);

    // add the "list" of package names
    pkg_remove.add_package(f_params[0]);

    if(pkg_remove.validate())
    {
        int i(pkg_remove.remove());
        if(i >= 0)
        {
            // in most cases it will already be deconfigured, but the
            // configuration files will still be there if we do not call
            // this function too
            if(!pkg_remove.deconfigure(i))
            {
                return;
            }
        }
    }
}


/** \brief Run the "unpack" command.
 *
 * This function runs an install which we expect will re-install a
 * removed package.
 */
void wpkgar_command::run_unpack()
{
    wpkgar::wpkgar_install pkg_install(f_manager);
    pkg_install.set_installing();

    // parameters
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_file_info, true);
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_quiet_file_info, true);
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_recursive, true);

    pkg_install.add_package(f_params[0]);

    if(pkg_install.validate())
    {
        if(pkg_install.pre_configure())
        {
            pkg_install.unpack();
        }
    }
}



} // no name namespace


/** \brief Rollback as specified in a journal.
 *
 * This function rolls back a process using the journal of the process that
 * needs to be rolled back.
 *
 * The journal is created top to bottom so when executing, we actually read
 * the entire journal and then execute from the end.
 *
 * In effect, the rollback() function is a script interpreter, although
 * very limited, it works very much like a shell. In most cases entries are
 * lists of command \<parameter> although at times multiple parameters
 * appear.
 *
 * In most cases this function is called when an installation failed and
 * the --force-rollback option was used. When the tracking is used in an
 * automatic way, as it is done when compiling a package, the rollback()
 * function is called automatically at the end whether an error occured
 * or everything succeeded, so that way the target is restored the way it
 * was before the compiling process.
 */
void wpkgar_tracker::rollback()
{
    // user called commit() at least once
    if(!f_committed)
    {
        // read the file
        memfile::memory_file script;
        script.read_file(f_filename);

        // parse all the commands & parameters
        int line(0);
        int offset(0);
        std::string command_line;
        command_list_t command_list;
        while(script.read_line(offset, command_line))
        {
            ++line;
            if(command_line.empty() || command_line[0] == '#')
            {
                // ignore empty lines and commented lines
                continue;
            }
            const std::string::size_type pos(command_line.find(' '));
            if(pos == std::string::npos)
            {
                wpkg_output::log("tracker:%1:%2: rollback script includes command %3 without parameters")
                        .arg(f_filename)
                        .arg(line)
                        .quoted_arg(command_line)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_run_script)
                    .action("rollback");
                return;
            }
            const std::string command(command_line.substr(0, pos));
            if(command == "failed")
            {
                // special command that marks a failure point; we expect that
                // the process stopped at that point and nothing else appears
                // after that command; also the command does not take any
                // parameters
                wpkg_output::log("tracker:%1:%2: a command failed")
                        .arg(f_filename)
                        .arg(line)
                        .quoted_arg(command_line)
                    .level(wpkg_output::level_warning)
                    .module(wpkg_output::module_run_script)
                    .action("rollback");
                break;
            }
            const std::string parameters(command_line.substr(pos + 1));
            const char *start(parameters.c_str());
            for(; isspace(*start); ++start);
            if(*start == '\0')
            {
                wpkg_output::log("tracker:%1:%2: rollback script includes command %3 without parameters")
                        .arg(f_filename)
                        .arg(line)
                        .quoted_arg(command_line)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_run_script)
                    .action("rollback");
                return;
            }
            // break up parameters
            // IMPORTANT: at this point we do not support parameters with spaces
            //            (we assume that a restore from .deb files requires a
            //            separate --repository option)
            wpkgar_command::param_list_t params;
            const char *s(NULL);
            for(; *start != '\0'; start = s)
            {
                if(*start == '"')
                {
                    ++start;
                    for(s = start; *s != '\0' && *s != '"'; ++s);
                    params.push_back(std::string(start, s - start));
                    // make sure we don't do ++s if *s == '\0'
                    if(*s == '"')
                    {
                        ++s;
                    }
                }
                else
                {
                    for(s = start; *s != '\0' && !isspace(*s); ++s);
                    params.push_back(std::string(start, s - start));
                }
                for(; isspace(*s); ++s);
            }
            // this extra test should always be false since we already checked
            // whether the string was empty before entering the loop
            if(params.empty())
            {
                wpkg_output::log("tracker:%1:%2: rollback script includes command %3 without parameters")
                        .arg(f_filename)
                        .arg(line)
                        .quoted_arg(command_line)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_run_script)
                    .action("rollback");
                return;
            }
            wpkgar_command cmd(f_manager, f_filename, line, command, params);
            // stack the commands because we have to execute them backward
            command_list.push_back(cmd);
        }

        std::shared_ptr<wpkgar::wpkgar_lock> lock_wpkg;
        if(!f_manager->is_locked())
        {
            // Add a "Rollbacking" status?
            lock_wpkg.reset(new wpkgar::wpkgar_lock(f_manager, "Removing"));
        }
        // go through the list backward
        for(command_list_t::reverse_iterator c(command_list.rbegin());
                                             c != command_list.rend();
                                             ++c)
        {
            c->run();
        }
    }

    if(!f_keep_file)
    {
        f_filename.os_unlink();
    }
}






}
// vim: ts=4 sw=4 et
