/*    wpkg_filename.h -- handle file names and low level disk access
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
 * \brief Handle a filename.
 *
 * This header defines the filename handler class. This class is used to
 * canonicalize filenames and allow use to access the different parts of
 * a path, handle file names of any size, and execute some basic operating
 * system functions such as rename, remove, stat, etc.
 */
#ifndef WPKG_FILENAME_H
#define WPKG_FILENAME_H

#include    "controlled_vars/controlled_vars_auto_init.h"
#include    "controlled_vars/controlled_vars_limited_auto_init.h"
#include    "libdebpackages/debian_export.h"
#include    <map>
#include    <memory>
#include    <stdexcept>
#include    <vector>

namespace wpkg_filename
{

// generic memfile exception
class wpkg_filename_exception : public std::runtime_error
{
public:
    wpkg_filename_exception(const std::string& msg) : runtime_error(msg) {}
};

// problem with compatibility
class wpkg_filename_exception_compatibility : public wpkg_filename_exception
{
public:
    wpkg_filename_exception_compatibility(const std::string& msg) : wpkg_filename_exception(msg) {}
};

// problem with I/O
class wpkg_filename_exception_io : public wpkg_filename_exception
{
public:
    wpkg_filename_exception_io(const std::string& msg) : wpkg_filename_exception(msg) {}
};

// invalid parameter
class wpkg_filename_exception_parameter : public wpkg_filename_exception
{
public:
    wpkg_filename_exception_parameter(const std::string& msg) : wpkg_filename_exception(msg) {}
};



class DEBIAN_PACKAGE_EXPORT uri_filename
{
public:
    enum interactive_mode_t
    {
        wpkgar_interactive_mode_no_interactions,
        wpkgar_interactive_mode_console,
        wpkgar_interactive_mode_gui
    };

    // replace struct stat
    class DEBIAN_PACKAGE_EXPORT file_stat
    {
    public:
        bool        is_valid() const;
        uint64_t    get_dev() const;
        uint64_t    get_inode() const;
        uint32_t    get_mode() const;
        bool        is_dir() const;
        bool        is_reg() const;
        uint64_t    get_nlink() const;
        uint32_t    get_uid() const;
        uint32_t    get_gid() const;
        uint64_t    get_rdev() const;
        int64_t     get_size() const;
        time_t      get_atime() const;
        uint64_t    get_atime_nano() const;
        double      get_atime_dbl() const;
        time_t      get_mtime() const;
        uint64_t    get_mtime_nano() const;
        double      get_mtime_dbl() const;
        time_t      get_ctime() const;
        uint64_t    get_ctime_nano() const;
        double      get_ctime_dbl() const;

        void        set_valid(bool valid = true);
        void        reset();
        void        set_dev(uint64_t device);
        void        set_inode(uint64_t inode);
        void        set_mode(uint32_t mode);
        void        set_nlink(uint64_t nlink);
        void        set_uid(int32_t uid);
        void        set_gid(int32_t gid);
        void        set_rdev(uint64_t rdev);
        void        set_size(int64_t size);
        void        set_atime(int64_t unix_time, int64_t nano = 0);
        void        set_atime(double unix_time);
        void        set_mtime(int64_t unix_time, int64_t nano = 0);
        void        set_mtime(double unix_time);
        void        set_ctime(int64_t unix_time, int64_t nano = 0);
        void        set_ctime(double unix_time);

    private:
        // the sizes are taken from Linux 64bit (since it will work in
        // 32 bits too) and Windows 64bit.
        controlled_vars::fbool_t        f_valid;
        controlled_vars::zuint64_t      f_dev;  // device where the file is defined
        controlled_vars::zuint64_t      f_inode;
        controlled_vars::zuint32_t      f_mode;
        controlled_vars::zuint64_t      f_nlink;
        controlled_vars::zuint32_t      f_uid;
        controlled_vars::zuint32_t      f_gid;
        controlled_vars::zuint64_t      f_rdev; // this special file device minor.major
        controlled_vars::zint64_t       f_size;
        controlled_vars::zint64_t       f_atime;
        controlled_vars::zuint64_t      f_atime_nano;
        controlled_vars::zint64_t       f_mtime;
        controlled_vars::zuint64_t      f_mtime_nano;
        controlled_vars::zint64_t       f_ctime;
        controlled_vars::zuint64_t      f_ctime_nano;
    };

    static const char * const uri_type_undefined;
    static const char * const uri_type_direct;
    static const char * const uri_type_unc;
    static const char * const uri_scheme_file;
    static const char * const uri_scheme_http;
    static const char * const uri_scheme_https;
    static const char * const uri_scheme_smb;
    static const char * const uri_scheme_smbs;
    static const char         uri_no_msdos_drive;

    // note: uri_no_msdos_drive is '\0'
    typedef controlled_vars::zchar_t drive_t;
    typedef std::vector<std::string> path_parts_t;
    typedef std::map<std::string, std::string> query_variables_t;

#ifdef MO_WINDOWS
    typedef wchar_t             os_char_t;
    typedef std::wstring        os_string_t;
#else
    typedef char                os_char_t;
    typedef std::string         os_string_t;
#endif
    class DEBIAN_PACKAGE_EXPORT os_filename_t
    {
    public:
        enum filename_format_t
        {
            filename_format_undefined,
            filename_format_utf8,
            filename_format_utf16,
            filename_format_both
        };
        typedef controlled_vars::limited_auto_init<filename_format_t, filename_format_undefined, filename_format_both, filename_format_undefined>  safe_filename_format_t;

