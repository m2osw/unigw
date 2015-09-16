/*    compatibility.cpp -- Windows compatibility definitions
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
 * \brief Implementation of different compatibility functions.
 *
 * A certain number of functions are not implemented under MS-Windows as
 * they are under Unix. In general, this implementation file has reworked
 * versions of the MS-Windows function so that way it can be used as is
 * every where else in (and out of) the library.
 */
#include    "libdebpackages/compatibility.h"
#include    <stdexcept>
#include    <vector>
#include    <time.h>

#ifdef _MSC_VER
#   if _MSC_VER < 1900
int snprintf(char *output, size_t size, const char *format, ...)
{
    va_list ap;
    va_start(ap, format);
    return _vsnprintf(output, size, format, ap);
}
#   endif

int strcasecmp(const char *a, const char *b)
{
    return _stricmp(a, b);
}

int strncasecmp(const char *a, const char *b, size_t c)
{
    return _strnicmp(a, b, c);
}
#endif




#ifdef MO_WINDOWS
int getuid()
{
    return 0;
}

int getgid()
{
    return 0;
}

int getpid()
{
    return GetCurrentProcessId();
}

int mkdir(const wchar_t *name, mode_t mode)
{
// mingwin does not use the underscore, but it still doesn't
// support the standard Unix like mkdir() function
    int r = ::_wmkdir(name);
    if(r == 0)
    {
        //SetFileAttributesA(name, GetFileAttributesA(name) & ~FILE_ATTRIBUTE_READONLY);
        r = ::_wchmod(name, mode & (_S_IREAD | _S_IWRITE));
    }
    return r;
}

int symlink(const wchar_t * /*destination*/, const wchar_t * /*symbolic_link*/)
{
    //errno = not implemented;
    throw std::runtime_error("symlink is not yet implemented");
    //return -1;
}
#endif


bool same_file(const std::string& a, const std::string& b)
{
#ifdef MO_WINDOWS
    class file_info
    {
    public:
        file_info(const std::string& filename)
            : f_handle(INVALID_HANDLE_VALUE),
              //f_info -- initialized if f_valid true
              f_valid(false)
        {
            f_handle = CreateFileW(
                    libutf8::mbstowcs(filename).c_str(),
                    GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);
            if(f_handle != INVALID_HANDLE_VALUE)
            {
                // make sure the BOOL is transformed to a bool
                f_valid = GetFileInformationByHandle(f_handle, &f_info) != 0;
            }
        }

        ~file_info()
        {
            if(f_handle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(f_handle);
            }
        }

        bool operator == (const file_info& rhs)
        {
            // if we could not open those files, it's going to fail when we
            // try to load the packages anyway
            if(!f_valid || !rhs.f_valid)
            {
                return false;
            }
            return f_info.dwVolumeSerialNumber == rhs.f_info.dwVolumeSerialNumber
                && f_info.nFileIndexHigh == rhs.f_info.nFileIndexHigh
                && f_info.nFileIndexLow == rhs.f_info.nFileIndexLow;
        }

    private:
        HANDLE                      f_handle;
        BY_HANDLE_FILE_INFORMATION  f_info;
        bool                        f_valid;
    };
    file_info ia(a);
    file_info ib(b);
    return ia == ib;
#else
    // this does not always work right when using a network drive
    struct stat sa;
    if(stat(a.c_str(), &sa) == 0)
    {
        struct stat sb;
        if(stat(b.c_str(), &sb) == 0)
        {
            return sa.st_dev == sb.st_dev && sa.st_ino == sb.st_ino;
        }
    }
    return false;
#endif
}



DEBIAN_PACKAGE_EXPORT size_t strftime_utf8(char *s, size_t max, const char *format, const struct tm *tm)
{
#ifdef MO_WINDOWS
    // under MS-Windows we want to use the Unicode version because the one of
    // the strings could make use of a Unicode character that would not print
    // with the plain strftime()

    // transform the format to a wide string
    const std::wstring wformat(libutf8::mbstowcs(format));
    std::vector<wchar_t> dest;
    dest.resize(max);
    size_t l(wcsftime(&dest[0], max, wformat.c_str(), tm));
    const std::string result(libutf8::wcstombs(&dest[0]));
    strncpy(s, result.c_str(), max);
    s[max - 1] = '\0';
    return l;
#else
    // we can use the strftime() function as is
    return strftime(s, max, format, tm);
#endif
}


// vim: ts=4 sw=4 et
