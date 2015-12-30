/*    wpkgar.cpp -- implementation of the wpkg archive
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
 * \brief Implementation of the package (archive) manager.
 *
 * This file is the implementation of the archive manager which handles the
 * loading and other management of packages.
 *
 * The class is used by most of the other archive handlers.
 */
#include    "libdebpackages/wpkgar.h"
#include    "libdebpackages/wpkgar_repository.h"
#include    "libdebpackages/debian_packages.h"
#include    "libdebpackages/wpkg_util.h"
#include    <algorithm>
#include    <fstream>
#include    <iostream>
#include    <sstream>
#include    <fcntl.h>
#include    <errno.h>
#include    <time.h>
#if defined(MO_WINDOWS)
#else
#   include    <unistd.h>
#endif


/** \mainpage
 *
 * The libdebpackages and other libraries and tools part of the wpkg
 * environment are used to manage Debian compatible packages. Online
 * documentation for the project as a whole can be found here:
 *
 * http://windowspackager.org/
 *
 * The library offers:
 *
 * \li A way to create source and binary packages.
 * \li A way to install packages on a target system.
 * \li A way to remove packages from a target system.
 * \li A way to manage repositories of source and binary packages.
 *
 * The build capability allows for a complete build system to be
 * created with wpkg and just a script to start wpkg in the right
 * directory on a periodic manner. wpkg is capable of computing
 * the list of source packages that need to be rebuilt and the
 * order in which they have to be rebuilt. Plus, by working with
 * a repository, you can build the entire repository with one
 * command.
 *
 * The installation and removal capabilities include configuration
 * and unpacking that are handled separately. It also includes a
 * plethora of validations which are similar to the dpkg tests
 * that, for example, require dependent packages to be installed
 * and conflicting packages to not be installed before your
 * package can be installed.
 *
 * The wpkg command line tool is documented only. Every single
 * command line option, including obsolete ones, are documented
 * here:
 *
 * http://windowspackager.org/documentation/wpkg
 *
 * Since version 0.9.0, the library documentation can be extracted using
 * doxygen. We suggest at least doxygen version 1.7.6.1 so it works as
 * expected. Once doxygen is installed on your system and
 * cmake finds it, the documentation will automatically be
 * generated in your build directory under:
 *
 * \code
 * .../build/documentation/wpkg-doc_<version>
 * \endcode
 *
 * You can then review the documentation with a browser.
 */


/** \brief The namespace for all the packager archive handling.
 *
 * This namespace is used by all the different functions handling
 * packages. Note that packages are archives hence the "ar" at the
 * end of the name.
 *
 * This namespace is used by the manager, the install, the remove,
 * the repository, and the tracker.
 */
