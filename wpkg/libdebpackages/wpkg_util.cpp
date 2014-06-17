/*    wpkg_util.cpp -- implementation of useful low level functions
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

/** \file
 * \brief Utilities.
 *
 * This file includes a set of useful functions that do not really pertain
 * to a specific class and are used by multiple classes. They are here so
 * we can avoid implementing the functions multiple times.
 */
#include    "libdebpackages/wpkg_util.h"
#include    "libdebpackages/case_insensitive_string.h"
#include    "libdebpackages/debian_version.h"
#include    "libdebpackages/compatibility.h"
#include    "libtld/tld.h"
#include    "libutf8/libutf8.h"
#include    <time.h>


/** \brief Utility functions.
 *
 * This namespace encompasses a set of useful functions that do not really
 * fit anywhere else.
 */
namespace wpkg_util
{


/** \class wpkg_util_exception
 * \brief Base exception class for the wpkg_util namespace.
 *
 * This exception is the base class for all the wpkg_util exceptions. It
 * is never itself raised, but it can be used to catch all the exceptions
 * that the wpkg_util functions raise.
 */


/** \class wpkg_util_exception_invalid
 * \brief Exception raised when an invalid value is detected.
 *
 * Whenever the code detects something that is considered invalid, this
 * exception is raised. For example, the versioncmp() function raises
 * the exception if one of the version strings is not a valid Debian
 * version.
 */


/** \brief Check whether a path part represents a reserved MS-Windows filename.
 *
 * This function checks \p path_part and if it is equal to a reserved
 * MS-Windows filename (also called a namespace filename) then the
 * function returns true.
 *
 * Reserved names are for backward compatibility of older software that
 * makes use of them to directly open hardware ports (serial ports,
 * parallel ports, console, auxiliary, and the null device.)
 *
 * These filenames are forbidden even with an extension. So this function
 * returns true whether you pass "NUL" or "NUL.txt".
 *
 * The files that are forbidden are:
 *
 * \li AUX -- the auxiliary
 * \li CON -- the console
 * \li COM1 to COM9 -- the 9 serial ports
 * \li LPT1 to LPT9 -- the 9 parallel ports
 * \li NUL -- the null device
 * \li PRN -- the printer
 *
 * In newer version of MS-Windows the preferred syntax is \\.\device-name
 * although there are other ways to access the hardware as MS-Windows
 * installs symbolic links from different locations (Since Windows 7).
 *
 * \param[in] path_part  The name to check against the MS-Windows reserved filenames
 *
 * \return true if the input matches an MS-Windows reserved filename.
 */
bool is_special_windows_filename(const std::string& path_part)
{
    case_insensitive::case_insensitive_string segment(path_part);

    std::string::size_type p(segment.find_last_of('.'));
    if(p != 0 && p != std::string::npos)
    {
        // remove the extension
        segment.erase(p);
    }

    if(!segment.empty())
    {
        switch(segment[0])
        {
        case 'a':
        case 'A':
            return segment == "aux";

        case 'c':
        case 'C':
            return segment == "con"  || segment == "com1" || segment == "com2"
                || segment == "com3" || segment == "com4" || segment == "com5"
                || segment == "com6" || segment == "com7" || segment == "com8"
                || segment == "com9";

        case 'l':
        case 'L':
            return segment == "lpt1" || segment == "lpt2" || segment == "lpt3"
                || segment == "lpt4" || segment == "lpt5" || segment == "lpt6"
                || segment == "lpt7" || segment == "lpt8" || segment == "lpt9";

        case 'n':
        case 'N':
            return segment == "nul";

        case 'p':
        case 'P':
            return segment == "prn";

        }
    }

    return false;
}


/** \brief Check all the characters of a filename for validity.
 *
 * A filename under the MS-Windows operating system cannot include
 * a certain number of characters as listed here:
 *
 * \li slash (/)
 * \li backslash (\\)
 * \li colon (:)
 * \li asterisk (*)
 * \li question mark (?)
 * \li double quote (")
 * \li smaller than (<)
 * \li larger than (>)
 * \li vertical bar (|)
 *
 * Note that we do not err on a colon (:), an asterisk (*), nor
 * a question mark (?) because those are often used in filenames
 * in some ways (colons are used in versions, asterisks and
 * question marks are used in patterns.)
 *
 * Along this test we also verify that filenames do not start or
 * end with spaces. These kind of files generate problems under
 * MS-Windows and can also be problematic under Unices.
 *
 * \note
 * These characters are considered invalid only in direct filenames.
 * This is important since a URI may very well include such a name
 * and still be considered perfectly valid.
 *
 * \param[in] filename  The filename to be checked.
 *
 * \return true if the filename is valid, false if the filename includes
 * characters that are not valid under MS-Windows.
 */
bool is_valid_windows_filename(const std::string& filename)
{
    for(const char *s(filename.c_str()); *s != '\0'; ++s)
    {
        switch(*s)
        {
        case '/':
        case '\\':
        //case ':': // used in versions
        //case '*': // used in patterns to read directories
        //case '?': // used in patterns to read directories
        case '"':
        case '<':
        case '>':
        case '|':
            return false;

        default:
            // other characters are accepted as is
            break;

        }
    }

    if(filename.length() > 0
    && (isspace(filename[0]) || isspace(filename[filename.size() - 1])))
    {
        return false;
    }

    return true;
}


/** \brief Verify that a package name is valid.
 *
 * The definition of the name of a Debian package is defined in the Debian
 * Policy paragraph 5.6.1:
 *
 * http://www.debian.org/doc/debian-policy/ch-controlfields.html#s-f-Source
 *
 * "Package names must consist only of lower case letters (a-z), digits (0-9),
 * plus (+) and minus (-) signs, and periods (.). They must be at least two
 * characters long and must start with an alphanumeric character."
 *
 * The corresponding regular expression goes like that:
 *
 * [0-9a-z][-+.0-9a-z]+
 *
 * However, we also forbid special characters (+-.) at the end of the name to
 * make it cleaner. So we really make use of the following specification:
 *
 * [0-9a-z][-+.0-9a-z]*[0-9a-z]
 *
 * Plus, for safety reasons, we prevent two periods one after another (..).
 * That makes the regular expression a bit more complex!
 *
 * \note
 * This function is used in the wpkg_filename() which is why it is here
 * instead of the wpkg_control::field_package_factory_t object.
 *
 * \param[in] name  The name to be checked.
 *
 * \return true if the name is indeed a valid package name
 */
bool is_package_name(const std::string& name)
{
    // name too short?
    if(name.length() < 2)
    {
        return false;
    }

    // name only using valid characters?
    for(const char *s(name.c_str()); *s != '\0'; ++s)
    {
        const char c(*s);
        if(c == '-' || c == '.' || c == '+')
        {
            // note that we forbid ".." in a package name
            if(s == name.c_str() || s[1] == '\0' || s[1] == '.')
            {
                // we do not accept a special character at the beginning
                // or the end of a name -- this is not Debian compatible
                // which allows those special characters at the end
                return false;
            }
        }
        else if((c < 'a' || c > 'z') && (c < '0' || c > '9'))
        {
            // upper case letters are not allowed!
            return false;
        }
    }

    // Name is acceptable as a filename in our environment?
    //
    // *** WARNING ***
    // *** WARNING ***
    // *** WARNING ***
    //
    // Do NOT prevent the "core" special name from being viewed as a
    // valid package name because the system actually uses that name
    // as a valid package name!
    //
    // *** WARNING ***
    // *** WARNING ***
    // *** WARNING ***
    if(name == "tmp")
    {
        return false;
    }

    return !is_special_windows_filename(name);
}


/** \brief Transform an md5sums file into a map.
 *
 * This function reads the list of filenames and md5sums from
 * an md5sums file and transforms that in a map. The index of
 * the map are the filenames.
 *
 * An md5sums file is composed of lines as follow:
 *
 * f81b74a7ddf5d44278145c59694bbac9  usr/bin/sample4
 *
 * The character right before the filename may be an asterisk
 * (*) which indicates that the file is considered binary.
 * At this point that character is not saved in the result,
 * but the character can only be a space (0x20) or an
 * asterisk or the function fails.
 *
 * \param[in,out] sums  The map where the sums are saved. It is NOT cleared by this function.
 * \param[in,out] md5file  The file to read for sums.
 */
void parse_md5sums(md5sums_map_t& sums, memfile::memory_file& md5file)
{
    int offset(0);
    std::string line;
    while(md5file.read_line(offset, line))
    {
        if(line.length() < 35)
        {
            throw wpkg_util_exception_invalid("input line is too short for an md5sums file");
        }
        std::string md5sum(line.substr(0, 32));
        if(line[32] != ' '
        || (line[33] != ' ' && line[33] != '*'))
        {
            throw wpkg_util_exception_invalid("invalid md5sum and filename separator \"  \" or \" *\" expected");
        }
        std::string filename(line.substr(34));
        if(isspace(*filename.begin())
        || isspace(*filename.rbegin()))
        {
            throw wpkg_util_exception_invalid("filename cannot start/end with a space");
        }
        sums[filename] = md5sum;
    }
}


/** \brief Transform the specified time in an RFC 2822 string.
 *
 * This function transform the specified time t in an RFC 2822
 * string.
 *
 * \param[in] t  The time to transform, if 0, then use time(&t) instead.
 */
std::string rfc2822_date(time_t t)
{
    // get a buffer way larger than any RFC 2822 date
    char buf[256];
    if(t == 0)
    {
        time(&t);
    }
    struct tm *m(localtime(&t));
#if defined(MO_WINDOWS)
    // the strftime() %z option under MS-Windows returns "ric" in my case,
    // which is not a valid timezone (at least, not that I know of); plus we
    // want the +0000 format instead
    TIME_ZONE_INFORMATION time_zone;
    if(GetTimeZoneInformation(&time_zone) == TIME_ZONE_ID_UNKNOWN)
    {
        // make sure we have an acceptable value
        time_zone.Bias = 0;
    }
    char tz[16];
    strftime_utf8(buf, sizeof(buf) / sizeof(buf[0]) - sizeof(tz) / sizeof(tz[0]) - 1, "%a, %d %b %Y %H:%M:%S ", m);
    _snprintf(tz, sizeof(tz) / sizeof(tz[0]) - 1, "%+03d%02d", time_zone.Bias / 60, labs(time_zone.Bias) % 60);
    strcat(buf, tz);
#else
    strftime_utf8(buf, sizeof(buf) / sizeof(buf[0]) - 1, "%a, %d %b %Y %H:%M:%S %z", m);
#endif
    buf[sizeof(buf) / sizeof(buf[0]) - 1] = '\0';
    return buf;
}


/** \brief Validate a URI.
 *
 * This function calls the libtld URI validation function to verify that
 * the specified \p uri is indeed valid.
 *
 * The default protocols are:
 *
 * \li http
 * \li https
 * \li ftp
 * \li sftp
 *
 * \param[in] uri  The URI to validate.
 *
 * \return true if the URI is considered valid.
 */
bool is_valid_uri(const std::string& uri, std::string protocols)
{
    if(protocols.empty())
    {
        protocols = "http,https,ftp,sftp";
    }
    tld_info info;
    tld_result r(tld_check_uri(uri.c_str(), &info, protocols.c_str(), 0));
    return r == TLD_RESULT_SUCCESS && info.f_status == TLD_STATUS_VALID;
}


/** \brief Quote the input string so it can be used in a console.
 *
 * Shell scripts called from the application need to have their parameters
 * and the name of the script itself quoted if we want to make it work
 * properly. This is particularly important when a filename includes a
 * space.
 *
 * Under MS-Windows we transform each double quote in two double quotes
 * and then quote such strings and strings that include a space.
 *
 * Under Unix we escape spaces with a backslash.
 *
 * \param[in] str  The string to be quoted.
 *
 * \return The quoted input string. If no quotation is required,
 * return str as is.
 */
std::string make_safe_console_string(const std::string& str)
{
    std::string result;
#ifdef MO_WINDOWS
    bool needs_quotes(false);
#endif
    for(const char *s(str.c_str()); *s != '\0'; ++s)
    {
#ifdef MO_WINDOWS
        if(isspace(*s))
        {
            needs_quotes = true;
        }
        else if(*s == '"')
        {
            needs_quotes = true;
            // quotes within a quoted parameter must be repeated
            result += *s;
        }
#else
        if(isspace(*s) || *s == '\\')
        {
            result += '\\';
        }
#endif
        result += *s;
    }
#ifdef MO_WINDOWS
    if(needs_quotes)
    {
        result = '"' + result + '"';
    }
#endif
    return result;
}


/** \brief Compare two versions between each others.
 *
 * This function compares Debian versions a and b and returns the comparison
 * results which are:
 *
 * \li -1 when a is smaller (older) than b
 * \li 0 when a and b represent the same version
 * \li 1 when a is larger (newer) than b
 *
 * \param[in] a  A string representing a Debian version.
 * \param[in] b  A string representing b Debian version.
 *
 * \return -1, 0, or 1 as the comparison dictates.
 */
int versioncmp(const std::string& a, const std::string& b)
{
    char error_string[256];

    debian_version_handle_t l(string_to_debian_version(a.c_str(), error_string, sizeof(error_string)));
    if(l == 0)
    {
        throw wpkg_util_exception_invalid("left hand side version " + a + " is invalid (" + error_string + ")");
    }

    debian_version_handle_t r(string_to_debian_version(b.c_str(), error_string, sizeof(error_string)));
    if(r == 0)
    {
        delete_debian_version(l);
        throw wpkg_util_exception_invalid("right hand side version " + b + " is invalid (" + error_string + ")");
    }

    int result = debian_versions_compare(l, r);

    delete_debian_version(l);
    delete_debian_version(r);

    return result;
}


/** \brief Ensure a valid version string for a filename.
 *
 * Debian versions make use of the colon (:) character which unfortunately is
 * not an acceptable character under MS-Windows file systems. We have to
 * change it to a semi-colon on that platform.
 *
 * Similarly, since we may have a version with a semi-colon, we force that
 * version to a semi-colon on Unix platforms.
 *
 * Repositories will function as expected since the filename we get from them
 * is found in the index.tar.gz file.
 *
 * \bug
 * At this point we canonicalize for the operating system we're running on.
 * If saving a file over a NetBIOS connection, the destination file system
 * may be a MS-Windows drive which will not accept the file.
 *
 * \param[in] version  The version to canonicalize for this operating system.
 */
std::string canonicalize_version_for_filename(const std::string& version)
{
    std::string result;

    for(const char *s(version.c_str()); *s != '\0'; ++s)
    {
#ifdef MO_WINDOWS
        if(*s == ':')
        {
            result += ';';
        }
        else
        {
            result += *s;
        }
#else
        if(*s == ';')
        {
            result += ':';
        }
        else
        {
            result += *s;
        }
#endif
    }

    return result;
}


/** \brief Ensure a canonicalized version string.
 *
 * Debian versions make use of the colon (:) character to separate the
 * epoch and the rest of the version which may also include colon
 * characters. The problem is that colons do not work in filenames of
 * the different MS-Windows file systems. Because of that, we convert
 * them to semi-colons (;) and thus we accept semi-colons in all versions.
 * This means you may write a version such as:
 *
 * \code
 * 3;1.5-9a
 * \endcode
 *
 * However, all other sub-systems will expected a colon in the version.
 * Thus we have this function to canonicalize the version string and
 * convert any semi-colon to a colon character. This is used in many
 * places.
 *
 * \param[in] version  The version to canonicalize for this operating system.
 */
std::string canonicalize_version(const std::string& version)
{
    std::string result;

    for(const char *s(version.c_str()); *s != '\0'; ++s)
    {
        if(*s == ';')
        {
            result += ':';
        }
        else
        {
            result += *s;
        }
    }

    return result;
}

/** \brief Get an environment variable.
 *
 * This function allows for getting an environment variable from any
 * operating system. The result is UTF-8, but under MS-Windows we use
 * the _wgetenv() to make sure that we get all the characters as expected.
 *
 * The function is given a list of names separated by commas. The first
 * variable that exists is returned. If none are defined then the specified
 * \p default_value is returned.
 *
 * \param[in] names  The list of variable names to check for.
 * \param[in] default_value  Default value to return in case the variables
 *                           are undefined
 *
 * \return The value of the first variable which is defined or \p default_value.
 */
std::string utf8_getenv(const std::string& names, const std::string& default_value)
{
    const char *n;
    for(const char *start(names.c_str()); *start != '\0'; start = n)
    {
        for(n = start; *n != '\0' && *n != ','; ++n);
        std::string varname;
        if(*n == ',')
        {
            varname = std::string(start, n - start);
            for(; *n == ','; ++n);
        }
        else
        {
            varname = start;
        }
#ifdef MO_WINDOWS
        // TODO: by default we should probably have a "User Data" path + "\temp"
        const wchar_t *wtemp(_wgetenv(libutf8::mbstowcs(varname).c_str()));
        if(wtemp != NULL)
        {
            return libutf8::wcstombs(wtemp);
        }
#else
        const char *temp(getenv(varname.c_str()));
        if(temp != NULL)
        {
            return temp;
        }
#endif
    }

    return default_value;
}

}   // namespace wpkg_util
// vim: ts=4 sw=4 et
