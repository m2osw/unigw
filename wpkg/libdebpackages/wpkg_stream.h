/*    wpkg_stream.h -- access file data, always in Unicode
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
 * \brief wpkg stream declaration.
 *
 * For all files to be handled with full Unicode support (UTF-8 under Unices
 * and UTF-16 under MS-Windows) we have our own wpk_stream class. This knows
 * how to open files with filenames that may include Unicode characters.
 *
 * Internally the stream makes use of the FILE object under Unices and of
 * the HANDLE object via a CreateFile() call.
 */
#ifndef WPKG_STREAM_H
#define WPKG_STREAM_H

#include    "libdebpackages/wpkg_filename.h"
#include    "libdebpackages/compatibility.h"
#include    "controlled_vars/controlled_vars_ptr_auto_init.h"
#include    "controlled_vars/controlled_vars_auto_enum_init.h"

namespace wpkg_stream
{




class DEBIAN_PACKAGE_EXPORT fstream
{
public:
    typedef int64_t             size_type;
    typedef int64_t             off_type;
    enum seekdir
    {
        beg,
        end,
        cur
    };

    virtual                     ~fstream();

    bool                        create(const wpkg_filename::uri_filename& filename);
    bool                        open(const wpkg_filename::uri_filename& filename);
    bool                        append(const wpkg_filename::uri_filename& filename);
    void                        close();

    bool                        good() const;
    void                        seek(off_type offset, seekdir dir);
    off_type                    tell() const;

    size_type                   read(void *buffer, size_type size) const;
    size_type                   write(const void *buffer, size_type size) const;

private:
    wpkg_filename::uri_filename             f_filename;
#if defined(MO_WINDOWS)
    wpkg_compatibility::standard_handle_t   f_file;
#else
    controlled_vars::ptr_auto_init<FILE>    f_file;
#endif
    controlled_vars::fbool_t                f_do_not_close;
};



} // namespace wpkg_stream
#endif
//#ifndef WPKG_STREAM_H
// vim: ts=4 sw=4 et
