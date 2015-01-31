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
 * \brief Implementation of an fstream like class to handle Unicode everywhere.
 *
 * It is difficult to always handle Unicode each time you have to open a
 * file and read or write to it. Everywhere we deal with filename that are
 * UTF-8 (see the wpkg_filename::uri_filename class) but that is not enough
 * under MS-Windows which requires you to first convert such filenames to
 * UTF-16 before accessing the file system.
 *
 * The wpkg_stream handles all of that under the hood making it very easy
 * to read and write files on any system in a seamless manner (without having
 * to include operating system specific code each time you need to access a
 * file.)
 *
 * The MS-Windows implementation actually makes use of HANDLE as returned
 * by the CreateFile() function. This ensures full Unicode compatibility.
 *
 * \note
 * The contents of the files is generally UTF-8 and has nothing to do with
 * the wpkg_stream class.
 */
#include    "libdebpackages/wpkg_stream.h"


/** \brief The wpkg_stream for the libdepackages file handling.
 *
 * This namespace includes the declarations and implementation of the
 * stream used to access files on a disk system only using UTF-8 filenames
 * from the outside (i.e. it makes the Microsoft Windows Operating System
 * look like it works with UTF-8 filenames instead of UTF-16.)
 */
namespace wpkg_stream
{


/** \class fstream
 * \brief The file stream
 *
 * The file stream is a replacement of the std::ofstream and std::ifstream
 * which do not properly handle Unicode filenames. Our implementation
 * supports the uri_filename which are UTF-8 and internally converts the
 * strings to UTF-16 when opening or creating files under Microsoft
 * Windows.
 *
 * Note that the primary reason for implementing this class was to solve
 * the problem of mingw not allowing wchar_t filenames in their standard
 * C++ stream implementation. However, this is useful in many other
 * situations which had the same problem for it takes time to always
 * have to write the necessary code to handle MS-Windows lack of UTF-8
 * support.
 */


/** \brief Clean up the fstream.
 *
 * This function ensures that the file gets closed on destruction if not
 * yet closed explicitly.
 */
fstream::~fstream()
{
    close();
}


/** \brief Create an output file.
 *
 * This function creates a new file or truncates an existing file and allows
 * for writing only.
 *
 * If the file is write protected, the function may fail.
 *
 * \param[in] filename  The name of the file to create.
 *
 * \return false if the file cannot be truncated and opened for writing.
 */
bool fstream::create(const wpkg_filename::uri_filename& filename)
{
    close();

#if defined(MO_WINDOWS)
    // MS-Windows uses Unicode (wchar_t) to open any one file
    // Also the CreateFileW() is generally better than _wfopen()
    f_file = CreateFileW(
            filename.os_filename().get_utf16().c_str(),
            GENERIC_WRITE,
            0, // no sharing
            NULL,
            CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    return f_file != INVALID_HANDLE_VALUE;
#else
    f_file = fopen(filename.os_filename().get_utf8().c_str(), "wb");
    return f_file;
#endif
}


/** \brief Open an existing file for reading.
 *
 * This function opens an existing file for reading. If the file does not
 * exist or cannot be read (permission denied) then an error occurs and
 * the function returns false.
 *
 * \param[in] filename  The name of the file to create.
 *
 * \return false if the file cannot be opened for reading.
 */
bool fstream::open(const wpkg_filename::uri_filename& filename)
{
    close();

#if defined(MO_WINDOWS)
    // MS-Windows uses Unicode (wchar_t) to open any one file
    // Also the CreateFileW() is generally better than _wfopen()
    f_file = CreateFileW(
            filename.os_filename().get_utf16().c_str(),
            GENERIC_READ,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL);
    return f_file != INVALID_HANDLE_VALUE;
#else
    f_file = fopen(filename.os_filename().get_utf8().c_str(), "rb");
    return f_file;
#endif
}


/** \brief Open a file for appending.
 *
 * This function creates a new file or open an existing file for writing
 * at the end (append) of the file.
 *
 * This function supports the special filename "-" as stdout. Note that
 * it is not really possible to seek in this file, whether or not you
 * use the "-" special filename.
 *
 * \param[in] filename  The name of the file to create.
 *
 * \return false if the file cannot be opened for appending.
 */
bool fstream::append(const wpkg_filename::uri_filename& filename)
{
    close();

#if defined(MO_WINDOWS)
    if(filename.original_filename() == "-")
    {
        f_file = GetStdHandle(STD_OUTPUT_HANDLE);
        f_do_not_close = true;
    }
    else
    {
        // MS-Windows uses Unicode (wchar_t) to open any one file
        // Also the CreateFileW() is generally better than _wfopen()
        f_file = CreateFileW(
                filename.os_filename().get_utf16().c_str(),
                FILE_APPEND_DATA,
                0, // no sharing
                NULL,
                OPEN_ALWAYS,
                FILE_ATTRIBUTE_NORMAL,
                NULL);
    }
    return f_file != INVALID_HANDLE_VALUE;
#else
    if(filename.original_filename() == "-")
    {
        f_file = stdout;
        f_do_not_close = true;
    }
    else
    {
        f_file = fopen(filename.os_filename().get_utf8().c_str(), "ab");
    }
    return f_file;
#endif
}


/** \brief Close the file stream.
 *
 * This function closes the file stream if still open. It does nothing if
 * the stream was already closed.
 */
void fstream::close()
{
#if defined(MO_WINDOWS)
    if(f_file != INVALID_HANDLE_VALUE)
    {
        if(!f_do_not_close)
        {
            CloseHandle(f_file);
        }
        f_file = INVALID_HANDLE_VALUE;
    }
#else
    if(f_file)
    {
        if(!f_do_not_close)
        {
            fclose(f_file.get());
        }
        f_file.reset();
    }
#endif
    f_do_not_close = false;
}


/** \brief Check whether the last command failed.
 *
 * This function returns the current status of the stream. If the function
 * returns false, then the file handling failed and it should not go any
 * further.
 *
 * \return true if everything is good with this file.
 */
bool fstream::good() const
{
#if defined(MO_WINDOWS)
    return f_file != INVALID_HANDLE_VALUE;
#else
    return f_file;
#endif
}


/** \brief Seek to the specified position.
 *
 * Change the seek pointer. This function changes the seek pointer of
 * the next read and write unless you created the file with append()
 * in which case the write pointer is never affected.
 *
 * Note that the create() and append() function do not let you read the
 * file so this function only changes the write position in that case.
 *
 * \param[in] offset  The new input offset.
 * \param[in] dir  The direction used to seek.
 */
void fstream::seek(off_type offset, seekdir dir)
{
#if defined(MO_WINDOWS)
    if(f_file != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER pos;
        pos.QuadPart = offset;
        DWORD d;
        switch(dir)
        {
        case beg:
            d = FILE_BEGIN;
            break;

        case end:
            d = FILE_END;
            break;

        case cur:
            d = FILE_CURRENT;
            break;

        }
        SetFilePointerEx(f_file, pos, NULL, d);
    }
#else
    // convert to an std io stream type
    int d = 0;
    switch(dir)
    {
    case beg:
        d = SEEK_SET;
        break;

    case end:
        d = SEEK_END;
        break;

    case cur:
        d = SEEK_CUR;
        break;

    }
    fseek(f_file, offset, d);
#endif
}


/** \brief Retrieve the current pointer position.
 *
 * Retrieve the pointer position of the file. This is the position where the
 * next write() or read() will act.
 *
 * \return The current stream pointer position.
 */
fstream::size_type fstream::tell() const
{
#if defined(MO_WINDOWS)
    if(f_file != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER pos;
        pos.QuadPart = 0;
        LARGE_INTEGER tell_pos;
        tell_pos.QuadPart = 0;
        if(SetFilePointerEx(f_file, pos, &tell_pos, FILE_CURRENT))
        {
            return tell_pos.QuadPart;
        }
    }
#else
    if(f_file)
    {
        return ftell(f_file.get());
    }
#endif
    return -1;
}


/** \brief Read data from a stream.
 *
 * This function reads up to \p size bytes from the stream and save them in
 * \p buffer.
 *
 * \param[in] buffer  The buffer where the data read is saved.
 * \param[in] size  The number of bytes to read from the stream.
 *
 * \return The number of bytes read from the stream.
 */
fstream::size_type fstream::read(void *buffer, size_type size) const
{
    size_type result(-1);

#if defined(MO_WINDOWS)
    if(f_file != INVALID_HANDLE_VALUE)
    {
        DWORD bytes_read;
        if(ReadFile(f_file, buffer, static_cast<DWORD>(size), &bytes_read, NULL))
        {
            result = bytes_read;
        }
    }
#else
    if(f_file)
    {
        result = fread(buffer, 1, size, f_file.get());
    }
#endif

    if(result == -1)
    {
        const_cast<fstream *>(this)->close();
    }
    return result;
}


/** \brief Write data to a stream.
 *
 * This function writes \p size bytes of \p buffer data to the stream.
 * If the file was opened with append(), then the bytes are always appended
 * at the end of the file. Otherwise, they are written at the current
 * position as defined by seek().
 *
 * \note
 * If an error occurs then the streams gets closed instantaneously. The
 * good() function then returns false.
 *
 * \param[in] buffer  The buffer of data to write to the stream.
 * \param[in] size  The number of bytes from buffer to write to the stream.
 *
 * \return The total number of bytes that were properly written to the stream.
 */
fstream::size_type fstream::write(const void *buffer, size_type size) const
{
    size_type result(-1);

#if defined(MO_WINDOWS)
    if(f_file != INVALID_HANDLE_VALUE)
    {
        DWORD bytes_written;
        if(WriteFile(f_file, buffer, static_cast<DWORD>(size), &bytes_written, NULL))
        {
            result = bytes_written;
        }
    }
#else
    if(f_file)
    {
        result = fwrite(buffer, 1, size, f_file.get());
    }
#endif

    if(result == -1)
    {
        const_cast<fstream *>(this)->close();
    }
    return result;
}


} // wpkg_stream namespace
// vim: ts=4 sw=4 et
