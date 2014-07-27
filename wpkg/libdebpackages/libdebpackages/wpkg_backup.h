/*    wpkg_backup.h -- manage a set of files being backed up
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
#ifndef WPKG_BACKUP_H
#define WPKG_BACKUP_H

/** \file
 * \brief Backup declarations.
 *
 * The wpkg library libdebpackages supports a way to create backup of files.
 * This is used to restore files in case an error occurs while doing an
 * installation or a removal.
 */
#include "libdebpackages/wpkgar.h"

namespace wpkg_backup
{

class wpkgar_backup
{
public:
    wpkgar_backup(wpkgar::wpkgar_manager *manager, const std::string& package_name, const char *log_action);
    ~wpkgar_backup();

    bool backup(const wpkg_filename::uri_filename& filename);
    void restore();
    void success();     // the unpack worked, do not restore backup!

private:
    // list of files indexed by the original file names
    // the second parameter is the name of the backup file
    typedef std::map<std::string, std::string>  backup_files_t;

    wpkgar::wpkgar_manager *    f_manager;
    std::string                 f_package_name;
    const char *                f_log_action;
    backup_files_t              f_files;
    controlled_vars::zuint32_t  f_count;
    controlled_vars::fbool_t    f_success;
};

} // wpkg_backup namespace
#endif
//#ifndef WPKG_BACKUP_H
// vim: ts=4 sw=4 et
