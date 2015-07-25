/*    wpkg_filename.cpp -- handle filenames and low level disk functions
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
 * \brief Handle the name of a file.
 *
 * Although in most cases people make use of a simple string for a filename,
 * we actually have an extremely advanced class to handle filenames. There
 * are several reasons for having this class:
 *
 * \li Very often we want to canonicalize the filenames to compare them
 *     between each others.
 * \li We want to know whether a certain segment in the path is "this" or
 *     "that".
 * \li We need to extract the basename, the extension, the \em previous
 *     extension, the directory part.
 * \li For complete URI, we want to know the domain name, the username,
 *     the password, and the port when specified.
 * \li Support very long filenames transparently.
 * \li Support UTF-8 filenames, yet access the file system with \em Unicode
 *     (wchar_t *) under MS-Windows.
 *
 * This implementation offers all of that and also basic access to the
 * file systems via functions such as the exists() function which runs
 * a stat() against the file and return true if it succeeded.
 */

// WARNING: The tcp_client_server.h MUST be first to avoid tons of errors
//          under MS-Windows (which apparently has quite a few problems
//          with their headers and backward compatibility.)
#include    "libdebpackages/tcp_client_server.h"

#include    "libdebpackages/wpkg_filename.h"
#include    "libdebpackages/wpkg_util.h"
#include    "libdebpackages/case_insensitive_string.h"
#include    "libdebpackages/compatibility.h"
#include    <algorithm>
#include    <sstream>
#include    <errno.h>
#include    <time.h>
#include    <sys/types.h>
#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
#   include <winnetwk.h>
#else
#   include <unistd.h>
#   include <netdb.h>
#endif
#if defined(MO_WINDOWS)
#else
#   include <dirent.h>
#endif
#if defined(MO_CYGWIN)
#   include <netdb.h>
#endif


/** \brief Declarations and implementation of filename related functions.
 *
 * This namespace includes all the filename related functions and classes.
 *
 * This is used to handle filenames in a canonicalized manner throughout
 * the entire library and toolset.
 */
namespace wpkg_filename
{


/** \class wpkg_filename_exception
 * \brief Base exception class for all the wpkg_filename classes.
 *
 * This class is used to create all the other wpkg_filename exceptions.
 * It is the base so it can be caught to catch all the wpkg_filename
 * exceptions in one go. However, it is never itself raised.
 */


/** \class wpkg_filename_exception_compatibility
 * \brief A filename is not what was expected.
 *
 * This exception is raised if a filename is used instead of a
 * directory.
 *
 * It is also used if the temporary directory path is set for a
 * second time.
 */


/** \class wpkg_filename_exception_io
 * \brief An I/O error occured.
 *
 * This exception is raised whenever the wpkg_filename is accessing
 * an I/O function and it fails for one reason or another.
 */


/** \class wpkg_filename_exception_parameter
 * \brief A function was called with an invalid parameter.
 *
 * This exception is raised if a function is called with an invalid
 * parameter.
 */


/** \class uri_filename
 * \brief Filenames manager.
 *
 * This class manage simple and complex filenames. Filenames can include
 * absolute paths or relative paths, but we also wanted to support
 * the full URI syntax with a scheme, a colon, two slashes, an optional
 * domain name with a username, password, and port; and at the end of
 * the name, a query string and anchor.
 */


/** \class uri_filename::file_stat
 * \brief File statistics of a URI filename.
 *
 * Because the system struct stat of each operating system we support is
 * different, we decided to create one that was compatible across the
 * board and invariant so all the other functions in the library did not
 * have to deal with crazy things such as finding the time in milliseconds
 * or microseconds, etc.
 *
 * The class also makes use of functions only to deal with the data making
 * it a lot safer to maintain data coherency.
 */


/** \class uri_filename::os_filename_t
 * \brief The filename as used by the current operating system.
 *
 * This class allows for the filename to be used as per the operating
 * system. This means UTF-8 under Unices and UTF-16 under MS-Windows.
 *
 * Note that this is not used as much as expected at this point since
 * we actually reimplemented most of the operating system operations
 * locally often using a different mean to access the right function.
 *
 * The function also allows for access to the UTF-8 and UTF-16
 * specifically.
 */


/** \class temporary_uri_filename
 * \brief Implementation of a way to generate temporary file names.
 *
 * This class actually views the specified filename as a file on disk.
 * When its destructor is called, the file gets deleted. It is very
 * important to understand that because variables of this type cannot
 * safely be copied since the first one being destroyed will delete
 * the corresponding temporary file.
 *
 * In order to allow for debugging a run of wpkg and other tools, the
 * library lets you prevent the destructor from deleting all the files.
 * This is done by calling keep_files() with true. If that is done,
 * then none of the temporary files get deleted. However, the class
 * prints out the name of all the files that are kept on destruction.
 * This way the user can easily find them and review them as required.
 */


// private data
namespace
{


/** \brief The current interactive mode.
 *
 * Because it does not otherwise make sense, the interactive mode is defined
 * as a global. Changing it for any wpkg_filename changes it for the entire
 * system. Actually, in most cases the filenames passed to a higher level
 * function does not make it to the one function that would otherwise need
 * the information.
 */
uri_filename::interactive_mode_t   g_interactive = uri_filename::wpkgar_interactive_mode_no_interactions;


/** \brief Substitution information.
 *
 * This structure is used to implement the "subst" command under Unices.
 * This transforms a drive reference in a new path.
 */
struct subst_entry_t
{
    /** \brief Initialize the subst entry.
     *
     * This function resets the subst entry with a "no drive" drive
     * specification, and the root path (/) in f_path.
     *
     * The current directory is set to the empty string which means
     * that no current directory applies.
     */
    subst_entry_t()
        : f_path("/")
    {
    }

    uri_filename::drive_t       f_drive;
    std::string                 f_path;
    std::string                 f_current_directory;
};
typedef std::map<uri_filename::drive_t, subst_entry_t>   subst_list_t;
bool g_subst_initialized;
subst_list_t g_subst_list;

subst_entry_t get_subst(const uri_filename::drive_t drive)
{
    if(!g_subst_initialized)
    {
        const std::string wpkg_subst(wpkg_util::utf8_getenv("WPKG_SUBST", ""));
        if(!wpkg_subst.empty())
        {
            // format is:
            //    <drive letter>=<path>|<current directory>:<drive letter>=...
            // we use the | and : as separator because these characters cannot
            // appear in valid FAT/NTFS filenames.
            const char *s(wpkg_subst.c_str());
            while(*s != 0)
            {
                subst_entry_t e;
                e.f_drive = *s++;
                if(e.f_drive >= 'a' && e.f_drive <= 'z')
                {
                    // make drive a capital
                    e.f_drive &= ~0x20;
                }
                else if(e.f_drive < 'A' || e.f_drive > 'Z')
                {
                    throw std::runtime_error("a drive letter in your WPKG_SUBST is not valid (it must be a letter between A and Z)");
                }
                if(*s != '=')
                {
                    throw std::runtime_error("drive letters in your WPKG_SUBST must be followed by an equal sign");
                }
                ++s;
                for(; *s != '\0' && *s != '|' && *s != ':'; ++s)
                {
                    switch(*s)
                    {
                    case '*':
                    case '?':
                    case '"':
                    case '<':
                    case '>':
                        throw std::runtime_error("WPKG_SUBST substitute path includes an invalid character (* ? \" < >)");

                    case '/':
                    case '\\':
                        // avoid "//" in the path
                        if(!e.f_path.empty() && e.f_path[e.f_path.length() - 1] != '/')
                        {
                            e.f_path += '/';
                        }
                        break;

                    default:
                        e.f_path += *s;
                        break;

                    }
                }
                if(*s == '|')
                {
                    ++s;
                    for(; *s != '\0' && *s != '|' && *s != ':'; ++s)
                    {
                        switch(*s)
                        {
                        case '*':
                        case '?':
                        case '"':
                        case '<':
                        case '>':
                            throw std::runtime_error("WPKG_SUBST current directory path includes an invalid character (* ? \" < >)");

                        case '/':
                        case '\\':
                            // avoid "//" in the path
                            if(e.f_current_directory.length() > 0 && e.f_current_directory[e.f_current_directory.length() - 1] != '/')
                            {
                                e.f_current_directory += '/';
                            }
                            break;

                        default:
                            e.f_current_directory += *s;
                            break;

                        }
                    }
                    // if we found another pipe we missed something
                    if(*s == '|')
                    {
                        throw std::runtime_error("invalid pipe (|) character in your WPKG_SUBST; do you have a missing colon (:)?");
                    }
                }

                // massage the data so we can write these two expressions
                // without having to work on the paths each time:
                //    e.f_path + absolute_path
                //    e.f_path + e.f_directory + relative_path

                // do not keep the ending '/' in the path;
                // it may end up empty which is fine!
                if(e.f_path.length() > 1 && e.f_path[e.f_path.length() - 1] == '/')
                {
                    e.f_path = e.f_path.substr(0, e.f_path.length() - 1);
                }
                // always end the current directory with a '/'
                if(e.f_current_directory.empty() || e.f_current_directory[e.f_current_directory.length() - 1] != '/')
                {
                    e.f_current_directory += '/';
                }
                // always make current directory absolute
                // (note that you cannot include a drive in the subst strings)
                if(e.f_current_directory[0] != '/')
                {
                    e.f_current_directory = "/" + e.f_current_directory;
                }

                // now we got a valid entry
                g_subst_list[e.f_drive] = e;

                // skip the colon(s) if necessary
                while(*s == ':')
                {
                    ++s;
                }
            }
        }
        g_subst_initialized = true;
    }
    subst_list_t::const_iterator r(g_subst_list.find(drive));
    if(r == g_subst_list.end())
    {
        // return an invalid entry
        return subst_entry_t();
    }
    return r->second;
}


#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
int64_t windows_filetime_to_unix_time_seconds(LARGE_INTEGER value)
{
    // FILTIME uses a 100-nano second precision
    const int64_t ticks_100nanoseconds(10000000LL);
    // from Jan 1, 1601
    const int64_t seconds_to_unix_time(11644473600LL);
    return value.QuadPart / ticks_100nanoseconds - seconds_to_unix_time;
}


int64_t windows_filetime_to_unix_time_seconds(FILETIME file_time)
{
    LARGE_INTEGER value;
    value.u.LowPart  = file_time.dwLowDateTime;
    value.u.HighPart = file_time.dwHighDateTime;
    return windows_filetime_to_unix_time_seconds(value);
}


int64_t windows_filetime_to_unix_time_nanoseconds(LARGE_INTEGER value)
{
    // FILTIME uses a 100-nano second precision
    const int64_t ticks_to_nano_seconds(100LL);
    // from Jan 1, 1601
    const int64_t ticks_per_second(1000000000LL);
    return (value.QuadPart * ticks_to_nano_seconds) % ticks_per_second;
}


int64_t windows_filetime_to_unix_time_nanoseconds(FILETIME file_time)
{
    LARGE_INTEGER value;
    value.u.LowPart  = file_time.dwLowDateTime;
    value.u.HighPart = file_time.dwHighDateTime;
    return windows_filetime_to_unix_time_nanoseconds(value);
}
#endif


int xdigit2int(char c)
{
    if(c >= 'a' && c <= 'f')
    {
        return c - 'a' + 10;
    }
    if(c >= 'A' && c <= 'F')
    {
        return c - 'A' + 10;
    }
    // what's left are digits
    return c - '0';
}

char get_any_char(const char *& s, bool decode)
{
    // make sure we do not increment s in this case
    if(*s == '\0')
    {
        return '\0';
    }

    // can character be decoded?
    if(decode)
    {
        // the + is used to represent a space (although this is the "old"
        // scheme, it still needs to be supported); to include a + in a
        // filename, use the %2B encoding
        if(*s == '+')
        {
            ++s;
            return ' ';
        }

        if(*s == '%'
        && isxdigit(s[1])
        && isxdigit(s[2]))
        {
            // encoded character, return the decoded value
            s += 3;
            return xdigit2int(s[-2]) * 16 + xdigit2int(s[-1]);
        }
        // if no % it is not encoded so return the character as is
    }

    // character as is
    return *s++;
}


void decode_string(std::string& str)
{
    std::string temp;
    for(const char *s(str.c_str()); *s != '\0'; )
    {
        temp += get_any_char(s, true);
    }
    str = temp;
}


char int2xdigit(int c)
{
    c &= 15;
    if(c < 10)
    {
        return c + '0';
    }
    return c + 'A' - 10;
}


std::string encode_string(const std::string& str)
{
    std::string result;
    for(const char *s(str.c_str()); *s != '\0'; ++s)
    {
        bool encode(false);
        switch(*s)
        {
        case ' ':
        case '#':
        case '?':
            encode = true;
            break;

        default:
            // anything that represents a UTF-8 character
            encode = static_cast<unsigned char>(*s) >= 0x7F;
            break;

        }
        if(encode)
        {
            result += "%";
            result += int2xdigit(*s >> 4);
            result += int2xdigit(*s);
        }
        else
        {
            result += *s;
        }
    }
    return result;
}


bool is_localhost(const std::string& host)
{
    // an empty host is considered to be the localhost
    if(host.empty() || host == "localhost")
    {
        return true;
    }

    // check the numeric value in case it is 127.0.0.1
    // TODO: add support for IPv6
    int ip[4] = { 127, 0, 0, 1 };
    int value(0);
    int pos(0);
    for(const char *h = host.c_str(); *h != '\0'; ++h)
    {
        if(*h == '.')
        {
            if(pos >= 4)
            {
                return false;
            }
            if(ip[pos] != value)
            {
                return false;
            }
            ++pos;
            value = 0;
        }
        else if(*h < '0' || *h > '9')
        {
            return false;
        }
        else
        {
            value = value * 10 + *h - '0';
        }
    }
    return true;
}


} // no name namespace



/** \brief Directory implementation.
 *
 * This class is the actual variable implementation of the os_dir class.
 * It knows how to handle the reading of a directory depending on the
 * Operating System the software runs on.
 */
class DEBIAN_PACKAGE_EXPORT os_dir_impl
{
public:
                                os_dir_impl(const wpkg_filename::uri_filename& dir_path);

// define the directory handle
#if defined(MO_WINDOWS)
    typedef intptr_t            hdir_t;
#else
    typedef DIR *               hdir_t;
#endif

    static hdir_t               null_dir();

