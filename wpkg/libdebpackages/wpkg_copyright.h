/*    wpkg_copyright.h -- declaration of the copyright file manager
 *    Copyright (C) 2013-2015  Made to Order Software Corporation
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
 * \brief Handling of the copyright file format.
 *
 * This file is a specialization of the wpkg_field class to handle fields as
 * defined in a copyright file. The copyright files define the license of
 * the project, each directory, and each file.
 *
 * The basic file format is similar to a control file with different entries
 * separated by empty lines.
 */
#ifndef WPKG_COPYRIGHT_H
#define WPKG_COPYRIGHT_H
#include    "wpkg_field.h"
#include    "wpkg_dependencies.h"


namespace wpkg_copyright
{


// helper macros to define fields without having to declare hundreds of functions

#define COPYRIGHT_FILE_FIELD_FACTORY_CLASS(field_name) \
    class field_##field_name##_factory_t : public copyright_field_factory_t { \
    public: field_##field_name##_factory_t(); \
    static const char *canonicalized_name(); \
    virtual char const *name() const; \
    virtual char const *help() const; \
    virtual std::shared_ptr<wpkg_field::field_file::field_t> create(const field_file& file, const std::string& fullname, const std::string& value) const; };

#define COPYRIGHT_FILE_FIELD_CLASS_START(field_name) \
    class field_##field_name##_t : public copyright_field_t { \
    public: field_##field_name##_t(const field_file& file, const std::string& name, const std::string& value); \
    virtual void verify_value() const;

#define COPYRIGHT_FILE_FIELD_CLASS_END(field_name) \
    };

#define COPYRIGHT_FILE_FIELD_CLASS(field_name) \
    COPYRIGHT_FILE_FIELD_CLASS_START(field_name) \
    COPYRIGHT_FILE_FIELD_CLASS_END(field_name)

#define COPYRIGHT_FILE_FIELD_CONSTRUCTOR(field_name, parent) \
    copyright_file::field_##field_name##_t::field_##field_name##_t(const field_file& file, const std::string& name, const std::string& value) \
    : parent##_field_t(file, name, value) {}