namespace wpkgar
{


/** \class wpkgar_exception
 * \brief The base exception of all the archive manager exceptions.
 *
 * This is the base exception of the archive manager. It is never itself
 * raised, but can be used to catch all the exception that the archive
 * manager can ever raise.
 */


/** \class wpkgar_exception_parameter
 * \brief A function parameter was not valid.
 *
 * This exception is raised whenever a function detects that the value of
 * one of its parameters is not valid.
 */


/** \class wpkgar_exception_invalid
 * \brief A function detected that a value is invalid.
 *
 * This exception is raised whenever a function detects that a value it has
 * to use it not valid for the purpose. This should be used whenever a
 * value other than a parameter is invalid.
 */

/** \class wpkgar_exception_invalid_emptydir
 * \brief A function detected that a path pointed to an empty directory.
 *
 * In some cases, handling of a directory that's empty is an error
 * (specifically, when creating a new package). When that case occurs,
 * this exception is raised.
 *
 * Note that at first it was an invalid exception but we needed to be
 * able to catch a different exception.
 */

/** \class wpkgar_exception_compatibility
 * \brief Something is not compatible with the wpkg archive manager.
 *
 * The wpkg archive manager does not yet support all the possible capabilities
 * that it could and some capabilities will just never be added. When such
 * is detected, this exception is raised.
 */

/** \class wpkgar_exception_undefined
 * \brief The code is attempting to access something that is still undefined.
 *
 * This exception is raised when a function is called to retrieve some
 * data that is not currently defined. For example, if you attempt to
 * get the database path before the initializing it with a
 * set_database_path() this exception is raised.
 */

/** \class wpkgar_exception_io
 * \brief The manager accesses the system I/O and it failed.
 *
 * Whenever a direct system I/O is attempted by the manager and it fails,
 * this exception is raised. Note that most I/O are performed by other
 * classes such as memfile and uri_filename.
 */

/** \class wpkgar_exception_defined_twice
 * \brief The manager detected the same thing twice.
 *
 * The exception is raised whenever a wpkg archive function detects something
 * that is defined twice. For example, when creating a package, we create a
 * tarball of all the data. That tarball cannot have two files with the same
 * name. Under most Unix systems, though, the file system is case sensitive
 * so README.txt and ReadMe.txt are two distinct files. For our packages,
 * these are the same file because if extracting that package under Microsoft
 * Windows, the second one would overwrite the first one. In such circumstances
 * this exception is raised.
 */

/** \class wpkgar_exception_locked
 * \brief The manager is locked.
 *
 * This exception is raised if the system is already locked when you are
 * attempting to use it.
 *
 * The lock makes use of a file which can be deleted with a command on the
 * wpkg command line. However, the tools that make use of the library should
 * already know how to manage the lock to not have this exception raised.
 *
 * Yet, if you attempt to run two tools that attempt to use the database
 * simultaneously, the second one will generate this error.
 */

/** \class wpkgar_exception_stop
 * \brief The user wants to interrupt the process.
 *
 * This exception is raised whenever the user attempts to interrupt the
 * running process. For console tools, this generally occurs when the
 * user hits Ctrl-C. In a graphical tool, this is when the user clicks
 * a Cancel button.
 *
 * The exception is generated whenever the manager wpkgar::check_interrupt()
 * function is called.
 */


/** \class wpkgar_interrupt
 * \brief The tool you implement can overwrite the interrupt handling.
 *
 * By default a cancel does not stop the process. Instead, it keeps going
 * until a clean location where the process can stop (after it finishes
 * installing a complete package.)
 *
 * This class can be overloaded and set in the manager with the
 * wpkgar_manager::set_interrupt_handler() function. This way you can
 * reprogram the stop_now() function to return true in some circumstances.
 * For example, after the user hit Ctrl-C, the stop_now() function returns
 * true in wpkg requesting the current process to stop as soon as possible.
 */


/** \class wpkgar_tracker_interface
 * \brief An interface class used to send the tracker information.
 *
 * The manager does not directly know about the tracker implementation
 * being used. However, it has to be capable of tracking things. So it
 * makes use of a tracker interface.
 *
 * Our tracker implementation derives from this interface and implements
 * the track() function.
 */


/** \class wpkgar_lock
 * \brief The package manager RAII lock class.
 *
 * To handle the lock in a way that is safe with exceptions, we created
 * this class which when destroyed also ensures that the lock is removed.
 */


/** \class wpkgar_rollback
 * \brief The RAII execution manager.
 *
 * Whenever running commands such as install and configure, their is a
 * counter command such as remove and deconfigure, that can be
 * executed to restore the system the way it was before.
 *
 * The tracker system allows us to track what happens and thus record
 * all the necessary instructions to restore the system in case an
 * unwanted event occurs and execute those instructions in reverse
 * order to rollback all the changes.
 *
 * This class is used for the purpose of rolling back if the changes
 * weren't commit yet when an error occurs. After the commit, the
 * changes are viewed as accepted and thus they will not be rolled
 * back.
 */


/** \class wpkgar_manager
 * \brief The base archive manager.
 *
 * This class implements the base archive manager which loads and
 * caches packages in memory. This class defines all sorts of common
 * functions used by all the other archive managers.
 *
 * For example, it will register all the repository directories.
 */


/** \class source
 * \brief A sources.lists manager.
 *
 * This class handles the sources.lists file format by reading it and
 * allowing the repository implementation use it to compute the different
 * indexes we talked about.
 */


/** \brief Initialize a package manager.
 *
 * This function initializes this package manager. By default pretty much
 * no parameters are considered set. The list of packages and control files
 * are empty, etc.
 */
wpkgar_manager::wpkgar_manager()
    : f_control_file_state(std::shared_ptr<wpkg_control::control_file::build_control_file_state_t>(new wpkg_control::control_file::build_control_file_state_t))
    //, f_root_path_is_defined(false) -- auto-init
    //, f_root_path("") -- auto-init
    //, f_inst_path("") -- auto-init
    //, f_database_path("") -- auto-init
    //, f_packages() -- auto-init
    //, f_field_variables() -- auto-init
    //, f_lock_filename("") -- auto-init
    //, f_lock_fd(-1) -- auto-init
    //, f_lock_count(0) -- auto-init
    //, f_interrupt_handler(0) -- auto-init
    //, f_selves(0) -- auto-init
    //, f_include_selves(NULL) -- auto-init
    //, f_tracker(NULL) -- auto-init
{
}


/** \brief Clear up a package manager object.
 *
 * The function ensures that the tracker, if there is one, gets destroyed
 * before anything else. This is quite important because the tracker makes
 * use of the manager to rollback all the changes. Also, the f_tracker field
 * is reset to avoid tracking any additional changes (i.e. the rollback
 * process doesn't get tracked!)
 */
wpkgar_manager::~wpkgar_manager()
{
    // safely clear the tracker before we get cleared
    std::shared_ptr<wpkgar_tracker_interface> tracker;
    tracker.swap(f_tracker);
    tracker.reset();
}


/** \brief Create a target database.
 *
 * This function is the one used to create a database so one can unpack,
 * install, remove, purge, configure, deconfigure packages.
 *
 * The function expects a control file with some required information such
 * as the architecture that the target will support.
 */
void wpkgar_manager::create_database(const wpkg_filename::uri_filename& ctrl_filename)
{
    // first check whether it exists, if so return immediately
    const wpkg_filename::uri_filename core_dir(get_database_path().append_child("core"));
    if(core_dir.exists())
    {
        // directory already exists, return silently
        if(!core_dir.is_dir())
        {
            throw wpkgar_exception_locked("the database \"core\" package is not a directory as expected.");
        }
        return;
    }

    // now verify the input file
    memfile::memory_file ctrl;
    wpkg_filename::uri_filename ctrl_file(ctrl_filename);
    ctrl.read_file(ctrl_file);
    int size(ctrl.size());
    std::string p;
    p.resize(size);
    ctrl.read(&p[0], 0, size);
    if(p[size - 1] != '\n')
    {
        // make sure the file ends with a newline
        ctrl.printf("\n");
    }
    std::transform(p.begin(), p.end(), p.begin(), ::tolower);
    if(p.find("version:") != std::string::npos)
    {
        throw wpkgar_exception_compatibility("the initial control file for database \"core\" package cannot include a Version field.");
    }
    ctrl.printf("Version: " DEBIAN_PACKAGES_VERSION_STRING "\n");
    if(p.find("package:") == std::string::npos)
    {
        ctrl.printf("Package: core\n");
    }
    if(p.find("description:") == std::string::npos)
    {
        ctrl.printf("Description: Database description and status.\n");
    }
    // allow variable/expression transformations on this one!
    wpkg_control::binary_control_file cf(std::shared_ptr<wpkg_control::control_file::build_control_file_state_t>(new wpkg_control::control_file::build_control_file_state_t));
    cf.set_input_file(&ctrl);
    cf.read();
    cf.set_input_file(NULL);
    if(cf.get_field("Package") != "core")
    {
        throw wpkgar_exception_compatibility("when specified, the Package field must be set to \"core\".");
    }
    // reformat as per our own specs
    ctrl.reset();
    cf.write(ctrl, wpkg_field::field_file::WRITE_MODE_FIELD_ONLY);

    // the control file is good, create the core directory
    // and the files that go in the "core" directory
    core_dir.os_mkdir_p();

    memfile::memory_file wpkgar_file;
    wpkgar_file.create(memfile::memory_file::file_format_wpkg);
    wpkgar_file.set_package_path(core_dir);

    memfile::memory_file::file_info info;
    info.set_filename("control");
    info.set_file_type(memfile::memory_file::file_info::regular_file);
    info.set_mode(0644);
    info.set_uid(getuid());
    info.set_gid(getgid());
    info.set_size(ctrl.size());
    info.set_mtime(time(NULL));
    wpkgar_file.append_file(info, ctrl);
    ctrl.write_file(core_dir.append_child("control"), true);

    memfile::memory_file status;
    std::string status_field("X-Status: ready\n");
    status.create(memfile::memory_file::file_format_other);
    status.write(status_field.c_str(), 0, static_cast<int>(status_field.length()));
    //memfile::memory_file::file_info info;
    info.set_filename("wpkg-status");
    info.set_file_type(memfile::memory_file::file_info::regular_file);
    info.set_mode(0644);
    info.set_uid(getuid());
    info.set_gid(getgid());
    info.set_size(status.size());
    info.set_mtime(time(NULL));
    wpkgar_file.append_file(info, status);
    status.write_file(core_dir.append_child("wpkg-status"), true);

    wpkgar_file.write_file(core_dir.append_child("index.wpkgar"), true);
}

void wpkgar_manager::lock(const std::string& status)
{
    // are we already locked?
    if(f_lock_fd == -1)
    {
        // open the wpkg lock file, if it fails, then we cannot lock and
        // thus we throw an error ending the process right there
        wpkg_filename::uri_filename lock_dir(get_database_path().append_child("core"));
        if(!lock_dir.exists())
        {
            if(errno != ENOENT)
            {
                throw wpkgar_exception_locked("the database \"core\" package is not accessible.");
            }
            throw wpkgar_exception_locked("the database \"core\" package does not exist under \"" + get_database_path().original_filename() + "\"; did you run --create-admindir or use --admindir?");
        }
        if(!lock_dir.is_dir())
        {
            throw wpkgar_exception_locked("the database \"core\" package is not a directory as expected.");
        }
        f_lock_filename = lock_dir.append_child("wpkg.lck");
        // here we still use os_open() to access the lock file because it
        // works in all cases (including mingw); but we probably should look
        // into using our wpkg_stream classes
        f_lock_fd = os_open(f_lock_filename.os_filename().get_os_string().c_str(), O_CREAT | O_EXCL | O_TRUNC, 0600);
        if(f_lock_fd == -1)
        {
            throw wpkgar_exception_locked("the lock file could not be created, this usually means another process is already working on this installation. If you are sure that it is not the case, then you may use the --remove-database-lock command line option to force the release of the lock.");
        }
        // it worked, load the core package, and change the database status
        load_package("core");

        // is the packager environment "ready"?
        if(package_status("core") != wpkgar_manager::ready)
        {
            // we break immediately in this case because we cannot really know what
            // the heck is up with the database...
            // note that we cannot reach here if we're extracting a source package
            // because of the database lock
            ++f_lock_count;
            // TBD -- the unlock will restore the core package status to "Ready"...
            //        is it sensible here to do that automatically?
            unlock();
            throw wpkgar_exception_parameter("the packager environment is not ready: cannot load the core package!");
        }

        set_field("core", wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), status, true);
    }
    ++f_lock_count;
}