    wpkg_filename::uri_filename f_path;
    hdir_t                      f_dir;
#if defined(MO_WINDOWS)
    // for win32 we need the first entry here...
    bool                        f_has_first;
    _wfinddata_t                f_dirent;
#endif
};

os_dir_impl::os_dir_impl(const wpkg_filename::uri_filename& dir_path)
    : f_path(dir_path)
    , f_dir(null_dir())
#if defined(MO_WINDOWS)
    , f_has_first(true)
    //, f_dirent -- initialized by _wfindfirst()
#endif
{
    if(f_path.empty())
    {
        throw wpkg_filename_exception_parameter("a directory path cannot be an empty string");
    }

#ifdef MO_WINDOWS
    wpkg_filename::uri_filename p(f_path.append_child("*"));
    f_dir = _wfindfirst(p.os_filename().get_utf16().c_str(), &f_dirent);
    if(f_dir == null_dir() && errno == ENOENT)
    {
        // this is a valid case of an empty directory
        // (since we should always see "." and ".." it shouldn't happen
        // too often I think)
        f_has_first = false;
        return;
    }
#else
    f_dir = opendir(f_path.os_filename().get_utf8().c_str());
#endif
    if(f_dir == null_dir())
    {
        throw wpkg_filename_exception_io("cannot access specified directory \"" + f_path.original_filename() + "\"");
    }
}


os_dir_impl::hdir_t os_dir_impl::null_dir()
{
#ifdef MO_WINDOWS
    return static_cast<hdir_t>(-1);
#else
    return NULL;
#endif
}



/** \class os_dir
 * \brief The operating system directory.
 *
 * The os_dir class is used to read a directory on any operating
 * system in a way that is transparent to the user.
 *
 * The class only reads the current directory. It has no concept
 * of sub-directories.
 */

os_dir::os_dir(const wpkg_filename::uri_filename& dir_path)
    : f_impl(new os_dir_impl(dir_path))
{
}

os_dir::~os_dir()
{
    close_dir();
}

void os_dir::close_dir()
{
    if(f_impl->f_dir != os_dir_impl::null_dir())
    {
#ifdef MO_WINDOWS
        _findclose(f_impl->f_dir);
#else
        closedir(f_impl->f_dir);
#endif
        f_impl->f_dir = os_dir_impl::null_dir();
    }
}

bool os_dir::read(wpkg_filename::uri_filename& filename)
{
    if(f_impl->f_dir == os_dir_impl::null_dir())
    {
        return false;
    }

    std::string fn;

#ifdef MO_WINDOWS
    // here we skip all "." and ".." because those do not always work well
    // when working with a network path (netbios)
    do
    {
        if(f_impl->f_has_first)
        {
            // _wfindfirst() read the first entry already!
            f_impl->f_has_first = false;
        }
        else
        {
            int const r(_wfindnext(f_impl->f_dir, &f_impl->f_dirent));
            if(r != 0)
            {
                if(errno == ENOENT)
                {
                    close_dir();
                    return false;
                }
                throw wpkg_filename_exception_io("I/O error while reading directory");
            }
        }
    }
    while(wcscmp(f_impl->f_dirent.name, L".") == 0 || wcscmp(f_impl->f_dirent.name, L"..") == 0);

    fn = libutf8::wcstombs(f_impl->f_dirent.name);
#else
    // we don't use readdir_r() because it's probably not available under mingwin
    errno = 0;
    struct dirent *ent(readdir(f_impl->f_dir));
    if(ent == NULL)
    {
        if(errno == 0)
        {
            // this is the end of the directory
            close_dir();
            return false;
        }
        throw wpkg_filename_exception_io("I/O error while reading directory (readdir() call failed)");
    }

    fn = ent->d_name;
#endif

    filename = f_impl->f_path.append_child(fn);

    return true;
}


/** \brief Read all the files matching pattern.
 *
 * Read all the files that match the specified pattern from this location.
 * The result is one long string with all the filenames.
 *
 * Each filename is made safe for use as a filename in your console.
 *
 * \param[in] pattern  A filename pattern.
 */
std::string os_dir::read_all(const char *pattern)
{
    std::string all;
    wpkg_filename::uri_filename filename;
    while(read(filename))
    {
        if(filename.glob(pattern))
        {
            if(!all.empty())
            {
                all += " ";
            }
            all += wpkg_util::make_safe_console_string(filename.full_path());
        }
    }
    return all;
}




const char * const uri_filename::uri_type_undefined = "?";
const char * const uri_filename::uri_type_direct = "DIRECT";
const char * const uri_filename::uri_type_unc = "UNC";
const char * const uri_filename::uri_scheme_file = "file";
const char * const uri_filename::uri_scheme_http = "http";
const char * const uri_filename::uri_scheme_https = "https";
const char * const uri_filename::uri_scheme_smb = "smb";
const char * const uri_filename::uri_scheme_smbs = "smbs";
const char         uri_filename::uri_no_msdos_drive = '\0';

/** \brief Initialize a URI filename object.
 *
 * The constructor accepts a literal string to create the
 * URI filename object.
 *
 * See the constructor accepting an std::string for more
 * information.
 *
 * \exception wpkg_filename_exception_parameter
 * The \p filename parameter does not represent a valid filename. Note
 * that the empty string is a very special case because it is considered
 * valid by set_filename() but the constructor does NOT call the
 * set_filename() function when \p filename is empty as a result marking
 * this uri_filename object as invalid.
 *
 * \param[in] filename  Any supported filename as a literal string.
 *
 * \sa set_filename()
 * \sa is_valid()
 */
uri_filename::uri_filename(const char *filename)
    //: f_original(filename) -- auto-init
    : f_type(uri_type_undefined)
    //, f_scheme("") -- auto-init
    //, f_decode(false) -- auto-init
    //, f_username("") -- auto-init
    //, f_password("") -- auto-init
    //, f_domain("") -- auto-init
    //, f_port("") -- auto-init
    //, f_share("") -- auto-init
    //, f_is_deb(false) -- auto-init
    //, f_drive(uri_no_msdos_drive) -- auto-init
    //, f_segments() -- auto-init
    //, f_dirname("") -- auto-init
    //, f_path("") -- auto-init
    //, f_basename("") -- auto-init
    //, f_extension("") -- auto-init
    //, f_previous_extension("") -- auto-init
    //, f_anchor("") -- auto-init
    //, f_query_variables() -- auto-init
    //, f_stat(...) -- initialized below
    //, f_real_path("") -- auto-init
{
    // call clear() because f_stat() is not set automatically
    clear();

    if(filename != NULL && *filename != '\0')
    {
        set_filename(filename);
    }
}

/** \brief Initialize a URI filename object.
 *
 * This object represents a resource in the wpkg environment. All
 * resources are identified with URI filenames instead of basic
 * strings so they can be consistent across the whole library.
 *
 * The object can be initialized with a filename which is
 * immediately canonicalized and all the other functions can
 * be used with ease. The canonicalization means transforming
 * the input string in a set of strings that are easy to
 * manage. It also means transforming the path part in a
 * Unix like path with only slash (/) separators, removing
 * double slashes (// becomes /) and transforming the
 * scheme to all lowercase letters.
 *
 * When modifying a filename, you call uri_filename functions
 * and it returns a new uri_filename object with the results
 * already canonicalized.
 *
 * We currently support all of the following types of filenames:
 *
 * \li Standard Unix paths
 * \li Standard MS-Windows paths
 * \li UNC MS-Windows paths
 * \li Long MS-Windows paths (with the \\?\UNC\... introducer)
 * \li HTTP URIs
 * \li NetBIOS URIs starting with smb:// or netbios://
 *
 * Filenames that start with a scheme (which means a set of at least
 * two letters, digits, dashes, and underscores followed by a colon
 * character and two slashes) are viewed as standard URI. For example,
 * an HTTP URI looks like this:
 *
 * \code
 * http://www.m2osw.com/sitemap.xml
 * \endcode
 *
 * A URI supports a username, a password, a domain name, and a port.
 * The scheme can also specify whether the connection is secure or
 * not. For example, the following is a URI with all those parts
 * defined:
 *
 * \code
 * https://alexis:secret@alexis.m2osw.com:888/cia/files.txt
 * \endcode
 *
 * This URI specify a username (alexis), a password (secret), a
 * domain (alexis.m2osw.com), a port (888), and a path (/cia/files.txt).
 * The scheme (https) requests a secure connection.
 *
 * We currently support the following schemes:
 *
 * \li file:// ... a full path of a file on the local disk.
 * \li http:// ... a URI to a file available on a web server.
 * \li https:// ... a URI to a file available on a secure web server.
 * \li smb:// ... a URI to an MS-Windows network file.
 * \li smbs:// ... a URI to a secure MS-Window network file.
 *
 * Note that the secure versions just mean that the connection between
 * the computer running this library and the computer being referenced
 * is secure, not that the other computer was indeed secured properly.
 *
 * The "netbios" and "nb" schemes are viewed as equivalent to the
 * "smb" scheme. The "nbs" scheme is viewed as an equivalent to
 * the "smbs" scheme.
 *
 * At this point other schemes are likely to generate an error since
 * we do not support them (because different scheme may require a
 * different parser.)
 *
 * More info about files names under MS-Windows:
 *
 * http://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
 *
 * \note
 * MS-Windows also supports the syntax "C:file.txt" which means the file
 * named "file.txt" in the C: drive current directory. We do not support
 * those very well and you are likely to get errors or unwanted results
 * with such filenames.
 *
 * \note
 * The function does not prevent filenames that end with a period or a space.
 * Although some other part of the system might (i.e. when building a
 * package.)
 *
 * \warning
 * If the constructor is called with an empty \p filename ("") then it
 * does not call the set_filename() function and as a result this
 * uri_filename object is considered invalid.
 *
 * \exception wpkg_filename_exception_parameter
 * The \p filename parameter does not represent a valid filename. Note
 * that the empty string is a very special case because it is considered
 * valid by set_filename() but the constructor does NOT call the
 * set_filename() function when \p filename is empty as a result marking
 * this uri_filename object as invalid.
 *
 * \param[in] filename  Any supported filename.
 *
 * \sa set_filename()
 * \sa is_valid()
 */
uri_filename::uri_filename(const std::string& filename)
    //: f_original(filename) -- auto-init
    : f_type(uri_type_undefined)
    //, f_scheme("") -- auto-init
    //, f_decode(false) -- auto-init
    //, f_username("") -- auto-init
    //, f_password("") -- auto-init
    //, f_domain("") -- auto-init
    //, f_port("") -- auto-init
    //, f_share("") -- auto-init
    //, f_is_deb(false) -- auto-init
    //, f_drive(uri_no_msdos_drive) -- auto-init
    //, f_segments() -- auto-init
    //, f_dirname("") -- auto-init
    //, f_path("") -- auto-init
    //, f_basename("") -- auto-init
    //, f_extension("") -- auto-init
    //, f_previous_extension("") -- auto-init
    //, f_anchor("") -- auto-init
    //, f_query_variables() -- auto-init
    //, f_stat(...) -- initialized below
    //, f_real_path("") -- auto-init
{
    // call clear() because f_stat() is not set automatically
    clear();

    if(!filename.empty())
    {
        set_filename(filename);
    }
}

/** \brief Change the filename in a URI object.
 *
 * This function updates the filename of this uri_filename object.
 *
 * The details about how the filename gets parsed are shown in the
 * constructor documentation of this class.
 *
 * Whether under Unix or not this function recognize drive letters
 * at the beginning of a filename. This drive letter can be used
 * under Unix along the WPKG_SUBST variable which can be set to
 * define what directory and current working directory drive
 * letters refer to.
 *
 * \note
 * The uri_filename object is not modified if the function throws
 * an error (i.e. the old information is left untouched in the
 * object.)
 *
 * \exception wpkg_filename_exception_parameter
 * The \p filename parameter was not a valid filename.
 *
 * \param[in] filename  The new filename to save in this uri_filename object.
 */
void uri_filename::set_filename(std::string filename)
{
    // all of those parameters are saved in these temporary strings
    // and copied in the object at the end assuming no error occured
    std::string type(uri_type_direct);
    std::string scheme(uri_scheme_file);
    bool decode(false);
    std::string username;
    std::string password;
    std::string domain;
    std::string port;
    std::string share;
    bool is_debian(!filename.empty());
    drive_t drive;
    path_parts_t segments;
    std::string path;
    std::string directories;
    std::string lastname;
    std::string ext;
    std::string previous_ext;
    query_variables_t query_variables;
    std::string anchor;

    const char *s(filename.c_str());
    bool has_scheme(false);
    bool invalid_windows_name(false);
    bool invalid_windows_character(false);

    if(*s == '~')
    {
        if(s[1] != '/' && s[1] != '\\' && s[1] != '\0')
        {
            throw wpkg_filename_exception_parameter("tilde + username is not supported; '~/' was expected at the start of your filename.");
        }
        const char *home(getenv("HOME"));
        if(home != NULL)
        {
            if(*home == '~')
            {
                // avoid infinite recursivity
                throw wpkg_filename_exception_parameter("$HOME path cannot itself start with a tilde (~).");
            }
            uri_filename chome(home);
            if(!chome.is_absolute())
            {
                throw wpkg_filename_exception_parameter("$HOME path is not absolute; we cannot safely replace the ~ character.");
            }
            filename = home + filename.substr(1);
            s = filename.c_str();
        }
    }

#if defined(MO_WINDOWS)
    // under MS-Windows we want to support the long and UNC paths
    // note that // in MS-Windows is seen as http://... so we
    // do not view // as \\ and thus we don't support // as a
    // UNC to make sure we're compatible
    if(s[0] == '\\' && s[1] == '\\')
    {
        if(s[2] == '\\')
        {
            // an MS-DOS console err on this one so do we
            throw wpkg_filename_exception_parameter("MS-Windows filename \"" + filename + "\" is incorrect (put too many \\?)");
        }
        // skip the \\ from the start of the name
        s += 2;

        // by default the type is UNC
        if(s[0] == '.' && s[1] == '\\')
        {
            throw wpkg_filename_exception_parameter("special MS-Windows filename \"" + filename + "\" is not supported");
        }
        if(s[0] == '?' && s[1] == '\\')
        {
            // long filename, this could still be a UNC
            type = "";
            for(s += 2; *s != '/' && *s != '\\' && *s != '\0'; ++s)
            {
                type += *s;
            }
            if(*s != '\0')
            {
                // skip the backslash (\)
                ++s;
            }
            if(type != uri_type_unc)
            {
                // long paths must be absolute (fully qualified)
                // so if not a UNC we only accept drive based paths at this point
                if(type.length() != 2
                || ((type[0] < 'a' || type[0] > 'z') && (type[0] < 'A' || type[0] > 'Z'))
                || type[1] != ':')
                {
                    throw wpkg_filename_exception_parameter("long MS-Windows filename \"" + filename + "\" is not supported (path not understood)");
                }
                // that's a full path, go back at the beginning of the path
                // (the fact that it is long is not important to us)
                s = filename.c_str() + 4;
                type = uri_type_direct;
            }
        }
        else
        {
            type = uri_type_unc;
        }
        if(type == uri_type_unc)
        {
            // a UNC path is an "smb" scheme
            scheme = uri_scheme_smb;
            for(; *s != '@' && *s != '/' && *s != '\\'; ++s)
            {
                domain += *s;
            }
            while(*s == '@')
            {
                ++s;
                std::string value;
                bool number(true);
                for(; *s != '\0' && *s != '/' && *s != '\\' && *s != '@'; ++s)
                {
                    const char c(*s);
                    if(c >= 'A' && c <= 'Z')
                    {
                        // make it lowercase
                        value += c | 0x20;
                    }
                    else
                    {
                        value += c;
                    }
                    if(c < '0' || c > '9')
                    {
                        number = false;
                    }
                }
                if(number)
                {
                    if(!port.empty())
                    {
                        throw wpkg_filename_exception_parameter("UNC path supports at most one port in \"" + filename + "\"");
                    }
                    port = value;
                }
                if(value == "ssl")
                {
                    scheme = uri_scheme_smbs;
                }
                // WARNING: if there is an "@" that we do not understand it gets
                //          removed; that's probably not correct!
            }
            // skip all the slashes before the share name
            while(*s == '/' || *s == '\\')
            {
                ++s;
            }
            if(*s == '\0')
            {
                // without that share name we cannot connect to the other computer
                throw wpkg_filename_exception_parameter("UNC paths require at least the share name not found in \"" + filename + "\"");
            }
            // WARNING: as we can see here we keep the share name separate
            //          from the domain and path
            for(; *s != '/' && *s != '\\' && *s != '\0'; ++s)
            {
                share += *s;
            }
        }
        // even long paths are considered as having a scheme
        has_scheme = true;
        is_debian = false;
    }
#endif

    // check for a drive letter first
    //
    // Note: we handle drive letters under Unix as well since we have
    //       support for a WPKG_SUBST variable and such filenames
    //       would not be compatible between Unix and MS-Windows
    //       (anyway, ':' is forbidden in direct filenames)
    if(type == uri_type_direct && ((*s >= 'a' && *s <= 'z') || (*s >= 'A' && *s <= 'Z')) && s[1] == ':')
    {
        // save the drive letter as a capital
        drive = *s & ~0x20;
        s += 2;
        is_debian = false;
    }

    // change "//" into "/" and "\" into "/"
    char previous('\0');
    const char *segment_start(s);
    for(; *s != '\0'; ++s)
    {
        char c(*s);

        // replace all "\" in "/" characters
        if(c == '\\')
        {
            c = '/';
        }

        // replace "//", "///", "////", etc. with "/"
        if(c == '/' && previous == '/')
        {
            // has_scheme = false; -- not necessary because that happens
            //                        below with the if() which checks
            //                        whether c is a valid scheme
            //                        character
            segment_start = s + 1;
            continue;
        }

        // got a segment?
        if(c == '/')
        {
            // Note: should the MS-Windows reserved names really be forbidden
            //       on Unix systems?
            //
            // (i.e. if you cross compile and cross build packages then you
            // want to check those for full compatibility)
            const std::string segment_str(segment_start, s - segment_start);
            // in case of an absolute path we get an empty segment the very
            // first time, we do not add that to our vector though
            if( !segment_str.empty() )
            {
                if(wpkg_util::is_special_windows_filename(segment_str))
                {
                    invalid_windows_name = true;
                }
                if(!wpkg_util::is_valid_windows_filename(segment_str))
                {
                    invalid_windows_character = true;
                }
                segments.push_back(segment_str);
            }
            segment_start = s + 1;
            directories = path;
            is_debian = false;
        }

        // found a scheme yet?
        // (or a character that prevents a scheme)
        if(c == ':' && !has_scheme)
        {
            has_scheme = true;

            // scheme names MUST be followed by // as in: "http://"
            if(s[1] == '/' && s[2] == '/')
            {
                // force the scheme to lowercase
                for(std::string::size_type i(0); i < path.length(); ++i)
                {
                    if(path[i] >= 'A' && path[i] <= 'Z')
                    {
                        path[i] |= 0x20;
                    }
                }

                if(path == "netbios" || path == "nb")
                {
                    // replace netbios:// or nb:// by smb://
                    scheme = uri_scheme_smb;
                }
                else if(path == "nbs")
                {
                    // replace nbs:// by smbs://
                    scheme = uri_scheme_smbs;
                }
                else
                {
                    scheme = path;
                }

                // a URI always has an absolute path
                previous = '/';
                path = "/";
                s += 3; // skip "://"

                // all URIs, including the "file" scheme must be decoded
                // (i.e. %20 is a space, not the 3 characters '%', '2', and
                // '0'); however, the "file" scheme filenames are not
                // re-encoded since we "just" return the path, not a URI
                decode = true;

                if(scheme != "file")
                {
                    // skip all slashes after the colon
                    // this is actually wrong, but allows for mistakes such
                    // as: http:///www.example.com/ which most people expect
                    // to work; however, we do not do that with files because
                    // a 3rd slash means the default domain name should be
                    // used (the default being localhost which we only support
                    // for the "file" scheme)
                    while(*s == '/' || *s == '\\')
                    {
                        ++s;
                    }
                }

                // retrieve the domain name
                for(; *s != '/' && *s != '\\' && *s != '\0'; ++s)
                {
                    domain += *s;
                }

                // skip all the slashes following the domain name
                while(*s == '/' || *s == '\\')
                {
                    ++s;
                }

                // special case for smb which needs a share folder
                if(scheme == uri_scheme_smb || scheme == uri_scheme_smbs)
                {
                    if(*s == '\0')
                    {
                        // without that share name we cannot connect to the other computer
                        throw wpkg_filename_exception_parameter("smb paths require at least the share name not found in \"" + filename + "\"");
                    }
                    // WARNING: as we can see here we keep the share name separate
                    //          from the domain and path
                    for(; *s != '/' && *s != '\\' && *s != '\0'; ++s)
                    {
                        share += *s;
                    }

                    // skip the '/' or '\'
                    while(*s == '/' || *s == '\\')
                    {
                        ++s;
                    }
                }

                // domain may include a username name password
                std::string::size_type p(domain.find_first_of('@'));
                if(p != std::string::npos)
                {
                    // got a username:password
                    std::string::size_type pp(domain.find_first_of(':'));
                    if(pp > p)
                    {
                        pp = std::string::npos;
                    }
                    if(pp != std::string::npos)
                    {
                        // got a user name and password
                        // (although either one may be an empty string)
                        username = domain.substr(0, pp);
                        password = domain.substr(pp + 1, p - pp - 1);
                    }
                    if(username.empty() || password.empty())
                    {
                        // something's missing
                        throw wpkg_filename_exception_parameter("when specifying a username and password, both must be valid (not empty): \"" + filename + "\"");
                    }
                    // get the domain:port part
                    domain = domain.substr(p + 1);
                }

                // check for a port now
                std::string::size_type pt(domain.find_first_of(':'));
                if(pt != std::string::npos)
                {
                    // retrieve the port and verify that it is exclusively
                    // composed of digits
                    port = domain.substr(pt + 1);
                    for(const char *prt(port.c_str()); *prt != '\0'; ++prt)
                    {
                        if(*prt < '0' || *prt > '9')
                        {
                            throw wpkg_filename_exception_parameter("a port in a URI must exclusively be composed of digits. \"" + port + "\" is not valid!");
                        }
                    }
                    domain = domain.substr(0, pt);
                }

                // in case of the "file" scheme, we also support a drive letter
                if(scheme == "file")
                {
                    // check for a drive letter first
                    const char *temp(s);
                    char drive_letter(get_any_char(temp, decode));
                    char drive_separator(get_any_char(temp, decode));
                    if(((drive_letter >= 'a' && drive_letter <= 'z') || (drive_letter >= 'A' && drive_letter <= 'Z'))
                    && (drive_separator == ':' || drive_separator == '|'))
                    {
                        // save the drive letter as a capital
                        drive = drive_letter & ~0x20;
                        s = temp;
                        is_debian = false;
                    }

                    // here we canonicalize the localhost too
                    if(is_localhost(domain))
                    {
                        domain = "";
                    }
                }
                else if(domain.empty())
                {
                    // the domain cannot be empty for all other schemes
                    throw wpkg_filename_exception_parameter("the resulting domain is empty and that is not valid for this scheme. \"" + filename + "\" is not valid!");
                }

                // TODO:
                // At this point if the domain is not empty we should
                // canonicalize it, especially if it is a simple numeric IP.
                // For comparisons, not having those canonicalized is not
                // so good!

                segment_start = s;
                // counter the ++s of the main for() loop
                --s;
                continue;
            }
        }

        if((c == '#' || c == '?') && decode)
        {
            // DO NOT CHANGE s IN THIS BLOCK
            // it is used after the main loop to check the last segment
            // in case we did not have a / just before the '#' or '?'
            const char *v(s);
            if(c == '?')
            {
                for(++v; *v != '#' && *v != '\0';)
                {
                    std::string var_name;
                    {
                        const char *start(v);
                        for(; *v != '#' && *v != '&' && *v != '=' && *v != '\0'; ++v)
                        {
                            if(isspace(*v))
                            {
                                throw wpkg_filename_exception_parameter("a URI query string variable name cannot include a space in \"" + filename + "\"");
                            }
                        }
                        var_name.assign(start, v - start);
                    }
                    std::string var_value;
                    bool has_value(*v == '=');
                    if(has_value)
                    {
                        // skip the equal
                        ++v;
                        const char *start(v);
                        for(; *v != '#' && *v != '&' && *v != '=' && *v != '\0'; ++v);
                        var_value.assign(start, v - start);
                    }
                    if(has_value && var_name.empty())
                    {
                        throw wpkg_filename_exception_parameter("a URI query string variable name cannot be empty in \"" + filename + "\"");
                    }
                    if(has_value || !var_name.empty())
                    {
                        query_variables[var_name] = var_value;
                    }
                    // there should be only one & to separate two variables,
                    // but we would anyway accept multiple
                    for(; *v == '&'; ++v);
                }
            }
            if(*v == '#')
            {
                anchor = v + 1;
            }
            break;
        }

        // valid character as far as the scheme goes?
        if(!has_scheme
        && (c < '0' || c > '9')
        && (c < 'a' || c > 'z')
        && (c < 'A' || c > 'Z')
        && c != '-' && c != '_')
        {
            // this may look confusing since we set the "has_scheme"
            // to true when it is indeed false when in fact we found something
            // that makes it impossible for the scheme to be specified
            // so we prevent the "://" from being accepted
            has_scheme = true;
        }

        // keep this character in the path
        path += c;
        previous = c;
    }

    // Note: should the MS-Windows reserved names really be forbidden
    //       on Unix systems?
    //
    // (i.e. if you cross compile and cross build packages then you
    // want to check those for full compatibility)
    lastname.assign(segment_start, s - segment_start);
    if(wpkg_util::is_special_windows_filename(lastname))
    {
        invalid_windows_name = true;
    }
    if(!wpkg_util::is_valid_windows_filename(lastname))
    {
        invalid_windows_character = true;
    }
    if(is_debian)
    {
        // if lastname is a valid package name then the
        // file is not considered a .deb (but really only
        // if there is just a lastname!)
        is_debian = wpkg_util::is_package_name(lastname);
    }
    segments.push_back(lastname);

    std::string::size_type period(lastname.find_last_of('.'));
    if(period != 0 && period != std::string::npos)
    {
        std::string::size_type previous_period(std::string::npos);
        ext = lastname.substr(period + 1);
#ifdef WINDOWS
        case_insensitive::case_insensitive_string cext(ext);
#else
        const std::string& cext(ext);
#endif
        if(cext == "gz"
        || cext == "bz2"
        || cext == "lzma"
        || cext == "xv")
        {
            previous_period = lastname.find_last_of('.', period - 1);
        }
        if(previous_period != 0 && previous_period != std::string::npos)
        {
            previous_ext = lastname.substr(previous_period + 1, period - previous_period - 1);
        }
        else
        {
            previous_ext = ext;
        }
    }

    // only now do we generate an error because of invalid names or characters
    // because only the direct schemes consider them invalid
    if(scheme == "file" || scheme == "smb" || scheme == "smbs")
    {
        if(invalid_windows_name)
        {
            throw wpkg_filename_exception_parameter("Win32 special file name in \"" + filename + "\" is not supported");
        }
        if(invalid_windows_character)
        {
            throw wpkg_filename_exception_parameter("file name \"" + filename + "\" includes characters that are unsupported under MS-Windows");
        }
    }

    // check the port for the scheme; if it is the default as per the
    // /etc/service definitions, then clear the port so it is canonicalized
    if(!scheme.empty() && !port.empty())
    {
        // I'm not too sure whether we should check tcp or udp first; from
        // what I've seen the port is the same for both for common services
        tcp_client_server::initialize_winsock();
        struct servent *e(getservbyname(scheme.c_str(), "tcp"));
        if(e == NULL)
        {
            e = getservbyname(scheme.c_str(), "udp");
        }
        if(e != NULL)
        {
            int portno(atoi(port.c_str()));
            if(htons(portno) == e->s_port)
            {
                port = "";
            }
        }
    }

    // if the path is not empty and the decode flag is true, decode it
    // now otherwise we could get all sorts of problems; it also means
    // we want to re-encode it when the user retrieves the path for a
    // scheme other than "file"...
    if(decode)
    {
        for(path_parts_t::size_type i(0); i < segments.size(); ++i)
        {
            decode_string(segments[i]);
        }
        decode_string(directories);
        decode_string(path);
        decode_string(lastname);
        decode_string(ext);
        decode_string(previous_ext);
    }

    f_original = filename;
    f_type = type;
    f_scheme = scheme;
    f_decode = decode;
    f_username = username;
    f_password = password;
    f_domain = domain;
    f_port = port;
    f_share = share;
    f_is_deb = is_debian;
    f_drive = drive;
    std::swap(f_segments, segments);
    f_dirname = directories;
    f_path = path;
    f_basename = lastname;
    f_extension = ext;
    f_previous_extension = previous_ext;
    f_stat.set_valid(false);
    f_anchor = anchor;
    std::swap(f_query_variables, query_variables);
}

/** \brief Clear the filename making it invalid.
 *
 * This function clears the uri_filename content as if it had been
 * created with an empty string.
 */
void uri_filename::clear()
{
    f_original = "";
    f_type = uri_type_undefined;
    f_scheme = "";
    f_decode = false;
    f_username = "";
    f_password = "";
    f_domain = "";
    f_port = "";
    f_share = "";
    f_is_deb = false;
    f_drive = drive_t();
    f_segments.clear();
    f_dirname = "";
    f_path = "";
    f_basename = "";
    f_extension = "";
    f_anchor = "";
    f_query_variables.clear();

    clear_cache();
}

/** \brief Clear the cache.
 *
 * This function can be used to clear the cached data. This is useful is you
 * run a function and this uri_filename cache may therefore become obsolete.
 *
 * The cache stores the following information:
 *
 * \li stat() call used by os_stat(), exists(), is_dir(), is_reg() and some
 * other functions.
 *
 * \li realpath() call used by the os_real_path().
 */
void uri_filename::clear_cache()
{
    // cache
    f_stat.set_valid(false);
    f_real_path.clear();
}

/** \brief Retrieve the original filename.
 *
 * This function returns the original filename as passed to the set_filename()
 * function. This is not canonicalized at all.
 *
 * Note that is the same filename passed to the constructor.
 *
 * \return The original filename as passed to set_filename().
 */
std::string uri_filename::original_filename() const
{
    return f_original;
}

/** \brief Return the type of the path.
 *
 * This function returns the type of this filename. At this time the following
 * few types are supported:
 *
 * \li DIRECT (uri_type_direct)
 * \li UNC (uri_type_unc)
 *
 * These are specific to MS-Windows. The Unix paths only use "DIRECT".
 *
 * \return The scheme of this uri_filename.
 */
std::string uri_filename::path_type() const
{
    return f_type;
}

/** \brief Return the scheme.
 *
 * This function returns the scheme of this uri_filename. All filenames
 * have a scheme. When not specified, it is most often set to "file".
 * It may be set to "smb" when UNC filenames are found.
 *
 * \return The scheme of this uri_filename.
 */
std::string uri_filename::path_scheme() const
{
    return f_scheme;
}

/** \brief Get the drive information.
 *
 * This function transform the specified drive parameter in a
 * string making use of the WPKG_SUBST content if appropriate.
 * If no drive is defined (uri_no_msdos_drive) then the function
 * returns an empty string.
 *
 * Otherwise it returns a path that can be prepended to the f_path
 * value to create a valid and complete path directly useable on
 * your platform.
 *
 * The use of the WPKG_SUBST allows us to test the drive letter
 * under Unices without having to run tests only under MS-Windows
 * although we do have specialized tests under MS-Windows to
 * ensure that drive letters are properly supported in all
 * cases (at least all those we have thought of so far and
 * tested in our unit tests.)
 *
 * The \p for_absolute_path defines whether the current directory
 * of the substitution will be used (false) or not (true). This
 * is because in MS-DOS each drive has a distinct current directory.
 *
 * To use the function from the outside you may call it this way:
 *
 * \code
 * drive_path = fn.drive_subst(fn.msdos_drive(), fn.is_absolute());
 * \endcode
 *
 * \param[in] drive  The drive letter to convert, usually the f_drive value.
 * \param[in] for_absolute_path  Whether the result will be used with an absolute path.
 */
std::string uri_filename::drive_subst(drive_t drive, bool for_absolute_path) const
{
    if(drive != uri_no_msdos_drive)
    {
        // check for a substitute
        subst_entry_t subst(get_subst(drive));
        if(subst.f_drive != uri_no_msdos_drive)
        {
            if(for_absolute_path)
            {
                return subst.f_path;
            }
            return subst.f_path + subst.f_current_directory;
        }

        // no substitute, return as is
        return static_cast<char>(drive) + std::string(":");
    }

    // no drive at all
    return "";
}

/** \brief Retrieve just the path.
 *
 * This function returns a copy of the path only.
 *
 * IMPORTANT NOTE: A NetBIOS path does not include the domain name nor the
 * share folder name. Therefore, in the following path:
 *
 * \code
 * \\server\share\file.txt
 * \endcode
 *
 * This function returns "file.txt"
 *
 * By default this function includes the drive if it was present in
 * the original filename. It is possible to ignore the drive by
 * setting \p with_drive to false.
 *
 * \param[in] with_drive  Whether the drive should be added when present.
 *
 * return Just the path part of the uri_filename.
 */
std::string uri_filename::path_only(bool with_drive) const
{
    if(with_drive)
    {
        std::string subst(drive_subst(f_drive, is_absolute()));
        return subst + f_path;
    }
    return f_path;
}

/** \brief Retrieve the canonicalized full path.
 *
 * This function is similar to the original_filename() function only it
 * returns the path that was canonicalized.
 *
 * Note that NetBIOS paths are returned as smb://... URIs.
 *
 * This string can be passed to a new uri_filename object to get an
 * exact copy of this uri_filename object.
 *
 * This function could have been called to_string().
 *
 * \param[in] replace_slashes  Whether to replace '/' with '\'.
 *
 * \return A string representing the URI filename.
 */
std::string uri_filename::full_path(bool replace_slashes) const
{
    std::string result;

    // special case or we get "://" as the full_path()
    if(f_original.empty())
    {
        return "";
    }

    const bool is_file(f_scheme == "file");
    if(!is_file)
    {
        replace_slashes = false;
        result += f_scheme + "://";
        if(!f_username.empty() && !f_password.empty())
        {
            result += f_username + ":" + f_password + "@";
        }
        result += f_domain;
        if(!f_port.empty())
        {
            result += ":" + f_port;
        }
    }
    if(!f_share.empty())
    {
        result += "/" + f_share;
    }
    result += drive_subst(f_drive, is_absolute());
    if(is_file)
    {
        result += f_path;
    }
    else
    {
        result += encode_string(f_path);

        // a direct file path cannot support query variables and an anchor
        // (this means we're possibly not returning what was passed to the
        // set_filename() function which I think is likely wrong...)
        char separator('?');
        for(query_variables_t::const_iterator it(f_query_variables.begin()); it != f_query_variables.end(); ++it)
        {
            result += separator;
            separator = '&';
            result += it->first + "=" + it->second;
        }

        if(!f_anchor.empty())
        {
            result += "#" + f_anchor;
        }
    }

#if defined(MO_WINDOWS)
    if(replace_slashes)
    {
        std::replace(result.begin(), result.end(), '/', '\\');
    }
#else
    static_cast<void>(replace_slashes);
#endif

    return result;
}

/** \brief Return the number of segments.
 *
 * This function returns the number of segments present in this path.
 * The number of segments can be used to retrieve each segment with
 * the segment() function.
 *
 * \return The number of segments.
 *
 * \sa segment()
 */
int uri_filename::segment_size() const
{
    return static_cast<int>(f_segments.size());
}

/** \brief Return a path segment.
 *
 * This function returns the specified segment of the path. A segment
 * is a part between two slashes (/) or the first or last part of the
 * filename as set by the set_filename() function.
 *
 * The number of segments can be retrieved by the segment_size() function.
 *
 * \param[in] idx  The index of the segment to retrieve.
 *
 * \return Segment \p idx of this path.
 */
std::string uri_filename::segment(int idx) const
{
    return f_segments[idx];
}

/** \brief Retrieve the basename of a file.
 *
 * This function returns just the basename of a uri_filename.
 * Note that it returns a direct string so you cannot use
 * the result to generate another valid uri_filename without
 * making sure that the username, password, domain, port,
 * etc. be copied too.
 *
 * Note that the function is quite optimized since the basename
 * is already known from the parsing done in set_filename().
 * However, this function further removes the last extension
 * or two (i.e. if a compression extension is defined, then it
 * is also removed.) The following compression extensions are
 * automatically removed along the other extensions:
 *
 * \li .gz
 * \li .bz2
 * \li .lzma
 * \li .xz
 *
 * So for example if you have a filename such as:
 *
 * \code
 * C:\path\file.tar.bz2
 * \endcode
 *
 * This function returns:
 *
 * \code
 * file
 * \endcode
 *
 * Since it removes the .bz2 and the .tar extensions as the .bz2 is a
 * recognized compression extension.
 *
 * \param[in] last_extension_only  Only remove the last extension even if it is a compression extension.
 *
 * \return The basename of this uri_filename.
 */
std::string uri_filename::basename(bool last_extension_only) const
{
    std::string bn(f_basename);
    std::string::size_type p = bn.find_last_of('.');
    if(p != 0 && p != std::string::npos
    && (p != 1 || bn[1] != '.')) // special case for filenames that start with ".."
    {
#ifdef WINDOWS
        case_insensitive::case_insensitive_string ext(bn.substr(p));
#else
        std::string ext(bn.substr(p));
#endif
        if(!last_extension_only
        && (ext == ".gz" || ext == ".bz2" || ext == ".lzam" || ext == ".xz"))
        {
            // remove both extensions if a known compression extension exists
            std::string::size_type e(bn.find_last_of('.', p - 1));
            if(e != std::string::npos)
            {
                p = e;
            }
        }
        bn = bn.substr(0, p);
    }
    return bn;
}

/** \brief Retrieve the directory name of this filename.
 *
 * This function retrieves the directory name of this filename. This
 * is the part of the path before the last slash (/).
 *
 * Note that if the path ended with a slash when you called the
 * set_filename() functions (or the constructor) then dirname returns
 * the same thing as path_only() minus the last slash (/) character.
 *
 * By default this function includes the drive if it was present in
 * the original filename. It is possible to ignore the drive by
 * setting \p with_drive to false.
 *
 * \param[in] with_drive  Whether to include the drive information in the result.
 *
 * \return The dirname part of the filename.
 */
std::string uri_filename::dirname(bool with_drive) const
{
    if(with_drive)
    {
        return drive_subst(f_drive, !f_dirname.length() && f_dirname[0] == '/') + f_dirname;
    }
    return f_dirname;
}

/** \brief Extension of the basename.
 *
 * This function returns the extension of the very last filename in a path.
 * A URI filename that ends with a slash (/) does not have an extension (even
 * if the last directory name has an extension.)
 *
 * A period found as the very first character in a filename is not considered
 * as part of an extension, only as a hidden file.
 *
 * This function returns the extension without the period (i.e. it returns
 * "deb" for a Debian package, and not ".deb".)
 *
 * \return The extension of the basename or an empty string.
 */
std::string uri_filename::extension() const
{
    return f_extension;
}

/** \brief The previous extension of the basename.
 *
 * This function returns the "previous" extension of the very last filename
 * in a path. A URI filename that ends with a slash (/) does not have an
 * extension (even if the last directory name has an extension.)
 *
 * A period found as the very first character in a filename is not considered
 * as part of an extension, only as a hidden file.
 *
 * The previous extension sees a filename with a compression extension as
 * possibly having two extensions. For example, this function returns
 * "tar" if the file ends with ".tar.gz", instead of just "gz" like
 * the extension() function. Note that if there is no previous extension,
 * then this function returns the same thing as extension().
 *
 * \warning
 * Note that as a side effect, a filename that ends with "..ext" has a
 * previous extension equal to the empty string ("").
 *
 * \return The extension of the basename or an empty string.
 *
 * \sa extension()
 */
std::string uri_filename::previous_extension() const
{
    return f_previous_extension;
}

/** \brief MS-DOS Drive of the filename.
 *
 * This function the drive letter used in the filename as specified in the
 * set_filename() function.
 *
 * \return The drive letter.
 */
char uri_filename::msdos_drive() const
{
    return f_drive;
}

/** \brief Return the username.
 *
 * This function returns the username. If no name was defined, this function
 * returns an empty string. A plain file has no password defined.
 *
 * \return The username as defined in the URI.
 */
std::string uri_filename::get_username() const
{
    return f_username;
}

/** \brief Return the password.
 *
 * This function returns the password. If no password was defined, this
 * function returns an empty string. A plain file has no password defined.
 *
 * \return The password as defined in the URI.
 */
std::string uri_filename::get_password() const
{
    return f_password;
}

/** \brief Return the domain.
 *
 * This function returns the domain name. If no domain was defined, this
 * function returns an empty string. A plain file has no domain defined.
 *
 * \return The domain as defined in the URI.
 */
std::string uri_filename::get_domain() const
{
    return f_domain;
}

/** \brief Return the port.
 *
 * This function returns the port name. If no port was defined, this
 * function returns an empty string. A plain file has no port defined.
 *
 * \return The port as defined in the URI.
 */
std::string uri_filename::get_port() const
{
    return f_port;
}

/** \brief Return the share folder.
 *
 * This function returns the share folder of an smb URI. If the URI is
 * not an smb URI then there is no share folder and this function
 * returns the empty string.
 *
 * If the URI represents an smb URI, then there has to be a shared
 * folder and it will not be the empty string.
 *
 * \return The share folder name of the smb URI.
 */
std::string uri_filename::get_share() const
{
    return f_share;
}

/** \brief Retrieve whether the URI was decoded.
 *
 * This function returns true when the URI was marked as decoded. This means
 * it is an HTTP like URI that may be using the % character to encode some of
 * the characters in its path had those % character transformed to actual
 * characters.
 *
 * \return true if the path, query strings, and anchor may require URI decoding.
 */
bool uri_filename::get_decode() const
{
    return f_decode;
}

/** \brief Retrieve the anchor of a URI.
 *
 * This function returns the anchor of a URI. The anchor is the string that
 * appears after the hash (#) character of a URI. When defined, that's the
 * very last part of the URI. Note that we do not have any heuristic to
 * understand a 'hash bang' (#!).
 *
 * \return A copy of the anchor of this uri_filename.
 */
std::string uri_filename::get_anchor() const
{
    return f_anchor;
}

/** \brief Return a query variable.
 *
 * This function returns the named query variable. If it wasn't defined, the
 * function returns an empty string.
 *
 * These variables are those defined at the end of a query string after the
 * question mark as in:
 *
 * \code
 * http://www.m2osw.com/?var=5&type=html
 * \endcode
 *
 * \param[in] name  The name of the query string variable to retrieve.
 *
 * \return The content of the named query string variable.
 */
std::string uri_filename::query_variable(const std::string& name) const
{
    query_variables_t::const_iterator it(f_query_variables.find(name));
    if(it == f_query_variables.end())
    {
        return "";
    }

    return it->second;
}

/** \brief Return the query variable map.
 *
 * This function returns a copy of the query variable map so you can
 * go through the list.
 *
 * \return A copy of the query variable map.
 *
 * \sa query_variable()
 */
uri_filename::query_variables_t uri_filename::all_query_variables() const
{
    return f_query_variables;
}

/** \brief Check whether the filename is empty.
 *
 * This function returns true if the original filename is empty.
 * This would be equivalent to testing a filename defined in
 * an std::string.
 *
 * \code
 * std::string filename("");
 * if(filename.empty())
 * {
 *    ...
 * }
 * \endcode
 *
 * Note that "." is not considered empty, although "" may be viewed
 * as an equivalent to ".".
 *
 * \return true if the original filename is empty, false otherwise.
 */
bool uri_filename::empty() const
{
    return f_original.empty();
}

/** \brief Check whether the file exists.
 *
 * This function returns true if the filename exists. Note that does not
 * currently tests whether the file exists on a remote device. It will
 * do so once we implemented cache support.
 *
 * \warning
 * This function makes use of the os_stat() function to determine whether
 * the file exists and that function caches the result of the stat() call.
 * This means if it returned true once, it will continue to return true
 * even if the file gets deleted afterward.
 *
 * \return true if the file exists, false otherwise.
 */
bool uri_filename::exists() const
{
    file_stat s;
    return os_stat(s) == 0;
}

/** \brief Check whether the file exists and is a regular file.
 *
 * This function returns true if the filename exists and is a regular file.
 *
 * \warning
 * If the file does not exists, this function returns false. To check
 * whether the file exists, make sure to call exists() first.
 *
 * \warning
 * If this function succeeds once (returns true) then it will always
 * return true because the result of the stat() function is cached.
 *
 * \return true if the file exists and is a regular file, false otherwise.
 */
bool uri_filename::is_reg() const
{
    file_stat s;
    if(os_stat(s) != 0)
    {
        return false;
    }
    return s.is_reg();
}

/** \brief Check whether the file exists and is a directory.
 *
 * This function returns true if the filename exists and is a directory.
 *
 * \warning
 * If the file does not exists, this function returns false. To check
 * whether the file exists, make sure to call exists() first.
 *
 * \warning
 * If this function succeeds once (returns true) then it will always
 * return true because the result of the stat() function is cached.
 *
 * \return true if the file exists and is a directory, false otherwise.
 */
bool uri_filename::is_dir() const
{
    file_stat s;
    if(os_stat(s) != 0)
    {
        return false;
    }
    return s.is_dir();
}

/** \brief Check whether the file represents a .deb file.
 *
 * This function returns true if the filename is considered to represent
 * a .deb file. The heuristic is that a simple package name cannot include
 * underscore (_) or slash (/) characters. Whether the file has a .deb
 * extension is ignored. If the assumption is wrong, the packager fails
 * at some point anyway as it will try to load the package and it will
 * have an incompatible format.
 *
 * Note that only the filename is checked. The file itself may not even
 * exist.
 *
 * \return true if the filename represents a .deb file.
 */
bool uri_filename::is_deb() const
{
    return f_is_deb;
}

/** \brief Check whether the uri_filename object is valid.
 *
 * This function returns true if it is determined that this object
 * was initialized successfully once. i.e. if you call the constructor
 * with an empty string then the URI is viewed as invalid. You should
 * call set_filename() at least once.
 *
 * Note that if set_filename() throws and the URI filename was valid
 * before that, then it is still valid because the set_filename()
 * function does not modify any of the internal values unless
 * everything works as expected.
 *
 * Note that calling set_filename() with an empty string is considered
 * valid and this object will be marked as valid.
 *
 * \return true if the URI filename is valid, false otherwise.
 */
bool uri_filename::is_valid() const
{
    return f_type != uri_type_undefined;
}

/** \brief Path is direct to a file.
 *
 * Check whether the path represents a direct filename or includes a
 * scheme such as HTTP that represents a remote file requiring a
 * more advanced read/write concept.
 *
 * There are two schemes understood as direct files: file and smb.
 * (Note that netbios and nb are equivalent to smb.)
 *
 * If the filename does not start with a scheme name then the name is
 * considered to be a direct filename.
 *
 * \return true if the file can be directly opened with the standard file system function calls.
 */
bool uri_filename::is_direct() const
{
    return f_scheme == uri_scheme_file || f_scheme == uri_scheme_smb || f_scheme == uri_scheme_smbs;
}

/** \brief Return whether the path is absolute.
 *
 * This function returns true if the path is considered absolute. Under
 * Microsoft it is often referenced as a fully qualified path.
 *
 * If a scheme other than "file" was used to define this uri_filename
 * then this function returns true. Otherwise it returns true only if
 * the filename represents an absolute filename in the current operating
 * system. This means a path that starts with a slash (/) or a letter
 * drive plus a slash (C:/).
 *
 * \return true if this uri_filename represents an absolute path.
 */
bool uri_filename::is_absolute() const
{
    // f_path does not include the drive letter so we can just test the path
    return !f_original.empty() && (f_scheme != uri_scheme_file || (!f_path.empty() && f_path[0] == '/'));
}

/** \brief Check whether a forbidden character was used.
 *
 * MS-Windows filenames have a few characters that are forbidden. This
 * function checks one segment for invalid characters. It is done by
 * segment because some characters such as spaces and periods should
 * not appear at the start or end of a filename.
 *
 * Note that when building a package we check those names. We do not otherwise
 * generally test the names at that level. The tests are also performed under
 * Unix systems. It is quite unlikely that you'd ever need to use the
 * forbidden characters in a Unix name either.
 *
 * The list of characters that are forbidden is a directory part are:
 *
 * \li less than (\<)
 * \li greater than (>)
 * \li colon (:)
 * \li double quote (")
 * \li forward slash (/)
 * \li backward slash (\)
 * \li vertical bar (|)
 * \li question mark (?)
 * \li asterisk (*)
 *
 * Note that includes the question mark and asterisk characters that are valid
 * in patterns. Thus this function returns false when checking a valid pattern.
 *
 * A part is otherwise not expected to include any forward (/) or backward (\)
 * slashes since we use those to determine each segment of a path.
 *
 * \param[in] path_part  Part to be checked for validity.
 *
 * \return true if the segment is valid, false if any forbidden characters were found.
 */
bool uri_filename::is_valid_windows_part(const std::string& path_part)
{
    const char *s(path_part.c_str());
    const char *start(s);

    if(*s == ' ')
    {
        // spaces are legal but cause terrible problems when used at the
        // beginning or the end of a filename
        return false;
    }

    for(; *s != '\0'; ++s)
    {
        switch(*s)
        {
        case '<':
        case '>':
        case ':':
        case '"':
        case '/':
        case '\\':
        case '|':
        case '?':
        case '*':
            return false;

        default:
            // all control characters
            if(*s < ' ')
            {
                return false;
            }
            break;

        }
    }

    if(s > start)
    {
        if(s[-1] == ' ' || s[-1] == '.')
        {
            // forbid spaces and periods at the end of filenames because these
            // are legal characters but they cause terrible problems when used
            // there
            return false;
        }
    }

    return true;
}

/** \brief Check a glob pattern against this uri_filename.
 *
 * This function applies a glob pattern (i.e. a Unix shell like pattern)
 * against a filename. It returns true if this filename matches the pattern.
 *
 * The pattern supports the following characters:
 *
 * \li '*' -- zero or more of any character
 * \li '?' -- exactly one of any character
 * \li '[...]' -- exactly one of the ranges of characters
 * \li '[!...]' or '[^...]' -- exactly one character not part of the ranges
 * \li ... -- all other characters must match as is
 *
 * The character classes ([...]) are defined as a character by itself, or a
 * range indicated by a character, a dash, and another character. The second
 * character is expected to be larger than the first (At this point the
 * function does not verify that the characters are in order.) For example,
 * to accept any uppercase ASCII letters: [A-Z]. To test the dash character,
 * use it first in the class: [-0-9] supports the dash (-) and the digits
 * 0 to 9.
 *
 * Note that a class that ends with a dash does not match the dash character.
 * So an entry such as [a-f0-] will only match lowercase letters 'a' to 'f'
 * and the character '0'.
 *
 * \note
 * Multiple '*' in a row in the pattern are the same as one '*'.
 *
 * \note
 * The pattern should never include two slashes one after another since
 * the path cannot include such (i.e. the path in the uri_filename was
 * parsed by the set_filename() function which remove such.)
 *
 * \warning
 * The function checks the pattern against the path only. The path does NOT
 * include the scheme, user name, password, domain name, port, the share
 * folder name, the drive, the anchor, or the query string.
 *
 * \warning
 * The function checks the patterns using lowercase letters only when
 * run under MS-Windows. Note however that the string is used as is
 * instead of internationalized so all characters are viewed as ISO-8859-1.
 *
 * \param[in] pattern  The glob pattern to check the \p filename against.
 *
 * \return true if \p filename matches \p pattern.
 */
bool uri_filename::glob(const char *pattern) const
{
    std::string path(f_path);
#if defined(MO_WINDOWS)
    std::transform(path.begin(), path.end(), path.begin(), ::tolower);
#endif
    const char *filename(path.c_str());

    std::string pat(pattern);
#if defined(MO_WINDOWS)
    std::transform(pat.begin(), pat.end(), pat.begin(), ::tolower);
#endif
    const char *p(pat.c_str());

    return glob(filename, p);
}

/** \brief Internal version of the glob() function.
 *
 * This function is internal because the glob() function is recursive
 * we need to have a separate internal version.
 *
 * \param[in] filename  The filename being checked against \p pattern.
 * \param[in] pattern  The pattern used to match \p filename.
 *
 * \return true if the pattern matches filename, false otherwise.
 */
bool uri_filename::glob(const char *filename, const char *pattern) const
{
    while(*pattern != '\0')
    {
        switch(*pattern)
        {
        case '*':
            do
            {
                ++pattern;
            }
            while(*pattern == '*');
            if(*pattern == '\0')
            {
                return true;
            }
            while(*filename != '\0')
            {
                if(glob(filename, pattern))
                {
                    return true;
                }
                ++filename;
            }
            return false;

        case '?':
            // accept any character except end of string
            if(*filename == '\0')
            {
                return false;
            }
            ++filename;
            ++pattern;
            break;

        case '[':
            // accept any character (or not) defined between [...]
            if(*filename == '\0')
            {
                return false;
            }
            ++pattern;
            {
                bool invert(*pattern == '!' || *pattern == '^');
                if(invert)
                {
                    ++pattern;
                }
                if(*pattern != '\0')
                {
                    do
                    {
                        char from(*pattern);
                        char to(from);
                        if(pattern[1] == '-')
                        {
                            if(pattern[2] != ']')
                            {
                                to = pattern[2];
                                pattern += 3;
                            }
                            else
                            {
                                pattern += 2;
                            }
                        }
                        else
                        {
                            ++pattern;
                        }
                        if((*filename >= from && *filename <= to) ^ invert)
                        {
                            // match!
                            filename++;
                            // skip the rest of the [...]
                            while(*pattern != ']' && *pattern != '\0')
                            {
                                ++pattern;
                            }
                            if(*pattern == ']')
                            {
                                ++pattern;
                            }
                            goto cont;
                        }
                    }
                    while(*pattern != ']' && *pattern != '\0');
                }
            }
            // couldn't match this character
            return false;

        case '/':
        case '\\':
            // the \ may happen under MS-Windows
            if(*filename != '/' && *filename != '\\')
            {
                return false;
            }
            ++filename;
            ++pattern;
            break;

        default:
            // direct comparison
            if(*filename != *pattern)
            {
                return false;
            }
            ++filename;
            ++pattern;
            break;

        }
cont:;
    }

    return *filename == '\0';
}

/** \brief Append a path to this URI filename.
 *
 * This function adds a path to the existing path and returns the
 * resulting URI. The path is appended before the query string and
 * anchor.
 *
 * The path is added as is. This function does not add or remove
 * slash characters (/). This function is often used to append an
 * extension to a filename.
 *
 * \param[in] path  The path to happen.
 *
 * \return The resulting URI.
 */
uri_filename uri_filename::append_path(const std::string& path) const
{
    uri_filename result(*this);

    result.f_path += path;

    // re-parse the result to refresh all the fields
    return result.full_path();
}

/** \brief Appended value to the path.
 *
 * This function adds the specified value to the path
 *
 * \param[in] value  The value to append.
 *
 * \return The resulting URI.
 */
uri_filename uri_filename::append_path(int value) const
{
    std::stringstream stream;
    stream << value;
    return append_path(stream.str());
}

/** \brief Append a filename or directory to a path.
 *
 * This function adds a filename to an existing path. The function is
 * optimized to avoid creating paths such as "./name" from "." and "name".
 *
 * The function also makes sure not to double the slash character between
 * the path and child parameter strings. Actually, if \p path ends with
 * one or more slash and \p child starts with one or more slash, the
 * result is still one single slash between path and child.
 *
 * \note
 * The input parameters are expected to already be canonicalized.
 *
 * \warning
 * If the \p child parameter is a path specified by the user or coming from
 * a package, configuration file, etc. then you should instead call the
 * append_safe_child() so \p child gets cleaned up first so it does not
 * include any ".." that would otherwise place the resulting file outside
 * of this uri_filename directory. For example:
 *
 * \code
 * wpkg_filename::uri_filename test("/this/directory");
 * test.append_child("../../or/there/file.txt");
 * \endcode
 *
 * \par
 * This will add the file.txt under /or/there/... instead of the
 * expected /this/directory/... whereas calling append_safe_child()
 * will remove the ".." entries and the file ends up under
 * /this/directory/or/there/... which may not be correct but is
 * more correct and safer than just /or/there/...
 *
 * \param[in] child  The filename or path to append to \p path.
 *
 * \return The concatenated path and child with exactly one slash in between.
 *
 * \sa append_safe_child()
 */
uri_filename uri_filename::append_child(const std::string& child) const
{
    uri_filename result(*this);

    if(result.f_path.empty() || result.f_path == ".")
    {
        // when path is empty, we have to return child as is to avoid
        // returning "/" + child (which is what the rest of the function
        // would otherwise do!)
        // also "./" + child is not useful so we don't do it either
        // re-parse the result to refresh all the fields
        return child;
    }
    std::string::size_type p(result.f_path.length());
    while(p > 0)
    {
        if(result.f_path[p - 1] != '/')
        {
            break;
        }
        --p;
    }
    std::string::size_type max(child.length());
    std::string::size_type c(0);
    while(c < max)
    {
        if(child[c] != '/')
        {
            break;
        }
        ++c;
    }
    result.f_path = result.f_path.substr(0, p) + "/" + child.substr(c);

    // re-parse the result to refresh all the fields
    return result.full_path();
}

/** \brief Prepend a path to this uri_filename and return the result.
 *
 * This function ensures that this uri_filename is a safe path to
 * have the \p path parameter prepended to it without the risk of
 * the path ending up outside of the input \p path directory.
 *
 * This means the f_path parameter of the uri_filename cannot
 * include a ".." that would result in the file ending up outside
 * of the directory indicated by the input \p path parameter.
 *
 * The function actually removes all the "." and ".." from this
 * uri_filename path before appending this to \p path with the
 * append_child() function:
 *
 * \code
 * result = *this;
 * // fix the content of result
 * ...
 * return path.append_child(result);
 * \endcode
 *
 * \param[in] path  The path to prepend to this uri_filename.
 *
 * \sa append_child()
 */
uri_filename uri_filename::append_safe_child(const uri_filename& child) const
{
    uri_filename result(child);

    // check each segment and remove the "." and ".." entries
    for(path_parts_t::size_type i(0); i < result.f_segments.size(); ++i)
    {
        if(result.f_segments[i] == ".")
        {
            result.f_segments.erase(result.f_segments.begin() + i);
            --i;
        }
        else if(result.f_segments[i] == "..")
        {
            // IMPORTANT NOTE: dpkg actually removes ALL parents,
            //                 not just the previous level like us here

            // first remove the ".."
            result.f_segments.erase(result.f_segments.begin() + i);
            --i;
            if(static_cast<int>(i) >= 0)
            {
                // got a parent, remove it too
                result.f_segments.erase(result.f_segments.begin() + i);
                --i;
            }
        }
    }
    // the following loop forces a "/" at the start of the path
    result.f_path = "";
    for(path_parts_t::size_type i(0); i < result.f_segments.size(); ++i)
    {
        result.f_path += "/";
        result.f_path += result.f_segments[i];
    }
    if(f_path.length() > 0 && f_path[0] != '/')
    {
        // if filename didn't start with "/" then remove that from 'result'
        result.f_path.erase(0, 1);
    }

    // re-parse the result so we can refresh all the fields
    // and then append to the path parameter
    return append_child(result.full_path());
}

/** \brief Remove all the common segments.
 *
 * This function compares all the segments of this filename and
 * the \p common_segments filename. It returns a copy of this
 * uri_filename without those segments.
 *
 * The result is a relative path if any segment matched, otherwise
 * it is equal to this uri_filename. Note that if only the drive
 * letters match, then the result may be an absolute path.
 *
 * The drive letter is considered to be a segment and if both filenames
 * have different drive names then no segments are removed.
 *
 * \param[in] common_segments  The segments to compare against.
 *
 * \return A new uri_filename without these common segments.
 */
uri_filename uri_filename::remove_common_segments(const uri_filename& common_segments) const
{
    // TODO: Should we check whether the filename is a direct filename?
    //       Because if not then the drive may not be the very first
    //       segment so we would have to test scheme, domain name,
    //       share path first!

    uri_filename result(*this);

    // we cannot remove anything if any of these are not all equal
    if(f_scheme   != common_segments.f_scheme
    || f_domain   != common_segments.f_domain
    || f_username != common_segments.f_username
    || f_password != common_segments.f_password
    || f_port     != common_segments.f_port
    || f_drive    != common_segments.f_drive)
    {
        return result;
    }

    // the path becomes relative if it had a domain attached
    // and thus we change the scheme to "file" because that's
    // the only scheme that supports relative paths
    result.f_scheme = "file";
    result.f_domain = "";
    result.f_username = "";
    result.f_password = "";
    result.f_port = "";
    result.f_drive = drive_t();

    path_parts_t::size_type i(0);
    for(; i < f_segments.size() && i < common_segments.f_segments.size(); ++i)
    {
        if(f_segments[i] != common_segments.f_segments[i])
        {
            break;
        }
        result.f_segments.erase(result.f_segments.begin());
    }
    if(i > 0)
    {
        // we changed the path
        result.f_path = "";
        for(path_parts_t::size_type j(0); j < result.f_segments.size(); ++j)
        {
            if(j != 0)
            {
                result.f_path += "/";
            }
            result.f_path += result.f_segments[j];
        }
    }

    return result.full_path();
}

/** \brief Make the input path a relative path.
 *
 * This function removes the '/' at the beginning of this uri_filename
 * and returns a new uri_filename which is considered relative.
 *
 * If the file is already relative, then this function simply returns
 * a copy of this uri_filename.
 *
 * \return A relative uri_filename.
 */
uri_filename uri_filename::relative_path() const
{
    uri_filename result(*this);

    if(result.f_path.length() > 0 && result.f_path[0] == '/')
    {
        result.f_path.erase(0, 1);
    }

    // re-parse the whole thing to define all the fields as required
    return result.full_path();
}

/** \brief Return a valid string to access the file.
 *
 * This function returns a valid filename to access this uri_filename.
 * This function only works with direct paths (i.e. is_direct() returns
 * true.)
 *
 * Since the returned value is expected to be used with a function
 * such as fstream::open(), stat(), or realpath(), it simply returns
 * a string.
 *
 * This function returns an os_filename_t object so it is easy to either
 * make use of UTF-16 or UTF-8.
 *
 * \return A string representing the name of this direct file.
 */
uri_filename::os_filename_t uri_filename::os_filename() const
{
    if(!is_direct())
    {
        throw wpkg_filename_exception_parameter("filename \"" + f_original + "\" is not a direct filename, os_filename() cannot work on such");
    }

    if(f_scheme == uri_scheme_file)
    {
        std::string filename(drive_subst(f_drive, is_absolute()) + f_path);
#if defined(MO_WINDOWS)
        // under MS-Windows very long filenames must be defined using
        // the \\?\... syntax
        wpkg_filename::uri_filename cwd;
        if(!is_absolute())
        {
            cwd = get_cwd();
        }

        if(cwd.full_path().length() + filename.length() > 245)
        {
            // os_real_path() calls os_filename() so we cannot call
            // it or we get an infinite loop; unfortunately this
            // means the path may not work if too long and not
            // absolute... (hence the test in the previous if().)
            //filename = os_real_path();

            if(!is_absolute())
            {
                // prepend the cwd to the filename
                filename = cwd.append_child(filename).full_path();
            }

            std::string result = "\\\\?";

            // the following is true when the filename starts with a drive
            // letter whether or not it is otherwise absolute
            if(filename[0] != '/' && filename[0] != '\\')
            {
                result += "\\";
            }

            // in this case we must return a string
            // with only \ characters so it works right
            for(const char *s(filename.c_str()); *s != '\0'; ++s)
            {
                if(*s == '/')
                {
                    result += "\\";
                }
                else
                {
                    result += *s;
                }
            }
            return result;
        }
        // filename is small enough, return as is
#endif
        return filename;
    }

#if defined(MO_WINDOWS)
    // smb or smbs
    if(f_scheme == uri_scheme_smb || f_scheme == uri_scheme_smbs)
    {
        // generate the network connection path (\\domain@param\share) to connect
        std::string result("\\\\");
        result += f_domain;
        if(f_scheme == uri_scheme_smbs)
        {
            result += "@SLL";
        }
        if(!f_port.empty())
        {
            result += "@" + f_port;
        }
        result += "\\" + f_share;

        typedef std::map<std::string, time_t> connection_t;
        static connection_t connections;
        // TODO? should we check time and if more than X min. since last
        //       access reconnect? (we could also check whether the drive
        //       is still connected)
        if(connections[result] == 0)
        {
            // connection not made yet
            // Documentation:
            // http://msdn.microsoft.com/en-us/library/aa385413(v=vs.85).aspx
            NETRESOURCEA net_resources;
            memset(&net_resources, 0, sizeof(net_resources));
            net_resources.dwType = RESOURCETYPE_DISK;
            net_resources.lpRemoteName = const_cast<char *>(result.c_str());
            DWORD flags(CONNECT_TEMPORARY);
            switch(g_interactive)
            {
            case wpkgar_interactive_mode_no_interactions:
                // the default is to fail without interaction
                break;

            case wpkgar_interactive_mode_console:
                flags = CONNECT_INTERACTIVE | CONNECT_COMMANDLINE;
                break;

            case wpkgar_interactive_mode_gui:
                flags = CONNECT_INTERACTIVE;
                break;

            }
            DWORD r(WNetAddConnection2A(&net_resources,
                                        f_password.empty() ? NULL : f_password.c_str(),
                                        f_username.empty() ? NULL : f_username.c_str(),
                                        flags));
            if(r != NO_ERROR)
            {
                // TODO: use FormatMessage() to generate the error message from Win32 error codes
                std::string msg;
                std::stringstream stream;
                stream << r;
                switch(r)
                {
                case ERROR_BAD_NETPATH:
                    msg = "invalid network path";
                    break;

                case ERROR_INVALID_PASSWORD:
                    msg = "invalid password";
                    break;

                default:
                    msg = "invalid credentials or path?";
                    break;

                }
                throw wpkg_filename_exception_parameter("could not connect to \"" + result + "\" (from \"" + f_original + "\"); error: " + stream.str() + " (" + msg + ")");
            }
            connections[result] = time(NULL);
        }

        // outside functions will work with "/" instead of "\"
        result = "//" + f_domain;
        if(f_scheme == uri_scheme_smbs)
        {
            result += "@SLL";
        }
        if(!f_port.empty())
        {
            result += "@" + f_port;
        }
        result += "/" + f_share;
        result += f_path;
        // the anchor, query string do not apply
        return result;
    }
#endif

    throw wpkg_filename_exception_parameter("scheme \"" + f_scheme + "\" in \"" + f_original + "\" is not compatible with os_filename()");
}

/** \brief Retrieve the real path.
 *
 * This function checks a file path and returns the real path
 * to the file. This is useful to ensures a full path (absolute)
 * whenever managing some directory paths such as the one specified
 * with the --root path.
 *
 * If the filename has a scheme other than "file" this function
 * returns a copy of this uri_filename.
 *
 * \return A uri_filename representing the real path of this uri_filename.
 */
uri_filename uri_filename::os_real_path() const
{
    // the system realpath() function returns an error on an empty string
    // when in many cases we consider an empty path as a special case so
    // we do not want to convert it to getcwd().
    if(f_path.empty() || f_scheme != uri_scheme_file)
    {
        // if not just a filename then this uri_filename
        // it considered a real path
        return *this;
    }
    if(!f_real_path.empty())
    {
        // return the path we already computed
        return f_real_path;
    }

    // get the OS specific filename
    os_filename_t path(os_filename());
    uri_filename result;

#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
    // in most cases PATH_MAX will be enough
    // (i.e. remember that we define PATH_MAX as an equivalent to MAX_PATH)
    wchar_t wreal_path[PATH_MAX + 1];
    wreal_path[PATH_MAX] = '\0';
    DWORD sz(GetFullPathNameW(path.get_utf16().c_str(), PATH_MAX, wreal_path, NULL));
    if(sz >= PATH_MAX)
    {
        std::vector<wchar_t> long_path;
        long_path.resize(sz + 1);
        sz = GetFullPathNameW(path.get_utf16().c_str(), sz + 1, &long_path[0], NULL);
        if(sz != 0)
        {
            long_path[sz] = '\0';
            f_real_path = libutf8::wcstombs(&long_path[0]);
        }
    }
    else
    {
        f_real_path = libutf8::wcstombs(wreal_path);
    }
    // note that if the path is empty we exit early so sz == 0 is not
    // possible here unless an error occurs
    if(sz == 0)
    {
        throw wpkg_filename_exception_io("could not determine the real path of \"" + path.get_utf8() + "\" (\"" + f_original + "\")");
    }
#else
    // IMPORTANT NOTE: Under Linux the documentation clearly says that the
    //                 path returned by realpath() is absolute; on other
    //                 Unix systems the result may not be absolute... which
    //                 is a problem here!
    //
    // the POSIX realpath() accepts NULL as the 2nd parameter since 2008
    char *p(realpath(path.get_os_string().c_str(), NULL));
    if(p == NULL)
    {
        throw wpkg_filename_exception_io("could not determine the real path of \"" + path.get_utf8() + "\" (\"" + f_original + "\")");
    }
    try
    {
        f_real_path = p;
        free(p);
    }
    catch(const std::bad_alloc&)
    {
        free(p);
        throw;
    }
#endif
    result.set_filename(f_real_path);
    if(!result.is_absolute())
    {
        throw wpkg_filename_exception_io("system realpath() returned a non-absolute path: \"" + f_real_path + "\" for \"" + path.get_utf8() + "\" (\"" + f_original + "\")");
    }
    return result;
}

/** \brief stat() a file.
 *
 * This function runs a stat() that works whatever the filename. It
 * will work with NetBIOS paths (also called Network paths) even though
 * we canonicalize paths.
 *
 * Note that paths that are neither a direct filename or a NetBIOS path
 * fail and the function returns false.
 *
 * The \p s out parameter does not get set when the stat() call
 * returns an error.
 *
 * \note
 * On success this call saves the stat() call result in a cache which is kept
 * until the next reset() or set_filename() call. This means you can continue
 * to call this function even after the file was modified or even deleted and
 * it successfully returns a copy of the stat() from the first successful
 * call.
 *
 * \param[out] s  The stat() call results, unchanged if the function returns -1.
 *
 * \return 0 if the call succeeds, -1 on error (i.e. the same as the stat(2) call)
 */
int uri_filename::os_stat(file_stat& s) const
{
    if(!f_stat.is_valid())
    {
        if(!is_direct())
        {
            // not a direct filename so we cannot call os_filename()
            return false;
        }

        f_stat.reset();
        os_filename_t cname(os_filename());
//::stat(cname.c_str(), &f_stat);
//::fprintf(stderr, "really stating [%s] from [%s] %d (%d/%d)\n", cname.c_str(), f_original.c_str(), (f_stat.st_mode & S_IFMT), S_IFDIR, S_IFREG);
#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
        // to fully support long filenames we have to use the actual
        // Windows API and not the Unix like stat() function which it
        // seems is limited to only 260 characters or so
        wpkg_compatibility::standard_handle_t h(CreateFileW(
                cname.get_utf16().c_str(),
                0,
                FILE_SHARE_READ,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_BACKUP_SEMANTICS,
                NULL));
        if(h == INVALID_HANDLE_VALUE)
        {
            switch(GetLastError())
            {
            case 2: // The system cannot find the file specified
            case 3: // The system cannot find the path specified
                errno = ENOENT;
                break;

            case 5: // Access is denied
                errno = EPERM;
                break;

            default:
                // this is like an "unknown error"
                errno = EINVAL;
                break;

            }
        }
        else
        {
            // the file exists, query its information
            BY_HANDLE_FILE_INFORMATION info;
            if(GetFileInformationByHandle(h, &info))
            {
                f_stat.set_dev(info.dwVolumeSerialNumber);
                f_stat.set_inode(static_cast<uint64_t>(info.nFileIndexLow) | (static_cast<uint64_t>(info.nFileIndexHigh) << 32));
                if((info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
                {
                    // completely ignore the READ ONLY flag in this case
                    f_stat.set_mode(S_IFDIR | 0777);
                }
                else
                {
                    uint32_t mode = (info.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0 ? 0444 : 0666;
                    if(f_extension == "bat"
                    || f_extension == "com"
                    || f_extension == "dll"
                    || f_extension == "exe"
                    || f_extension == "ocx")
                    {
                        // these are executables
                        mode |= 0111;
                    }
                    f_stat.set_mode(S_IFREG | mode);
                }
                f_stat.set_nlink(info.nNumberOfLinks);
                f_stat.set_uid(0);
                f_stat.set_gid(0);
                f_stat.set_rdev(0);
                f_stat.set_size(static_cast<uint64_t>(info.nFileSizeLow) | (static_cast<uint64_t>(info.nFileSizeHigh) << 32));
                f_stat.set_atime(windows_filetime_to_unix_time_seconds(info.ftLastAccessTime), windows_filetime_to_unix_time_nanoseconds(info.ftLastAccessTime));
                f_stat.set_mtime(windows_filetime_to_unix_time_seconds(info.ftLastWriteTime),  windows_filetime_to_unix_time_nanoseconds(info.ftLastWriteTime));
                f_stat.set_ctime(f_stat.get_mtime(), f_stat.get_mtime_nano());

                if(!f_stat.is_dir())
                {
                    FILE_BASIC_INFO basic_info;
                    if(GetFileInformationByHandleEx(h, FileBasicInfo, &basic_info, sizeof(basic_info)))
                    {
                        f_stat.set_ctime(windows_filetime_to_unix_time_seconds(basic_info.ChangeTime), windows_filetime_to_unix_time_nanoseconds(basic_info.ChangeTime));
                    }
                }

                f_stat.set_valid();
            }
        }
#else
#ifdef MO_FREEBSD
        // stat is 64 bits by itself
        struct stat st;
        int r(::stat(cname.get_utf8().c_str(), &st));
#else
        struct stat64 st;
        int r(::stat64(cname.get_utf8().c_str(), &st));
#endif
        if(r == 0)
        {
            f_stat.set_dev(st.st_dev);
            f_stat.set_inode(st.st_ino);
            f_stat.set_mode(st.st_mode);
            f_stat.set_nlink(st.st_nlink);
            f_stat.set_uid(st.st_uid);
            f_stat.set_gid(st.st_gid);
            f_stat.set_rdev(st.st_rdev);
            f_stat.set_size(st.st_size);
//#if defined(__USE_MISC) || defined(__USE_XOPEN2K8)
//            f_stat.set_atime(st.st_atime, st.st_atimensec);
//            f_stat.set_mtime(st.st_mtime, st.st_mtimensec);
//            f_stat.set_ctime(st.st_ctime, st.st_ctimensec);
//#elif defined(_BSD_SOURCE) 
//   || defined(_SVID_SOURCE) 
//   || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200809L) 
//   || (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 700)
//            f_stat.set_atime(st.st_atime, st_atim.tv_nsec);
//            f_stat.set_mtime(st.st_mtime, st_mtim.tv_nsec);
//            f_stat.set_ctime(st.st_ctime, st_ctim.tv_nsec);
//#else
//            // no known nano seconds
            f_stat.set_atime(st.st_atime, 0);
            f_stat.set_mtime(st.st_mtime, 0);
            f_stat.set_ctime(st.st_ctime, 0);
//#endif
            f_stat.set_valid();
        }
#endif
        if(!f_stat.is_valid())
        {
            return -1;
        }
    }
    s = f_stat;
    return 0;
}

/** \brief lstat() a file.
 *
 * This function runs an lstat() that works whatever the filename.
 * When called on MS-Windows, it just calls stat() since lstat() does not
 * exist.
 *
 * \note
 * This call does not cache the result.
 *
 * \param[out] st  The stat() call results.
 *
 * \return 0 if the call succeeds, -1 on error (i.e. the same as the stat(2) call)
 */
int uri_filename::os_lstat(file_stat& s) const
{
#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
    return os_stat(s);
#else
    os_filename_t cname(os_filename());
//::stat(cname.c_str(), st);
//::fprintf(stderr, "really l-stating [%s] from [%s] %d (%d/%d)\n", cname.c_str(), f_original.c_str(), (st->st_mode & S_IFMT), S_IFDIR, S_IFREG);
#ifdef MO_FREEBSD
    // stat is 64 bits by itself
    struct stat st;
    int r(::stat(cname.get_utf8().c_str(), &st));
#else
    struct stat64 st;
    int r(::lstat64(cname.get_utf8().c_str(), &st));
#endif
    if(r == 0)
    {
        s.set_dev(st.st_dev);
        s.set_inode(st.st_ino);
        s.set_mode(st.st_mode);
        s.set_nlink(st.st_nlink);
        s.set_uid(st.st_uid);
        s.set_gid(st.st_gid);
        s.set_rdev(st.st_rdev);
        s.set_size(st.st_size);
//#if defined(__USE_MISC) || defined(__USE_XOPEN2K8)
//            s.set_atime(st.st_atime, st.st_atimensec);
//            s.set_mtime(st.st_mtime, st.st_mtimensec);
//            s.set_ctime(st.st_ctime, st.st_ctimensec);
//#elif defined(_BSD_SOURCE) 
//   || defined(_SVID_SOURCE) 
//   || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200809L) 
//   || (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 700)
//            s.set_atime(st.st_atime, st_atim.tv_nsec);
//            s.set_mtime(st.st_mtime, st_mtim.tv_nsec);
//            s.set_ctime(st.st_ctime, st_ctim.tv_nsec);
//#else
//            // no known nano seconds
        s.set_atime(static_cast<int64_t>(st.st_atime));
        s.set_mtime(static_cast<int64_t>(st.st_mtime));
        s.set_ctime(static_cast<int64_t>(st.st_ctime));
//#endif
        s.set_valid();
    }
    return s.is_valid() ? 0 : -1;
#endif
}

/** \brief Create a directory and all of its parents.
 *
 * This function is used to make a directory from this uri_filename. If
 * the file specifies a remote file, it is likely to fail (read-write
 * permissions missing.)
 *
 * Note that the function ignores the cached os_stat() information and
 * check the validity of each segment of the path anew.
 *
 * To create the directory from a full path filename you may use:
 *
 * \code
 * uri_filename dir(file.dirname());
 * dir.os_mkdir_p();
 * \endcode
 *
 * \note
 * Although the function creates directories, in regard to the
 * uri_filename object itself it is constant.
 *
 * \param[in] mode  The mode used to create the directories.
 */
void uri_filename::os_mkdir_p(int mode) const
{
    uri_filename path(*this);

    for(path_parts_t::size_type i(0); i < f_segments.size(); ++i)
    {
        path.f_path = "";
        for(path_parts_t::size_type j(0); j <= i; ++j)
        {
            if(j != 0 || f_path[0] == '/')
            {
                path.f_path += "/";
            }
            path.f_path += f_segments[j];
        }
        // no need to test the root path
        if(path.f_path != "/")
        {
            path.f_stat.set_valid(false);
            if(path.exists())
            {
                if(!path.is_dir())
                {
                    throw wpkg_filename_exception_compatibility("expected \"" + path.f_path + "\" to be a directory, found another file type instead");
                }
                // it is already there, we're good
            }
            else if(errno == ENOENT)
            {
                // this file does not exist, create the directory!
                os_filename_t p(path.os_filename());
#if defined(MO_WINDOWS)
                if(CreateDirectoryW(p.get_utf16().c_str(), NULL) == 0)
#else
                if(mkdir(p.get_os_string().c_str(), mode) != 0)
#endif
                {
                    throw wpkg_filename_exception_io("could not create directory \"" + path.f_path + "\" (" + p.get_utf8() + ")");
                }
            }
            else
            {
                throw wpkg_filename_exception_io("an error occured while in mkdir_p(\"" + path.f_path + "\")");
            }
        }
    }
}

/** \brief Remove this file from the disk.
 *
 * This function removes the file from the disk. It will throw if the
 * unlink() call fails. Note that if the file doesn't exist, then no
 * error is generated and the function returns silently.
 *
 * \note
 * This function automatically clears the cache of this URI filename object
 * so functions such as os_stat(), exists(), and os_real_path() work as
 * expected.
 *
 * \exception wpkg_filename_exception_io
 * This exception is raised when the file cannot be removed and that
 * happens for a reason other than the fact that the file did not exist
 * in the first place (i.e. ENOENT.)
 *
 * \return true if the file was removed, false if the file did not exist.
 */
bool uri_filename::os_unlink() const
{
    bool result(true);

    if(unlink(os_filename().get_os_string().c_str()) != 0)
    {
        result = false;

        // this is an error only if the file exists and cannot be deleted
        if(errno != ENOENT)
        {
            throw wpkg_filename_exception_io("file \"" + f_original + "\" could not be removed");
        }
    }

    // clear the cache since we know that the source file is now gone
    const_cast<uri_filename *>(this)->clear_cache();

    return result;
}

/** \brief Remove this directory from the disk.
 *
 * This function removes all the files in the specified directory
 * and then the directory itself. The function is recursive so if
 * the directory includes a sub-directory, it will also get deleted.
 *
 * Note that if the filename is just a file, then just that file is
 * removed.
 *
 * The rf in the function name comes from the options of the rm command
 * line as in:
 *
 * \code
 * rm -rf <filename>
 * \endcode
 *
 * The dryrun parameter should be used to ensure that you call the
 * function with the correct path. Once that was tested, remove
 * the parameter from your call or set it to false. At that point
 * the files get deleted for real.
 *
 * \exception wpkg_filename_exception_io
 * This exception is raised when the file cannot be removed and that
 * happens for a reason other than the fact that the file did not exist
 * in the first place (i.e. ENOENT.)
 *
 * \param[in] dryrun  Whether to only pretend that the files get removed.
 *
 * \return true if the file was removed, false if the file did not exist.
 */
bool uri_filename::os_unlink_rf(bool dryrun) const
{
    bool result(true);

    struct
    {
        int do_unlink(const wpkg_filename::uri_filename& unlink_filename, bool dryrun_flag)
        {
            int r;
            if(dryrun_flag)
            {
                r = unlink_filename.is_dir() ? -1 : 0;
                if(r == -1)
                {
                    f_errno = ENOTEMPTY;
                }
                fprintf(stderr, "uri_filename::os_unlink_rf(\"%s\", true); -> %d\n", unlink_filename.original_filename().c_str(), r);
            }
            else
            {
                // the real thing!
                r = unlink(unlink_filename.os_filename().get_os_string().c_str());
                f_errno = errno;
                // Under MS-Windows we may get an EACCESS error instead of EISDIR
                if(r != 0 && (f_errno == EISDIR || f_errno == EACCES))
                {
                    // we need an rmdir() for a directory
                    r = rmdir(unlink_filename.os_filename().get_os_string().c_str());
                    f_errno = errno;
                }
            }
            return r;
        }
        int f_errno;
    } u;

    int r(u.do_unlink(*this, dryrun));
    if(r != 0)
    {
        // MS-Windows may return EACCESS instead of ENOTEMPTY
        if(is_dir() && (u.f_errno == ENOTEMPTY || u.f_errno == EACCES))
        {
            {
                os_dir dir(*this);
                uri_filename sub_filename;
                while(dir.read(sub_filename))
                {
                    const std::string p(sub_filename.basename());
                    if(p != "." && p != "..")
                    {
                        if(!sub_filename.os_unlink_rf(dryrun))
                        {
                            return false;
                        }
                    }
                }
            }
            // try it again
            r = u.do_unlink(*this, dryrun);
#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
            if(!dryrun && r != 0 && (u.f_errno == ENOTEMPTY || u.f_errno == EACCES))
            {
                // under MS-Windows the closing of the directory may
                // take time...
                Sleep(200);
                r = u.do_unlink(*this, dryrun);
            }
#endif
            // MS-Windows may return EACCESS instead of ENOTEMPTY
            if(dryrun && r == -1 && (u.f_errno == ENOTEMPTY || u.f_errno == EACCES))
            {
                // pretend that it worked
                r = 0;
            }
        }

        // on the second time, ENOTEMPTY is an error
        // (this can happen if another process creates a file in the directory
        // before we have time to remove it.)
        if(r != 0)
        {
            result = false;

            // this is an error only if the file exists and cannot be deleted
            if(u.f_errno != ENOENT)
            {
                throw wpkg_filename_exception_io("file \"" + original_filename() + "\" could not be removed");
            }
        }
    }

    // clear the cache since we know that the source file is now gone
    const_cast<uri_filename *>(this)->clear_cache();

    return result;
}

/** \brief Create a symbolic link.
 *
 * This function creates a symbolic link which name is this uri_filename
 * and destination is the specified uri_filename.
 *
 * \param[in] destination  The destination of the symbolic link.
 */
void uri_filename::os_symlink(const uri_filename& destination) const
{
    int retval(symlink(os_filename().get_os_string().c_str(), destination.os_filename().get_os_string().c_str()));
    if(retval == -1)
    {
        // if the link already exists, it doesn't get overwritten by default
        if(errno == EEXIST)
        {
            destination.os_unlink();
            retval = symlink(os_filename().get_os_string().c_str(), destination.os_filename().get_os_string().c_str());
        }
        if(retval == -1)
        {
            throw memfile::memfile_exception_io("I/O error creating soft-link \"" + f_original + " -> " + destination.f_original + "\"");
        }
    }
}

/** \brief Rename a file.
 *
 * This function renames this file with the specified destination filename.
 * If the rename fails, the function throws an error.
 *
 * \note
 * This function automatically clears the cache of this URI filename object
 * so functions such as os_stat(), exists(), and os_real_path() work as
 * expected.
 *
 * \warning
 * This URI filename object is NOT modified so the filename in this URI
 * filename is the old name.
 *
 * \exception wpkg_filename_exception_io
 * The filename exception is raised if the file could not be renamed and
 * the \p ignore_errors parameter was not set to true.
 *
 * \param[in] destination  The new name for this file.
 * \param[in] ignore_errors  Whether errors are reported with a throw.
 *
 * \return true on success, false otherwise, but only if ignore_errors is true
 */
bool uri_filename::os_rename(const uri_filename& destination, bool ignore_errors) const
{
#if defined(MO_MINGW32)
    if(_wrename(os_filename().get_os_string().c_str(), destination.os_filename().get_os_string().c_str()) != 0)
#else
    if(rename(os_filename().get_os_string().c_str(), destination.os_filename().get_os_string().c_str()) != 0)
#endif
    {
        if(!ignore_errors)
        {
            return false;
        }
        throw wpkg_filename_exception_io("file \"" + f_original + "\" could not be renamed \"" + destination.f_original + "\"");
    }

    // clear the cache since we know that the source file is now gone
    const_cast<uri_filename *>(this)->clear_cache();

    return true;
}


/** \brief Compare two filenames for equality.
 *
 * This function compares the two uri_filename's for equality.
 *
 * The function uses the full_path() to compare the two filenames.
 *
 * \param[in] rhs  The right hand side.
 */
bool uri_filename::operator == (const uri_filename& rhs) const
{
    return full_path() == rhs.full_path();
}


/** \brief Compare two filenames for inequality.
 *
 * This function compares the two uri_filename's for inequality.
 *
 * The function uses the full_path() to compare the two filenames.
 *
 * \param[in] rhs  The right hand side.
 */
bool uri_filename::operator != (const uri_filename& rhs) const
{
    return full_path() != rhs.full_path();
}


/** \brief Compare two filenames for inequality.
 *
 * This function compares the two uri_filename's for inequality.
 *
 * The function uses the full_path() to compare the two filenames.
 *
 * \param[in] rhs  The right hand side.
 */
bool uri_filename::operator < (const uri_filename& rhs) const
{
    return full_path() < rhs.full_path();
}


/** \brief Compare two filenames for inequality.
 *
 * This function compares the two uri_filename's for inequality.
 *
 * The function uses the full_path() to compare the two filenames.
 *
 * \param[in] rhs  The right hand side.
 */
bool uri_filename::operator <= (const uri_filename& rhs) const
{
    return full_path() <= rhs.full_path();
}


/** \brief Compare two filenames for inequality.
 *
 * This function compares the two uri_filename's for inequality.
 *
 * The function uses the full_path() to compare the two filenames.
 *
 * \param[in] rhs  The right hand side.
 */
bool uri_filename::operator > (const uri_filename& rhs) const
{
    return full_path() > rhs.full_path();
}


/** \brief Compare two filenames for inequality.
 *
 * This function compares the two uri_filename's for inequality.
 *
 * The function uses the full_path() to compare the two filenames.
 *
 * \param[in] rhs  The right hand side.
 */
bool uri_filename::operator >= (const uri_filename& rhs) const
{
    return full_path() >= rhs.full_path();
}


/** \brief Set the interactive mode to use when credentials are required.
 *
 * This function sets the interactive mode of the run of the system. It
 * can be changed at a later time, although it should really only be set
 * once early on.
 *
 * Other processes can query the current mode with the get_interactive()
 * function.
 *
 * \param[in] mode  The interactive mode to use with the system.
 */
void uri_filename::set_interactive(interactive_mode_t mode)
{
    g_interactive = mode;
}

/** \brief Retrieve the current interactive mode.
 *
 * This function is used to query the current interactive mode of the
 * run. This can be set to one of the interactive_mode_t values.
 *
 * \return Return the current interactive mode of this run.
 */
uri_filename::interactive_mode_t uri_filename::get_interactive()
{
    return g_interactive;
}

namespace
{

/** \brief User defined path to temporary directory.
 *
 * This variable holds the temporary directory path as defined by the
 * user. The user is expected to specify a temporary directory with
 * the --tmpdir command line option (in wpkg) which sets this variable.
 *
 * It is not required to set this variable as a default will always be
 * provided. However, the default may be on a disc that is not large
 * enough or on which the user does not have write permission (?).
 */
std::string g_tmpdir_path;

/** \brief Temporary directory.
 *
 * When someone calls the uri_filename::tmpdir() function, this global
 * variable is used to save that directory. It stays available until
 * the software quits (i.e. it won't change once it is set.)
 *
 * At the time the software quits, it checks whether that directory was
 * created. If so, it gets deleted.
 */
wpkg_filename::temporary_uri_filename     g_tmpdir;

/** \brief Whether the temporary files should be deleted.
 *
 * If you use the debug flag debug_detail_files, then the temporary directory
 * is expected to be kept around. Note that the tool setting the debug flags
 * is expected to handle this keep temporary files flag by calling the
 * temporary_uri_filename::keep_files() function.
 */
bool g_keep_temporary_files = false;

} // no name namespace


/** \brief Retrieve a temporary directory.
 *
 * This function is used to define a temporary directory to be used to load
 * a package, create a build environment, and other similar things.
 *
 * The returned value is a URI filename.
 *
 * The directory is created by default, if you just want a path, set the
 * \p create parameter to false.
 *
 * The path looks something like this:
 *
 * \code
 * /tmp/wpkg-123/<sub-directory>
 * \endcode
 *
 * Where the 123 number is the process identifier of this process and the
 * \<sub-directory> path is the \p sub_directory parameter appended to the
 * main temporary directory of the library.
 *
 * \param[in] sub_directory  The name of the sub-directory to create in the
 *                           wpkg temporary directory.
 * \param[in] create  If true, create the directory before returning.
 *
 * \return The temporary directory path as a URI filename.
 */
uri_filename uri_filename::tmpdir(const std::string& sub_directory, bool create)
{
    if(g_tmpdir.empty())
    {
        std::string temp;
        if(g_tmpdir_path.empty())
        {
#ifdef MO_WINDOWS
            // TODO: by default we should probably have a "User Data" path + "\temp"
            const wchar_t *wtemp(_wgetenv(L"TEMP"));
            if(wtemp == NULL)
            {
                wtemp = _wgetenv(L"TMP");
            }
            if(wtemp == NULL)
            {
                // use the default for MS-Windows
                wtemp = L"C:\\WINDOWS\\Temp";
            }
            temp = libutf8::wcstombs(wtemp);
#else
            // TBD: should we look into $TEMP/$TMP too?
            // Is there a way to determine the temporary directory under Unix?
            temp = "/tmp";
#endif
        }
        else
        {
            temp = g_tmpdir_path;
        }

        // create the actual path
        std::stringstream ss;
        ss << "wpkg-" << getpid();
        uri_filename tmpdir_temp(temp);
        g_tmpdir = tmpdir_temp.os_real_path().append_child(ss.str());
    }

    uri_filename tmp(g_tmpdir);

    if(!sub_directory.empty())
    {
        tmp = tmp.append_child(sub_directory);
    }

    // make sure it exists
    if(create)
    {
        tmp.os_mkdir_p(0700);
    }

    return tmp;
}

/** \brief Get the current working directory.
 *
 * This function returns the current working directory in a uri_filename.
 * The path is expected to be absolute although it is not guaranteed.
 *
 * \note
 * The underscore in the name is to avoid #define problems under MS-Windows.
 * (i.e. #define getcwd _wgetcwd, for example)
 *
 * \return The current working directory.
 */
uri_filename uri_filename::get_cwd()
{
    wpkg_filename::uri_filename::os_filename_t cwd("...undefined folder...");
    wpkg_filename::uri_filename::os_char_t buf[32 * 1024]; // limit is 32Kb under MS-Windows
#if defined(MO_MINGW32) || defined(MO_MINGW64)
    if(GetCurrentDirectoryW(sizeof(buf) / sizeof(buf[0]), buf) != 0)
#else
    if(::getcwd(buf, sizeof(buf) / sizeof(buf[0])) != NULL)
#endif
    {
        cwd.reset(buf);
    }
    return cwd.get_utf8();
}



/** \brief Delete this temporary file on destruction.
 *
 * This destructor is used to remove a temporary file once we are done with
 * it. The file may be a directory in which case the directory and all of
 * its contents are removed.
 */
temporary_uri_filename::~temporary_uri_filename()
{
    if(!empty())
    {
        if(g_keep_temporary_files)
        {
            char buf[1024];
            time_t t;
            time(&t);
            struct tm *m(localtime(&t));
            // WARNING: remember that format needs to work on MS-Windows
            strftime_utf8(buf, sizeof(buf) - 1, "%Y/%m/%d %H:%M:%S", m);
            buf[sizeof(buf) / sizeof(buf[0]) - 1] = '\0';
            fprintf(stderr, "wpkg:info: %s: temporary files kept under \"%s\".\n", buf, original_filename().c_str());
        }
        else
        {
            os_unlink_rf();
        }
    }
}


/** \brief Copy a URI filename in a temporary URI filename.
 *
 * This function copies a URI filename in a temporary URI filename.
 * Note that this has a strong side effect:
 *
 * \li The existing temporary URI filename is lost and if a file was
 *     created with the old name, it will not be removed by this
 *     object destructor;
 *
 * \li The new filename will be deleted when the temporary URI
 *     filename function is called.
 *
 * Note that if you copy a temporary URI filename in another, then the
 * file will be deleted twice. The first time may be sooner than you'd
 * otherwise expect. You may want to consider using the std::swap() function
 * instead.
 *
 * \param[in] rhs  The source URI filename to copy.
 *
 * \return A reference to this temporary_uri_filename
 */
temporary_uri_filename& temporary_uri_filename::operator = (const uri_filename& rhs)
{
    if(this != &rhs)
    {
        uri_filename::operator = (rhs);
    }

    return *this;
}


/** \brief Defined user path to temporary directory.
 *
 * By default the temporary directory is dynamically assigned, in
 * most cases it is set to /tmp/wpkg-\<pid> under Unix. Under MS-Windows
 * it makes use of the $TEMP variable (or $TMP) instead of "/tmp" and
 * also makes use of wpkg-<pid> to make the directory unique per instance
 * of your tool.
 *
 * The temporary directory appends wpkg-\<pid> to this temporary
 * directory. SO if the user passes "/tmp", the directory will again
 * be "/tmp/wpkg-\<pid>". If the use passes a directory name such as
 * "~/tmp", then he will get something like "/home/alexis/tmp/wpkg-\<pid>".
 *
 * This variable can only be set if the g_tmpdir was not yet assigned
 * (i.e. in your tool, make sure to call the set_tmpdir() very early
 * on or you'll get an error.)
 *
 * The path must be valid, although it is not tested when set. It will be
 * made absolute when created to avoid potential problems (i.e. scripts
 * using cd and then the temporary path.)
 *
 * \param[in] tmpdir  The temporary directory path.
 */
void temporary_uri_filename::set_tmpdir(const std::string& tmpdir)
{
    if(!g_tmpdir.empty())
    {
        throw wpkg_filename_exception_compatibility("the temporary directory was already initialized, it cannot be changed with set_tmpdir() anymore");
    }

    g_tmpdir_path = tmpdir;
}


/** \brief Set whether temporary files should be deleted or not.
 *
 * This function changes the flag used to know whether temporary files
 * should be deleted (false) or not (true.)
 *
 * By default the flag is set to false so all temporary files do get
 * deleted when a temporary_uri_filename is destroyed.
 *
 * It is possible to change the flag to true so temporary files do not
 * get deleted which can be useful to a user who wants to review the
 * temporary files after a run of a tool to see that they are what is
 * expected.
 *
 * Note that the flag is global so all the temporary_uri_filename's are
 * affected by this call.
 *
 * \param[in] keep  Whether the files should be kept or not.
 */
void temporary_uri_filename::keep_files(bool keep)
{
    g_keep_temporary_files = keep;
}


/** \brief Initialize an os_filename_t object.
 *
 * This constructor marks the os_filename_t object as undefined.
 * If you call the get_utf8() or get_utf16() functions before you
 * call one of the reset() functions, an std::logic_error exception
 * is raised (i.e. the filename is not considered valid, opposed to
 * the filename is empty.)
 *
 * \param[in] filename  The default name for this os_filename_t.
 */
uri_filename::os_filename_t::os_filename_t()
{
}


/** \brief Initialize an os_filename_t object with a UTF-8 string.
 *
 * This function calls the corresponding reset() function to initialize
 * the filename with the UTF-8 input filename.
 *
 * \param[in] filename  The default name for this os_filename_t.
 */
uri_filename::os_filename_t::os_filename_t(const std::string& filename)
{
    reset(filename);
}


/** \brief Initialize an os_filename_t object with a UTF-16 string.
 *
 * This function calls the corresponding reset() function to initialize
 * the filename with the UTF-16 input filename.
 *
 * \param[in] filename  The default name for this os_filename_t.
 */
uri_filename::os_filename_t::os_filename_t(const std::wstring& filename)
{
    reset(filename);
}


/** \brief Set the filename using a UTF-8 string.
 *
 * This function accepts a string (std::string) to setup this
 * filename object as. The filename is automatically converted to
 * UTF-16 if the corresponding function is called.
 *
 * \param[in] filename  The filename to set this os_filename_t to.
 */
void uri_filename::os_filename_t::reset(const std::string& filename)
{
    f_format = filename_format_utf8;
    f_utf8_filename = filename;
}


/** \brief Set the filename using a UTF-16 string.
 *
 * This function accepts a wide string (std::wstring) to setup this
 * filename object as. The filename is automatically converted to
 * UTF-8 if the corresponding function is called.
 *
 * \param[in] filename  The filename to set this os_filename_t to.
 */
void uri_filename::os_filename_t::reset(const std::wstring& filename)
{
    f_format = filename_format_utf16;
    f_utf16_filename = filename;
}


/** \brief Return the filename as a UTF-8 string.
 *
 * This function returns the filename as a normal string (std::string) which
 * makes use of UTF-8 as the format. Note that UTF-8 is the default internal
 * format so in most cases you will get UTF-8 filenames everywhere except in
 * a very few places.
 *
 * If the filename was encoded as UTF-16 then the filename is automatically
 * converted to UTF-8.
 *
 * \exception std::logic_error
 * If the filename is still undefined then this exception is raised.
 *
 * \return The filename in UTF-8 format.
 */
std::string uri_filename::os_filename_t::get_utf8() const
{
    if(filename_format_undefined == f_format)
    {
        throw std::logic_error("this os_filename_t object was not defined");
    }

    if(filename_format_utf16 == f_format)
    {
        f_format = filename_format_both;
        f_utf8_filename = libutf8::wcstombs(f_utf16_filename);
    }

    return f_utf8_filename;
}


/** \brief Return the filename as a wide string.
 *
 * This function returns the filename as a wide string (std::wstring) which
 * makes use of UTF-16 as the format. This is to support MS-Windows. All other
 * file systems support UTF-8 so we otherwise have UTF-8.
 *
 * \exception std::logic_error
 * If the filename is still undefined then this exception is raised.
 *
 * \return The filename in UTF-16 format.
 */
std::wstring uri_filename::os_filename_t::get_utf16() const
{
    if(filename_format_undefined == f_format)
    {
        throw std::logic_error("this os_filename_t object was not defined");
    }

    if(filename_format_utf8 == f_format)
    {
        f_format = filename_format_both;
        f_utf16_filename = libutf8::mbstowcs(f_utf8_filename);
    }

    return f_utf16_filename;
}


/** \brief Get string in OS format.
 *
 * This function returns the string in OS format so we do not need to know
 * what that format is which is practical in many cases.
 *
 * \return The string in OS format (UTF-8 or UTF-16 depending on the OS.)
 */
uri_filename::os_string_t uri_filename::os_filename_t::get_os_string() const
{
#ifdef MO_WINDOWS
    return get_utf16();
#else
    return get_utf8();
#endif
}


bool uri_filename::file_stat::is_valid() const
{
    return f_valid;
}

void uri_filename::file_stat::reset()
{
    f_valid = false;
    f_dev = 0;
    f_inode = 0;
    f_mode = 0;
    f_nlink = 0;
    f_uid = 0;
    f_gid = 0;
    f_rdev = 0;
    f_size = 0;
    f_atime = 0;
    f_atime_nano = 0;
    f_mtime = 0;
    f_mtime_nano = 0;
    f_ctime = 0;
    f_ctime_nano = 0;
}

uint64_t uri_filename::file_stat::get_dev() const
{
    return f_dev;
}

uint64_t uri_filename::file_stat::get_inode() const
{
    return f_inode;
}

uint32_t uri_filename::file_stat::get_mode() const
{
    return f_mode;
}

bool uri_filename::file_stat::is_dir() const
{
    return (f_mode & S_IFMT) == S_IFDIR;
}

bool uri_filename::file_stat::is_reg() const
{
    return (f_mode & S_IFMT) == S_IFREG;
}

uint64_t uri_filename::file_stat::get_nlink() const
{
    return f_nlink;
}

uint32_t uri_filename::file_stat::get_uid() const
{
    return f_uid;
}

uint32_t uri_filename::file_stat::get_gid() const
{
    return f_gid;
}

uint64_t uri_filename::file_stat::get_rdev() const
{
    return f_rdev;
}

int64_t uri_filename::file_stat::get_size() const
{
    return f_size;
}

time_t uri_filename::file_stat::get_atime() const
{
    return f_atime;
}

uint64_t uri_filename::file_stat::get_atime_nano() const
{
    return f_atime_nano;
}

double uri_filename::file_stat::get_atime_dbl() const
{
    return static_cast<double>(f_atime) + static_cast<double>(f_atime_nano) / 1000000000.0;
}

time_t uri_filename::file_stat::get_mtime() const
{
    return f_mtime;
}

uint64_t uri_filename::file_stat::get_mtime_nano() const
{
    return f_mtime_nano;
}

double uri_filename::file_stat::get_mtime_dbl() const
{
    return static_cast<double>(f_mtime) + static_cast<double>(f_mtime_nano) / 1000000000.0;
}

time_t uri_filename::file_stat::get_ctime() const
{
    return f_ctime;
}

uint64_t uri_filename::file_stat::get_ctime_nano() const
{
    return f_ctime_nano;
}

double uri_filename::file_stat::get_ctime_dbl() const
{
    return static_cast<double>(f_ctime) + static_cast<double>(f_ctime_nano) / 1000000000.0;
}


void uri_filename::file_stat::set_valid(bool valid)
{
    f_valid = valid;
}

void uri_filename::file_stat::set_dev(uint64_t device)
{
    f_dev = device;
}

void uri_filename::file_stat::set_inode(uint64_t inode)
{
    f_inode = inode;
}

void uri_filename::file_stat::set_mode(uint32_t mode)
{
    f_mode = mode;
}

void uri_filename::file_stat::set_nlink(uint64_t nlink)
{
    f_nlink = nlink;
}

void uri_filename::file_stat::set_uid(int32_t uid)
{
    f_uid = uid;
}

void uri_filename::file_stat::set_gid(int32_t gid)
{
    f_gid = gid;
}

void uri_filename::file_stat::set_rdev(uint64_t rdev)
{
    f_rdev = rdev;
}

void uri_filename::file_stat::set_size(int64_t size)
{
    f_size = size;
}

void uri_filename::file_stat::set_atime(int64_t unix_time, int64_t nano)
{
    f_atime = unix_time;
    f_atime_nano = nano;
}

void uri_filename::file_stat::set_atime(double unix_time)
{
    f_atime = static_cast<int64_t>(unix_time);
    f_atime_nano = static_cast<uint64_t>((unix_time - static_cast<double>(f_atime)) * 1000000000);
}

void uri_filename::file_stat::set_mtime(int64_t unix_time, int64_t nano)
{
    f_mtime = unix_time;
    f_mtime_nano = nano;
}

void uri_filename::file_stat::set_mtime(double unix_time)
{
    f_mtime = static_cast<int64_t>(unix_time);
    f_mtime_nano = static_cast<uint64_t>((unix_time - static_cast<double>(f_mtime)) * 1000000000);
}

void uri_filename::file_stat::set_ctime(int64_t unix_time, int64_t nano)
{
    f_ctime = unix_time;
    f_ctime_nano = nano;
}

void uri_filename::file_stat::set_ctime(double unix_time)
{
    f_ctime = static_cast<int64_t>(unix_time);
    f_ctime_nano = static_cast<uint64_t>((unix_time - static_cast<double>(f_ctime)) * 1000000000);
}



} // namespace filename
// vim: ts=4 sw=4 et
