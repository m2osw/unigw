/*    wpkg_architecture.h -- declaration of the architecture file format
 *    Copyright (C) 2012-2013  Made to Order Software Corporation
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
#ifndef WPKG_ARCHITECTURE_H
#define WPKG_ARCHITECTURE_H

/** \file
 * \brief Declaration of the architecture classes.
 *
 * This file includes the definitions of the architecture class. This class
 * is used to parse and compare architectures against each other. It can
 * also output a canonicalized version of the architecture.
 */
#include    "memfile.h"
#include    "case_insensitive_string.h"
#include    "wpkg_output.h"


namespace wpkg_architecture
{

class wpkg_architecture_exception : public std::runtime_error
{
public:
    wpkg_architecture_exception(const std::string& what_msg) : runtime_error(what_msg) {}
};

class wpkg_architecture_exception_invalid : public wpkg_architecture_exception
{
public:
    wpkg_architecture_exception_invalid(const std::string& what_msg) : wpkg_architecture_exception(what_msg) {}
};






class DEBIAN_PACKAGE_EXPORT architecture
{
public:
    struct abbreviation_t
    {
        const char * const  f_abbreviation;
        const char * const  f_os;
        const char * const  f_processor;
    };
    struct os_t
    {
        const char * const  f_name;
    };

    typedef unsigned char   endian_t;
    static const endian_t   PROCESSOR_ENDIAN_UNKNOWN = 0;
    static const endian_t   PROCESSOR_ENDIAN_LITTLE  = 1;
    static const endian_t   PROCESSOR_ENDIAN_BIG     = 2;
    static const endian_t   PROCESSOR_ENDIAN_BI      = 3;
    static const endian_t   PROCESSOR_ENDIAN_MIDDLE  = 4;
    struct processor_t
    {
        const char * const  f_name;
        const char * const  f_other_names;
        const char          f_bits;
        const endian_t      f_endian;
    };

    static bool valid_vendor(const std::string& vendor);
    static const abbreviation_t *find_abbreviation(const std::string& abbreviation);
    static const os_t *find_os(const std::string& os);
    static const processor_t *find_processor(const std::string& processor, bool extended);
    static const abbreviation_t *abbreviation_list();
    static const os_t *os_list();
    static const processor_t *processor_list();

    architecture();
    architecture(const std::string& arch, bool ignore_vendor_field = false);
    architecture(const architecture& arch);

    bool empty() const;
    bool is_pattern() const;

    bool set(const std::string& arch);
    std::string to_string() const;

    void set_ignore_vendor(bool ignore_vendor_field);
    bool ignore_vendor() const;

    architecture& operator = (const architecture& rhs);

    operator std::string () const;
    operator bool () const;
    bool operator ! () const;
    bool operator == (const architecture& rhs) const;
    bool operator != (const architecture& rhs) const;
    bool operator <  (const architecture& rhs) const;
    bool operator <= (const architecture& rhs) const;
    bool operator >  (const architecture& rhs) const;
    bool operator >= (const architecture& rhs) const;

private:

    // architecture triplet <os>-<vendor>-<processor>
    std::string                 f_os;
    std::string                 f_vendor;
    std::string                 f_processor;

    // while doing a compare, ignore vendor field?
    controlled_vars::fbool_t    f_ignore_vendor;
};



}       // namespace wpkg_architecture

#endif
//#ifndef WPKG_ARCHITECTURE_H
// vim: ts=4 sw=4 et