void wpkgar_manager::unlock()
{
    // still locked?
    if(f_lock_count <= 0)
    {
        // if you use the RAII class (wpkgar_lock) this should never happen
        throw wpkgar_exception_locked("when the lock is not active you cannot call unlock()");
    }
    --f_lock_count;
    if(f_lock_count == 0)
    {
        // restore the status also
        load_package("core");
        set_field("core", wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Ready", true);
        // release the lock
        close(f_lock_fd);
        f_lock_fd = -1;
        f_lock_filename.os_unlink();
    }
}

// whether it was locked by us in this process
bool wpkgar_manager::was_locked() const
{
    // are re already locked?
    return f_lock_count > 0;
}

// whether the database lock file exists, if so we consider it locked
bool wpkgar_manager::is_locked() const
{
    // are re already locked?
    wpkg_filename::uri_filename lock_dir(get_database_path().append_child("core/wpkg.lck"));
    if(!lock_dir.exists())
    {
        if(errno != ENOENT)
        {
            throw wpkgar_exception_locked("the database lock file is not accessible.");
        }
        return false;
    }
    if(!lock_dir.is_reg())
    {
        throw wpkgar_exception_locked("the database lock file is not a regular file as expected.");
    }
    return true;
}

bool wpkgar_manager::remove_lock()
{
    wpkg_filename::uri_filename lock_dir(get_database_path().append_child("core"));
    if(!lock_dir.exists())
    {
        throw wpkgar_exception_locked("the database \"core\" package does not exist; did you run --create-admindir?");
    }
    if(!lock_dir.is_dir())
    {
        throw wpkgar_exception_locked("the database \"core\" package is not a directory as expected.");
    }
    wpkg_filename::uri_filename lock_filename(lock_dir.append_child("wpkg.lck"));
    if(!lock_filename.exists())
    {
        if(errno == ENOENT)
        {
            return false;
        }
        throw wpkgar_exception_locked("the database lock file cannot be accessed");
    }
    if(!lock_filename.is_reg())
    {
        throw wpkgar_exception_locked("the database lock file is not a regular file as expected.");
    }

    lock_filename.os_unlink();

    // restore the status also
    load_package("core");
    set_field("core", wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "Ready", true);

    // we had to unlock the database and it worked!
    return true;
}

void wpkgar_manager::set_control_file_state(std::shared_ptr<wpkg_control::control_file::control_file_state_t> state)
{
    f_control_file_state = state;
}

#if 0
void wpkgar_manager::set_field_variable(const std::string& name, const std::string& value)
{
    if(name.empty())
    {
        throw wpkgar_exception_invalid("wpkgar_manager does not accept field variables with an empty name (set_field_variable)");
    }

    // we have one table of variable for all the control files
    f_field_variables[name] = value;
}

void wpkgar_manager::set_control_variables(wpkg_control::control_file& control)
{
    for(field_variables_t::const_iterator i(f_field_variables.begin()); i != f_field_variables.end(); ++i)
    {
        control.set_field_variable(i->first, i->second);
    }
}
#endif

void wpkgar_manager::set_package_selection_to_reject(const std::string& package_name)
{
    // a package can be selected as a "Reject" only if it is not already
    // unpacked or installed; all other valid states are okay
    wpkg_filename::uri_filename path(get_database_path().append_child(package_name));
    if(path.exists())
    {
        // the path exists, therefore the package can be loaded
        // and its selection setup (see below)
        load_package(package_name);

        // check the current status
        wpkgar_manager::package_status_t status(package_status(package_name));
        switch(status)
        {
        case config_files:
        case not_installed:
            // these are acceptable
            break;

        default:
            {
            std::string msg;
            wpkg_output::log(msg, "package %1 is unpacked or installed or in an invalid state and it cannot be marked as being rejected")
                    .quoted_arg(package_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_configure_package)
                .package(package_name)
                .action("select-configure");
            throw wpkgar_exception_invalid(msg);
            }

        }
        set_field(package_name, wpkg_control::control_file::field_xselection_factory_t::canonicalized_name(), "Reject", true);
    }
    else
    {
        // the package does not exist in this database, create a fake
        // entry so we can mark it as rejected
        path.os_mkdir_p();
        memfile::memory_file index;
        index.create(memfile::memory_file::file_format_wpkg);
        {
            wpkg_control::binary_control_file control(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::control_file_state_t));
            control.set_field(wpkg_control::control_file::field_package_factory_t::canonicalized_name(), package_name);
            control.set_field(wpkg_control::control_file::field_version_factory_t::canonicalized_name(), "0.0.0.1");
            control.set_field(wpkg_control::control_file::field_maintainer_factory_t::canonicalized_name(), "no-maintainer@example.com");
            std::string architecture(get_field("core", wpkg_control::control_file::field_architecture_factory_t::canonicalized_name()));
            control.set_field(wpkg_control::control_file::field_architecture_factory_t::canonicalized_name(), architecture);
            control.set_field(wpkg_control::control_file::field_description_factory_t::canonicalized_name(), "Fake package used to prevent the installation of a package");
            control.set_field(wpkg_control::control_file::field_packagerversion_factory_t::canonicalized_name(), debian_packages_version_string());
            control.set_field(wpkg_control::control_file::field_date_factory_t::canonicalized_name(), wpkg_util::rfc2822_date());
            memfile::memory_file control_out;
            control.write(control_out, wpkg_field::field_file::WRITE_MODE_FIELD_ONLY);
            wpkg_filename::uri_filename control_filename(path.append_child("control"));
            control_out.write_file(control_filename);
            memfile::memory_file::file_info info;
            memfile::memory_file::disk_file_to_info(control_filename, info);
            index.append_file(info, control_out);
        }
        {
            wpkg_control::status_control_file status;
            status.set_field(wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name(), "not-installed");
            status.set_field(wpkg_control::control_file::field_xselection_factory_t::canonicalized_name(), "Reject");
            memfile::memory_file status_out;
            status.write(status_out, wpkg_field::field_file::WRITE_MODE_FIELD_ONLY);
            wpkg_filename::uri_filename status_filename(path.append_child("wpkg-status"));
            status_out.write_file(status_filename);
            memfile::memory_file::file_info info;
            memfile::memory_file::disk_file_to_info(status_filename, info);
            index.append_file(info, status_out);
        }
        index.write_file(path.append_child("index.wpkgar"));
    }
}

const wpkg_filename::uri_filename& wpkgar_manager::get_root_path() const
{
    if(!f_root_path_is_defined)
    {
#ifdef WIN32
        // get the location of the wpkg module
        char name[1024];
        DWORD length(GetModuleFileNameA(NULL, name, sizeof(name) - 20));
        if(length == 0 || length == sizeof(name) - 20)
        {
            throw wpkgar_exception_invalid("could not determine the path to wpkg");
        }

        // search the root
        int count(2);
        while(length > 0)
        {
            --length;
            if(name[length] == '\\')
            {
                --count;
                if(count == 0)
                {
                    break;
                }
            }
        }

        // verify we are in the right directory
        if(length < 1 || count != 0 || strncasecmp(name + length, "\\bin\\", 5) != 0)
        {
            throw wpkgar_exception_invalid("wpkg does not seem to be installed under /bin");
        }

        // okay, we got the root now
        if(length == 2 && name[length - 1] == ':')
        {
            // keep the \ when it's a drive only (i.e. C:\)
            name[length + 1] = '\0';
        }
        else
        {
            name[length] = '\0';
        }
        const_cast<wpkgar_manager *>(this)->f_root_path = name;
#else
        // Unix is simple
        const_cast<wpkgar_manager *>(this)->f_root_path = "/";
#endif
        const_cast<wpkgar_manager *>(this)->f_root_path_is_defined = true;
    }

    return f_root_path;
}

void wpkgar_manager::set_root_path(const wpkg_filename::uri_filename& root_path)
{
    if(f_root_path_is_defined)
    {
        throw wpkgar_exception_invalid("the root path is already defined");
    }

    f_root_path = root_path.os_real_path();
    f_root_path_is_defined = true;
}

const wpkg_filename::uri_filename wpkgar_manager::get_inst_path() const
{
    if(f_inst_path.empty())
    {
        return f_root_path;
    }
    if(f_inst_path.is_absolute())
    {
        return f_inst_path;
    }
    return f_root_path.append_child(f_inst_path.path_only());
}

void wpkgar_manager::set_inst_path(const wpkg_filename::uri_filename& inst_path)
{
    if(!f_inst_path.empty())
    {
        throw wpkgar_exception_invalid("the installation path is already defined");
    }

    f_inst_path = inst_path;
}

const wpkg_filename::uri_filename wpkgar_manager::get_database_path() const
{
    if(f_database_path.empty())
    {
        throw wpkgar_exception_undefined("the database path was not defined yet");
    }
    if(f_database_path.is_absolute())
    {
        return f_database_path;
    }
    return f_root_path.append_child(f_database_path.path_only());
}

void wpkgar_manager::set_database_path(const wpkg_filename::uri_filename& database_path)
{
    if(!f_packages.empty())
    {
        throw wpkgar_exception_parameter("cannot change the database path once packages were read");
    }
    f_database_path = database_path;
}

/** \brief Load a package in memory.
 *
 * This function loads a package in memory. Note that all the files may not
 * be loaded all at once. The filename can reference an installed package,
 * in which case the name must be a valid name for the Package field, otherwise
 * it is expected to be a .deb filename.
 *
 * A standard package is loaded from the database specified by the --admindir
 * command line option.
 *
 * A .deb package is loaded by first decompressing the package in a temporary
 * directory (see --tmpdir). The path may vary, but in general it is as
 * follow:
 *
 * \code
 * // Unix
 * /tmp/wpkg-<pid>/packages/<package name>_<version>_<architecture>/...
 * // MS-Windows
 * $TEMP/wpkg-<pid>/packages/<package name>_<version>_<architecture>/...
 * \endcode
 *
 * This is done by the load_temporary_package() function.
 *
 * The determination of which function to use to load the package is defined
 * by the uri_filename is_deb() function. If is_deb() returns true, then the
 * package is assumed installed. Otherwise it tries to load a .deb file.
 *
 * \exception wpkgar_exception_parameter
 * The package name includes two periods in a row ("..") which is forbidden.
 *
 * \param[in] filename  The name of the file to load.
 * \param[in] force_reload  If the package is installed, force a reload
 *                          (useful after an install/upgrade.)
 */
