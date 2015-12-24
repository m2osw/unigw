/*    wpkg_backup.cpp -- manage a set of files being backed up
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
 * \brief Copy files to a backup directory.
 *
 * This class implementation is used to copy files from one directory to
 * another and eventually restore the backup if the current process fails.
 */
#include "libdebpackages/wpkg_backup.h"

#include    <errno.h>
#if defined(MO_LINUX) || defined(MO_DARWIN)
#   include <unistd.h>
#endif

namespace wpkg_backup
{


/** \class wpkgar_backup
 * \brief The backup class to keep track of backed up files.
 *
 * The backup class implements functions useful to backup (copy) files
 * from their current location to a backup location (the temporary folder).
 * A backup object destructor ensures that the backed up files are restored
 * unless the process marked the backup as successful.
 *
 * The backup can be used to copy existing files or mark that the file
 * did not exit in the first place (i.e. if a restore occurs, the new file
 * gets deleted.)
 *
 * \todo
 * The current backup process does not save any of the meta data of files
 * which means that the restore feature cannot actually properly restore
 * everything the way it was.
 */


/** \brief Initialize a backup object.
 *
 * This function initializes a backup object so a process can update files
 * safely (i.e. if an error occurs while updating, the backup can restore
 * the state from before the update started.)
 */
wpkgar_backup::wpkgar_backup(wpkgar::wpkgar_manager::pointer_t manager, const std::string& package_name, const char *log_action)
    : f_manager(manager)
    , f_package_name(package_name)
    , f_log_action(log_action)
    //, f_files() -- auto-init
    //, f_count(0) -- auto-init
    //, f_success(false) -- auto-init
{
}


/** \brief Destroy the backup object.
 *
 * This function calls the restore() function which restores the status of
 * the system before the backup started, unless the success() function was
 * called in between and thus the backup files are removed but the rest
 * stays in place.
 */
wpkgar_backup::~wpkgar_backup()
{
    restore();
}


/** \brief Backup the specified file.
 *
 * This function informs the backup object that we are about to replace
 * the specified file. If the file already exists, the function makes
 * a copy by reading the whole file in memory and then saving that back
 * to disk.
 *
 * The backup function also understands that when the file does not
 * exist yet, the \em backup means marking that the new file will need
 * to be deleted on a restore.
 *
 * However, if the file cannot be backed up, because of an I/O error or
 * because of permissions or similar error, then the backup function
 * returns false.
 *
 * Note that if you try to backup the same file twice, the function returns
 * false as if the second backup failed. This is used as a safeguard.
 *
 * \return true if the file gets backed up.
 */
bool wpkgar_backup::backup(const wpkg_filename::uri_filename& filename)
{
    // make sure we did not already make a backup because
    // when we work on the upgrade item we generally have
    // many identical files
    if(f_files.find(filename.full_path()) != f_files.end())
    {
        return false;
    }

    if(!filename.exists())
    {
        if(errno == ENOENT)
        {
            // if the file did not exist we'll have to remove it on error
            f_files[filename.full_path()] = "";
        }
        else
        {
            wpkg_output::log("file %1 could not be backed up, stat() failed.")
                    .quoted_arg(filename)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_unpack_package)
                .action(f_log_action);
        }
        return false;
    }
    if(filename.is_dir())
    {
        // TODO:
        // recursively backup the directory!!!
        throw std::runtime_error("backup directory not implemented");
    }

    ++f_count; // start with file1.bak
    wpkg_filename::uri_filename destination(f_manager->get_database_path().append_child("tmp/backup/file").append_path(f_count).append_path(".bak"));

    // the following may throw if the copy doesn't work (i.e. read or
    // write fails)
    {
        memfile::memory_file f;
        f.read_file(filename);
        f.write_file(destination, true); // create backup folder if necessary
    }

    // it worked, save the info in our f_files map
    f_files[filename.full_path()] = destination.full_path();

    wpkg_output::log("%1 backed up as %2...")
            .quoted_arg(filename)
            .quoted_arg(destination)
        .debug(wpkg_output::debug_flags::debug_detail_files)
        .module(wpkg_output::module_unpack_package)
        .action(f_log_action);

    return true;
}


/** \brief Restore the original state.
 *
 * This function is the real purpose of the backup function. It restores all
 * the files as they were before the process requiring the backup started.
 *
 * If your process worked, however, it should call the success() function.
 * The restore itself does not restore the files anymore, however, it still
 * gets rid of the backup files.
 *
 * Note that once you called the restore() function, you cannot backup files
 * anymore. (This is not a requirement at this point, but it is likely to
 * become one at some point.)
 *
 * Note that this function never throws, mainly because it is not really
 * sensible. However, if an error occurs, the restore did not work and you
 * do not really know about the fact.
 */
void wpkgar_backup::restore()
{
    try
    {
        // restore and delete files as required
        for(backup_files_t::const_iterator it(f_files.begin()); it != f_files.end(); ++it)
        {
            if(!f_success)
            {
                // unpack failed, restore the files from their backup
                try
                {
                    if(it->second == "")
                    {
                        wpkg_filename::uri_filename to_delete(it->first);
                        to_delete.os_unlink();
                    }
                    else
                    {
                        memfile::memory_file f;
                        f.read_file(it->second);
                        f.write_file(it->first);
                    }
                }
                catch(const std::exception&)
                {
                    // ignore errors because we want to try to restore as many
                    // files as possible; plus we're likely in a destructor and
                    // throwing is not welcome there
                    wpkg_output::log("file %1 could not be restored (backup is here: %2.)")
                            .quoted_arg(it->first)
                            .quoted_arg(it->second)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_unpack_package)
                        .action(f_log_action);
                }
            }
        }
        // delete the backups if any
        // we do this in a second loop just in case it were to generate an error we
        // do not catch; that way we first restore everything and then we remove
        // backups which do or do not need to be valid
        for(backup_files_t::const_iterator it(f_files.begin()); it != f_files.end(); ++it)
        {
            if(it->second != "")
            {
                try
                {
                    wpkg_filename::uri_filename to_delete(it->second);
                    to_delete.os_unlink();
                }
                catch(const std::exception&)
                {
                    wpkg_output::log("backup file %1 could not be removed.")
                            .quoted_arg(it->second)
                        .level(wpkg_output::level_warning)
                        .module(wpkg_output::module_unpack_package)
                        .action(f_log_action);
                }
            }
        }
    }
    catch(...)
    {
    }

    // make sure we don't restore more than once
    f_files.clear();
}


/** \break Mark the the process succeeded so the backup can be forfeited.
 *
 * Call this function once you are done with your process in order to
 * restore the files that got backed up or delete those that were newly
 * created.
 *
 * Note that either way the backup files will get deleted when the backup
 * object gets destroyed or restore() is called.
 */
void wpkgar_backup::success()
{
    f_success = true;
}

} // wpkg_backup namespace
// vim: ts=4 sw=4 et
