/*    wpkg_control.h -- declaration of the control file manager
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
 * \brief Handling of control files.
 *
 * This implementation is a specialization of the wpkg_field file format which
 * handles fields as in a Debian control file.
 *
 * The format supports plain control and control.info files. We do not yet
 * support a source control file (i.e. a file with multiple entries within
 * one file each separated by an empty line.)
 *
 * The list of fields can be obtained from wpkg with the help command:
 *
 * \code
 * wpkg --help field
 * wpkg --help field <field name>
 * \endcode
 *
 * The --verbose option shows the entire help of fields.
 */
#ifndef WPKG_CONTROL_H
#define WPKG_CONTROL_H
#include    "wpkg_field.h"
#include    "wpkg_dependencies.h"
#include    "compatibility.h"


namespace wpkg_control
{

class wpkg_control_exception : public std::runtime_error
{
public:
    wpkg_control_exception(const std::string& what_msg) : runtime_error(what_msg) {}
};

class wpkg_control_exception_invalid : public wpkg_control_exception
{
public:
    wpkg_control_exception_invalid(const std::string& what_msg) : wpkg_control_exception(what_msg) {}
};



class DEBIAN_PACKAGE_EXPORT standards_version
{
public:
    enum standards_version_number_t
    {
        standards_major_version,
        standards_minor_version,
        standards_major_patch_level,
        standards_minor_patch_level,
        standards_version_max
    };

    void set_version(const std::string& version);

    bool is_defined() const;
    uint32_t get_version(standards_version_number_t n) const;

private:
    bool parse_version(const std::string& version);

    controlled_vars::fbool_t        f_defined;
    controlled_vars::zuint32_t      f_version[standards_version_max];
};


class DEBIAN_PACKAGE_EXPORT file_item
{
public:
    enum format_t
    {
        format_unknown = -1,
        format_not_specified,
        format_list,               // <filename>
        format_modelist,           // <mode> <filename>
        format_conffiles,          // <filename> <md5sum>
        format_md5sum,             // <md5sum> <size> <filename>
        format_sha1,               // <sha1> <size> <filename>
        format_sha256,             // <sha256> <size> <filename>
        format_longlist,           // <mode> <size> <md5sum> <filename>
        format_metadata,           // <mode> <user/uid> <group/gid> <size>|<major,minor> <mtime> <filename>

        format_choose_best
    };
    typedef controlled_vars::limited_auto_init<format_t, format_unknown, format_choose_best, format_unknown> limited_format_t;

    static const int undefined_uid = -1;
    static const int undefined_gid = -1;
    static const int undefined_device = -1;
    typedef controlled_vars::auto_init<int, undefined_device> safe_dev_t;

    void set_format(format_t format);
    void set_filename(const std::string& filename);
    void set_mode(mode_t mode);
    void set_mode(const std::string& mode);
    void set_user(const std::string& user);
    void set_uid(int uid);
    void set_uid(const std::string& uid);
    void set_user_uid(const std::string& user_uid);
    void set_group(const std::string& group);
    void set_gid(int gid);
    void set_gid(const std::string& gid);
    void set_group_gid(const std::string& group_gid);
    void set_mtime(time_t time);
    void set_mtime(const std::string& date);
    void set_dev(int dev_major, int dev_minor);
    void set_dev(const std::string& dev);
    void set_size(size_t size);
    void set_size(const std::string& size);
    void set_checksum(const std::string& checksum);

    format_t get_format() const;
    format_t best_format(format_t b) const;
    const std::string& get_filename() const;
    mode_t get_mode() const;
    std::string get_mode_string() const;
    size_t get_size() const;
    const std::string& get_checksum() const;

    std::string to_string(format_t format = format_unknown) const;

private:
    format_t determine_format() const;

    limited_format_t                f_format;
    std::string                     f_filename;
    controlled_vars::zuint16_t      f_mode;
    std::string                     f_user;
    controlled_vars::zint32_t       f_uid;
    std::string                     f_group;
    controlled_vars::zint32_t       f_gid;
    controlled_vars::zuint32_t      f_size;
    safe_dev_t                      f_dev_major;
    safe_dev_t                      f_dev_minor;
    controlled_vars::ztime_t        f_mtime;
    std::string                     f_checksum;
};


class DEBIAN_PACKAGE_EXPORT file_list_t : public std::vector<file_item>
{
public:
    file_list_t(const std::string& name);