void wpkgar_manager::load_package(const wpkg_filename::uri_filename& filename, bool force_reload)
{
    // a .deb package MUST include at least one _ generally two
    // (one if the architecture is not specified); the uri_filename
    // checks for that case; if the filename cannot represent a valid
    // Debian Package name (as defined in the Package field) then the
    // is_deb() function returns false
    if(!filename.is_deb())
    {
        // load a "temporary" package
        // the name is expected to be a filename
        load_temporary_package(filename);
        return;
    }

    // note that here the basename() represent <package name> only
    // (the name found in the Package field); this is different from the
    // .deb files that we handle with load_temporary_package() which
    // makes use of the package name, version, and architecture.
    packages_t::iterator pkg(f_packages.find(filename.basename()));
    if(pkg != f_packages.end())
    {
        // it's already loaded!
        if(force_reload)
        {
            // since the package is a shared pointer, it will get deleted
            // once released by all users
            f_packages.erase(pkg);
        }
        else
        {
            return;
        }
    }

    std::shared_ptr<wpkgar_package> package(new wpkgar_package(filename, f_control_file_state));
    package->set_package_path(get_database_path().append_child(filename.path_only()));
    package->read_package();
    f_packages[filename.basename()] = package;
}


/** \brief Internal function called when loading a non-installed package.
 *
 * This function loads a .deb file, partly in memory and partly in a
 * temporary directory. The function expects the filename to point to
 * a .deb file. The extension does not need to be .deb but the format
 * must be a binary package supported by Debian.
 *
 * The temporary directory can be found under the administration
 * directory.
 *
 * The filename is expected to look like this:
 *
 * \code
 * .../<path>/<package name>_<version>_<architecture>.deb
 * .../<path>/<package name>_<version>.deb [source package do not include an architecture]
 * \endcode
 *
 * \exception wpkgar_exception_invalid
 * This exception is raised whenever the function discovered that two
 * different packages with the same basename are being loaded from
 * two different locations (i.e. the same file or two different files
 * with exactly the same basename loaded from two different directories
 * creates a conflict.) This exception is also raised when the file
 * being loaded is not an ar archive (i.e. not a valid .deb file.)
 *
 * \param[in] filename  The name of the package file to load.
 */
void wpkgar_manager::load_temporary_package(const wpkg_filename::uri_filename& filename)
{
    // note that in this case the basename of the package is something like:
    //     <package name>_<version>_<architecture>
    // or
    //     <package name>_<version>
    // which cannot match the load of an installed package:
    //     <package name>
    const std::string basename(filename.basename());

    const wpkg_filename::uri_filename fullname(filename.os_real_path());

    packages_t::const_iterator pkg(f_packages.find(basename));
    if(pkg != f_packages.end())
    {
        // the file was already loaded, verify both entries full path
        // because it could be two completely different locations
        // (i.e. basename does not include the path and extensions)
        // we could strengthen the test later with an md5sum
        // (which we had in older versions, but that was just way too
        // slow when done 10 times per package while validating an
        // installation!)
        auto second_package( pkg->second );
        if(second_package->get_fullname().full_path() != fullname.full_path())
        {
            // Note: here we could add an md5sum test (slow but we err anyway)
            std::stringstream ss;
            ss << "two packages with the same basename ('" << basename << "') have different full names: '"
               << second_package->get_fullname().full_path() << "'' vs '" << fullname.full_path() << "'."
               << " They cannot be used at the same time! Please reinitialize your distribution root as it is likely corrupt!";
            throw wpkgar_exception_invalid(ss.str());
        }
        return;
    }

    // in this case filename is a direct reference to a package (the .deb file)
    memfile::memory_file p;
    p.read_file(filename);
    if(p.is_compressed())
    {
        // the file should not be compressed though
        // (the contents are compresses, but not the .deb itself)
        memfile::memory_file d;
        p.copy(d);
        d.decompress(p);
    }
    if(p.get_format() != memfile::memory_file::file_format_ar)
    {
        throw wpkgar_exception_invalid("cannot load file, it is not a valid package");
    }

    // the file looks proper, create a package and load the files
    std::shared_ptr<wpkgar_package> package(new wpkgar_package(fullname, f_control_file_state));
    package->set_package_path(wpkg_filename::uri_filename::tmpdir("packages").append_child(basename));
    package->read_archive(p);
    f_packages[basename] = package;
}

/** \brief Get the path to the package.
 *
 * This function returns the path to the package data.
 *
 * \warning
 * This is the path where the data is temporarily saved for
 * processing. If you are manipulating a .deb file, then this
 * path is not the path to the .deb, instead, it is the path
 * to the temporary directory where the .deb was extracted.
 *
 * \param[in] package_name  The name of the package to read.
 *
 * \return The filename where the package is being read from.
 */
wpkg_filename::uri_filename wpkgar_manager::get_package_path(const wpkg_filename::uri_filename& package_name) const
{
    return get_package(package_name)->get_package_path();
}

/** \brief Get a copy of a file from the package.
 *
 * This function retrieves a copy of a file in your \p wpkgar_file
 * parameter. The file is read from the package named \p package_name
 * which is expected to already be loaded.
 *
 * \param[in] package_name  The name of the package searched for the file.
 * \param[in,out] wpkgar_file  The contents of the file if found.
 */
void wpkgar_manager::get_wpkgar_file(const wpkg_filename::uri_filename& package_name, memfile::memory_file *& wpkgar_file)
{
    get_package(package_name)->get_wpkgar_file(wpkgar_file);
}

/** \brief Retrieve the status of an installed package.
 *
 * This function retrieves the status as defined in the X-Status field
 * of the wpkg-status file. Note that only packages that are or were
 * installed have such a status. (i.e. a .deb file does not include a
 * status.) Yet .deb files are automatically assigned the no_package,
 * unknown, or not_installed status depending on how they are used.
 *
 * \param[in] name  Name of the package to check for an X-Status
 *
 * \return The current package status, or no_package if the name is invalid
 */
wpkgar_manager::package_status_t wpkgar_manager::package_status(const wpkg_filename::uri_filename& name)
{
    // if the package is not in memory, we try to load it
    packages_t::const_iterator it(f_packages.find(name.basename()));
    if(it == f_packages.end())
    {
        if(!name.is_deb())
        {
            // if the name includes characters that cannot be part of the
            // Package field then it definitively was not installed
            return no_package;
        }
        load_package(name);

        // try again
        it = f_packages.find(name.basename());
        if(it == f_packages.end())
        {
            return not_installed;
        }
    }

    const wpkg_control::control_file& status(it->second->get_status_file_info());
    const case_insensitive::case_insensitive_string& x_status(status.get_field(wpkg_control::control_file::field_xstatus_factory_t::canonicalized_name()));
    if(x_status == "not-installed")     // heard of it, but not installed
    {
        return not_installed;
    }
    if(x_status == "config-files")      // was removed, not purged
    {
        return config_files;
    }
    if(x_status == "installing")        // in the act of installing right now
    {
        return installing;
    }
    if(x_status == "upgrading")         // in the act of upgrading right now
    {
        return upgrading;
    }
    if(x_status == "half-installed")    // install/update failed
    {
        return half_installed;
    }
    if(x_status == "unpacked")          // install/update succeeded
    {
        return unpacked;
    }
    if(x_status == "half-configured")   // configuration failed
    {
        return half_configured;
    }
    if(x_status == "installed")         // unpacked and configured
    {
        return installed;
    }
    if(x_status == "removing")          // in the act of removing
    {
        return removing;
    }
    if(x_status == "purging")           // in the act of purging
    {
        return purging;
    }
    if(x_status == "listing")           // core when reading information
    {
        return listing;
    }
    if(x_status == "verifying")         // core when verifying
    {
        return verifying;
    }
    if(x_status == "ready")             // core when in a normal state
    {
        return ready;
    }
    return unknown;
}