#define COPYRIGHT_FILE_FIELD_FACTORY(field_name, canonicalized, help_string) \
    copyright_file::field_##field_name##_factory_t::field_##field_name##_factory_t() { register_field(this); } \
    const char *copyright_file::field_##field_name##_factory_t::canonicalized_name() { return canonicalized; } \
    char const *copyright_file::field_##field_name##_factory_t::name() const { return canonicalized; } \
    char const *copyright_file::field_##field_name##_factory_t::help() const { return help_string; } \
    std::shared_ptr<wpkg_field::field_file::field_t> copyright_file::field_##field_name##_factory_t::create(const field_file& file, const std::string& fullname, const std::string& value) const \
    { return std::shared_ptr<field_t>(static_cast<field_t *>(new field_##field_name##_t(file, fullname, value))); } \
    namespace { copyright_file::field_##field_name##_factory_t g_field_##field_name; }



class DEBIAN_PACKAGE_EXPORT copyright_file : public wpkg_field::field_file
{
public:
    // use this one when loading binary packages for installation
    class DEBIAN_PACKAGE_EXPORT copyright_file_state_t : public field_file_state_t
    {
    public:
        virtual bool accept_sub_packages() const;
    };

    // copyright field description; this intermediate class is used to
    // ensure grouping of all the field descriptions in a copyright
    // file
    class DEBIAN_PACKAGE_EXPORT copyright_field_factory_t : public field_factory_t
    {
    public:
        static void register_field(copyright_field_factory_t *field_factory);
    };

    typedef std::map<const case_insensitive::case_insensitive_string,
         const copyright_field_factory_t *,
         std::less<const case_insensitive::case_insensitive_string> > field_factory_map_t;

    // first we define all the fields understood in copyright files

    // Common class from which all the other fields derive from so they
    // gain access to a set of common functions that are useful in this
    // environment
    class DEBIAN_PACKAGE_EXPORT copyright_field_t : public field_t
    {
    public:
                        copyright_field_t(const field_file& file, const std::string& name, const std::string& value);

        void            verify_emails() const;
        void            verify_uri() const;
    };

    // Comment
    COPYRIGHT_FILE_FIELD_FACTORY_CLASS(comment)
    COPYRIGHT_FILE_FIELD_CLASS(comment)

    // Copyright
    COPYRIGHT_FILE_FIELD_FACTORY_CLASS(copyright)
    COPYRIGHT_FILE_FIELD_CLASS(copyright)

    // Disclaimer
    COPYRIGHT_FILE_FIELD_FACTORY_CLASS(disclaimer)
    COPYRIGHT_FILE_FIELD_CLASS(disclaimer)

    // Files
    COPYRIGHT_FILE_FIELD_FACTORY_CLASS(files)
    COPYRIGHT_FILE_FIELD_CLASS(files)

    // Format
    COPYRIGHT_FILE_FIELD_FACTORY_CLASS(format)
    COPYRIGHT_FILE_FIELD_CLASS(format)

    // License
    COPYRIGHT_FILE_FIELD_FACTORY_CLASS(license)
    COPYRIGHT_FILE_FIELD_CLASS(license)

    // Source
    COPYRIGHT_FILE_FIELD_FACTORY_CLASS(source)
    COPYRIGHT_FILE_FIELD_CLASS(source)

    // Upstream-Name
    COPYRIGHT_FILE_FIELD_FACTORY_CLASS(upstreamname)
    COPYRIGHT_FILE_FIELD_CLASS(upstreamname)

    // Upstream-Contact
    COPYRIGHT_FILE_FIELD_FACTORY_CLASS(upstreamcontact)
    COPYRIGHT_FILE_FIELD_CLASS(upstreamcontact)



    typedef std::vector<std::string>    field_name_list_t;

    copyright_file(const std::shared_ptr<field_file_state_t> state);

    static const field_factory_map_t *field_factory_map();

protected:
    virtual std::shared_ptr<field_t> field_factory(const case_insensitive::case_insensitive_string& name, const std::string& value) const;
    virtual void verify_file() const;

private:
    friend class field_standardsversion_t;

    // avoid copies
    copyright_file(const copyright_file& rhs);
    copyright_file& operator = (const copyright_file& rhs);
};


class DEBIAN_PACKAGE_EXPORT header_copyright_file : public copyright_file
{
public:
    header_copyright_file(const std::shared_ptr<field_file_state_t> state);

protected:
    virtual void verify_file() const;
};


// files or stand-alone license
class DEBIAN_PACKAGE_EXPORT files_copyright_file : public copyright_file
{
public:
    files_copyright_file(const std::shared_ptr<field_file_state_t> state);

    bool is_license() const;

protected:
    virtual void verify_file() const;
};


class DEBIAN_PACKAGE_EXPORT copyright_info
{
public:
    copyright_info();

    bool read(const memfile::memory_file *input);

    header_copyright_file *get_header() const;
    int get_files_count() const;
    std::shared_ptr<copyright_file> get_file(int idx);
    int get_licenses_count() const;
    std::shared_ptr<copyright_file> get_license(int idx);

private:
    typedef std::shared_ptr<copyright_file>     copyright_file_ptr_t;
    typedef std::vector<copyright_file_ptr_t>   copyright_file_list_t;

    const std::shared_ptr<copyright_file::copyright_file_state_t>     f_state;
    header_copyright_file                       f_copyright_header;
    copyright_file_list_t                       f_copyright_files;
    copyright_file_list_t                       f_copyright_licenses;
};




}       // namespace wpkg_copyright

#endif
//#ifndef WPKG_COPYRIGHT_H
// vim: ts=4 sw=4 et