    void set(const std::string& fields);
    std::string to_string(file_item::format_t format = file_item::format_choose_best, bool print_format = true) const;

private:
    const std::string       f_name;
};




// helper macros to define fields without having to declare hundreds of functions

#define CONTROL_FILE_FIELD_FACTORY_CLASS(field_name) \
    class DEBIAN_PACKAGE_EXPORT field_##field_name##_factory_t : public control_field_factory_t { \
    public: field_##field_name##_factory_t(); \
    static const char *canonicalized_name(); \
    virtual char const *name() const; \
    virtual char const *help() const; \
    virtual std::shared_ptr<wpkg_field::field_file::field_t> create(const field_file& file, const std::string& fullname, const std::string& value) const; };

#define CONTROL_FILE_FIELD_CLASS_START(field_name) \
    class DEBIAN_PACKAGE_EXPORT field_##field_name##_t : public control_field_t { \
    public: field_##field_name##_t(const field_file& file, const std::string& name, const std::string& value); \
    virtual void verify_value() const;

#define CONTROL_FILE_FIELD_CLASS_END(field_name) \
    };

#define CONTROL_FILE_FIELD_CLASS(field_name) \
    CONTROL_FILE_FIELD_CLASS_START(field_name) \
    CONTROL_FILE_FIELD_CLASS_END(field_name)

#define CONTROL_FILE_DEPENDENCY_FIELD_CLASS(field_name) \
    class DEBIAN_PACKAGE_EXPORT field_##field_name##_t : public dependency_field_t { \
    public: field_##field_name##_t(const field_file& file, const std::string& name, const std::string& value); };

#define CONTROL_FILE_FIELD_CONSTRUCTOR(field_name, parent) \
    control_file::field_##field_name##_t::field_##field_name##_t(const field_file& file, const std::string& name, const std::string& value) \
    : parent##_field_t(file, name, value) {}