/** \brief Safely retrieve the status of a package.
 *
 * This function calls the package_status() function but if an error occurs
 * (i.e. an exception is thrown) then the function returns not_installed
 * instead of propagating the exception. This is useful to determine whether
 * a package is installed or not.
 *
 * Note that an error may represent an error other than not_installed. We
 * will ameliorate the code as we move forward, but in most cases that's
 * going to represent the correct result (i.e. half-installed packages are
 * not considered installed either.)
 *
 * \param[in] name  The name of the package to check.
 *
 * \return The current status of the package or not_installed.
 */
wpkgar_manager::package_status_t wpkgar_manager::safe_package_status(const wpkg_filename::uri_filename& name)
{
    try
    {
        return package_status(name);
    }
    catch(...)
    {
        return not_installed;
    }
}


/** \brief Add a self package.
 *
 * This function adds a self package to the list of self packages of the
 * manager. The package name represents one of the packages that, when
 * upgraded, means that we're upgrading the package that is being used
 * to do the upgrade.
 *
 * This is required under MS-Windows that does not support overwriting
 * an executable while it is running (i.e. wpkg.exe).
 *
 * The complete test is on installation:
 *
 * \li The package registered itself as a possibly self-upgrading package.
 * \li The package being installed match one of the self-packages.
 * \li The package being installed is an upgrade.
 *
 * \param[in] package  Name of the package that may self-upgrade.
 */
void wpkgar_manager::add_self(const std::string& package)
{
    f_selves[package] = 0;
}

/** \brief Mark whether a self package is being installed.
 *
 * This function checks whether we're upgrading ourselves by adding 1 to
 * the corresponding self entry.
 *
 * A package that makes use of the libdebpackages library to run an
 * equivalent of the --instal or --remove commands must make sure to copy
 * itself and run from the copy when running under MS-Windows because that
 * operating system prevents overwriting executables that are currently
 * running (i.e. those are locked and thus the file system has no concept
 * of files still being opened when deleted.)
 *
 * \return This function returns true if the package name specified here
 * is found in the list of package names added with add_self().
 *
 * \sa add_self()
 */
bool wpkgar_manager::include_self(const std::string& package)
{
    if(f_selves.find(package) != f_selves.end())
    {
        return f_include_selves = true;
    }

    return false;
}

/** \brief Check whether a package exists in the list of selves.
 *
 * This function has no side effect (opposed to the include_self() function)
 * and can be used to know whether the name of a package was added as a
 * "self" package. Packages that may run an update or a remove function
 * using the libdebpackages library need to add themselves to avoid problems
 * when attempting to run those functions. However, the auto-remove function
 * defines a list of selves that change over time and hence the use of this
 * function.
 *
 * \param[in] package  The package to check against the list of selves.
 *
 * \return true if the package is part of the list of selves.
 */
bool wpkgar_manager::exists_as_self(const std::string& package)
{
    return f_selves.find(package) != f_selves.end();
}

/** \brief Check whether we're upgrading ourself.
 *
 * This function returns true if at least one call to include_self()
 * returned true.
 *
 * \return true if we are upgrading ourselves.
 *
 * \sa add_self()
 * \sa include_self()
 */
bool wpkgar_manager::is_self() const
{
    return f_include_selves;
}

/** \brief List installed packages.
 *
 * This function searches for the list of installed packages in the
 * administration directory.
 *
 * The \p list parameter is cleared on entry.
 *
 * Only filenames that are also valid package names are returned.
 *
 * The resulting list is sorted by package name.
 *
 * \warning
 * Note that the results are not cached. Each time you call this function
 * the disk is accessed for the most current list of packages.
 *
 * \param[out] list  Where the list of packages is saved.
 */
void wpkgar_manager::list_installed_packages(package_list_t& list)
{
    if( f_installed_packages.empty() )
    {
        memfile::memory_file packages;
        packages.dir_rewind(get_database_path(), false);
        memfile::memory_file::file_info info;
        while(packages.dir_next(info))
        {
            if(info.get_file_type() == memfile::memory_file::file_info::directory)
            {
                const std::string& name(info.get_basename());
                // /tmp/wpkg-<pid>/packages includes all the temporarily extracted
                // packages; note that by default this is deleted on exit
                //
                // "core" is used for the global status of the installation
                // also a name must be a valid package name
                if(name != "core" && wpkg_util::is_package_name(name))
                {
                    f_installed_packages.push_back(name);
                }
            }
        }
        std::sort(f_installed_packages.begin(), f_installed_packages.end());
    }

    list = f_installed_packages;
}


/** \brief Load installed packages.
 *
 * This function loads all installed packages into memory.
 *
 */
void wpkgar_manager::load_installed_packages()
{
    package_list_t list;
    list_installed_packages( list );
    for( auto pkg : list )
    {
        load_package( pkg );
    }
}


/*===================================================================================*/
std::string source::get_type() const
{
    return f_type;
}

std::string source::get_parameter(const std::string& name, const std::string& def_value) const
{
    parameter_map_t::const_iterator it(f_parameters.find(name));
    if(it == f_parameters.end())
    {
        return def_value;
    }
    return it->second;
}

source::parameter_map_t source::get_parameters() const
{
    return f_parameters;
}

std::string source::get_uri() const
{
    return f_uri;
}

std::string source::get_distribution() const
{
    return f_distribution;
}

int source::get_component_size() const
{
    return static_cast<int>(f_components.size());
}

std::string source::get_component(int index) const
{
    return f_components[index];
}


void source::set_type(const std::string& type)
{
    f_type = type;
}

void source::add_parameter(const std::string& name, const std::string& value)
{
    f_parameters[name] = value;
}

void source::set_uri(const std::string& uri)
{
    f_uri = uri;
}

void source::set_distribution(const std::string& distribution)
{
    f_distribution = distribution;
}

void source::add_component(const std::string& component)
{
    f_components.push_back(component);
}


/** \brief Add a repository directory.
 *
 * This function is used to add one or more repository directories to the
 * remote source.
 *
 * \param[in] source  The source to add.
 */
void wpkgar_manager::add_repository( const source& source_repo )
{
    wpkg_filename::uri_filename repo_base;
    repo_base = source_repo.get_uri();
    if( !source_repo.get_distribution().empty() )
    {
        repo_base = repo_base.append_child("/");
        repo_base = repo_base.append_child( source_repo.get_distribution() );
    }

    const int cnt(source_repo.get_component_size());
    if( cnt > 0 )
    {
        for( int j(0); j < cnt; ++j )
        {
            wpkg_filename::uri_filename repo( repo_base );
            repo = repo.append_child("/");
            repo = repo.append_child( source_repo.get_component(j) );
            f_repository.push_back( repo );
        }
    }
    else
    {
        f_repository.push_back( repo_base );
    }
}


/** \brief Add a repository directory.
 *
 * This function is used to add one or more repository directories to the
 * remote object. This list is used whenever the rollback feature is used
 * and an error occurs. To reinstalled the package it gets loaded from one
 * of the repositories.
 *
 * You can only add one repository directory at a time.
 *
 * \param[in] repository  The repository directory to add.
 */
void wpkgar_manager::add_repository(const wpkg_filename::uri_filename& repository)
{
    // Note: although we test now whether those repository directories
    //       exist, at the time we use them they could have been
    //       deleted or replaced with another type of file
    if(repository.is_direct())
    {
        if(!repository.exists())
        {
            wpkg_output::log("repository %1 does not exist or is not accessible.")
                    .quoted_arg(repository)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_repository)
                .action("validation");
            return;
        }
        if(!repository.is_dir())
        {
            wpkg_output::log("repository %1 is not a directory as expected.")
                    .quoted_arg(repository)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_repository)
                .action("validation");
            return;
        }
    }
    // This message is annoying because you get it each time you install a package from the repository. And it's not
    // really very useful--you're not going to check if an "http:" scheme URL is valid or not until you try to access it.
    else
    {
        wpkg_output::log("repository %1 is not a local file and cannot be checked prior to actually attempting to use it.")
                .quoted_arg(repository)
            .level(wpkg_output::level_warning)
            .module(wpkg_output::module_repository)
            .action("validation");
    }

    f_repository.push_back(repository);
}


/** \brief Replace the list of repositories.
 *
 * This function replaces the current list of repositories with a new
 * list. Note that each repository in the list is actually added using
 * the add_repository() function after the list gets emptied. This
 * means each directory is check for validity at the time this function
 * gets called.
 *
 * \param[in] repositories  The list of repositories to save in the manager.
 */
