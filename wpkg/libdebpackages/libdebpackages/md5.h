// Copyright (c) 2007-2014  Made to Order Software Corporation
// This code is based on an MD5 library written by:
// WIDE Project, Poul-Henning Kamp, University of California, Matthias Kramm
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
#ifndef MD5_H
#define MD5_H

/** \file
 * \brief List of md5 functions one can use to compute an md5 checksum.
 *
 * This file has the md5 class declaration. That class is used to compute
 * the md5 checksum of files on disk and in memory.
 *
 * Note that the library supports \em raw or binary md5 sums. It can also
 * generate an hex string as it is often saved in text files.
 */
#include    "debian_export.h"

#include    <stdint.h>
#include    <string>

namespace md5
{

struct DEBIAN_PACKAGE_EXPORT raw_md5sum
{
    static const int MD5SUM_RAW_BUFSIZ = 16;

    bool operator == (const raw_md5sum& rhs) const;
    bool operator != (const raw_md5sum& rhs) const;

    uint8_t     f_sum[MD5SUM_RAW_BUFSIZ];
};


// md5 uses a context to offer an incremental interface
// 
// Usage:
//  md5sum.clear() to reset any other md5sum
//  md5sum.push_back(buffer, len) -- repeat as required (i.e. if reading
//                                   a file little by little for instance)
//  // retrieve raw sum (16 bytes hex)
//  raw_md5sum raw;
//  md5sum.raw_sum(raw);
//  // or for a string:
//  md5sum_str = md5sum.sum();
//
//  empty() is used to know whether push_bash() has been called before
//  size() returns the total length of data passed to push_back()
//
class DEBIAN_PACKAGE_EXPORT md5sum
{
public:

                        md5sum();

    void                clear();
    bool                empty() const;
    uint64_t            size() const;
    void                push_back(const uint8_t *data, size_t size);
    void                raw_sum(raw_md5sum& raw) const;
    std::string         sum() const;
    static std::string  sum(const raw_md5sum& raw);

private:
    void                calc();
    uint32_t            round1(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t k, uint32_t s, uint32_t i);
    uint32_t            round2(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t k, uint32_t s, uint32_t i);
    uint32_t            round3(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t k, uint32_t s, uint32_t i);
    uint32_t            round4(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t k, uint32_t s, uint32_t i);

    uint64_t            f_size;     // byte size

    uint32_t            f_a;        // state
    uint32_t            f_b;
    uint32_t            f_c;
    uint32_t            f_d;

    uint32_t            f_pos;      // buffer index
    uint32_t            f_buffer[16];   // 64 bytes
};



}   // namespace md5
#endif
// vim: ts=4 sw=4 et