#define CONTROL_FILE_FIELD_FACTORY(field_name, canonicalized, help_string) \
    control_file::field_##field_name##_factory_t::field_##field_name##_factory_t() { register_field(this); } \
    const char *control_file::field_##field_name##_factory_t::canonicalized_name() { return canonicalized; } \
    char const *control_file::field_##field_name##_factory_t::name() const { return canonicalized; } \
    char const *control_file::field_##field_name##_factory_t::help() const { return help_string; } \
    std::shared_ptr<wpkg_field::field_file::field_t> control_file::field_##field_name##_factory_t::create(const field_file& file, const std::string& fullname, const std::string& value) const \
    { return std::shared_ptr<field_t>(static_cast<field_t *>(new field_##field_name##_t(file, fullname, value))); } \
    namespace { control_file::field_##field_name##_factory_t g_field_##field_name; }



class DEBIAN_PACKAGE_EXPORT control_file : public wpkg_field::field_file
{
public:
    // use this one when loading binary packages for installation
    class DEBIAN_PACKAGE_EXPORT control_file_state_t : public field_file_state_t
    {
    public:
        virtual bool reading_contents() const;
        virtual bool prevent_source() const;
    };

    // use this one when loading control files to build binary packages
    class DEBIAN_PACKAGE_EXPORT build_control_file_state_t : public control_file_state_t
    {
    public:
        virtual bool allow_transformations() const;
        virtual bool prevent_source() const;
    };

    // use this one when loading binary packages for perusal
    class DEBIAN_PACKAGE_EXPORT contents_control_file_state_t : public control_file_state_t
    {
    public:
        virtual bool reading_contents() const;
    };

    // control field description; this intermediate class is used to
    // ensure grouping of all the field descriptions in a control
    // file
    class DEBIAN_PACKAGE_EXPORT control_field_factory_t : public field_factory_t
    {
    public:
        static void register_field(control_field_factory_t *field_factory);
    };

    typedef std::map<const case_insensitive::case_insensitive_string,
         const wpkg_control::control_file::control_field_factory_t *,
         std::less<const case_insensitive::case_insensitive_string> > field_factory_map_t;

    // first we define all the fields as we specialize many of them
    // in control files; see the field_factory() for details
    // the fields are defined in alphabetical order since there are many

    // Common class from which all the other fields derive from so they
    // gain access to a set of common functions that are useful in this
    // environment
    class DEBIAN_PACKAGE_EXPORT control_field_t : public field_t
    {
    public:
                        control_field_t(const field_file& file, const std::string& name, const std::string& value);

        void            verify_date() const;
        void            verify_dependencies() const;
        void            verify_emails() const;
        void            verify_file() const;
        void            verify_no_sub_package_name() const;
        void            verify_uri() const;
        void            verify_version() const;
    };

    // There are many dependency fields so we have a specific entry
    // that all those dependency fields can derive from
    class DEBIAN_PACKAGE_EXPORT dependency_field_t : public control_field_t
    {
    public:
                        dependency_field_t(const field_file& file, const std::string& name, const std::string& value);

        virtual void    verify_value() const;
    };

    struct DEBIAN_PACKAGE_EXPORT list_of_terms_t
    {
        const char *        f_term;
        const char *        f_help;
    };

	static const list_of_terms_t *find_term(const list_of_terms_t *list, const std::string& term, const bool case_insensitive = true);

    // Architecture
    CONTROL_FILE_FIELD_FACTORY_CLASS(architecture)
    CONTROL_FILE_FIELD_CLASS(architecture)

    // Breaks
    CONTROL_FILE_FIELD_FACTORY_CLASS(breaks)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(breaks)

    // Bugs
    CONTROL_FILE_FIELD_FACTORY_CLASS(bugs)
    CONTROL_FILE_FIELD_CLASS(bugs)

    // Build-Conflicts
    CONTROL_FILE_FIELD_FACTORY_CLASS(buildconflicts)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(buildconflicts)

    // Build-Conflicts-Arch
    CONTROL_FILE_FIELD_FACTORY_CLASS(buildconflictsarch)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(buildconflictsarch)

    // Build-Conflicts-Indep
    CONTROL_FILE_FIELD_FACTORY_CLASS(buildconflictsindep)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(buildconflictsindep)

    // Build-Depends
    CONTROL_FILE_FIELD_FACTORY_CLASS(builddepends)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(builddepends)

    // Build-Depends-Arch
    CONTROL_FILE_FIELD_FACTORY_CLASS(builddependsarch)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(builddependsarch)

    // Build-Depends-Indep
    CONTROL_FILE_FIELD_FACTORY_CLASS(builddependsindep)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(builddependsindep)

    // Build-Number
    CONTROL_FILE_FIELD_FACTORY_CLASS(buildnumber)
    CONTROL_FILE_FIELD_CLASS(buildnumber)

    // Built-Using
    CONTROL_FILE_FIELD_FACTORY_CLASS(builtusing)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(builtusing)

    // Changed-By
    CONTROL_FILE_FIELD_FACTORY_CLASS(changedby)
    CONTROL_FILE_FIELD_CLASS(changedby)

    // Changes
    CONTROL_FILE_FIELD_FACTORY_CLASS(changes)
    CONTROL_FILE_FIELD_CLASS(changes)

    // Changes-Date
    CONTROL_FILE_FIELD_FACTORY_CLASS(changesdate)
    CONTROL_FILE_FIELD_CLASS(changesdate)

    // Checksums-Sha1
    CONTROL_FILE_FIELD_FACTORY_CLASS(checksumssha1)
    CONTROL_FILE_FIELD_CLASS(checksumssha1)

    // Checksums-Sha256
    CONTROL_FILE_FIELD_FACTORY_CLASS(checksumssha256)
    CONTROL_FILE_FIELD_CLASS(checksumssha256)

    // Component
    CONTROL_FILE_FIELD_FACTORY_CLASS(component)
    CONTROL_FILE_FIELD_CLASS(component)

    // Conf-Files
    CONTROL_FILE_FIELD_FACTORY_CLASS(conffiles)
    CONTROL_FILE_FIELD_CLASS(conffiles)

    // Conflicts
    CONTROL_FILE_FIELD_FACTORY_CLASS(conflicts)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(conflicts)

    // Date
    CONTROL_FILE_FIELD_FACTORY_CLASS(date)
    CONTROL_FILE_FIELD_CLASS(date)

    // Depends
    CONTROL_FILE_FIELD_FACTORY_CLASS(depends)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(depends)

    // Description
    CONTROL_FILE_FIELD_FACTORY_CLASS(description)
    CONTROL_FILE_FIELD_CLASS(description)

    // Distribution
    CONTROL_FILE_FIELD_FACTORY_CLASS(distribution)
    CONTROL_FILE_FIELD_CLASS(distribution)

    // DM-Upload-Allowed
    CONTROL_FILE_FIELD_FACTORY_CLASS(dmuploadallowed)
    CONTROL_FILE_FIELD_CLASS(dmuploadallowed)

    // Enhances
    CONTROL_FILE_FIELD_FACTORY_CLASS(enhances)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(enhances)

    // Essential
    CONTROL_FILE_FIELD_FACTORY_CLASS(essential)
    CONTROL_FILE_FIELD_CLASS(essential)

    // Files
    CONTROL_FILE_FIELD_FACTORY_CLASS(files)
    CONTROL_FILE_FIELD_CLASS(files)

    // Homepage
    CONTROL_FILE_FIELD_FACTORY_CLASS(homepage)
    CONTROL_FILE_FIELD_CLASS(homepage)

    // Install-Prefix
    CONTROL_FILE_FIELD_FACTORY_CLASS(installprefix)
    CONTROL_FILE_FIELD_CLASS(installprefix)

    // Maintainer
    CONTROL_FILE_FIELD_FACTORY_CLASS(maintainer)
    CONTROL_FILE_FIELD_CLASS(maintainer)

    // Minimum-Upgradable-Version
    CONTROL_FILE_FIELD_FACTORY_CLASS(minimumupgradableversion)
    CONTROL_FILE_FIELD_CLASS_START(minimumupgradableversion)
        virtual void set_value(const std::string& value);
    CONTROL_FILE_FIELD_CLASS_END(minimumupgradableversion)

    // Origin
    CONTROL_FILE_FIELD_FACTORY_CLASS(origin)
    CONTROL_FILE_FIELD_CLASS(origin)

    // Package
    CONTROL_FILE_FIELD_FACTORY_CLASS(package)
    CONTROL_FILE_FIELD_CLASS(package)

    // Packager-Version
    CONTROL_FILE_FIELD_FACTORY_CLASS(packagerversion)
    CONTROL_FILE_FIELD_CLASS(packagerversion)

    // Pre-Depends
    CONTROL_FILE_FIELD_FACTORY_CLASS(predepends)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(predepends)

    // Priority
    CONTROL_FILE_FIELD_FACTORY_CLASS(priority)
    CONTROL_FILE_FIELD_CLASS_START(priority)
        static const list_of_terms_t *list();
        static bool is_valid(const std::string& priority);
    CONTROL_FILE_FIELD_CLASS_END(priority)

    // Provides
    CONTROL_FILE_FIELD_FACTORY_CLASS(provides)
    CONTROL_FILE_FIELD_CLASS(provides)

    // Recommends
    CONTROL_FILE_FIELD_FACTORY_CLASS(recommends)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(recommends)

    // Replaces
    CONTROL_FILE_FIELD_FACTORY_CLASS(replaces)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(replaces)

    // Section
    CONTROL_FILE_FIELD_FACTORY_CLASS(section)
    CONTROL_FILE_FIELD_CLASS_START(section)
        static const list_of_terms_t *list();
        static bool is_valid(const std::string& value, std::string& section, std::string& area);
    CONTROL_FILE_FIELD_CLASS_END(section)

    // Source
    CONTROL_FILE_FIELD_FACTORY_CLASS(source)
    CONTROL_FILE_FIELD_CLASS(source)

    // Standards-Version
    CONTROL_FILE_FIELD_FACTORY_CLASS(standardsversion)
    CONTROL_FILE_FIELD_CLASS(standardsversion)

    // Sub-Packages
    CONTROL_FILE_FIELD_FACTORY_CLASS(subpackages)
    CONTROL_FILE_FIELD_CLASS(subpackages)

    // Suggests
    CONTROL_FILE_FIELD_FACTORY_CLASS(suggests)
    CONTROL_FILE_DEPENDENCY_FIELD_CLASS(suggests)

    // Uploaders
    CONTROL_FILE_FIELD_FACTORY_CLASS(uploaders)
    CONTROL_FILE_FIELD_CLASS(uploaders)

    // Urgency
    CONTROL_FILE_FIELD_FACTORY_CLASS(urgency)
    CONTROL_FILE_FIELD_CLASS_START(urgency)
        static const list_of_terms_t *list();
        static bool is_valid(const std::string& value, std::string& urgency, std::string& comment);
    CONTROL_FILE_FIELD_CLASS_END(urgency)

    // Vcs-Browser
    CONTROL_FILE_FIELD_FACTORY_CLASS(vcsbrowser)
    CONTROL_FILE_FIELD_CLASS(vcsbrowser)

    // Version
    CONTROL_FILE_FIELD_FACTORY_CLASS(version)
    CONTROL_FILE_FIELD_CLASS_START(version)
        virtual void set_value(const std::string& value);
    CONTROL_FILE_FIELD_CLASS_END(version)

    // X-PrimarySection
    CONTROL_FILE_FIELD_FACTORY_CLASS(xprimarysection)
    CONTROL_FILE_FIELD_CLASS(xprimarysection)

    // X-SecondarySection
    CONTROL_FILE_FIELD_FACTORY_CLASS(xsecondarysection)
    CONTROL_FILE_FIELD_CLASS(xsecondarysection)

    // X-Selection
    CONTROL_FILE_FIELD_FACTORY_CLASS(xselection)
    CONTROL_FILE_FIELD_CLASS_START(xselection)
        enum selection_t
        {
            selection_unknown = -1,
            selection_auto = 0,
            selection_normal, // == manual
            selection_hold,
            selection_reject
        };
        static const list_of_terms_t *list();
        static bool is_valid(const std::string& selection);
        static selection_t validate_selection(const std::string& selection);
    CONTROL_FILE_FIELD_CLASS_END(xselection)

    // X-Status
    CONTROL_FILE_FIELD_FACTORY_CLASS(xstatus)
    CONTROL_FILE_FIELD_CLASS(xstatus)



    control_file(const std::shared_ptr<control_file_state_t> state);

    const standards_version& get_standards_version() const;

    // some specialized fields handling
    file_list_t get_files(const std::string& name) const;
    wpkg_dependencies::dependencies get_dependencies(const std::string& name) const;
    void rewrite_dependencies();
    std::string get_description(const std::string& name, std::string& long_description) const;

    static const field_factory_map_t *field_factory_map();

protected:
    virtual std::shared_ptr<field_t> field_factory(const case_insensitive::case_insensitive_string& name, const std::string& value) const;
    virtual void verify_file() const = 0; // force the use of specialized control files only

private:
    friend class field_standardsversion_t;

    // avoid copies
    control_file(const control_file& rhs);
    control_file& operator = (const control_file& rhs);

    static bool validate_standards_version(const std::string& version);

    standards_version       f_standards_version;
};


class DEBIAN_PACKAGE_EXPORT binary_control_file : public control_file
{
public:
    binary_control_file(const std::shared_ptr<control_file_state_t> state);

protected:
    virtual void verify_file() const;
};


class DEBIAN_PACKAGE_EXPORT status_control_file : public control_file
{
public:
    status_control_file();

protected:
    virtual void verify_file() const;
};


class DEBIAN_PACKAGE_EXPORT info_control_file : public control_file
{
public:
    info_control_file();

protected:
    virtual void verify_file() const;
};


class DEBIAN_PACKAGE_EXPORT source_control_file : public control_file
{
public:
    source_control_file();

protected:
    virtual void verify_file() const;
};


}       // namespace wpkg_control

#endif
//#ifndef WPKG_CONTROL_H
// vim: ts=4 sw=4 et