void wpkgar_manager::set_repositories(const wpkg_filename::filename_list_t& repositories)
{
    f_repository.clear();
    for(wpkg_filename::filename_list_t::const_iterator f(repositories.begin());
                                                       f != repositories.end();
                                                       ++f)
    {
        add_repository(*f);
    }
}


/** \brief Retrieve the list of repositories.
 *
 * This function returns a reference to the existing list of repositories
 * in the manager. It is generally used when the list of repositories is
 * used or to make a copy of it.
 *
 * \return A constant reference to the list of files.
 */
const wpkg_filename::filename_list_t& wpkgar_manager::get_repositories() const
{
    return f_repository;
}


/** \brief Add the list of repositories in core/sources.list.
 *
 * Take the list of repositories from the core package's sources.list,
 * and add them as repositories. This will allow the wpkgar_install object
 * to detect all dependencies in the user's prefered source list.
 */
void wpkgar_manager::add_sources_list()
{
    wpkgar::wpkgar_repository repository( this->shared_from_this() );

    wpkg_filename::uri_filename sources_list( get_database_path() );
    sources_list = sources_list.append_child( "core/sources.list" );
    if( sources_list.exists() )
    {
        memfile::memory_file sources_file;
        sources_file.read_file( sources_list );

        source_vector_t	sources;
        repository.read_sources( sources_file, sources );

        for( const auto& src : sources )
        {
            add_repository( src );
        }
    }
}


bool wpkgar_manager::has_package(const wpkg_filename::uri_filename& package_name) const
{
    packages_t::const_iterator it(f_packages.find(package_name.basename()));
    if(it == f_packages.end())
    {
        return false;
    }

    return true;
}

const std::shared_ptr<wpkgar_package> wpkgar_manager::get_package(const wpkg_filename::uri_filename& package_name) const
{
    packages_t::const_iterator it(f_packages.find(package_name.basename()));
    if(it == f_packages.end())
    {
        throw wpkgar_exception_undefined("unknown package: \"" + package_name.original_filename() + "\"");
    }

    return it->second;
}

bool wpkgar_manager::has_control_file(const wpkg_filename::uri_filename& package_name, const std::string& control_filename) const
{
    // this checks whether a control file exists
    return get_package(package_name)->has_control_file(control_filename);
}

void wpkgar_manager::get_control_file(memfile::memory_file& p, const wpkg_filename::uri_filename& package_name, std::string& control_filename, bool compress)
{
    // this reads any control file, including control.tar.gz
    get_package(package_name)->read_control_file(p, control_filename, compress);
}

bool wpkgar_manager::validate_fields(const wpkg_filename::uri_filename& package_name, const std::string& expression)
{
    return get_package(package_name)->validate_fields(expression);
}

void wpkgar_manager::conffiles(const wpkg_filename::uri_filename& package_name, conffiles_t& filenames) const
{
    get_package(package_name)->conffiles(filenames);
}

bool wpkgar_manager::is_conffile(const wpkg_filename::uri_filename& package_name, const std::string& filename) const
{
    return get_package(package_name)->is_conffile(filename);
}

bool wpkgar_manager::field_is_defined(const wpkg_filename::uri_filename& package_name, const std::string& name) const
{
    return get_package(package_name)->get_control_file_info().field_is_defined(name)
        || get_package(package_name)->get_status_file_info().field_is_defined(name);
}

void wpkgar_manager::set_field(const wpkg_filename::uri_filename& package_name, const std::string& name, const std::string& value, bool save)
{
    const std::shared_ptr<wpkgar_package> p(get_package(package_name));
    wpkg_control::control_file& cf(p->get_status_file_info());
    cf.set_field(name, value);
    if(save)
    {
        memfile::memory_file ctrl;
        cf.write(ctrl, wpkg_field::field_file::WRITE_MODE_FIELD_ONLY);
        ctrl.write_file(p->get_package_path().append_child("wpkg-status"), true);
    }
}

void wpkgar_manager::set_field(const wpkg_filename::uri_filename& package_name, const std::string& name, long value, bool save)
{
    const std::shared_ptr<wpkgar_package> p(get_package(package_name));
    wpkg_control::control_file& cf(p->get_status_file_info());
    cf.set_field(name, value);
    if(save)
    {
        memfile::memory_file ctrl;
        cf.write(ctrl, wpkg_field::field_file::WRITE_MODE_FIELD_ONLY);
        ctrl.write_file(p->get_package_path().append_child("wpkg-status"), true);
    }
}

std::string wpkgar_manager::get_field(const wpkg_filename::uri_filename& package_name, const std::string& name) const
{
    const wpkg_control::control_file& control_info(get_package(package_name)->get_control_file_info());
    if(control_info.field_is_defined(name))
    {
        return control_info.get_field(name);
    }
    return get_package(package_name)->get_status_file_info().get_field(name);
}

std::string wpkgar_manager::get_description(const wpkg_filename::uri_filename& package_name, const std::string& name, std::string& long_description) const
{
    const wpkg_control::control_file& control_info(get_package(package_name)->get_control_file_info());
    if(control_info.field_is_defined(name))
    {
        return control_info.get_description(name, long_description);
    }
    return get_package(package_name)->get_status_file_info().get_description(name, long_description);
}

wpkg_dependencies::dependencies wpkgar_manager::get_dependencies(const wpkg_filename::uri_filename& package_name, const std::string& name) const
{
    const wpkg_control::control_file& control_info(get_package(package_name)->get_control_file_info());
    if(control_info.field_is_defined(name))
    {
        return control_info.get_dependencies(name);
    }
    return get_package(package_name)->get_status_file_info().get_dependencies(name);
}

const wpkg_control::control_file::field_file::list_t wpkgar_manager::get_field_list(const wpkg_filename::uri_filename& package_name, const std::string& name) const
{
    const wpkg_control::control_file& control_info(get_package(package_name)->get_control_file_info());
    if(control_info.field_is_defined(name))
    {
        return control_info.get_field_list(name);
    }
    return get_package(package_name)->get_status_file_info().get_field_list(name);
}

std::string wpkgar_manager::get_field_first_line(const wpkg_filename::uri_filename& package_name, const std::string& name) const
{
    const wpkg_control::control_file& control_info(get_package(package_name)->get_control_file_info());
    if(control_info.field_is_defined(name))
    {
        return control_info.get_field_first_line(name);
    }
    return get_package(package_name)->get_status_file_info().get_field_first_line(name);
}

std::string wpkgar_manager::get_field_long_value(const wpkg_filename::uri_filename& package_name, const std::string& name) const
{
    const wpkg_control::control_file& control_info(get_package(package_name)->get_control_file_info());
    if(control_info.field_is_defined(name))
    {
        return control_info.get_field_long_value(name);
    }
    return get_package(package_name)->get_status_file_info().get_field_long_value(name);
}

bool wpkgar_manager::get_field_boolean(const wpkg_filename::uri_filename& package_name, const std::string& name) const
{
    const wpkg_control::control_file& control_info(get_package(package_name)->get_control_file_info());
    if(control_info.field_is_defined(name))
    {
        return control_info.get_field_boolean(name);
    }
    return get_package(package_name)->get_status_file_info().get_field_boolean(name);
}

long wpkgar_manager::get_field_integer(const wpkg_filename::uri_filename& package_name, const std::string& name) const
{
    const wpkg_control::control_file& control_info(get_package(package_name)->get_control_file_info());
    if(control_info.field_is_defined(name))
    {
        return control_info.get_field_integer(name);
    }
    return get_package(package_name)->get_status_file_info().get_field_integer(name);
}

int wpkgar_manager::number_of_fields(const wpkg_filename::uri_filename& package_name) const
{
    return get_package(package_name)->get_control_file_info().number_of_fields()
         + get_package(package_name)->get_status_file_info().number_of_fields();
}

const std::string& wpkgar_manager::get_field_name(const wpkg_filename::uri_filename& package_name, int idx) const
{
    const wpkg_control::control_file& control_info(get_package(package_name)->get_control_file_info());
    int max_idx(control_info.number_of_fields());
    if(idx < max_idx)
    {
        return control_info.get_field_name(idx);
    }
    return get_package(package_name)->get_status_file_info().get_field_name(idx - max_idx);
}


void wpkgar_manager::set_tracker(std::shared_ptr<wpkgar_tracker_interface> tracker)
{
    f_tracker = tracker;
}


