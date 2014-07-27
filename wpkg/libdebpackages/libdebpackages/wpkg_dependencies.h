/*    wpkg_dependencies.h -- declaration of the dependencies class
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
 * \brief Definition of the dependencies classes.
 *
 * The dependencies class allow one to parse lists of dependencies as those
 * found on the Depends or similar fields. The class is also capable to
 * spit the information back out as read (although canonicalized) and for
 * a specific architecture.
 */
#ifndef WPKG_DEPENDENCIES_H
#define WPKG_DEPENDENCIES_H
#include    "debian_export.h"
#include    "controlled_vars/controlled_vars_auto_init.h"
#include    "controlled_vars/controlled_vars_limited_auto_init.h"
#include    <stdexcept>
#include    <vector>


namespace wpkg_dependencies
{

class wpkg_dependencies_exception : public std::runtime_error
{
public:
    wpkg_dependencies_exception(const std::string& what_msg) : runtime_error(what_msg) {}
};

class wpkg_dependencies_exception_invalid : public wpkg_dependencies_exception
{
public:
    wpkg_dependencies_exception_invalid(const std::string& what_msg) : wpkg_dependencies_exception(what_msg) {}
};



class DEBIAN_PACKAGE_EXPORT dependencies
{
public:
    enum dependency_operator_t {
        operator_any,
        operator_lt,
        operator_le,
        operator_eq,
        operator_ne, // not valid in control files
        operator_ge,
        operator_gt
    };
    typedef controlled_vars::limited_auto_init<dependency_operator_t, operator_any, operator_gt, operator_any> limited_dependency_operator_t;
    struct DEBIAN_PACKAGE_EXPORT dependency_t
    {
        std::string operator_to_string() const;
        std::string to_string(bool remove_arch = true) const;

        std::string                     f_name;
        std::string                     f_version;
        limited_dependency_operator_t   f_operator;
        controlled_vars::zbool_t        f_or;
        controlled_vars::zbool_t        f_not_arch;
        std::vector<std::string>        f_architectures;
    };

    dependencies(const std::string& dependency_field);

    int size() const;
    const dependency_t& get_dependency(int idx);
    void delete_dependency(int idx);

    std::string to_string(const std::string& architecture, bool remove_arch = true) const;
    static bool is_architecture_valid(const std::string& architecture);
    static bool match_architectures(const std::string& architecture, const std::string& pattern, const bool ignore_vendor_field = false);

private:
    typedef std::vector<dependency_t>   dependency_vector_t;

    dependency_vector_t     f_dependencies;
};


}       // namespace wpkg_dependencies

#endif
//#ifndef WPKG_DEPENDENCIES_H
// vim: ts=4 sw=4 et
