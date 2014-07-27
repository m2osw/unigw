/*    wpkgar_block.h -- declaration of the wpkg archive block
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

/** \file
 * \brief The wpkg archive file for indexes.
 *
 * When installing a package with wpkg, it creates an index using the wpkg
 * archive format which is the list of files without any data (think of it
 * as a tarball on which you can run `tar tvf archive` but cannot
 * `tar xf archive`).
 *
 * The header defined in this file is the header used to define each file.
 * Although in most cases we use exactly one block per file, when a file
 * has a name that does not fit in the block header (i.e. 300 UTF-8
 * bytes) then additional blocks are used to define the full filename.
 * This also applies to symbolic links.
 *
 * The size of one block is exactly 1Kb.
 */
#ifndef WPKGAR_BLOCK_H
#define WPKGAR_BLOCK_H

#include    "debian_export.h"

#include    <stdint.h>


namespace wpkgar {

// a wpkg archive is a list of filenames that were installed on a system
// this is similar to a tar header without the data of the file
//
// Note: this structure is saved on disk but it is not expected to be
//       shared between systems and thus we use binary values which
//       should not generate endianess concerns although the magic is
//       inverted (on purpose) when the endianess is inverted

const uint32_t  WPKGAR_MAGIC = ('W' << 24) | ('P' << 16) | ('K' << 8) | 'G';
const uint32_t  WPKGAR_MAGIC_OTHER_ENDIAN = ('G' << 24) | ('K' << 16) | ('P' << 8) | 'W';
DEBIAN_PACKAGE_EXPORT extern const uint8_t WPKGAR_VERSION_1_0[4];
DEBIAN_PACKAGE_EXPORT extern const uint8_t WPKGAR_VERSION_1_1[4];

// the wpkgar file is a set of these blocks
struct DEBIAN_PACKAGE_EXPORT wpkgar_block_t
{
    enum wpkgar_type_t {
        WPKGAR_TYPE_REGULAR = 0,
        WPKGAR_TYPE_HARD_LINK = 1,
        WPKGAR_TYPE_SYMBOLIC_LINK = 2,
        WPKGAR_TYPE_CHARACTER_SPECIAL = 3,
        WPKGAR_TYPE_BLOCK_SPECIAL = 4,
        WPKGAR_TYPE_DIRECTORY = 5,
        WPKGAR_TYPE_FIFO = 6,
        WPKGAR_TYPE_CONTINUOUS = 7,
        WPKGAR_TYPE_PACKAGE = 8     // control file from package
    };
    enum wpkgar_compression_t {
        WPKGAR_COMPRESSION_NONE,
        WPKGAR_COMPRESSION_GZ,
        WPKGAR_COMPRESSION_BZ2,
        WPKGAR_COMPRESSION_LZMA,
        WPKGAR_COMPRESSION_XZ
    };
    enum wpkgar_usage_t {
        WPKGAR_USAGE_UNKNOWN = 0,
        WPKGAR_USAGE_PROGRAM = 1,
        WPKGAR_USAGE_DATA = 2,
        WPKGAR_USAGE_CONFIGURATION = 3
    };
    enum wpkgar_status_t {
        WPKGAR_STATUS_UNKNOWN = 0,      // state is not currently known
        WPKGAR_STATUS_NOT_INSTALLED = 1,// file is being looked at, prepared
        WPKGAR_STATUS_INSTALLED = 2,    // installed from package
        WPKGAR_STATUS_CREATED = 3,      // this file was created
        WPKGAR_STATUS_INSTALLING = 4,   // started installing...
        WPKGAR_STATUS_MODIFIED = 5,     // configuration files
        WPKGAR_STATUS_CONFLICT = 6,     // file was replaced because of conflict
        WPKGAR_STATUS_CORRUPT = 7       // md5sum mismatch and not configuration
    };

    wpkgar_block_t();

    uint32_t    f_magic;        // 'WPKG' (GKPW if endian is inverted)
    uint8_t     f_version[4];   // '1.0\0' or '1.1\0' (not endian affected)
    uint8_t     f_type;         // wpkgar_type_t
    uint8_t     f_original_compression; // for files we store uncompressed (control.tar & data.tar)
    uint8_t     f_use;          // wpkgar_usage_t
    uint8_t     f_status;       // wpkgar_status_t
    uint32_t    f_uid;          // user identifier (if f_user undefined)
    uint32_t    f_gid;          // group identifier (if f_group undefined)
    uint32_t    f_mode;         // "rwxrwxrwx" mode, may include s & t as well
    uint32_t    f_size;         // size of the file in the source package
    uint32_t    f_mtime;        // last modification time in the source package
    uint32_t    f_dev_major;    // if type is character or block special or 0
    uint32_t    f_dev_minor;    // if type is character or block special or 0
    uint8_t     f_name[300];    // filename including path
    uint8_t     f_link[300];    // hard/symbolic link destination
    uint8_t     f_user[32];     // user name when available
    uint8_t     f_group[32];    // group name when available
    uint8_t     f_md5sum[16];   // the original file md5sum (raw)
    uint16_t    f_name_size;    // extended filename if not zero (up to 64Kb - 1) (since version 1.1)
    uint16_t    f_link_size;    // extended symbolic link if not zero (up to 64Kb - 1) (since version 1.1)

    // space left blank so the structure is exactly 1Kb (1024 bytes)
    // we'll use that space as we see fit
    // if the number of reserved bytes becomes null or negative then
    // the compiler will complain
    uint8_t     f_reserved[1024 - (4 + 4 + 1 + 1 + 1 + 1 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 300 + 300 + 32 + 32 + 16 + 2 + 2 + 4)];
    uint32_t    f_checksum;     // sum of all the header as uint8_t with f_checksum = 0 at the time
};

}       // namespace wpkgar

#endif
//#ifndef WPKGAR_BLOCK_H
// vim: ts=4 sw=4 et