std::shared_ptr<wpkgar_tracker_interface> wpkgar_manager::get_tracker() const
{
    return f_tracker;
}


void wpkgar_manager::track(const std::string& command, const std::string& package_name)
{
    if(f_tracker)
    {
        f_tracker->track(command, package_name);
    }
}


/** \brief Add one global hook.
 *
 * This function adds one global hook to the wpkg administration system.
 * The hook will be called each time the system installs or removes a
 * package and the corresponding function is used (i.e. validate, preinst,
 * postinst, prerm, postrm.)
 *
 * The name of the hook is expected to be \<unique-name>_\<function>[.sh|bat]
 * and it can include a path if the file is not in the current directory.
 * This function makes a copy of the script from the existing location to
 * the specified wpkg administration directory.
 *
 * \param[in] script_name  Name of the script being added, including a path if necessary.
 */
void wpkgar_manager::add_global_hook(const wpkg_filename::uri_filename& script_name)
{
    if(!script_name.exists())
    {
        throw wpkgar_exception_invalid("the global hook script \"" + script_name.original_filename() + "\" does not exist.");
    }
    if(!script_name.is_reg())
    {
        throw wpkgar_exception_invalid("the global hook script \"" + script_name.original_filename() + "\" is not a regular file.");
    }
    memfile::memory_file script;
    script.read_file(script_name);

    // we'll need to have the core package ready
    load_package("core");
    const std::shared_ptr<wpkgar_package> core(get_package("core"));
    const wpkg_filename::uri_filename& core_package_path(core->get_package_path());
    const wpkg_filename::uri_filename hooks_path(core_package_path.append_child("hooks"));
#if defined(MO_WINDOWS)
    const char *ext(".bat");
#else
    const char *ext("");
#endif
    wpkg_filename::uri_filename destination(hooks_path.append_child("core_" + script_name.basename() + ext));
    script.write_file(destination, true);
}


/** \brief Remove one global hook.
 *
 * This function removes the specified global hook from the wpkg
 * administration system. The hook is simply deleted in this case.
 *
 * The name of the hook is expected to be \<unique-name>_\<function>[.sh|bat]
 * as it was specified when adding the hook. However, it cannot include a
 * path.
 *
 * \param[in] script_name  Name of the script being removed.
 *
 * \return true if the hook was successfully removed, false otherwise.
 */
bool wpkgar_manager::remove_global_hook(const wpkg_filename::uri_filename& script_name)
{
    if(script_name.segment_size() > 1)
    {
        throw wpkgar_exception_invalid("the global hook script \"" + script_name.original_filename() + "\" cannot include a path.");
    }

    // we'll need to have the core package ready
    load_package("core");
    const std::shared_ptr<wpkgar_package> core(get_package("core"));
    const wpkg_filename::uri_filename& core_package_path(core->get_package_path());
    const wpkg_filename::uri_filename hooks_path(core_package_path.append_child("hooks"));
    wpkg_filename::uri_filename destination(hooks_path.append_child("core_" + script_name.full_path()));
    return destination.os_unlink();
}


/** \brief Install the hooks of the specified package.
 *
 * This function installs all the hooks of a package in the system. This hook
 * installation is specifically for a package hook. The distinction is
 * important because user defined hooks (opposed to package hooks) make use
 * of the special package name "core" and these can be managed from the wpkg
 * tool command line whereas package hooks cannot.
 *
 * \param[in] package_name  Name of the package which is receiving a hook.
 */
void wpkgar_manager::install_hooks(const std::string& package_name)
{
    // we'll need to have the core package ready
    load_package("core");
    const std::shared_ptr<wpkgar_package> core(get_package("core"));
    const wpkg_filename::uri_filename& core_package_path(core->get_package_path());
    const wpkg_filename::uri_filename hooks_path(core_package_path.append_child("hooks"));

    const std::string prefix(package_name + "_");

    const std::shared_ptr<wpkgar_package> package(get_package(package_name));
    const wpkg_filename::uri_filename& package_path(package->get_package_path());
    memfile::memory_file package_dir;
    package_dir.dir_rewind(package_path, false);
    for(;;)
    {
        // IMPORTANT NOTE: We probably should not read the data of all
        //                 the files in this case since we really only
        //                 are interested by the data of the very few
        //                 hooks defined in this package (possibly zero!)
        memfile::memory_file::file_info info;
        memfile::memory_file data;
        if(!package_dir.dir_next(info, &data))
        {
            break;
        }
        if(info.get_file_type() != memfile::memory_file::file_info::regular_file)
        {
            // we're only interested by regular files, anything
            // else we skip silently (that includes "." and "..")
            continue;
        }
        std::string basename(info.get_basename());
        // if there is one, remove the extension from basename
        std::string::size_type pos(basename.find_last_of('.'));
        if(pos > 0 && pos != std::string::npos)
        {
            basename = basename.substr(0, pos);
        }
#if defined(MO_WINDOWS)
        // as a side note: the name of the package (and thus "prefix") is
        //                 always in lowercase or it is not valid
        std::transform(basename.begin(), basename.end(), basename.begin(), ::tolower);
#endif

        // if that's a hook, install it in the core/hooks/... directory
        if(basename.length() > prefix.length()
        && basename.substr(0, prefix.length()) == prefix)
        {
            const std::string hook(basename.substr(prefix.length()));
            if(hook == "validate"
            || hook == "preinst"
            || hook == "postinst"
            || hook == "prerm"
            || hook == "postrm")
            {
                // this is a hook, install it
#ifdef MO_WINDOWS
                const char *ext(".bat");
#else
                const char *ext("");
#endif
                const wpkg_filename::uri_filename destination(hooks_path.append_child(basename + ext));
                data.write_file(destination, true);
            }
        }
    }
}


/** \brief Remove all the hooks of the specified package.
 *
 * This function goes through the hooks directory of the specified
 * installation directory and deletes all the hooks that correspond to
 * the named package.
 *
 * \param[in] package_name  The name of the package for which hooks are to be removed.
 */
void wpkgar_manager::remove_hooks(const std::string& package_name)
{
    const wpkg_filename::uri_filename path(get_database_path().append_child("core/hooks"));
    if(path.is_dir())
    {
        memfile::memory_file dir;
        dir.dir_rewind(path.full_path());
        const std::string pattern("*/core/hooks/" + package_name + "_*");
        for(;;)
        {
            memfile::memory_file::file_info info;
            if(!dir.dir_next(info, NULL))
            {
                break;
            }
            if(info.get_uri().glob(pattern.c_str()))
            {
                // it's a match, get rid of it
                info.get_uri().os_unlink();
            }
        }
    }
}


/** \brief The list of currently installed hooks.
 *
 * This function reads the list of currently installed hooks and returns
 * it. The global hooks include the "core_" prefix in the name. It is up
 * to you to present those names to the end users without the "core_"
 * prefix and instead a mark to show that said hooks are global (opposed
 * to part of a specific package.)
 *
 * \return A list of hook names.
 */
wpkgar_manager::hooks_t wpkgar_manager::list_hooks() const
{
    hooks_t result;
    const wpkg_filename::uri_filename path(get_database_path().append_child("core/hooks"));
    if(path.is_dir())
    {
        memfile::memory_file dir;
        dir.dir_rewind(path.full_path());
        for(;;)
        {
            memfile::memory_file::file_info info;
            if(!dir.dir_next(info, NULL))
            {
                break;
            }
            if(info.get_file_type() == memfile::memory_file::file_info::regular_file)
            {
                result.push_back(info.get_basename());
            }
        }
    }
    return result;
}