        os_filename_t();
        os_filename_t(const std::string& filename);
        os_filename_t(const std::wstring& filename);

        void reset(const std::string& filename);
        void reset(const std::wstring& filename);

        std::string get_utf8() const;
        std::wstring get_utf16() const;
        os_string_t get_os_string() const;

    private:
        mutable safe_filename_format_t  f_format;
        mutable std::string             f_utf8_filename;
        mutable std::wstring            f_utf16_filename;
    };

                                uri_filename(const char *filename = NULL);
                                uri_filename(const std::string& filename);

    void                        set_filename(std::string filename);
    void                        clear();
    void                        clear_cache();

    std::string                 original_filename() const;
    std::string                 path_type() const;
    std::string                 path_scheme() const;
    std::string                 drive_subst(drive_t drive, bool for_absolute_path) const;
    std::string                 path_only(bool with_drive = true) const;
    std::string                 full_path(bool replace_slashes = false) const;
    int                         segment_size() const;
    std::string                 segment(int idx) const;
    std::string                 dirname(bool with_drive = true) const;
    std::string                 basename(bool last_extension_only = false) const;
    std::string                 extension() const;
    std::string                 previous_extension() const;
    char                        msdos_drive() const;
    std::string                 get_username() const;
    std::string                 get_password() const;
    std::string                 get_domain() const;
    std::string                 get_port() const;
    std::string                 get_share() const;
    bool                        get_decode() const;
    std::string                 get_anchor() const;
    std::string                 query_variable(const std::string& name) const;
    query_variables_t           all_query_variables() const;

    bool                        empty() const;
    bool                        exists() const;
    bool                        is_reg() const;
    bool                        is_dir() const;
    bool                        is_deb() const;
    bool                        is_valid() const;
    bool                        is_direct() const;
    bool                        is_absolute() const;
    static bool                 is_valid_windows_part(const std::string& path_part);
    bool                        glob(const char *pattern) const;

    uri_filename                append_path(const std::string& path) const;
    uri_filename                append_path(int value) const;
    uri_filename                append_child(const std::string& child) const;
    uri_filename                append_safe_child(const uri_filename& child) const;
    uri_filename                remove_common_segments(const uri_filename& common_segments) const;
    uri_filename                relative_path() const;

    os_filename_t               os_filename() const;
    uri_filename                os_real_path() const;
    int                         os_stat(file_stat& st) const;
    int                         os_lstat(file_stat& st) const;
    void                        os_mkdir_p(int mode = 0755) const;
    bool                        os_unlink() const;
    bool                        os_unlink_rf(bool dryrun = false) const;
    void                        os_symlink(const uri_filename& destination) const;
    bool                        os_rename(const uri_filename& destination, bool ignore_errors = false) const;

    bool                        operator == (const uri_filename& rhs) const;
    bool                        operator != (const uri_filename& rhs) const;
    bool                        operator <  (const uri_filename& rhs) const;
    bool                        operator <= (const uri_filename& rhs) const;
    bool                        operator >  (const uri_filename& rhs) const;
    bool                        operator >= (const uri_filename& rhs) const;

    static void                 set_interactive(interactive_mode_t mode);
    static interactive_mode_t   get_interactive();
    static uri_filename         tmpdir(const std::string& sub_directory, bool create = true);
    static uri_filename         get_cwd();

private:
    bool                        glob(const char *filename, const char *pattern) const;

    std::string                 f_original;
    std::string                 f_type; // type of URI
    std::string                 f_scheme;
    controlled_vars::fbool_t    f_decode;
    std::string                 f_username;
    std::string                 f_password;
    std::string                 f_domain;
    std::string                 f_port;
    std::string                 f_share; // netbios share
    controlled_vars::fbool_t    f_is_deb;
    drive_t                     f_drive;
    path_parts_t                f_segments;
    std::string                 f_dirname;
    std::string                 f_path;
    std::string                 f_basename; // last part in path (duplicate from path)
    std::string                 f_extension;
    std::string                 f_previous_extension;
    std::string                 f_anchor;
    query_variables_t           f_query_variables;

    // cache
    mutable file_stat           f_stat; // if st_size == -1 then it is still undefined
    mutable std::string         f_real_path; // if empty() then it is still undefined
};


typedef std::vector<wpkg_filename::uri_filename>        filename_list_t;


class DEBIAN_PACKAGE_EXPORT temporary_uri_filename : public uri_filename
{
public:
                                ~temporary_uri_filename();

    //temporary_uri_filename&     operator = (const temporary_uri_filename& rhs);
    temporary_uri_filename&     operator = (const uri_filename& rhs);

    static void                 set_tmpdir(const std::string& tmpdir);
    static void                 keep_files(bool keep = true);
};


class DEBIAN_PACKAGE_EXPORT os_dir_impl;
class DEBIAN_PACKAGE_EXPORT os_dir
{
public:
                                os_dir(const wpkg_filename::uri_filename& dir_path);
                                ~os_dir();

    wpkg_filename::uri_filename path() const;
    void                        close_dir();
    bool                        read(wpkg_filename::uri_filename& filename);
    std::string                 read_all(const char *pattern);

private:
    std::shared_ptr<os_dir_impl>    f_impl;
};


} // namespace wpkg_filename
#endif
//#ifndef WPKG_FILENAME_H
// vim: ts=4 sw=4 et
