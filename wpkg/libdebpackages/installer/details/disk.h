/*    disk.h -- disk utilities for non-bsd systems and non-sunos
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
 *	   Doug Barbieri  doug@m2osw.com
 */

/** \file
 * \brief Declaration of the classes that provide under-the-hood access to the disk
 */
#pragma once

#if !defined(MO_DARWIN) && !defined(MO_SUNOS) && !defined(MO_FREEBSD)

#include "libdebpackages/wpkgar.h"
#include "libdebpackages/installer/flags.h"
#include "libdebpackages/installer/package_item.h"
#include "libdebpackages/installer/package_list.h"

#include <memory>

namespace wpkgar
{

namespace installer
{

namespace details
{


/** \brief The disk class to manage installation sizes.
 *
 * This class handles disk metadeta when computing the installation
 * sizes of a package (i.e. evaluating whether the destination disk
 * is large enough to accept the packages that are about to be
 * installed on it.)
 *
 * This class has a function called match() which detects whether
 * a given filename represents a file to be installed on that disk.
 *
 * \note
 * At this time this functionality is not available at all on any system
 * because there are problems on all except Linux. The current Microsoft
 * Windows implementation does not properly detect what a letter drive
 * really represents and as such would revert to C:\\ and often generate
 * and error when the other drive(s) where the packages would really
 * be installed were more than large enough.
 */
class DEBIAN_PACKAGE_EXPORT disk_t
{
public:
    disk_t(const wpkg_filename::uri_filename& path);

    const wpkg_filename::uri_filename& get_path() const;
    bool match(const wpkg_filename::uri_filename& path);
    void add_size(int64_t size);
    //int64_t size() const;
    void set_block_size(uint64_t block);
    void set_free_space(uint64_t space);
    //uint64_t free_space() const;
    void set_readonly();
    bool is_valid() const;

private:
    wpkg_filename::uri_filename     f_path;
    controlled_vars::zint64_t       f_size;
    controlled_vars::zint64_t       f_block_size;
    controlled_vars::zuint64_t      f_free_space;
    controlled_vars::fbool_t        f_readonly;
};


/** \brief The list of all the disks accessible on this system.
 *
 * As we are installing files on one or more disks, this object holds a
 * list of all the disks that were accessed so far.
 *
 * This class offers an add_size() function which checks by going through
 * the list of disks which one is a match. Once the match was found, it
 * then calls the add_size() on that disk.
 *
 * The compute_size_and_verify_overwrite() function is used to find out
 * whether all the disks have enough room for all the data being added
 * to each one of them.
 */
class DEBIAN_PACKAGE_EXPORT disk_list_t
{
public:
    disk_list_t
        ( wpkgar_manager::pointer_t manager
        , package_list::pointer_t package_list
        , flags::pointer_t flags
        );
    void add_size(const std::string& path, int64_t size);
    void compute_size_and_verify_overwrite
        ( const package_item_t::list_t::size_type idx
        , const package_item_t& item
        , const wpkg_filename::uri_filename& root
        , memfile::memory_file *data
        , memfile::memory_file *upgrade
        , int factor
        );
    bool are_valid() const;

private:
    typedef std::map<std::string, memfile::memory_file::file_info> filename_info_map_t;

    disk_t *find_disk(const std::string& path);

    wpkgar_manager::pointer_t       f_manager;
    package_list::pointer_t         f_package_list;
    flags::pointer_t                f_flags;
    std::vector<disk_t>             f_disks;
    disk_t *                        f_default_disk; // used in win32 only
    filename_info_map_t             f_filenames;
};

}   // details namespace

}   // installer namespace

}   // namespace wpkgar

#endif //!MO_DARWIN && !MO_SUNOS && !MO_FREEBSD

// vim: ts=4 sw=4 expandtab syntax=cpp.doxygen fdm=marker