/** \brief Run an installation or removal script.
 *
 * This function checks for one of the installation or removal scripts
 * and if it exists, executes it. If the script does not exist, then the
 * function returns immediately as if the script had succeeded.
 *
 * The function can be given a set of parameters in the \p param vector.
 *
 * wpkg changes the current directory to the root directory as defined
 * in the wpkg archive manager. This is done within the system() call so
 * we do not change the current directory of the packager tool itself.
 *
 * The system() call must return 0 for this function to return true
 * (i.e. ran with success.) Any other return value is a failure. If the
 * system() function call returns -1, the manager generates an error
 * saying that the script cannot be run. Other errors mean that the
 * script itself ran, but generated an error.
 *
 * \note
 * dpkg (the Debian packager tool) does not change directory before
 * executing script meaning that it runs the script in the directory where
 * dpkg is started.
 *
 * \par
 * The following is an example running against sample4 which prints
 * out the current directory when installing it on an Ubuntu system
 * using dpkg. As we can see the from directory shows the development
 * directory from where I work on wpkg instead of say / or some other
 * more logical directory for a package installer.
 *
 * \code
 * prompt# dpkg -i sample4_5.0_amd64.deb 
 * (Reading database ... 423914 files and directories currently installed.)
 * Unpacking sample4 (from sample4_5.0_amd64.deb) ...
 * preinst script called with: [install] from [.../usys/wpkg/mainline]
 * Setting up sample4 (5.0) ...
 * postinst script called with: [configure ] from [.../usys/wpkg/mainline]
 * prompt# dpkg --purge sample4
 * (Reading database ... 423917 files and directories currently installed.)
 * Removing sample4 ...
 * prerm script called with: [remove] from [.../usys/wpkg/mainline]
 * postrm script called with: [remove] from [.../usys/wpkg/mainline]
 * Purging configuration files for sample4 ...
 * postrm script called with: [purge] from [.../usys/wpkg/mainline]
 * \endcode
 *
 * \param[in] package_name  Name of the package of which the script is used.
 * \param[in] script  The script to run.
 * \param[in] params  A list of parameters which is not expected to be empty.
 *
 * \sa get_root_path()
 */
bool wpkgar_manager::run_script(const wpkg_filename::uri_filename& package_name, script_t script, script_parameters_t params)
{
    // make sure it's loaded
    load_package(package_name);

    // search for that script
    std::string control_filename;
    switch(script)
    {
    case wpkgar_script_validate:
        control_filename = "validate";
        break;

    case wpkgar_script_preinst:
        control_filename = "preinst";
        break;

    case wpkgar_script_postinst:
        control_filename = "postinst";
        break;

    case wpkgar_script_prerm:
        control_filename = "prerm";
        break;

    case wpkgar_script_postrm:
        control_filename = "postrm";
        break;

    default:
        throw std::logic_error("invalid script enumeration code");

    }
#ifdef MO_WINDOWS
    // at this time we do not support executables
    // (the --build does not check for binaries yet)
    control_filename += ".bat";
    // the MS-Windows batch cmd:
    const std::string interpreter("%COMSPEC% /q /c");
#else
    // default interpreter for Unix systems
    const std::string interpreter("sh -e");
#endif
    std::string parameters;
    for(script_parameters_t::const_iterator it(params.begin()); it != params.end(); ++it)
    {
        parameters += " " + wpkg_util::make_safe_console_string(*it);
    }
    if(package_name.original_filename() == "core")
    {
        // if the package name is core then the name of the scripts are
        // in a different location and we have to use a memory dir
        if(get_database_path().append_child("core/hooks").is_dir())
        {
            // the Unix directory feature does not support a globbing pattern
            // and we did not integrate that in there either so at this point
            // we read all the files and check the filename inside the loop
            std::string path(get_database_path().append_child("core/hooks").full_path());
            memfile::memory_file dir;
            dir.dir_rewind(path);
            const std::string pattern("*/core/hooks/*_" + control_filename);
            for(;;)
            {
                memfile::memory_file::file_info info;
                if(!dir.dir_next(info, NULL))
                {
                    break;
                }
                if(info.get_uri().glob(pattern.c_str()))
                {
                    if(!run_one_script(package_name, interpreter, info.get_filename(), parameters))
                    {
                        return false;
                    }
                }
            }
        }
    }
    else if(has_control_file(package_name, control_filename))
    {
        const wpkg_filename::uri_filename& path(get_package(package_name)->get_package_path());
        wpkg_filename::uri_filename script_name(path.append_child(control_filename));
        return run_one_script(package_name, interpreter, script_name, parameters);
    }

    return true;
}


bool wpkgar_manager::run_one_script(const wpkg_filename::uri_filename& package_name, const std::string& interpreter, const wpkg_filename::uri_filename& script_name, const std::string& parameters)
{
    std::string cmd("cd " + wpkg_util::make_safe_console_string(get_root_path().full_path()) + " && " + interpreter + " ");
    std::string script(wpkg_util::make_safe_console_string(script_name.full_path()));
#ifdef MO_WINDOWS
    if(script[0] == '"')
    {
        cmd += '"';
    }
#endif
    cmd += script;
    cmd += parameters;
#ifdef MO_WINDOWS
    if(script[0] == '"')
    {
        cmd += '"';
    }
#endif

    wpkg_output::log("system(%1).")
            .quoted_arg(cmd)
        .level(wpkg_output::level_info)
        .module(wpkg_output::module_run_script)
        .package(package_name)
        .action("execute-script");

    // Set a number of environment variables to give the running process some
    // info about the packager (i.e. package name, version, root path, full
    // name of the package being installed, os, vendor, processor, etc.); also
    // we may want to reset our own environment to not leak stuff that should
    // not be visible to those scripts
    //
    typedef std::map<std::string, std::string> env_map_t;
    env_map_t env;
    //
    env["WPKG_ROOT_PATH"]     = f_root_path.os_filename().get_utf8();
    env["WPKG_DATABASE_PATH"] = f_database_path.os_filename().get_utf8();
    env["WPKG_PACKAGE_NAME"]  = package_name.os_filename().get_utf8();

#ifdef MO_WINDOWS
    int r;

    for( env_map_t::const_iterator it = env.cbegin(); it != env.cend(); ++it )
    {
        std::string value (it->second);
        //
        // Since this is MSWindows, we have to make sure the
        // slash is the "right" slash for the OS (e.g. '\', not '/').
        //
        std::replace(value.begin(), value.end(), '/', '\\');
        //
        r = _wputenv_s(libutf8::mbstowcs(it->first).c_str(),
                       libutf8::mbstowcs(value).c_str());
    }

    // under MS-Windows we have to convert the UTF-8 to UTF-16 before
    // calling the system function or it will fail if characters outside
    // of the locale are used
    r = _wsystem(libutf8::mbstowcs(cmd).c_str());

    // windows does not always flush properly although the /q should
    // prevent a lot of the output from the script from showing up
    fflush(stdout);
    fflush(stderr);
#else
    int r;

    for( env_map_t::const_iterator it = env.cbegin(); it != env.cend(); ++it )
    {
        r = setenv( it->first.c_str(), it->second.c_str(), 1 );
    }

    r = system( cmd.c_str() );
#endif
    wpkg_output::log("system() call returned %1")
            .arg(r)
        .debug(wpkg_output::debug_flags::debug_scripts)
        .module(wpkg_output::module_run_script)
        .package(package_name)
        .action("execute-script");
    if(r == -1)
    {
        wpkg_output::log("upgrade script %1 could not be run properly (it looks like it did not even start!)")
                .quoted_arg(script_name)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_run_script)
            .package(package_name)
            .action("execute-script");
        return false;
    }
    else if(r != 0)
    {
        return false;
    }
    return true;
}

void wpkgar_manager::set_interrupt_handler(wpkgar_interrupt *handler)
{
    f_interrupt_handler = handler;
}

void wpkgar_manager::check_interrupt() const
{
    if(f_interrupt_handler && f_interrupt_handler->stop_now())
    {
        throw wpkgar_exception_stop("external interrupt point triggered");
    }
}




wpkgar_lock::wpkgar_lock(wpkgar_manager::pointer_t manager, const std::string& status)
    : f_manager(manager)
{
    f_manager->lock(status);
}


wpkgar_lock::~wpkgar_lock()
{
    unlock();
}

void wpkgar_lock::unlock()
{
    if(f_manager != NULL)
    {
        f_manager->unlock();
        f_manager = NULL;
    }
}

wpkgar_interrupt::~wpkgar_interrupt()
{
}

bool wpkgar_interrupt::stop_now()
{
    return false;
}


wpkgar_tracker_interface::~wpkgar_tracker_interface()
{
}

/** \brief Track an event.
 *
 * The default function just logs the event as a debug_full level.
 *
 * You should call this function first, then apply your journaling.
 *
 * \param[in] command  The command being tracked.
 * \param[in] package_name  The name of the package for errors and log.
 */
void wpkgar_tracker_interface::track(const std::string& command, const std::string& package_name)
{
    wpkg_output::log("save tracking command %1")
            .quoted_arg(command)
        .debug(wpkg_output::debug_flags::debug_full)
        .module(wpkg_output::module_track)
        .package(package_name)
        .action("package-track");

}


}
// vim: ts=4 sw=4 et
