/*    wpkgar_block.cpp -- declaration of the wpkg block for archives
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
 * \brief Implementation of the block archive class.
 *
 * The block archive is a class used to manage wpkg specific archives.
 * The implementation defines a few things that could not fit properly
 * in the header file.
 */
#include    "libdebpackages/wpkgar_block.h"
#include    "controlled_vars/controlled_vars_static_assert.h"
#include    <cstring>
#ifdef _MSC_VER
// Warning C4127: "conditional expression is constant"
#pragma warning(disable: 4127)
#endif

namespace wpkgar
{

/** \struct wpkgar_block_t
 * \brief The block of a wpkgar archive file used to index installed files.
 *
 * The installation of a package generates an index with the name of each
 * one of the files being installed. This structure represents a block in
 * that file.
 *
 * Most such index files are composed of such structures one after another.
 * However, since version 1.1, it is possible to find blocks that actually
 * represents the filename or a symbolic link that are longer than 300
 * characters.
 *
 * The structure has one function, a constructor which makes sure that a
 * new block gets initialized with all zeroes.
 *
 * The metadata of the files are also saved in this block. These meta data
 * are similar to the metadata saved in a tar header.
 */


/** \brief The wpkgar version.
 *
 * This version is saved in each block of the wpkgar files. It is used to make
 * sure we do support the data defined in the file.
 */
const uint8_t   WPKGAR_VERSION_1_0[4] = { '1', '.', '0', '\0' }; // exactly 4 bytes
const uint8_t   WPKGAR_VERSION_1_1[4] = { '1', '.', '1', '\0' }; // exactly 4 bytes

// compile time verification of the size of wpkgar_block_t
CONTROLLED_VARS_STATIC_ASSERT(sizeof(wpkgar_block_t) == 1024);

wpkgar_block_t::wpkgar_block_t()
{
    memset(&f_magic, 0, sizeof(wpkgar_block_t));
}

}
// vim: ts=4 sw=4 et
