/*    memfile.cpp -- handle files in memory
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
 * \brief Memory file handling.
 *
 * This file is the implementation of the memory_file and sub-classes that
 * are used to handle files in memory and on disk.
 *
 * A memory file can be defined by reading a file with read_file() and can
 * be saved on disk with write_file().
 *
 * A memory file object is capable of reading all the different type of
 * archives supported (tar, ar, wpkgar, ...) and compress or decompress
 * data with the supported compression libraries (zlib, bz2).
 *
 * The current implementation has a hard coded block size which can be a
 * problem with dealing with small files. However, it otherwise handles very
 * large files extremely quickly.
 */

// WARNING: The tcp_client_server.h MUST be first to avoid tons of errors
//          under MS-Windows (which apparently has quite a few problems
//          with their headers and backward compatibility.)
#include    "libdebpackages/tcp_client_server.h"

#include    "libdebpackages/memfile.h"
#include    "libdebpackages/wpkg_stream.h"
#include    "libdebpackages/wpkgar_block.h"
#include    "libdebpackages/case_insensitive_string.h"

#ifdef debpackages_EXPORTS
#define BZ_IMPORT 1
#define BZ_DLL 1
#define ZLIB_DLL 1
#endif
#include    "zlib.h"
#include    "bzlib.h"

#include    <errno.h>
#include    <stdlib.h>
#include    <memory.h>
#include    <stdarg.h>
#include    <ctime>
#include    <algorithm>
#include    <iostream>
#if defined(MO_WINDOWS)
#include    "libdebpackages/comptr.h"
#include    <objidl.h>
#include    <shlobj.h>
// "conditional expression is constant"
#pragma warning(disable: 4127)
// "unreachable code"
#pragma warning(disable: 4702)
// "unknown pragma"
#pragma warning(disable: 4068)
#else
#include    <pwd.h>
#include    <grp.h>
#include    <unistd.h>
#endif


/** \brief The memory file namespace.
 *
 * This namespace is used to declare and implement all the memory file
 * functions.
 */
namespace memfile
{


#ifndef _MSC_VER
// somehow g++ has problems with those in some cases
const int                               memory_file::block_manager::BLOCK_MANAGER_BUFFER_BITS; // init in class
const int                               memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE; // init in class

const int                               memory_file::file_info_throw; // init in class
const int                               memory_file::file_info_return_errors; // init in class
const int                               memory_file::file_info_permissions_error; // init in class
const int                               memory_file::file_info_owner_error; // init in class
#endif
memory_file::block_manager::buffer_t    memory_file::block_manager::g_free_buffers; // auto-init to empty
controlled_vars::zint32_t               memory_file::block_manager::g_total_allocated; // max. memory allocated for blocks (init to zero)

// make sure that the number of bits is at least 10
CONTROLLED_VARS_STATIC_ASSERT(memory_file::block_manager::BLOCK_MANAGER_BUFFER_BITS >= 10);


/** \class memory_file
 * \brief Handle a file in memory.
 *
 * To work with all the files the packager need to handle, we created a
 * memory file object. This object is able to read files from disk and
 * remote location (HTTP at this time) and write files to disk.
 *
 * The object includes support for compressing and decompressing data from
 * one memory file to another.
 *
 * The object knows about many different archives and operating system
 * directories. This means it can read all the file names, detailed
 * information (such as permissions, size, owner, etc.), and the actual
 * content of the files.
 *
 * The object handles the files using blocks of memory of 64Kb. This eases
 * the handling by avoiding moving large amounts of data and memory
 * fragmentation (we do not free these blocks of data, we mark them as
 * available once not necessary any more.)
 */


/** \class memory_file::block_manager
 * \brief Manage all the blocks used by the memory file.
 *
 * The block manager allocates new blocks of memory to be used by the memory
 * file. Also, it stores the blocks that are not in use anymore so as to
 * be able to reuse them and avoid many malloc()/free() calls.
 *
 * As you may know, allocating and free large quantity of memory blocks can
 * be slow and also generates fragmentation (when larger block of memory are
 * required to be allocated at the end of the heap because all other areas
 * are too small for it.)
 *
 * This manager solves both problems at once, although at this time it is
 * limited to one size blocks. Later we may want to enhance this
 * implementation with the use of 2 or more sizes as smaller files (such
 * as control files) do not need large blocks as we are using now (for
 * those a 1Kb block size would do very well.)
 */


/** \class memory_file::file_info
 * \brief Manage the detailed information of a file.
 *
 * This class is used to handle all the meta data of a file. This detailed
 * information includes things such as the owner and group names, the
 * permissions, and time stamps of a file. These details are used in the
 * file system, the archives where files are saved or read from, and when
 * new files are created.
 *
 * The file_info structure is exactly the same for all operating systems
 * making it easy to setup whether you are using MS-Windows or a Unix system.
 * Of course, some of the information does not fit all the file systems this
 * library may be used with. This being said, it will do its best to keep as
 * much of the information intact.
 */


/** \class memfile_exception
 * \brief The memory file base exception.
 *
 * This exception is the base of all the memory file exceptions. It is
 * never thrown directly, but can be used to catch all the exceptions
 * raised by the memory file implementation.
 */


/** \class memfile_exception_compatibility
 * \brief A function was called with an unsupported type.
 *
 * When attempting to run a function, parameters are checked to make sure
 * that the library can handle the process. If not, then this exception
 * is raised.
 */


/** \class memfile_exception_io
 * \brief A system I/O function failed.
 *
 * When reading or writing from a system file or over the network fails,
 * the library throws this exception. This generally makes it a lot easier
 * to handle errors at a higher level because you do not have to check
 * the return value of each function to know whether it was successful or
 * not. If it returns, it was successful. If it throws, it was not.
 */


/** \class memfile_exception_parameter
 * \brief A parameter was not valid.
 *
 * When a function is called with an invalid parameter, this exception is
 * raised. This is most often a programmer exception.
 *
 * For example, calling a function to retrieve the data of a file with an
 * offset outside of the buffer size raises this exception.
 */

/** \class memfile_exception_undefined
 * \brief Attempt to access an undefined object.
 *
 * This exception is raised when the programmer attempts to access the
 * memory buffer before it is created and other similar situations.
 */


/** \class memfile_exception_invalid
 * \brief Some input is invalid and cannot be used.
 *
 * When reading data, some of it may be invalid. This exception is raised
 * if something invalid and considered bad enough to be unrecoverable is
 * found.
 *
 * Note that at times this exception is used instead of the
 * memfile_exception_parameter, if you notice such, let us know and we'll
 * look into fixing the problem.
 */


memory_file::block_manager::block_manager()
    //: f_size(0) -- auto-init
    //  f_available_size(0) -- auto-init
    //  f_buffers() -- auto-init
{
}

memory_file::block_manager::~block_manager()
{
    // release all the buffers this manager allocated
    g_free_buffers.insert(g_free_buffers.end(), f_buffers.begin(), f_buffers.end());
}

int memory_file::block_manager::max_allocated()
{
    return g_total_allocated;
}

void memory_file::block_manager::clear()
{
    // release all the buffers
    g_free_buffers.insert(g_free_buffers.end(), f_buffers.begin(), f_buffers.end());
    f_buffers.clear();
    f_size = 0;
    f_available_size = 0;
}

int memory_file::block_manager::read(char *buffer, int offset, int bufsize) const
{
    if(offset < 0 || offset > f_size)
    {
        throw memfile_exception_parameter("offset is out of bounds");
    }
    if(offset + bufsize > f_size)
    {
        bufsize = f_size - offset;
    }
    if(bufsize > 0) {
        // copy bytes between offset and next block boundary
        int pos(offset & (BLOCK_MANAGER_BUFFER_SIZE - 1));
        int page(offset >> BLOCK_MANAGER_BUFFER_BITS);
        int sz(std::min(bufsize, BLOCK_MANAGER_BUFFER_SIZE - pos));
        memcpy(buffer, f_buffers[page] + pos, sz);
        buffer += sz;
        // copy full pages at once unless size left is less than a page
        int size_left(bufsize - sz);
        while(size_left >= BLOCK_MANAGER_BUFFER_SIZE) {
            ++page;
            memcpy(buffer, f_buffers[page], BLOCK_MANAGER_BUFFER_SIZE);
            buffer += BLOCK_MANAGER_BUFFER_SIZE;
            size_left -= BLOCK_MANAGER_BUFFER_SIZE;
        }
        // copy a bit, what's left afterward
        if(size_left > 0) {
            // page is not incremented yet
            memcpy(buffer, f_buffers[page + 1], size_left);
        }
    }
    return bufsize;
}

int memory_file::block_manager::write(const char *buffer, const int offset, const int bufsize)
{
    if(offset < 0) {
        throw memfile_exception_parameter("offset is out of bounds");
    }

    // compute total size
    int total(offset + bufsize);

    // Increased the maximum size to 1Gb instead of 128Mb
    // I think we should have a command line flag so you can impose a limit
    // although there should be no reason other than package optimization
    // (i.e. make sure you don't include the "wrong" thing in your packages)
    if(total > 1024 * 1024 * 1024) {
        throw memfile_exception_parameter("memory file size too large (over 1Gb?!)");
    }

    // allocate blocks to satisfy the total size
    while(total > f_available_size) {
        if(g_free_buffers.empty()) {
            f_buffers.push_back(new char[BLOCK_MANAGER_BUFFER_SIZE]);
            g_total_allocated += BLOCK_MANAGER_BUFFER_SIZE;
        }
        else {
            f_buffers.push_back(g_free_buffers.back());
            g_free_buffers.pop_back();
        }
        f_available_size += BLOCK_MANAGER_BUFFER_SIZE;
    }

    // if offset is larger than size we want to clear the buffers in between
    if(offset > f_size) {
        int pos(f_size & (BLOCK_MANAGER_BUFFER_SIZE - 1));
        int page(f_size >> BLOCK_MANAGER_BUFFER_BITS);
        int sz(std::min(offset - f_size, BLOCK_MANAGER_BUFFER_SIZE - pos));
        memset(f_buffers[page] + pos, 0, sz);
        f_size += sz;
        while(offset > f_size) {
            ++page;
            sz = std::min(offset - f_size, BLOCK_MANAGER_BUFFER_SIZE);
            memset(f_buffers[page], 0, sz);
            f_size += sz;
        }
    }

    // now copy buffer to our blocks
    if(bufsize > 0) {
        // copy up to the end of the current block
        const int pos(offset & (BLOCK_MANAGER_BUFFER_SIZE - 1));
        int page(offset >> BLOCK_MANAGER_BUFFER_BITS);
        const int sz(std::min(BLOCK_MANAGER_BUFFER_SIZE - pos, bufsize));
        int buffer_size(bufsize);
        memcpy(f_buffers[page] + pos, buffer, sz);
        buffer += sz;
        buffer_size -= sz;
        // copy entire blocks if possible
        while(buffer_size >= BLOCK_MANAGER_BUFFER_SIZE) {
            ++page;
            memcpy(f_buffers[page], buffer, BLOCK_MANAGER_BUFFER_SIZE);
            buffer += BLOCK_MANAGER_BUFFER_SIZE;
            buffer_size -= BLOCK_MANAGER_BUFFER_SIZE;
        }
        // copy the remainder if any
        if(buffer_size > 0) {
            memcpy(f_buffers[page + 1], buffer, buffer_size);
        }
    }

    f_size = std::max(static_cast<int>(f_size), total);

    return bufsize;
}

int memory_file::block_manager::compare(const block_manager& rhs) const
{
    int r;
    int32_t sz(std::min(f_size, rhs.f_size));
    int page(0);
    for(; sz >= BLOCK_MANAGER_BUFFER_SIZE; sz -= BLOCK_MANAGER_BUFFER_SIZE, ++page)
    {
        r = memcmp(f_buffers[page], rhs.f_buffers[page], BLOCK_MANAGER_BUFFER_SIZE);
        if(r != 0)
        {
            return r;
        }
    }
    r = memcmp(f_buffers[page], rhs.f_buffers[page], sz);
    if(r != 0)
    {
        return r;
    }
    if(f_size == rhs.f_size)
    {
        return 0;
    }
    return f_size < rhs.f_size ? -1 : 1;
}

memory_file::file_format_t memory_file::block_manager::data_to_format(int offset, int /*bufsize*/ ) const
{
    char buf[1024];
    int sz(read(buf, offset, sizeof(buf)));
    return memory_file::data_to_format(buf, sz);
}





namespace
{

uint32_t tar_check_sum(const char *s)
{
    uint32_t result = 8 * ' '; // the checksum field
    // name + mode + uid + gid + size + mtime = 148 bytes
    for(int n = 148; n > 0; --n, ++s) {
        result += *s;
    }
    s += 8; // skip the checksum field
    // everything after the checksum is another 356 bytes
    for(int n = 356; n > 0; --n, ++s) {
        result += *s;
    }
    return result;
}

uint32_t wpkg_check_sum(const uint8_t *s)
{
    // we ignore the checksum field which is the last 4 bytes
    uint32_t result = 0;
    for(int i = 0; i < 1024 - 4; ++i, ++s) {
        result += *s;
    }
    return result;
}

/** \brief Base class used to handle errors of the z library
 *
 * The z library may generate an error code that the check_error()
 * command handles for the compressor and decompressor alike.
 */
class gz_lib
{
public:
    gz_lib()
    {
        memset(&f_zstream, 0, sizeof(f_zstream));
    }

    void check_error(int zerr)
    {
        if(zerr != Z_OK && zerr != Z_STREAM_END && zerr != Z_BUF_ERROR) {
            if(zerr == Z_MEM_ERROR) {
                // use standard memory allocation failure exception
                throw std::bad_alloc();
            }
            char buf[32];
            snprintf(buf, sizeof(buf), "%d", zerr);
            throw memfile_exception_io(std::string("gz compression failed with error code ") + buf);
        }
    }

protected:
    z_stream            f_zstream;
};


/** \brief Handle z compressions of file contents in memory.
 *
 * This class handles the z compression and is also RAII since the destructor
 * ensures that the zstream gets cleaned up.
 */
class gz_deflate : private gz_lib
{
public:
    gz_deflate(int zlevel)
    {
        // we need the + 16 to have a wrap of 2 which is required to have a header!
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
        check_error(deflateInit2(&f_zstream, zlevel, Z_DEFLATED, 15 + 16, 9, Z_DEFAULT_STRATEGY));
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
        memset(&f_zheader, 0, sizeof(f_zheader));
        f_zheader.time = static_cast<int>(time(NULL));
        // what value are valid for os? it is undefined in the header...
        // search for RFC 1952 for a list
#if defined(MO_WINDOWS)
        f_zheader.os = 0; // FAT (i.e. Windows, OS/2, MS-DOS), we could use 11 for NTFS
#elif defined(MO_LINUX)
        f_zheader.os = 3; // Unix
#else
        f_zheader.os = 255; // unknown
#endif
        check_error(deflateSetHeader(&f_zstream, &f_zheader));
    }

    ~gz_deflate()
    {
        deflateEnd(&f_zstream);
    }

    void compress(memory_file& result, const memory_file::block_manager& block)
    {
        result.create(memory_file::file_format_gz);
        Bytef out[1024 * 64]; // 64Kb like the block manager at this time
        char in[memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE];
        int in_offset(0);
        int offset(0);
        int sz(block.size());
        f_zstream.next_in = reinterpret_cast<Bytef *>(in);
        f_zstream.avail_in = 0;
        while(sz > 0) {
            if(f_zstream.avail_in < static_cast<uInt>(memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE)) {
                // move what's left to the start of the buffer
                memmove(in, f_zstream.next_in, f_zstream.avail_in);
                int left_used(std::min(sz, memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE - static_cast<int>(f_zstream.avail_in)));
                block.read(in + f_zstream.avail_in, in_offset, left_used);
                sz -= left_used;
                in_offset += left_used;
                f_zstream.next_in = reinterpret_cast<Bytef *>(in);
                f_zstream.avail_in += static_cast<uInt>(left_used);
            }
            int r(Z_OK);
            do {
                f_zstream.next_out = out;
                f_zstream.avail_out = static_cast<uInt>(sizeof(out));
                // using Z_NO_FLUSH is the best value to get the best compression
                // it is also the most annoying to use because avail_in may not
                // go to zero and thus we need to handle that special case
                r = deflate(&f_zstream, sz == 0 ? Z_FINISH : Z_NO_FLUSH);
                check_error(r);
                int size_used(static_cast<int>(sizeof(out) - f_zstream.avail_out));
                result.write(reinterpret_cast<char *>(out), offset, size_used);
                offset += size_used;
            } while(f_zstream.avail_out == 0 && r != Z_STREAM_END && r != Z_BUF_ERROR);
        }
    }

private:
    gz_header       f_zheader;
};


/** \brief Class used to decompress a buffer the was compressed with the z library.
 *
 * This class is also an RAII wrapper of the zstream buffer which needs to
 * be cleaned up on exception or errors.
 */
class gz_inflate : private gz_lib
{
public:
    gz_inflate()
    {
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
        // window bits defaults to 15, +16 to decode gzip only
        check_error(inflateInit2(&f_zstream, 15 + 16));
#if (__GNUC__ >= 4) && (__GNUC_MINOR__ >= 6)
#pragma GCC diagnostic pop
#endif
    }

    ~gz_inflate()
    {
        inflateEnd(&f_zstream);
    }

    void decompress(memory_file& result, const memory_file::block_manager& block)
    {
        result.create(memory_file::file_format_other);
        Bytef out[1024 * 64];
        char in[memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE];
        int sz(block.size());
        int in_offset(0);
        int offset(0);
        f_zstream.next_in = reinterpret_cast<Bytef *>(in);
        f_zstream.avail_in = 0;
        while(sz > 0) {
            if(f_zstream.avail_in < static_cast<uInt>(memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE)) {
                // move what's left to the start of the buffer
                memmove(in, f_zstream.next_in, f_zstream.avail_in);
                int left_used(std::min(sz, memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE - static_cast<int>(f_zstream.avail_in)));
                block.read(in + f_zstream.avail_in, in_offset, left_used);
                sz -= left_used;
                in_offset += left_used;
                f_zstream.next_in = reinterpret_cast<Bytef *>(in);
                f_zstream.avail_in += static_cast<uInt>(left_used);
            }
            int r(Z_OK);
            do {
                f_zstream.next_out = out;
                f_zstream.avail_out = static_cast<uInt>(sizeof(out));
                // using Z_NO_FLUSH is the best value to get the best compression
                // it is also the most annoying to use because avail_in may not
                // go to zero and thus we need to handle that special case
                r = inflate(&f_zstream, sz == 0 ? Z_NO_FLUSH : Z_NO_FLUSH);
                check_error(r);
                int size_used(static_cast<int>(sizeof(out) - f_zstream.avail_out));
                result.write(reinterpret_cast<char *>(out), offset, size_used);
                offset += size_used;
            } while(f_zstream.avail_out == 0 && r != Z_STREAM_END && r != Z_BUF_ERROR);
        }
        result.guess_format_from_data();
    }
};


/** \brief Handling of the bz2 compression format.
 *
 * This class is the base class for the bz2 compressor and decompressor
 * classes. It handles the errors in a common way and holds the
 * bz_stream buffer.
 */
class bz2_lib
{
public:
    bz2_lib()
    {
        memset(&f_bzstream, 0, sizeof(f_bzstream));
    }

    void check_error(int bzerr)
    {
        if(bzerr != BZ_OK && bzerr != BZ_RUN_OK && bzerr != BZ_FINISH_OK && bzerr != BZ_STREAM_END) {
            if(bzerr == BZ_MEM_ERROR) {
                // use standard memory allocation failure exception
                throw std::bad_alloc();
            }
            throw memfile_exception_io("bz2 compression failed");
        }
    }

protected:
    bz_stream       f_bzstream;
};


/** \brief Deflate class to compress bz2 streams.
 *
 * This class is used to compress a stream of data using the bz2
 * compressor.
 *
 * The compress() function is the one used to compress a set of input
 * blocks in a resulting bz2 compressed buffer.
 */
class bz2_deflate : private bz2_lib
{
public:
    bz2_deflate(int bzlevel)
    {
        // compression level, no verbosity, default work factor
        check_error(BZ2_bzCompressInit(&f_bzstream, bzlevel, 0, 0));
    }

    ~bz2_deflate()
    {
        check_error(BZ2_bzCompressEnd(&f_bzstream));
    }

    void compress(memory_file& result, const memory_file::block_manager& block)
    {
        result.create(memory_file::file_format_bz2);
        char out[1024 * 64]; // 64Kb like the block manager at this time
        int out_offset(0);
        char in[memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE];
        int in_offset(0);
        int sz(block.size());
        f_bzstream.next_in = in;
        f_bzstream.avail_in = 0;
        while(sz > 0)
        {
            if(f_bzstream.avail_in < static_cast<unsigned int>(memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE))
            {
                // move what's left to the start of the buffer
                memmove(in, f_bzstream.next_in, f_bzstream.avail_in);
                const int left_used(std::min(sz, memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE - static_cast<int>(f_bzstream.avail_in)));
                block.read(in + f_bzstream.avail_in, in_offset, left_used);
                sz -= left_used;
                in_offset += left_used;
                f_bzstream.next_in = in;
                f_bzstream.avail_in += static_cast<unsigned int>(left_used);
            }
            int r(BZ_OK);
            do
            {
                f_bzstream.next_out = out;
                f_bzstream.avail_out = static_cast<unsigned int>(sizeof(out));
                // using BZ_RUN is the best value to get the best compression
                // it is also the most annoying to use because avail_in may not
                // go to zero and thus we need to handle that special case
                r = BZ2_bzCompress(&f_bzstream, sz == 0 ? BZ_FINISH : BZ_RUN);
                check_error(r);
                const int size_used(static_cast<int>(sizeof(out) - f_bzstream.avail_out));
                result.write(reinterpret_cast<char *>(out), out_offset, size_used);
                out_offset += size_used;
            }
            while(f_bzstream.avail_out == 0 && r != BZ_STREAM_END);
        }
        // note that we do not need to check for BZ_STREAM_END
        // because avail_out != 0 in that case
    }
};


/** \brief Decompress a bz2 compressed buffer in memory.
 *
 * This class is used to decompress a buffer that was previously compressed
 * with the bz2 compressor.
 */
class bz2_inflate : private bz2_lib
{
public:
    bz2_inflate()
    {
        // no verbosity, default work factor
        check_error(BZ2_bzDecompressInit(&f_bzstream, 0, 0));
    }

    ~bz2_inflate()
    {
        check_error(BZ2_bzDecompressEnd(&f_bzstream));
    }

    void decompress(memory_file& result, const memory_file::block_manager& block)
    {
        result.create(memory_file::file_format_other);
        char out[1024 * 64]; // 64Kb like the block manager at this time
        int out_offset(0);
        char in[memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE];
        int in_offset(0);
        int sz(block.size());
        f_bzstream.next_in = in;
        f_bzstream.avail_in = 0;
        while(sz > 0)
        {
            if(f_bzstream.avail_in < static_cast<unsigned int>(memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE))
            {
                // move what's left to the start of the buffer
                memmove(in, f_bzstream.next_in, f_bzstream.avail_in);
                int left_used(std::min(sz, memory_file::block_manager::BLOCK_MANAGER_BUFFER_SIZE - static_cast<int>(f_bzstream.avail_in)));
                block.read(in + f_bzstream.avail_in, in_offset, left_used);
                sz -= left_used;
                in_offset += left_used;
                f_bzstream.next_in = in;
                f_bzstream.avail_in += static_cast<unsigned int>(left_used);
            }
            int r(BZ_OK);
            do
            {
                f_bzstream.next_out = out;
                f_bzstream.avail_out = static_cast<unsigned int>(sizeof(out));
                // using BZ_RUN is the best value to get the best compression
                // it is also the most annoying to use because avail_in may not
                // go to zero and thus we need to handle that special case
                r = BZ2_bzDecompress(&f_bzstream);
                check_error(r);
                int size_used(static_cast<int>(sizeof(out) - f_bzstream.avail_out));
                result.write(reinterpret_cast<char *>(out), out_offset, size_used);
                out_offset += size_used;
            }
            while(f_bzstream.avail_out == 0 && r != BZ_STREAM_END);
        }
        result.guess_format_from_data();
    }
};


} // no name namespace







/** \brief Read information about a file and save it in the info object.
 *
 * This function reads all the available information about the specified
 * file and saves that in the info object.
 *
 * Note that if the filename represents a non-direct file (as defined by
 * the path_is_direct() function) then this function does NOTHING. In
 * other words, you should have gotten the necessary info when reading
 * the file from the remote computer instead.
 *
 * \param[in] filename  The name of the file to read the info about.
 * \param[in,out] info  The information about the file.
 */
void memory_file::disk_file_to_info(const wpkg_filename::uri_filename& filename, file_info& info)
{
    wpkg_filename::uri_filename::file_stat s;

    // TBD -- is that correct?
    //        necessary for memfile_dir::read() -- side effects on others?
    info.set_uri(filename);

    if(!filename.is_direct())
    {
        // we have to assume that the caller gets the information in another
        // way because we do not want to requery a remote file (although for
        // an HTTP request we could use a HEAD request, but in most cases
        // this would happen after a GET which already sent us a HEAD,
        // see the read_file() function for details on that one.)
        return;
    }

#if defined(MO_WINDOWS)
    // is this filename pointing to a "softlink" (shortcut)?
    // (note that we already know that the file is direct meaning that it
    // it either local or on a samba connection)
    case_insensitive::case_insensitive_string ext(filename.extension());
    if(ext == "lnk")
    {
        ComPtr<IShellLinkW> shell_link;
        HRESULT hr(CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, shell_link.AddressOf()));
        if(SUCCEEDED(hr))
        {
            ComPtr<IPersistFile> persist_file;
            hr = shell_link->QueryInterface(IID_IPersistFile, persist_file.AddressOf());
            if(SUCCEEDED(hr))
            {
                hr = persist_file->Load(filename.os_filename().get_utf16().c_str(), STGM_READ);
                if(SUCCEEDED(hr))
                {
                    wchar_t lnk[MAX_PATH];
                    hr = shell_link->GetPath(lnk, MAX_PATH, NULL, 0);
                    if(SUCCEEDED(hr))
                    {
                        // keep the filename without the .lnk extension
                        std::string fullname(filename.original_filename());
                        const wpkg_filename::uri_filename plain_filename(fullname.substr(0, fullname.length() - 4));
                        info.set_uri(plain_filename);
                        info.set_filename(plain_filename.path_only());

                        // the file is a "softlink"
                        info.set_link(libutf8::wcstombs(lnk));
                        info.set_file_type(memory_file::file_info::symbolic_link);

                        // symbolic link size must be 0 in tarballs
                        info.set_size(0);

                        // get a few info from the .lnk file itself
                        if(plain_filename.os_stat(s) != 0) {
                            throw memfile_exception_io("I/O error while reading directory (stat() call failed for \"" + filename.original_filename() + "\")");
                        }
                        info.set_mtime(s.get_mtime());

                        info.set_user("Administrator");
                        info.set_group("Administrators");
                        info.set_mode(0777);
                        return;
                    }
                }
            }
        }

        throw memfile_exception_io("I/O error while reading symbolic link");
    }

    if(filename.os_stat(s) != 0)
    {
        throw memfile_exception_io("I/O error while reading directory (stat() call failed for \"" + filename.original_filename() + "\")");
    }

    // just in case, save that info; it's not saved in packages, but we
    // may need it if we are handling a directory
#else
    // we use lstat() so we get symbolic link stats and not their target
    if(filename.os_lstat(s) == -1)
    {
        throw memfile_exception_io("I/O error while reading directory (lstat() call failed for \"" + filename.original_filename() + "\")");
    }
#endif

    info.set_uri(filename);
    info.set_filename(filename.path_only());

    switch(s.get_mode() & S_IFMT)
    {
    case S_IFREG:
        // We can detect whether a file is sparse using
        // the following check:
        // if(s.st_blksize * s.st_blocks >= s.st_size) ...
        // However, tar offers either REGTYPE or CONTTYPE,
        // not REGTYPE and SPARTYPE... so we use regular
        // and ignore CONTTYPE.
        //
        // Hard links could be detected with:
        // if(s.st_nlink != 0) ...
        // but those are only for files created with two files
        // using the exact same device & inode numbers.
        // In other words, a file_info::hard_link is a reference
        // to a file that exists in the tarball!
        info.set_file_type(memory_file::file_info::regular_file);
        break;

    case S_IFDIR:
        info.set_file_type(memory_file::file_info::directory);
        break;

    case S_IFCHR:
        info.set_file_type(memory_file::file_info::character_special);
        break;

#if !defined(MO_WINDOWS)
    case S_IFLNK:
        info.set_file_type(memory_file::file_info::symbolic_link);
        {
            // get the softlink destination
            char buf[4096];
            const size_t len = readlink( info.get_filename().c_str(), buf, sizeof(buf) );
            if(len <= 0)
            {
                throw memfile_exception_io("I/O error reading soft-link \"" + info.get_filename() + "\"");
            }
            std::string link;
            link.assign( buf, len );
            info.set_link( link );
        }
        break;

    case S_IFBLK:
        info.set_file_type(memory_file::file_info::block_special);
        break;

    case S_IFIFO:
        info.set_file_type(memory_file::file_info::fifo);
        break;
#endif

    default:
        throw memfile_exception_io("I/O error unknown stat() file format");

    }

    info.set_mtime(s.get_mtime());

    switch(s.get_mode() & S_IFMT)
    {
    case S_IFCHR:
#if !defined(MO_WINDOWS)
    case S_IFBLK:
#endif
        info.set_dev_major((s.get_rdev() >> 8) & 255);
        info.set_dev_minor(s.get_rdev() & 255);
        break;

    }

    // gather the user and group names even if we're generally not
    // going to use them (because we prefer to use safer names!)
#if defined(MO_WINDOWS)
    info.set_user("Administrator");
    info.set_group("Administrators");
#else
    struct passwd *pw(getpwuid(s.get_uid()));
    if(pw != NULL)
    {
        info.set_user(pw->pw_name);
    }
    struct group *gr(getgrgid(s.get_gid()));
    if(gr != NULL)
    {
        info.set_group(gr->gr_name);
    }
#endif

    info.set_uid(s.get_uid());
    info.set_gid(s.get_gid());
    info.set_mode(s.get_mode() & ~(S_IFMT));

    switch(s.get_mode() & S_IFMT)
    {
    case S_IFREG:
    case S_IFDIR:
        // no one supports directory sizes in a tarball
        // but we need to have it for dir_size()
        info.set_size(static_cast<int>(s.get_size()));
        break;

    }
}


/** \brief Assign info to a file.
 *
 * This function takes the information defined in the \p info structure
 * and saves it to the file on disk.
 *
 * Note that the err parameter is an in,out which means the function does
 * NOT clear the existing errors. The function sets the following errors
 * flags, eventually:
 *
 * \li file_info_permissions_error
 * \li file_info_owner_error
 *
 * \note
 * The use of the \p err parameter is to implement the --force-file-info
 * and allow failures defining a file information parameters.
 *
 * \exception memfile_exception_io
 * If the function fails, it raises this exception unless the \p err
 * parameter has the file_info_return_errors flag set in which case
 * the function sets an error flag in err and return instead of
 * raising this exception.
 *
 * \param[in] filename  The name of the file to receive the information.
 * \param[in] info  The information to assign to \p filename.
 * \param[in,out] err  A set of flags representing errors that happened.
 */
void memory_file::info_to_disk_file(const wpkg_filename::uri_filename& filename, const file_info& info, int& err)
{
    wpkg_filename::uri_filename::os_filename_t os_name(filename.os_filename());

#ifdef MO_WINDOWS
    // under windows there are 3 flags we could handle:
    //   read-only (done)
    //   hidden
    //   system
    // there is also the archive flag although that should probably
    // not be tweaked by us; at this point I'm not too sure how to
    // get the hidden and system flags in the info flags
    if((info.get_mode() & 0200) == 0) // write on?
    {
        if(!SetFileAttributesW(os_name.get_utf16().c_str(), GetFileAttributesW(os_name.get_utf16().c_str()) | FILE_ATTRIBUTE_READONLY))
        {
            if(err & file_info_return_errors)
            {
                err |= file_info_permissions_error;
            }
            else
            {
                throw memfile_exception_io("cannot SetFileAttributes() of \"" + filename.original_filename() + "\" as expected (not running as Administrator?)");
            }
        }
    }
#else
    if(chmod(os_name.get_utf8().c_str(), info.get_mode()) != 0)
    {
        if(err & file_info_return_errors)
        {
            err |= file_info_permissions_error;
        }
        else
        {
            throw memfile_exception_io("cannot chmod permissions of \"" + filename.original_filename() + "\" as expected (not running as root?)");
        }
    }
#endif

    // gather the user and group names even if we're generally not
    // going to use them (because we prefer to use safer names!)
#ifdef MO_WINDOWS
    // how do we do that? (we certainly need to be an admin to do it too!)
    //info.get_user("Administrator");
    //info.get_group("Administrators");
#else
    struct passwd *pw(getpwnam(info.get_user().c_str()));
    uid_t uid(pw == NULL ? (info.get_user() == "Administrator" ? 0 : info.get_uid()) : pw->pw_uid);
    struct group *gr(getgrnam(info.get_group().c_str()));
    gid_t gid(gr == NULL ? (info.get_user() == "Administrators" ? 0 : info.get_gid()) : gr->gr_gid);
    if(chown(os_name.get_utf8().c_str(), uid, gid) != 0)
    {
        if(err & file_info_return_errors)
        {
            err |= file_info_owner_error;
        }
        else
        {
            throw memfile_exception_io("cannot chown owner/group of \"" + filename.original_filename() + "\" as expected (not running as Administrator/root?)");
        }
    }
#endif
}




memory_file::memory_file()
    //: ... initialized in reset() plus all vars are auto-initialized!
{
    reset();
}

void memory_file::set_filename(const wpkg_filename::uri_filename& filename)
{
    f_filename = filename;
}

const wpkg_filename::uri_filename& memory_file::get_filename() const
{
    return f_filename;
}

void memory_file::guess_format_from_data()
{
    if(!f_created && !f_loaded) {
        f_format = static_cast<int>(file_format_undefined); // FIXME (cast)
        return;
    }
    f_format = static_cast<int>(f_buffer.data_to_format(0, f_buffer.size())); // FIXME (cast)
}

memory_file::file_format_t memory_file::get_format() const
{
    return f_format;
}

bool memory_file::is_text() const
{
    if(!f_created && !f_loaded)
    {
        throw memfile_exception_undefined("this memory file is still undefined, whether it is a text file cannot be determined");
    }

    // TODO: add a test to see whether the file starts with a BOM
    //       and if so verify the file as the corresponding Unicode
    //       encoding instead (i.e. UTF-8, UCS-2, UCS-4)
    int offset(0);
    while(offset < f_buffer.size())
    {
        // TODO: to avoid the read() altogether, move this function inside the
        //       f_buffer implementation so it can directly access the
        //       data without copying it
        int sz(std::min(f_buffer.size() - offset, block_manager::BLOCK_MANAGER_BUFFER_SIZE));
        char buf[block_manager::BLOCK_MANAGER_BUFFER_SIZE];
        f_buffer.read(buf, offset, sz);
        offset += sz;

        for(int i(0); i < sz; ++i) {
            unsigned char c(static_cast<unsigned char>(buf[i]));
            if((c < ' ' || c > 126)
            && (c < 0xA0 /*|| c > 0xFF -- always false warning */)
            && c != '\n' && c != '\r' && c != '\t' && c != '\f') {
                return false;
            }
        }
    }

    return true;
}

memory_file::file_format_t memory_file::data_to_format(const char *data, int bufsize)
{
    if(bufsize >= 3 && data[0] == 0x1F
    && static_cast<unsigned char>(data[1]) == 0x8B && data[2] == 0x08) {
        return file_format_gz;
    }
    if(bufsize >= 3 && data[0] == 'B' && data[1] == 'Z' && data[2] == 'h') {
        return file_format_bz2;
    }
    if(bufsize >= 8 && data[0] == '!' && data[1] == '<' && data[2] == 'a'
    && data[3] == 'r' && data[4] == 'c' && data[5] == 'h' && data[6] == '>'
    && data[7] == 0x0A) {
        return file_format_ar;
    }
    if(bufsize >= 6 && static_cast<unsigned char>(data[0]) == 0xFD
    && data[1] == '7' && data[2] == 'z'
    && data[3] == 'X' && data[4] == 'Z' && data[5] == 0) {
        // http://svn.python.org/projects/external/xz-5.0.3/doc/xz-file-format.txt
        // we cannot throw here since this is used to check files going inside a
        // package as well and these could be compressed with xz
        //throw memfile_exception_compatibility("xz compression is not yet supported");
        return file_format_xz;
    }
    // tarballs should have 'ustar\0' but it could be 'ustar '
    if(bufsize >= 512 && data[0x101] == 'u' && data[0x102] == 's'
    && data[0x103] == 't' && data[0x104] == 'a' && data[0x105] == 'r'
    && (data[0x106] == ' ' || data[0x106] == '\0')) {
        return file_format_tar;
    }
    if(bufsize >= 1024 && data[0] == 'G' && data[1] == 'K'
    && data[2] == 'P' && data[3] == 'W') {
        // at this time we do not support big endian
        return file_format_wpkg;
    }
    // lzma does not have a magic code; the header is defined as:
    //   byte     0 -- properties, usually 0x5D
    //   byte  1..4 -- dictionary size, usually 0x8000 (little endian)
    //   byte 5..13 -- decompressed size or FFFF:FFFF:FFFF:FFFF
    // note that xz is the successor and it should be used whenever
    // possible instead of the lzma format; the heuristic below is based
    // on a document describing the lzma tools and the values they are
    // likely to use in the header
    // http://svn.python.org/projects/external/xz-5.0.3/doc/lzma-file-format.txt
    //
    // dictionary is customarily between 2^16 and 2^25
    if(bufsize >= 13) {
        uint32_t dictionary_size(
              (static_cast<uint32_t>(data[1 + 3]) << 24)
            | (static_cast<uint32_t>(data[1 + 2]) << 16)
            | (static_cast<uint32_t>(data[1 + 1]) <<  8)
            | (static_cast<uint32_t>(data[1 + 0]) <<  0)
        );
        // decompressed size is -1 (unknown) or up to 256Gb
        uint64_t decompressed_size(
              (static_cast<uint64_t>(data[5 + 7]) << 56)
            | (static_cast<uint64_t>(data[5 + 6]) << 48)
            | (static_cast<uint64_t>(data[5 + 5]) << 40)
            | (static_cast<uint64_t>(data[5 + 4]) << 32)
            | (static_cast<uint64_t>(data[5 + 3]) << 24)
            | (static_cast<uint64_t>(data[5 + 2]) << 16)
            | (static_cast<uint64_t>(data[5 + 1]) <<  8)
            | (static_cast<uint64_t>(data[5 + 0]) <<  0)
        );
        if(static_cast<const unsigned char>(data[0]) < (4 * 5 + 4) * 9 + 8 // often data[0] == 0x5D
        && dictionary_size >= 0x8000 && dictionary_size <= 0x02000000
        && (decompressed_size == 0xFFFFFFFFFFFFFFFF || decompressed_size < 0x04000000000)) {
            // we cannot throw here since this is used to check files going inside a
            // package as well and these could be compressed with lzma
            //throw memfile_exception_compatibility("lzma compression is not yet supported");
            return file_format_lzma;
        }
    }
    return file_format_other;
}

/** \brief Transform the filename extension in a file format.
 *
 * This function can be used to transform a filename extension (or previous
 * extension) in a format. In most cases, this is used to infer the output
 * format of a file that's about to be created just by looking at its
 * filename. For example, the following file:
 *
 * \code
 * file.tar.gz
 * \endcode
 *
 * Is a gzip compressed tarball. The format returned is file_format_gz
 * by default. If you set \p ignore_compression to true, then you get
 * file_format_tar instead.
 *
 * Note that this function should never be used for an existing file.
 * In that case you should instead read the file and check its content
 * to infer its format as the data_to_format() function does.
 *
 * \param[in] filename  The name of the file.
 * \param[in] ignore_compression  Ignore the compression extension if there is one.
 *
 * \sa data_to_format()
 * \sa wpkg_filename::uri_filename::extension()
 * \sa wpkg_filename::uri_filename::previous_extension()
 */
memory_file::file_format_t memory_file::filename_extension_to_format(const wpkg_filename::uri_filename& filename, bool ignore_compression)
{
#ifdef WINDOWS
    case_insensitive::case_insensitive_string ext(filename.extension());
#else
    std::string ext(filename.extension());
#endif
    if(ext.empty())
    {
        // no extension, return the default
        return file_format_other;
    }

    // first test compressions so we can then test the previous
    // extension (i.e. so .tar.gz returns file_format_tar)
    file_format_t format(file_format_other);
    if(ext == "gz")
    {
        format = file_format_gz;
    }
    else if(ext == "bz2")
    {
        format = file_format_bz2;
    }
    else if(ext == "lzam")
    {
        // TODO: to fully support dpkg we need to support .lzma (from 7zip)
        format = file_format_lzma;
        // we cannot throw here since this is used to check files going inside a
        // package as well and these could be compressed with xz
        //throw memfile_exception_compatibility("lzma compression is not yet supported");
    }
    else if(ext == "xz")
    {
        // TODO: to fully support dpkg we need to support .xz (from 7zip)
        format = file_format_xz;
        // we cannot throw here since this is used to check files going inside a
        // package as well and these could be compressed with xz
        //throw memfile_exception_compatibility("xz compression is not yet supported");
    }
    if(format != file_format_other)
    {
        if(!ignore_compression)
        {
            // only consider the last extension
            return format;
        }
        // note that previous_extension() == extension() if there is no
        // compression extension
        ext = filename.previous_extension();
    }
    if(ext == "a" || ext == "deb")
    {
        return file_format_ar;
    }
    if(ext == "tar")
    {
        return file_format_tar;
    }
    if(ext == "wpkgar")
    {
        return file_format_wpkg;
    }
    return format;
}

/** \brief Convert a binary buffer to Base64 data.
 *
 * This function converts the memory buffer content pointed by \p buf
 * to a Base64 string. Note however that this conversion does NOT
 * add intermediate new line characters (as required in an email to
 * have lines of about 70 characters instead of one long line.)
 *
 * The \p size parameter indicates the exact size of the buffer in
 * bytes. Note that the encoding requires the addition of padding a
 * the end so the result is always a multiple of 4 characters.
 *
 * \note
 * The code is pretty much a verbatim copy from the QByteArray::toBase64()
 * function from the Qt library.
 *
 * \param[in] buf  The binary buffer to convert.
 * \param[in] size  The number of bytes to convert from \p buf.
 *
 * \return The encoded buffer in Base64 encoding.
 */
std::string memory_file::to_base64(const char *buf, size_t size)
{
    const char alphabet[] = "ABCDEFGH" "IJKLMNOP" "QRSTUVWX" "YZabcdef"
                            "ghijklmn" "opqrstuv" "wxyz0123" "456789+/";
    const char padchar = '=';
    int padlen = 0;

    std::string result;
    result.reserve(size * 4 / 3 + 3);

    for(size_t i(0); i < size; ) {
        // take 3 bytes of input
        int chunk((buf[i++] & 255) << 16);
        if(i == size)
        {
            // only one byte was defined, make sure to add 2 padding bytes
            padlen = 2;
        }
        else
        {
            chunk |= (buf[i++] & 255) << 8;
            if(i == size)
            {
                // only two bytes were defined, make sure to add 1 padding byte
                padlen = 1;
            }
            else
            {
                chunk |= buf[i++] & 255;
            }
        }
     
        // save 4 characters of output
        result += alphabet[chunk >> 18]; // & 0x3F not required because we do not have extra bits
        result += alphabet[(chunk >> 12) & 0x3F];
        if(padlen == 2)
        {
            result += padchar;
        }
        else
        {
            result += alphabet[(chunk >> 6) & 0x3F];
        }
        if(padlen != 0)
        {
            result += padchar;
        }
        else
        {
            result += alphabet[chunk & 0x3F];
        }
    }

    return result;
}

/** \brief Read a file from disk or a remote system.
 *
 * This function reads a file from disk (direct filename) or a remote system
 * (a filename with a scheme that this library understands such as http.)
 *
 * The file is read all at once in this memory file. The load reads blocks
 * as defined by the size of one block (64Kb by default.)
 *
 * At this time the library understands the following filenames:
 *
 * \li a direct filename (i.e. this/file.txt)
 * \li a filename using the file scheme (i.e. file:///full/path/to/this/file.txt)
 * \li a "samba" filename (i.e. smb://server/share/full/path/to/this/file.txt)
 * \li an HTTP URI (i.e. http://server/path/to/this/file.txt)
 *
 * A filename with a scheme other than "file" supports a username and
 * password.
 *
 * \param[in] filename  The name of a file to read from.
 * \param[in] info  The information about this file when available.
 */
void memory_file::read_file(const wpkg_filename::uri_filename& filename, file_info *info)
{
    reset();

    f_filename = filename;

    // WARNING: here the filename may NOT have been canonicalized
    std::string scheme(filename.path_scheme());

//::fprintf(stderr, "read file from [%s] -> [%s] [%s] [%s] [%s] [%s]\n",
//            filename.original_filename().c_str(),
//            filename.path_scheme().c_str(),
//            filename.get_domain().c_str(),
//            filename.get_port().c_str(),
//            filename.get_username().c_str(),
//            filename.get_password().c_str());

    if(scheme == "file" || scheme == "smb")
    {
        wpkg_filename::uri_filename::os_filename_t os_name(filename.os_filename());
        wpkg_stream::fstream file;
        file.open(filename);
        if(!file.good())
        {
            wpkg_filename::uri_filename cwd(wpkg_filename::uri_filename::get_cwd());
            throw memfile_exception_io("cannot open \"" + filename.original_filename() + "\" for reading from current working directory \"" + cwd.os_filename().get_utf8() + "\"");
        }
        file.seek(0, wpkg_stream::fstream::end);
        int file_size(static_cast<int>(file.tell()));
        if(file_size < 0 || !file.good())
        {
            throw memfile_exception_io("invalid file size while reading the file");
        }
        if(file_size > 0)
        {
            file.seek(0, wpkg_stream::fstream::beg);

            // read per block (at most) to avoid allocating a really big buffer
            char buf[block_manager::BLOCK_MANAGER_BUFFER_SIZE];
            int sz(file_size);
            int pos(0);
            while(sz > 0)
            {
                int read_size(std::min(sz, block_manager::BLOCK_MANAGER_BUFFER_SIZE));
                file.read(buf, read_size);
                if(!file.good())
                {
                    // reading of the entire file failed
                    reset();
                    throw memfile_exception_io("reading entire input file \"" + filename.original_filename() + "\" failed");
                }
                f_buffer.write(buf, pos, read_size);
                pos += read_size;
                sz -= read_size;
            }
        }
    }
    else if(scheme == "http" /*|| scheme == "https"*/)
    {
        // make a copy of filename so we can handle redirects and not
        // lose the original filename
        wpkg_filename::uri_filename uri(filename);

        // the only type of files we can gather from HTTP are regular files
        if(info != NULL)
        {
            info->set_file_type(memory_file::file_info::regular_file);
            info->set_mode(0644);
        }
        std::auto_ptr<tcp_client_server::tcp_client> http_client;
        bool redirect;
        std::string location;
        int content_length(-1);
        // TODO: add cache support
        do
        {
            std::string name(uri.path_only());
            redirect = false;
            location.clear();
            int port_number(80);
            std::string port(uri.get_port());
            if(!port.empty())
            {
                port_number = file_info::str_to_int(port.c_str(), static_cast<int>(port.length()), 10);
            }
            if(info != NULL)
            {
                info->set_filename(name);
            }
            std::string request("GET " + name + " HTTP/1.1\r\nHost: " + uri.get_domain() + "\r\n");
            if(!filename.get_username().empty() && !filename.get_password().empty())
            {
                std::string credentials(filename.get_username() + ":" + filename.get_password());
                request += ("Authorization: Basic " + to_base64(credentials.c_str(), credentials.length()) + "\r\n");
            }
            request += "\r\n"; // add an empty line
            http_client.reset(new tcp_client_server::tcp_client(uri.get_domain(), port_number));
            if(http_client->write(request.c_str(), request.length()) != static_cast<int>(request.length()))
            {
                throw memfile_exception_io("error while writing HTTP request for \"" + filename.original_filename() + "\"");
            }

            // the reply is a header followed by the data, here we read the
            // header because it is throw away data at this point
            bool first_line(true);
            for(;;)
            {
                case_insensitive::case_insensitive_string field_name("");
                std::string field_value;
                bool got_name(false), trim(true);
                for(;;)
                {
                    char c;
                    if(http_client->read(&c, 1) != 1)
                    {
                        throw memfile_exception_io("error while reading HTTP response for \"" + filename.original_filename() + "\"");
                    }
                    if(c == '\r')
                    {
                        if(http_client->read(&c, 1) != 1)
                        {
                            throw memfile_exception_io("error while reading HTTP response for \"" + filename.original_filename() + "\"");
                        }
                        if(c != '\n')
                        {
                            throw memfile_exception_io("error while reading HTTP response for \"" + filename.original_filename() + "\": expected \\n");
                        }
                        break;
                    }
                    if(c == '\n')
                    {
                        // '\r' missing?!
                        break;
                    }
                    if(got_name)
                    {
                        if(trim)
                        {
                            trim = isspace(c) != 0;
                        }
                        if(!trim)
                        {
                            field_value += c;
                        }
                    }
                    else if(c == ':' && !first_line)
                    {
                        got_name = true;
                    }
                    else
                    {
                        field_name += c;
                    }
                }
                if(field_name.empty())
                {
                    break;
                }
                if(first_line)
                {
                    first_line = false;
                    // the first line must be HTTP/1.0 200 OK or HTTP/1.1 200 OK
                    // although we want to support 301, 302, and 303 redirects
                    std::string http_protocol(field_name.substr(0, 9));
                    if(http_protocol != "HTTP/1.0 " && http_protocol != "HTTP/1.1 ")
                    {
                        throw memfile_exception_io("HTTP response: is not HTTP/1.0 or HTTP/1.1");
                    }
                    int http_response(file_info::str_to_int(field_name.c_str() + 9, 4, 10));
                    switch(http_response)
                    {
                    case 301: // Moved permanently
                    case 302: // Found
                    case 303: // See Other
                    case 307: // Temporary Redirect
                    case 308: // Permanent Redirect
                        // handle redirect
                        redirect = true;
                        break;

                    case 200: // OK
                        // valid response!
                        break;

                    case 401: // Unauthorized
                        // TBD:
                        // at times servers force you to reply to this one instead of
                        // directly accepting the Authorization: Basic ... field!?
                    default:
                        // TODO: we MUST test the field_name string before printing for security reasons
                        throw memfile_exception_io("HTTP response was " + field_name.substr(9, 3) + ", expected 200 or a redirect");

                    }
                }
                else
                {
                    if(field_name == "Location")
                    {
                        location = field_value;
                    }
                    else if(field_name == "Content-Length")
                    {
                        content_length = file_info::str_to_int(field_value.c_str(), static_cast<int>(field_value.length()), 10);
                    }
                    else if(info != NULL)
                    {
                        if(field_name == "Last-Modified")
                        {
                            struct tm time_info;
                            if(strptime(field_value.c_str(), "%a, %d %b %Y %H:%M:%S %z", &time_info) != NULL)
                            {
                                // unfortunately the tar format does not support time64_t
                                info->set_mtime(mktime(&time_info));
                            }
                            // else -- silent error?
                        }
                    }
                    // other fields of interest?
                }
            }
            if(location.empty())
            {
                if(redirect)
                {
                    throw memfile_exception_io("received an HTTP redirect without a Location field");
                }
            }
            else
            {
                if(!redirect)
                {
                    throw memfile_exception_io("received an HTTP Location field without a redirect response");
                }
                uri.set_filename(location);
                std::string location_scheme(uri.path_scheme());
                if(location_scheme != "http" && location_scheme != "https")
                {
                    throw memfile_exception_io("HTTP redirect has a location not using the HTTP or HTTPS scheme");
                }
                // note that we ignore the new user and password parameters since
                // we continue to use filename.get_username() and filename.get_password()
                // when generating the credentials
            }
        }
        while(!location.empty());

        // now read the file contents
        // we do not trust the Content-Size (or even whether it is present)
        // so we read until we get a read_size of zero
        int pos(0);
        for(; content_length == -1 || pos < content_length;)
        {
            //const int sz(content_length == -1 ? block_manager::BLOCK_MANAGER_BUFFER_SIZE : std::min(content_length - pos, block_manager::BLOCK_MANAGER_BUFFER_SIZE));
            char buf[block_manager::BLOCK_MANAGER_BUFFER_SIZE];
            const int read_size(http_client->read(buf, block_manager::BLOCK_MANAGER_BUFFER_SIZE));
            if(read_size == -1)
            {
                // reading of the entire file failed
                reset();
                throw memfile_exception_io("I/O error while reading HTTP file");
            }
            if(read_size == 0)
            {
                // done!
                break;
            }
            f_buffer.write(buf, pos, read_size);
            pos += read_size;
        }
        if(info != NULL)
        {
            info->set_size(pos);
        }
    }
    else
    {
        throw memfile_exception_parameter("scheme \"" + scheme + "\" (in \"" + filename.original_filename() + "\") not supported by libdebpackages at this point");
    }

    // determine the file format
    if(filename.basename() == "filesmetadata")
    {
        // we cannot really detect the meta format
        f_format = static_cast<int>(file_format_meta); // FIXME cast
    }
    else
    {
        f_format = static_cast<int>(f_buffer.data_to_format(0, f_buffer.size())); // FIXME cast
    }
    if(file_format_wpkg == f_format)
    {
        // in this case we should be able to set the package path automatically
        f_package_path.set_filename(filename.dirname());
        if(f_package_path.empty())
        {
            f_package_path.set_filename(".");
        }
    }

    // file loaded successfully
    f_loaded = true;

    // if user passed an info pointer get extra disk information
    if(info != NULL)
    {
        disk_file_to_info(filename, *info);
    }
}

void memory_file::write_file(const wpkg_filename::uri_filename& filename, bool create_folders, bool force) const
{
    if(!f_created && !f_loaded)
    {
        throw memfile_exception_undefined("this memory file is still undefined and it cannot be written to \"" + filename.original_filename() + "\"");
    }

    if(!filename.is_direct())
    {
        // path has a scheme other than file or smb
        throw memfile_exception_undefined("the specified filename \"" + filename.original_filename() + "\" is not a direct path to a file or network file, write is not permitted");
    }

    if(create_folders)
    {
        // when we create a new file in a sub-folder in the database we
        // want that folder to automatically be created; it's done here
        wpkg_filename::uri_filename dirname(filename.dirname());
        dirname.os_mkdir_p();
    }

    // this assignment may not always be correct, but in most cases it should be fine
    const_cast<wpkg_filename::uri_filename&>(f_filename) = filename;

    wpkg_stream::fstream file;
    file.create(filename);
    if(!file.good())
    {
        if(force)
        {
            // files that are read-only cannot be overwritten without
            // first getting deleted and chmod()'ed so try again if
            // force is true (i.e. we're unpacking a file)
            filename.os_unlink();
            file.create(filename);
        }
        if(!file.good())
        {
            throw memfile_exception_io("opening the output file \"" + filename.original_filename() + "\" failed");
        }
    }
    int offset(0);
    int sz(f_buffer.size());
    while(sz > 0)
    {
        char buf[block_manager::BLOCK_MANAGER_BUFFER_SIZE];
        int write_size(std::min(sz, block_manager::BLOCK_MANAGER_BUFFER_SIZE));
        f_buffer.read(buf, offset, write_size);
        file.write(buf, write_size);
        if(!file.good())
        {
            throw memfile_exception_io("writing the entire file to the output file \"" + filename.original_filename() + "\" failed");
        }
        offset += write_size;
        sz -= write_size;
    }
}


void memory_file::copy(memory_file& destination) const
{
    switch(f_format) {
    case file_format_undefined:
    case file_format_directory:
        throw memfile_exception_parameter("the source file in a copy() call cannot be undefined or a directory");

    default:
        destination.create(f_format);
        {
            char buf[block_manager::BLOCK_MANAGER_BUFFER_SIZE];
            int offset(0);
            int sz(f_buffer.size());
            while(sz >= block_manager::BLOCK_MANAGER_BUFFER_SIZE) {
                f_buffer.read(buf, offset, block_manager::BLOCK_MANAGER_BUFFER_SIZE);
                destination.write(buf, offset, block_manager::BLOCK_MANAGER_BUFFER_SIZE);
                offset += block_manager::BLOCK_MANAGER_BUFFER_SIZE;
                sz -= block_manager::BLOCK_MANAGER_BUFFER_SIZE;
            }
            if(sz > 0) {
                f_buffer.read(buf, offset, sz);
                destination.write(buf, offset, sz);
            }
        }
        break;

    }
}

int memory_file::compare(const memory_file& rhs) const
{
    return f_buffer.compare(rhs.f_buffer);
}

bool memory_file::is_compressed() const
{
    return f_format == file_format_gz || f_format == file_format_bz2
        || f_format == file_format_lzma || f_format == file_format_xz;
}

void memory_file::compress(memory_file& result, file_format_t format, int zlevel) const
{
    if(!f_created && !f_loaded)
    {
        throw memfile_exception_undefined("this memory file is still undefined and it cannot be compressed");
    }
    if(zlevel < 1 || zlevel > 9)
    {
        throw memfile_exception_parameter("zlevel must be between 1 and 9");
    }
    // already compressed?
    switch(f_format)
    {
    case file_format_undefined: // this should not happen here
    case file_format_gz:
    case file_format_bz2:
    case file_format_lzma:
    case file_format_xz:
        throw memfile_exception_compatibility("this memory file is already compressed");

    default:
        // accept others
        break;

    }

    switch(format)
    {
    case file_format_best:
        // in this case we want to try all the compressors
        // and keep the smallest result
        {
            memory_file r1;
            compress_to_gz(result, zlevel);
            compress_to_bz2(r1, zlevel);
            if(r1.size() < result.size()) {
                r1.copy(result);
            }
            // TODO: add the other compressions
        }
        break;

    case file_format_gz:
        compress_to_gz(result, zlevel);
        break;

    case file_format_bz2:
        compress_to_bz2(result, zlevel);
        break;

    // TODO add support for lzma and xz
    default:
        throw memfile_exception_compatibility("the output format must be a supported compressed format");

    }
}

void memory_file::decompress(memory_file& result) const
{
    if(!f_created && !f_loaded) {
        throw memfile_exception_undefined("this memory file is still undefined and it cannot be decompressed");
    }
    // already compressed?
    switch(f_format) {
    case file_format_gz:
        decompress_from_gz(result);
        break;

    case file_format_bz2:
        decompress_from_bz2(result);
        break;

    // TODO add support for lzma and xz
    case file_format_lzma:
    case file_format_xz:
        throw memfile_exception_compatibility("this compression (lzma, xz) is not yet supported by wpkg");

    default:
        throw memfile_exception_compatibility("this memory file is not compressed, see is_compressed()");

    }
}

void memory_file::reset()
{
    f_filename.set_filename("");
    f_format = static_cast<int>(file_format_undefined); // FIXME cast
    f_created = false;
    f_loaded = false;
    f_directory = false;
    f_recursive = true;
    f_dir.reset();
    f_dir_stack.clear();
    f_dir_pos = 0;
    f_buffer.clear();
    f_package_path.clear();
}

void memory_file::create(file_format_t format)
{
    if(format == file_format_undefined)
    {
        throw memfile_exception_parameter("you cannot create an undefined file (use reset() instead?)");
    }

    reset();
    f_format = static_cast<int>(format); // FIXME cast
    f_created = true;

    if(file_format_ar == f_format)
    {
        // the ar format makes use of a magic at the beginning
        // and since we are generating the whole thing we need
        // to write that magic number ourselves
        write("!<arch>\n", 0, 8);
    }
}

void memory_file::end_archive()
{
    switch(f_format) {
    case file_format_tar:
        // tarballs must end with at least 2 empty (all NULLs) blocks
        // then it has to be a multiple of 20 blocks (10Kb)
        char buf[512];
        memset(buf, 0, sizeof(buf));
        f_buffer.write(buf, f_buffer.size(), sizeof(buf));
        f_buffer.write(buf, f_buffer.size(), sizeof(buf));
        while(f_buffer.size() % 10240 != 0) {
            f_buffer.write(buf, f_buffer.size(), sizeof(buf));
        }
        break;

    default:
        // throw if not an archive?
        break;

    }
}

int memory_file::read(char *buffer, int offset, int bufsize) const
{
    if(!f_created && !f_loaded) {
        throw memfile_exception_undefined("you cannot read data from an undefined file");
    }
    if(offset < 0 || offset > f_buffer.size()) {
        throw memfile_exception_parameter("offset is out of bounds");
    }
    if(offset + bufsize > f_buffer.size()) {
        bufsize = f_buffer.size() - offset;
    }
    if(bufsize > 0) {
        f_buffer.read(buffer, offset, bufsize);
    }
    return bufsize;
}

/** \brief Read one line from this memory file.
 *
 * This function reads one line of data from this memory file. Lines are
 * expected to be delimited by "\n", "\r", or "\r\n".
 *
 * The function reads the line in the \p result parameter.
 *
 * The offset is an in-out parameter which gets updated as data is being read
 * from the input file.
 *
 * Note that you should not write to the file when reading data from it in this
 * way as the offset may end up being out of synchronization.
 *
 * If the end of the file is reached, the result parameter is returned empty.
 *
 * \param[in,out] offset  The current position where we read from, usually starts with 0.
 * \param[out] result  The resulting line of data being read.
 *
 * \return true if more data is available, false once the end of the file was reached.
 */
bool memory_file::read_line(int& offset, std::string& result) const
{
    result.clear();

    if(!f_created && !f_loaded) {
        throw memfile_exception_undefined("you cannot read a line of data from an undefined file");
    }
    if(offset < 0 || offset > f_buffer.size()) {
        throw memfile_exception_parameter("offset is out of bounds");
    }
    if(offset == f_buffer.size()) {
        return false;
    }

    // TODO: we may want to move the following to the block manager
    //       so we avoid many small reads and instead go through the
    //       buffers; although buffers are 64Kb each so that's quite
    //       a bit of testing to know which buffer we're currently
    //       in to load the next byte

    // read this line (empty line are returned)
    char buf[1];
    for(; offset < f_buffer.size(); ++offset) {
        f_buffer.read(buf, offset, 1);
        if(buf[0] == '\n' || buf[0] == '\r') {
            break;
        }
        result += buf[0];
    }

    // skip the newline now
    if(offset < f_buffer.size()) {
        f_buffer.read(buf, offset, 1);
        if(buf[0] == '\r') {
            ++offset;
            if(offset < f_buffer.size()) {
                f_buffer.read(buf, offset, 1);
                if(buf[0] == '\n') {
                    // skipping "\r\n" (MS-Windows)
                    ++offset;
                }
            }
            // else skipping "\r" (Mac)
        }
        else if(buf[0] == '\n') {
            // skipping "\n" (Unix)
            ++offset;
        }
    }

    return true;
}

int memory_file::write(const char *buffer, int offset, int bufsize)
{
    if(file_format_undefined == f_format)
    {
        throw memfile_exception_undefined("you cannot write data to an undefined file; use create() or read_file() first");
    }
    f_buffer.write(buffer, offset, bufsize);
    return bufsize;
}

void memory_file::printf(const char *format, ...)
{
    // we don't expect to use this function with format so large
    // that 1Kb is not enough
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    vsnprintf(buf, sizeof(buf) - 1, format, ap);
    buf[sizeof(buf) - 1] = '\0';
    int len(static_cast<int>(strlen(buf)));
    if(len >= static_cast<int>(sizeof(buf) - 1)) {
        throw memfile_exception_parameter("buffer too small in memory_file::printf()");
    }
    // we always append in this function
    write(buf, f_buffer.size(), len);
}

void memory_file::append_file(const file_info& info, const memory_file& data)
{
    switch(f_format) {
    case file_format_ar:
        append_ar(info, data);
        break;

    case file_format_tar:
        append_tar(info, data);
        break;

    case file_format_wpkg:
        append_wpkg(info, data);
        break;

    default:
        throw memfile_exception_compatibility("only archive files support the append_file() function");

    }
}

/** \brief Return the size, in bytes, of the file.
 *
 * This function returns the byte size of the memory file. Note that a disk
 * directory (as opened with a call to the dir_rewind() function) has a
 * size equal to the number of files read so far in the directory (actually
 * +1 while reading.)
 *
 * If the file was not created or loaded, then the size is zero. It is not
 * an error to retrieve the size of such memory_file objects.
 *
 * \return The memory file size in byte (or number of files for a directory.)
 */
int memory_file::size() const
{
    switch(f_format) {
    case file_format_directory:
        return f_dir_size;

    default:
        return f_buffer.size();

    }
}

void memory_file::dir_rewind(const wpkg_filename::uri_filename& path, bool recursive)
{
    f_dir.reset();

    f_directory = !path.empty();
    if(f_directory)
    {
        f_format = static_cast<int>(file_format_directory); // FIXME cast
        f_recursive = recursive;
        f_dir.reset(new wpkg_filename::os_dir(path));
        f_dir_size = 1;
    }
    else
    {
        // that does not really apply in this case
        f_recursive = false;
    }

    // in case of an ar archive we want to skip the magic code
    // at the very beginning of the file
    f_dir_pos = file_format_ar == f_format ? 8 : 0;
    if(f_dir_pos > f_buffer.size())
    {
        f_dir_pos = f_buffer.size();
    }
}

int memory_file::dir_pos() const
{
    // Note: at this point we do not know whether it is legal to call this
    // function (i.e. whether dir_rewind() was ever called)
    switch(f_format)
    {
    case file_format_directory:
    case file_format_ar:
    case file_format_tar:
    case file_format_zip:
    case file_format_7z:
    case file_format_wpkg:
    case file_format_meta:
        break;

    default:
        throw std::logic_error("dir_pos() cannot be called with a file that is not an archive or a directory");

    }
    return f_dir_pos;
}

bool memory_file::dir_next(file_info& info, memory_file *data) const
{
    if(data != NULL)
    {
        data->reset();
    }
    info.reset();

    if(!f_created && !f_loaded && !f_directory)
    {
        throw memfile_exception_undefined("you cannot read a directory from an undefined or incompatible file");
    }

    if(f_dir_pos >= size())
    {
        // end of directory reached
        return false;
    }

    int block_size(1);
    switch(f_format)
    {
    case file_format_directory:
        // the block size and stuff won't work right for disk directories
        // instead we handle the f_dir_pos and f_size in dir_next_dir(),
        // the f_dir_pos represents a file number rather than an offset
        if(!dir_next_dir(info))
        {
            return false;
        }
        switch(info.get_file_type())
        {
        case file_info::regular_file:
        case file_info::continuous: // this should not happen
            if(data != NULL) {
                // user wants a copy of the data!
                data->read_file(info.get_filename());
            }
            break;

        default:
            // ignore special files and directories
            break;

        }
        return true;

    case file_format_ar:
        dir_next_ar(info);
        block_size = 2;
        break;

    case file_format_tar:
        if(!dir_next_tar(info))
        {
            return false;
        }
        block_size = 512;
        break;

    case file_format_wpkg:
        // data read by dir_next_wpkg() if required
        dir_next_wpkg(info, data);
        return true;

    case file_format_meta:
        if(!dir_next_meta(info))
        {
            return false;
        }
        break;

    default:
        throw memfile_exception_compatibility("you cannot read a directory from a file that is not an archive");

    }

    int adjusted_size((info.get_size() + block_size - 1) & ~(block_size - 1));
    if(f_dir_pos + adjusted_size > f_buffer.size())
    {
        info.set_size(0);
        throw memfile_exception_io("archive file data out of bounds (invalid size)");
    }

    // the size counts only if the file is a regular file or continuous
    switch(info.get_file_type()) {
    case file_info::regular_file:
    case file_info::continuous:
        if(data != NULL)
        {
            // user wants a copy of the data!
            data->create(f_buffer.data_to_format(f_dir_pos, info.get_size()));
            char buf[block_manager::BLOCK_MANAGER_BUFFER_SIZE];
            int in_offset(f_dir_pos);
            int out_offset(0);
            int sz(info.get_size());
            while(sz >= block_manager::BLOCK_MANAGER_BUFFER_SIZE)
            {
                f_buffer.read(buf, in_offset, block_manager::BLOCK_MANAGER_BUFFER_SIZE);
                data->write(buf, out_offset, block_manager::BLOCK_MANAGER_BUFFER_SIZE);
                in_offset += block_manager::BLOCK_MANAGER_BUFFER_SIZE;
                out_offset += block_manager::BLOCK_MANAGER_BUFFER_SIZE;
                sz -= block_manager::BLOCK_MANAGER_BUFFER_SIZE;
            }
            if(sz > 0) {
                f_buffer.read(buf, in_offset, sz);
                data->write(buf, out_offset, sz);
            }
        }

        f_dir_pos += adjusted_size;
        break;

    default:
        // by special files or directory data have no data per se
        break;

    }

    return true;
}

int memory_file::dir_size(const wpkg_filename::uri_filename& path, int& disk_size, int block_size)
{
    int byte_size(0);
    disk_size = 0;
    if(!path.exists())
    {
        throw memfile_exception_io("cannot access specified directory or file \"" + path.original_filename() + "\"");
    }
    if(!path.is_dir())
    {
        // it's not a directory, just return that one file size
        wpkg_filename::uri_filename::file_stat s;
        path.os_stat(s);
        byte_size = static_cast<int>(s.get_size());
        disk_size = (byte_size + block_size - 1) / block_size;
    }
    else
    {
        dir_rewind(path);
        file_info info;
        while(dir_next(info))
        {
            std::string bn(info.get_basename());
            if(bn != "." && bn != "..")
            {
                // note: this does not compute the real size used on disk, but
                //       it generally is close, although the more files we
                //       handle the wider the gap becomes it is acceptable
                //       because the computers used to create packages are often
                //       different from the computers where they will be
                //       installed and trying to get a perfect size will not
                //       help at this stage
                int file_size(static_cast<int>(info.get_size()));
                byte_size += file_size;
                disk_size += (file_size + block_size - 1) / block_size;
            }
        }
    }
    disk_size *= block_size;
    return byte_size;
}

void memory_file::set_package_path(const wpkg_filename::uri_filename& path)
{
    f_package_path = path;
}

void memory_file::raw_md5sum(md5::raw_md5sum& raw) const
{
    if(!f_created && !f_loaded)
    {
        throw memfile_exception_undefined("you cannot compute an md5 sum from an undefined file");
    }
    md5::md5sum sum;

    char buf[block_manager::BLOCK_MANAGER_BUFFER_SIZE];
    int offset(0);
    int sz(f_buffer.size());
    while(sz >= block_manager::BLOCK_MANAGER_BUFFER_SIZE)
    {
        f_buffer.read(buf, offset, block_manager::BLOCK_MANAGER_BUFFER_SIZE);
        sum.push_back(reinterpret_cast<uint8_t *>(buf), block_manager::BLOCK_MANAGER_BUFFER_SIZE);
        offset += block_manager::BLOCK_MANAGER_BUFFER_SIZE;
        sz -= block_manager::BLOCK_MANAGER_BUFFER_SIZE;
    }
    if(sz > 0)
    {
        f_buffer.read(buf, offset, sz);
        sum.push_back(reinterpret_cast<uint8_t *>(buf), sz);
    }

    sum.raw_sum(raw);
}

std::string memory_file::md5sum() const
{
    if(!f_created && !f_loaded)
    {
        throw memfile_exception_undefined("you cannot compute an md5 sum from an undefined file");
    }

    md5::md5sum sum;

    char buf[block_manager::BLOCK_MANAGER_BUFFER_SIZE];
    int offset(0);
    int sz(f_buffer.size());
    while(sz >= block_manager::BLOCK_MANAGER_BUFFER_SIZE)
    {
        f_buffer.read(buf, offset, block_manager::BLOCK_MANAGER_BUFFER_SIZE);
        sum.push_back(reinterpret_cast<uint8_t *>(buf), block_manager::BLOCK_MANAGER_BUFFER_SIZE);
        offset += block_manager::BLOCK_MANAGER_BUFFER_SIZE;
        sz -= block_manager::BLOCK_MANAGER_BUFFER_SIZE;
    }
    if(sz > 0)
    {
        f_buffer.read(buf, offset, sz);
        sum.push_back(reinterpret_cast<uint8_t *>(buf), sz);
    }

    return sum.sum();
}

void memory_file::compress_to_gz(memory_file& result, int zlevel) const
{
    gz_deflate gz(zlevel);
    gz.compress(result, f_buffer);
}

void memory_file::compress_to_bz2(memory_file& result, int zlevel) const
{
    bz2_deflate bz2(zlevel);
    bz2.compress(result, f_buffer);
}

void memory_file::decompress_from_gz(memory_file& result) const
{
    gz_inflate gz;
    gz.decompress(result, f_buffer);
}

/** \brief Decompress this memory file.
 *
 * This function decompresses this memory file in the \p result
 * memory file. This function makes use of our internal bz2_inflate
 * class to handle the feat.
 */
void memory_file::decompress_from_bz2(memory_file& result) const
{
    bz2_inflate bz2;
    bz2.decompress(result, f_buffer);
}

/** \brief Read information about a file from disk.
 *
 * This function directly reads information from the disk directory
 * as specified in the dir_rewind() function.
 *
 * When the function returns false, the \p info parameter was not modified.
 *
 * \param[out] info  The file_info structure where the file information is saved.
 *
 * \return true if a file was read, false otherwise
 */
bool memory_file::dir_next_dir(file_info& info) const
{
    wpkg_filename::uri_filename file;
    while(!f_dir->read(file))
    {
        if(f_dir_stack.size() == 0)
        {
            // f_size is 1 more than we've read so far
            // decrement by one and we're at EOD...
            --const_cast<memory_file *>(this)->f_dir_size;
            return false;
        }
        f_dir = f_dir_stack.back();
        f_dir_stack.pop_back();
    }

    memory_file::disk_file_to_info(file, info);
    if(f_recursive && info.get_file_type() == file_info::directory)
    {
        // never recurse through the "." and ".." folders
        std::string bn(info.get_basename());
        if(bn != "." && bn != "..")
        {
            f_dir_stack.push_back(f_dir);
            f_dir.reset(new wpkg_filename::os_dir(info.get_uri()));
        }
    }
    ++const_cast<memory_file *>(this)->f_dir_size;
    ++f_dir_pos;
    return true;
}

/** \brief Read an ar archive header.
 *
 * This function reads one file from an ar archive.
 *
 * A file header in an ar archive is defined as follow:
 *
 * \code
 * char ar_name[16];        -- Member file name, sometimes / terminated
 * char ar_date[12];        -- File date, decimal seconds since Epoch
 * char ar_uid[6];          -- User and group IDs, in ASCII decimal
 * char ar_gid[6];
 * char ar_mode[8];         -- File mode, in ASCII octal
 * char ar_size[10];        -- File size, in ASCII decimal
 * char ar_fmag[2];         -- Always contains "`\n"
 * \endcode
 *
 * The total size of the header is 60 characters.
 *
 * \param[out] info  The file information
 */
void memory_file::dir_next_ar(file_info& info) const
{
    // archive file information is only defined even boundaries
    if((f_dir_pos & 1) != 0)
    {
        throw memfile_exception_compatibility("f_dir_pos cannot be odd when reading an ar archive");
    }

    if(f_dir_pos + 60 > size())
    {
        throw memfile_exception_io("ar header out of bounds (invalid size)");
    }
    char p[60];
    f_buffer.read(p, f_dir_pos, 60);

    // verify the magic code first
    if(p[58] != '`' || p[59] != 0x0A) // char ar_fmag[2]
    {
        throw memfile_exception_io("invalid magic code in ar header");
    }
    // if the filename is // followed by spaces then it's a long filename
    // which makes use of a second (long) header
    if(p[0] == '/' && p[1] == '/')
    {
        throw memfile_exception_io("long ar filename are not yet supported");
    }

    // remove the / at the end of the name if present
    const char *e(reinterpret_cast<const char *>(memchr(p, '/', 16)));
    if(e != NULL)
    {
        const int sz(static_cast<int>(e - p));
        if(sz == 0)
        {
            throw memfile_exception_io("found an empty ar filename (starting with a /)");
        }
        info.set_filename(p, sz); // char ar_name[16]
    }
    else
    {
        // debian archives do not include the '/'
        int sz(16);
        for(; sz > 0 && p[sz - 1] == ' '; --sz);
        if(sz == 0)
        {
            throw memfile_exception_io("found an empty ar filename (all spaces)");
        }
        info.set_filename(p, sz); // char ar_name[16]
    }
    info.set_mtime(p + 16,  12, 10); // char ar_date[12]
    info.set_uid  (p + 16 + 12,  6, 10); // char ar_uid[6]
    info.set_gid  (p + 16 + 12 + 6,  6, 10); // char ar_gid[6]
    info.set_mode (p + 16 + 12 + 6 + 6,  8, 8); // char ar_mode[8]
    info.set_size (p + 16 + 12 + 6 + 6 + 8, 10, 10); // char ar_size[10]

    f_dir_pos += 60;
}

/** \brief Copy a tar archive header into a file_info structure.
 *
 * \code
 * from tar.h
 * field    offset  size    comment
 * name     0       100     NUL-terminated if NUL fits
 * mode     100     8
 * uid      108     8
 * gid      116     8
 * size     124     12
 * mtime    136     12
 * chksum   148     8
 * typeflag 156     1       see below
 * linkname 157     100     NUL-terminated if NUL fits
 * magic    257     6       must be TMAGIC (NUL term.)
 * version  263     2       must be TVERSION
 * uname    265     32      NUL-terminated
 * gname    297     32      NUL-terminated
 * devmajor 329     8
 * devminor 337     8
 * prefix   345     155     NUL-terminated if NUL fits
 * unused   500     12
 * \endcode
 *
 * \param[out] info  The file information
 *
 * \return The tar archive may include empty blocks at the end (up to 21 blocks)
 * this function returns false when such a block is found, true otherwise.
 */
bool memory_file::dir_next_tar(file_info& info) const
{
    if(!dir_next_tar_read(info))
    {
        return false;
    }

    std::string long_symlink;
    if(info.get_file_type() == file_info::long_symlink)
    {
        int adjusted_size((info.get_size() + 511) & ~511);
        if(f_dir_pos + adjusted_size > f_buffer.size())
        {
            info.set_size(0);
            throw memfile_exception_io("archive file data out of bounds when looking into reading a GNU long link (invalid size)");
        }

        // note that the size is likely to include a null terminator
        long_symlink.resize(info.get_size());
        f_buffer.read(&long_symlink[0], f_dir_pos, static_cast<int>(long_symlink.size()));
        if(long_symlink[info.get_size() - 1] == '\0')
        {
            long_symlink.resize(info.get_size() - 1);
        }

        f_dir_pos += adjusted_size;

        // we expect the real info or a long filename now
        if(!dir_next_tar_read(info))
        {
            return false;
        }
    }

    std::string long_filename;
    if(info.get_file_type() == file_info::long_filename)
    {
        int adjusted_size((info.get_size() + 511) & ~511);
        if(f_dir_pos + adjusted_size > f_buffer.size())
        {
            info.set_size(0);
            throw memfile_exception_io("archive file data out of bounds when looking into reading a GNU long filename (invalid size)");
        }

        // note that the size is likely to include a null terminator
        long_filename.resize(info.get_size());
        f_buffer.read(&long_filename[0], f_dir_pos, static_cast<int>(long_filename.size()));
        if(long_filename[info.get_size() - 1] == '\0')
        {
            long_filename.resize(info.get_size() - 1);
        }

        f_dir_pos += adjusted_size;

        // we expect the real info now
        if(!dir_next_tar_read(info))
        {
            return false;
        }
    }

    std::string long_mtime;
    std::string long_ctime;
    std::string long_atime;
    if(info.get_file_type() == file_info::pax_header)
    {
        int adjusted_size((info.get_size() + 511) & ~511);
        if(f_dir_pos + adjusted_size > f_buffer.size())
        {
            info.set_size(0);
            throw memfile_exception_io("archive file data out of bounds when looking into reading a PaxHeader (invalid size)");
        }

        // a PaxHeader is formed by a set of lines defined as:
        //   "<size> <name>=<value>\n"
        // see IBM website:
        //   http://publib.boulder.ibm.com/infocenter/zos/v1r13/index.jsp?topic=%2Fcom.ibm.zos.r13.bpxa500%2Fpxarchfm.htm
        std::string paxheader;
        paxheader.resize(info.get_size());
        f_buffer.read(&paxheader[0], f_dir_pos, static_cast<int>(paxheader.size()));
        f_dir_pos += adjusted_size;

        for(const char *x(paxheader.c_str()); *x != '\0'; ++x)
        {
            const char *start(x);
            for(; *x != '\0' && *x != '\n'; ++x);
            std::string l(start, x - start);
            std::string::size_type space_pos(l.find_first_of(' '));
            if(space_pos == std::string::npos)
            {
                throw memfile_exception_io("invalid PaxHeader (no space in a line)");
            }
            // TODO: verify the size?
            std::string v(l.substr(space_pos + 1));
            std::string::size_type equal_pos(v.find_first_of('='));
            if(equal_pos == std::string::npos)
            {
                throw memfile_exception_io("invalid PaxHeader (no equal for the field/value entry)");
            }
            std::string name(v.substr(0, equal_pos));
            std::string value(v.substr(equal_pos + 1));
            if(name == "path")
            {
                long_filename = value;
            }
            else if(name == "mtime")
            {
                long_mtime = value;
            }
            else if(name == "ctime")
            {
                long_ctime = value;
            }
            else if(name == "atime")
            {
                long_atime = value;
            }
        }

        // we expect the real info now
        if(!dir_next_tar_read(info))
        {
            return false;
        }
    }

    // at this point we must have a "normal" block
    if(info.get_file_type() == file_info::long_filename
    || info.get_file_type() == file_info::long_symlink
    || info.get_file_type() == file_info::pax_header)
    {
        info.set_size(0);
        throw memfile_exception_io("invalid GNU extension found in archive file (file content expected)");
    }

    if(!long_symlink.empty())
    {
        info.set_link(long_symlink);
    }
    if(!long_filename.empty())
    {
        info.set_filename(long_filename);
    }
    if(!long_mtime.empty())
    {
        std::string::size_type p(long_mtime.find_first_of('.'));
        if(p == std::string::npos)
        {
            p = long_mtime.length();
        }
        info.set_mtime(long_mtime.c_str(), static_cast<int>(p), 10);
    }
    if(!long_ctime.empty())
    {
        std::string::size_type p(long_ctime.find_first_of('.'));
        if(p == std::string::npos)
        {
            p = long_ctime.length();
        }
        info.set_ctime(long_ctime.c_str(), static_cast<int>(p), 10);
    }
    if(!long_atime.empty())
    {
        std::string::size_type p(long_atime.find_first_of('.'));
        if(p == std::string::npos)
        {
            p = long_atime.length();
        }
        info.set_atime(long_atime.c_str(), static_cast<int>(p), 10);
    }

    return true;
}


bool memory_file::dir_next_tar_read(file_info& info) const
{
    // archive file information is only defined even boundaries
    if((f_dir_pos & 511) != 0)
    {
        throw memfile_exception_compatibility("f_dir_pos must be a multiple of 512 when reading a tar archive");
    }

    if(f_dir_pos + 512 > size())
    {
        throw memfile_exception_io("tar header out of bounds (invalid size)");
    }
    char p[512];
    f_buffer.read(p, f_dir_pos, 512);

    // verify the magic code first (ignore the version)
    if(p[257] != 'u' || p[258] != 's' || p[259] != 't' || p[260] != 'a'
    || p[261] != 'r' || (p[262] != ' ' && p[262] != '\0'))
    {
        // if the ustar is not present, we may have reached the end of the file
        // in that case it has to be all zeroes; we do not force such empty
        // blocks but if there we verify they are as expected (zeroes)
        for(;;)
        {
            for(const char *e(p); e < p + 512; ++e)
            {
                if(*e != 0)
                {
                    throw memfile_exception_io("invalid magic code in tar header");
                }
            }
            f_dir_pos += 512;
            if(f_dir_pos == size())
            {
                return false;
            }
            if(f_dir_pos + 512 > size())
            {
                throw memfile_exception_io("tar header out of bounds (invalid size)");
            }
            f_buffer.read(p, f_dir_pos, 512);
        }
    }
    if(tar_check_sum(p) != static_cast<uint32_t>(file_info::str_to_int(p + 148, 8, 8)))
    {
        throw memfile_exception_io("invalid checksum code in tar header");
    }

    // a tar filename may be broken up in two
    // also, we canonicalize filenames (in case \ instead of / was used...)
    if(p[345] != '\0')
    {
        // we have a prefix
        std::string prefix, name;
        prefix.assign(p + 345, file_info::strnlen(p + 345, 155));
        name.assign(p + 0, file_info::strnlen(p + 0, 100));
        wpkg_filename::uri_filename filename(prefix);
        filename = filename.append_child(name);
        info.set_uri(filename);
        info.set_filename(filename.path_only(false));
    }
    else
    {
        std::string name;
        name.assign(p + 0, file_info::strnlen(p + 0, 100));
        const wpkg_filename::uri_filename filename(name);
        info.set_uri(filename);
        info.set_filename(filename.path_only(false));
    }

    switch(p[156]) // file type (typeflag)
    {
    case '\0':
    case '0':
        info.set_file_type(file_info::regular_file);
        break;

    case '1':
        info.set_file_type(file_info::hard_link);
        break;

    case '2':
        info.set_file_type(file_info::symbolic_link);
        break;

    case '3':
        info.set_file_type(file_info::character_special);
        break;

    case '4':
        info.set_file_type(file_info::block_special);
        break;

    case '5':
        info.set_file_type(file_info::directory);
        break;

    case '6':
        info.set_file_type(file_info::fifo);
        break;

    case '7':
        info.set_file_type(file_info::continuous);
        break;

    case 'K': // symlink
        info.set_file_type(file_info::long_symlink);
        break;

    case 'L': // long name
        info.set_file_type(file_info::long_filename);
        break;

    case 'x': // PaxHeader
        info.set_file_type(file_info::pax_header);
        break;

    default:
        {
        std::string type(p + 156, 1);
        throw memfile_exception_compatibility("unknown tar file type: '" + type + "'");
        }

    }

    info.set_mode     (p + 100,  8, 8);
    info.set_uid      (p + 108,  8, 8);
    info.set_gid      (p + 116,  8, 8);
    info.set_size     (p + 124, 12, 8);
    info.set_mtime    (p + 136, 12, 8);
    info.set_link     (p + 157, 100);
    info.set_user     (p + 265, 32);
    info.set_group    (p + 297, 32);
    info.set_dev_major(p + 329, 8, 8);
    info.set_dev_minor(p + 337, 8, 8);

    f_dir_pos += 512;

    return true;
}

void memory_file::dir_next_wpkg(file_info& info, memory_file *data) const
{
    // archive file information is only defined even boundaries
    if((f_dir_pos & 1023) != 0)
    {
        throw memfile_exception_compatibility("f_dir_pos must be a multiple of 1024 when reading a wpkgar archive");
    }

    if(f_dir_pos + 1024 > size())
    {
        throw memfile_exception_io("wpkg archive header out of bounds (invalid size)");
    }
    char p[sizeof(wpkgar::wpkgar_block_t)];
    f_buffer.read(p, f_dir_pos, sizeof(wpkgar::wpkgar_block_t));
    f_dir_pos += sizeof(wpkgar::wpkgar_block_t);
    const wpkgar::wpkgar_block_t *header(reinterpret_cast<const wpkgar::wpkgar_block_t *>(p));

    // verify the magic code first (ignore the version)
    if(header->f_magic != wpkgar::WPKGAR_MAGIC)
    {
        throw memfile_exception_io("invalid magic code in wpkg archive header");
    }
    // we support version 1.0 (1000) and 1.1 (1001)
    int version(0);
    if(memcmp(header->f_version, wpkgar::WPKGAR_VERSION_1_0, sizeof(header->f_version)) == 0)
    {
        version = 1000;
    }
    else if(memcmp(header->f_version, wpkgar::WPKGAR_VERSION_1_1, sizeof(header->f_version)) == 0)
    {
        version = 1001;
    }
    else
    {
        throw memfile_exception_io("unsupported version in wpkg archive header");
    }
    if(wpkg_check_sum(reinterpret_cast<const uint8_t *>(p)) != header->f_checksum)
    {
        throw memfile_exception_io("invalid checksum code in wpkg archive header");
    }

    switch(header->f_type) {
    case wpkgar::wpkgar_block_t::WPKGAR_TYPE_REGULAR:
    case wpkgar::wpkgar_block_t::WPKGAR_TYPE_PACKAGE:
        info.set_file_type(file_info::regular_file);
        break;

    case wpkgar::wpkgar_block_t::WPKGAR_TYPE_HARD_LINK:
        info.set_file_type(file_info::hard_link);
        break;

    case wpkgar::wpkgar_block_t::WPKGAR_TYPE_SYMBOLIC_LINK:
        info.set_file_type(file_info::symbolic_link);
        break;

    case wpkgar::wpkgar_block_t::WPKGAR_TYPE_CHARACTER_SPECIAL:
        info.set_file_type(file_info::character_special);
        break;

    case wpkgar::wpkgar_block_t::WPKGAR_TYPE_BLOCK_SPECIAL:
        info.set_file_type(file_info::block_special);
        break;

    case wpkgar::wpkgar_block_t::WPKGAR_TYPE_DIRECTORY:
        info.set_file_type(file_info::directory);
        break;

    case wpkgar::wpkgar_block_t::WPKGAR_TYPE_FIFO:
        info.set_file_type(file_info::fifo);
        break;

    case wpkgar::wpkgar_block_t::WPKGAR_TYPE_CONTINUOUS:
        info.set_file_type(file_info::continuous);
        break;

    default:
        throw memfile_exception_compatibility("unknown wpkgar file type");

    }

    info.set_uid(header->f_uid);
    info.set_gid(header->f_gid);
    info.set_mode(header->f_mode);
    info.set_size(header->f_size);
    info.set_mtime(header->f_mtime);
    info.set_dev_major(header->f_dev_major);
    info.set_dev_minor(header->f_dev_minor);
    md5::raw_md5sum raw;
    memcpy(raw.f_sum, header->f_md5sum, md5::raw_md5sum::MD5SUM_RAW_BUFSIZ);
    info.set_raw_md5sum(raw);
    info.set_original_compression(static_cast<wpkgar::wpkgar_block_t::wpkgar_compression_t>(header->f_original_compression));

    info.set_filename(reinterpret_cast<const char *>(header->f_name), 300);
    info.set_link(reinterpret_cast<const char *>(header->f_link), 300);

    info.set_user(reinterpret_cast<const char *>(header->f_user), 32);
    info.set_group(reinterpret_cast<const char *>(header->f_group), 32);

    if(version == 1001)
    {
        // filename too long?
        if(header->f_name_size > 0)
        {
            std::vector<char> filename;
            const int filename_size((header->f_name_size + sizeof(wpkgar::wpkgar_block_t) - 1) & ~(sizeof(wpkgar::wpkgar_block_t) - 1));
            filename.resize(filename_size);
            f_buffer.read(&filename[0], f_dir_pos, filename_size);
            info.set_filename(&filename[0], header->f_name_size);
            f_dir_pos += filename_size;
        }
        // symbolic link too long?
        if(header->f_link_size > 0)
        {
            std::vector<char> link;
            const int link_size((header->f_name_size + sizeof(wpkgar::wpkgar_block_t) - 1) & ~(sizeof(wpkgar::wpkgar_block_t) - 1));
            link.resize(link_size);
            f_buffer.read(&link[0], f_dir_pos, link_size);
            info.set_link(&link[0], header->f_name_size);
            f_dir_pos += link_size;
        }
    }

    if(data != NULL) {
        switch(header->f_type) {
        case wpkgar::wpkgar_block_t::WPKGAR_TYPE_REGULAR:
        case wpkgar::wpkgar_block_t::WPKGAR_TYPE_CONTINUOUS:
            // user requested for the file to be loaded
            if(f_package_path.empty())
            {
                throw memfile_exception_parameter("the f_package_path was not defined, call set_package_path()");
            }
            data->read_file(f_package_path.append_child(info.get_filename()));
            break;

        }
    }
}

namespace {
bool not_isspace(char c)
{
    return !isspace(c);
}
}

bool memory_file::dir_next_meta(file_info& info) const
{
    // read the next line, if empty or comment, silently skip
    std::string line;
    do
    {
        int offset(f_dir_pos);
        if(!read_line(offset, line))
        {
            return false;
        }
        f_dir_pos = offset;
        // left trim
        // find_if_not() is C++11
        line.erase(0, std::find_if(line.begin(), line.end(), not_isspace) - line.begin());
    }
    while(line.empty() || line[0] == '#');

    // parse the line
    // format is:
    //      1. Type/Mode
    //      2. User
    //      3. Group
    //      4. Date
    //      5. Device
    //      6. Filename
    const char *l(line.c_str());

    // 1. Type/Mode
    if(l[0] == '-' && isspace(l[1]))
    {
        // no type/permission specified
        l += 2;
    }
    else
    {
        int mode(0);
        int i(0);
        for(; !isspace(*l); ++i, ++l)
        {
            if(*l == '\0')
            {
                throw memfile_exception_invalid("file meta data cannot only include a type");
            }
            if(i > 10)
            {
                throw memfile_exception_invalid("file meta data type and permission field has to be exactly 10 characters");
            }
            switch(i)
            {
            case 0:
                // type
                switch(*l)
                {
                case 'b': // block special
                    info.set_file_type(file_info::block_special);
                    break;

                case 'C': // continuous
                    info.set_file_type(file_info::continuous);
                    break;

                case 'c': // character special
                    info.set_file_type(file_info::character_special);
                    break;

                case 'd': // directory
                    info.set_file_type(file_info::directory);
                    break;

                case 'p': // fifo
                    info.set_file_type(file_info::fifo);
                    break;

                case 'h': // hard link
                    info.set_file_type(file_info::hard_link);
                    break;

                case 'l': // symbolic link
                    info.set_file_type(file_info::symbolic_link);
                    break;

                case '-': // regular file
                    info.set_file_type(file_info::regular_file);
                    break;

                default:
                    throw memfile_exception_invalid("unknown file type in the mode of a file meta data definition, \"" + line + "\"");

                }
                continue;

            case 1: // owner read
            case 4: // group read
            case 7: // other read
                if(*l != '-' && *l != 'r')
                {
                    throw memfile_exception_invalid("a read flag in your mode must either be 'r' or '-', \"" + line + "\"");
                }
                break;

            case 2: // owner write
            case 5: // group write
            case 8: // other write
                if(*l != '-' && *l != 'w')
                {
                    throw memfile_exception_invalid("a write flag in your mode must either be 'w' or '-', \"" + line + "\"");
                }
                break;

            case 3: // owner execute
                if(*l == 's')
                {
                    mode |= 04100;
                    continue;
                }
                if(*l == 'S')
                {
                    mode |= 04000;
                    continue;
                }
            case 6: // group execute
                if(*l == 's')
                {
                    mode |= 02010;
                    continue;
                }
                if(*l == 'S')
                {
                    mode |= 02000;
                    continue;
                }
            case 9: // other execute
                if(i == 9)
                {
                    if(*l == 't')
                    {
                        mode |= 01001;
                        continue;
                    }
                    if(*l == 'T')
                    {
                        mode |= 01000;
                        continue;
                    }
                }
                if(*l != '-' && *l != 'x')
                {
                    throw memfile_exception_invalid("an execute flag in your mode must either be 'x' or '-', \"" + line + "\"");
                }
                break;

            }
            if(*l != '-')
            {
                mode |= 1 << (9 - i);
            }
        }
        if(i != 10)
        {
            throw memfile_exception_invalid("file meta data type and permission field has to be exactly 10 characters");
        }
        info.set_mode(mode);
    }
    for(; isspace(*l); ++l);

    // 2. User
    if(l[0] == '-' && isspace(l[1]))
    {
        // no uid/user specified
        l += 2;
    }
    else if(l[0] == '-' && l[1] == '/' && l[2] == '-' && isspace(l[3]))
    {
        // no uid/user specified
        l += 4;
    }
    else
    {
        const char *uid(l);
        for(; *l != '/'; ++l)
        {
            if(*l == '\0' || isspace(*l))
            {
                throw memfile_exception_invalid("unterminated user identifier and name in \"" + line + "\"");
            }
        }
        if(l == uid)
        {
            throw memfile_exception_invalid("a user identifier cannot be empty");
        }
        if(l - uid > 1 || *uid != '-')
        {
            info.set_uid(uid, static_cast<int>(l - uid), 10);
        }
        ++l;  // skip the slash
        const char *name(l);
        for(; !isspace(*l); ++l)
        {
            if(*l == '\0')
            {
                throw memfile_exception_invalid("file meta data cannot only include a type and user specification");
            }
        }
        if(name == l)
        {
            throw memfile_exception_invalid("a user name cannot be empty");
        }
        if(l - name > 1 || *name != '-')
        {
            info.set_user(name, static_cast<int>(l - name));
        }
    }
    for(; isspace(*l); ++l);

    // 3. Group
    if(l[0] == '-' && isspace(l[1]))
    {
        // no gid/group specified
        l += 2;
    }
    else if(l[0] == '-' && l[1] == '/' && l[2] == '-' && isspace(l[3]))
    {
        // no gid/group specified
        l += 4;
    }
    else
    {
        const char *gid(l);
        for(; *l != '/'; ++l)
        {
            if(*l == '\0' || isspace(*l))
            {
                throw memfile_exception_invalid("unterminated group identifier and name in \"" + line + "\"");
            }
        }
        if(l == gid)
        {
            throw memfile_exception_invalid("a group identifier cannot be empty");
        }
        if(l - gid > 1 || *gid != '-')
        {
            info.set_gid(gid, static_cast<int>(l - gid), 10);
        }
        ++l;  // skip the slash
        const char *name(l);
        for(; !isspace(*l); ++l)
        {
            if(*l == '\0')
            {
                throw memfile_exception_invalid("file meta data cannot only include a type, user and group specification");
            }
        }
        if(name == l)
        {
            throw memfile_exception_invalid("a group name cannot be empty");
        }
        if(l - name > 1 || *name != '-')
        {
            info.set_group(name, static_cast<int>(l - name));
        }
    }
    for(; isspace(*l); ++l);

    // 4. Date
    if(l[0] == '-' && isspace(l[1]))
    {
        // no date
        l += 2;
    }
    else
    {
        // We only accept the following format (YYYYMMDD [letter T] HHMMSS)
        // 20130101T000000
        const char *date(l);
        for(; !isspace(*l); ++l)
        {
            if(*l == '\0')
            {
                throw memfile_exception_invalid("unterminated date entry in \"" + line + "\"");
            }
            if(*l < '0' || *l > '9')
            {
                if((*l != 'T' && *l != 't') || l - date != 8)
                {
                    throw memfile_exception_invalid("invalid date entry in \"" + line + "\", we only accept YYYYmmDD or YYYYmmDDTHHMMSS, digits only except for the T");
                }
            }
        }
        char d[5];
        struct tm t;
        memset(&t, 0, sizeof(t));
        if(l - date != 8)
        {
            if(l - date != 15)
            {
                throw memfile_exception_invalid("wpkg accepts two date formats YYYYmmDD and YYYYmmDDTHHMMSS, where T is the letter T itself");
            }
            // we can parse the time here (we already checked the T

            // hour
            d[0] = date[9];
            d[1] = date[10];
            d[2] = '\0';
            t.tm_hour = atol(d);
            if(t.tm_hour < 0 || t.tm_hour > 23)
            {
                throw memfile_exception_invalid("the hour must be between 0 and 23 inclusive");
            }

            // minute
            d[0] = date[11];
            d[1] = date[12];
            d[2] = '\0';
            t.tm_min = atol(d);
            if(t.tm_min < 0 || t.tm_min > 59)
            {
                throw memfile_exception_invalid("the hour must be between 0 and 59 inclusive");
            }

            // second
            d[0] = date[13];
            d[1] = date[14];
            d[2] = '\0';
            t.tm_sec = atol(d);
            if(t.tm_sec < 0 || t.tm_sec > 60)
            {
                // note that we accept leap seconds, although it probably isn't accepted by mktime()?
                throw memfile_exception_invalid("the hour must be between 0 and 59 inclusive");
            }
        }

        // parse the date part

        // year
        d[0] = date[0];
        d[1] = date[1];
        d[2] = date[2];
        d[3] = date[3];
        d[4] = '\0';
        t.tm_year = atol(d);
        if(t.tm_year < 1970 || t.tm_year > 2067)
        {
            throw memfile_exception_invalid("the year must be between 1970 and 2067 inclusive");
        }
        t.tm_year -= 1900;

        // month
        d[0] = date[4];
        d[1] = date[5];
        d[2] = '\0';
        t.tm_mon = atol(d);
        if(t.tm_mon < 1 || t.tm_mon > 12)
        {
            throw memfile_exception_invalid("the month must be between 1 and 12 inclusive");
        }
        --t.tm_mon;

        // day
        d[0] = date[6];
        d[1] = date[7];
        d[2] = '\0';
        t.tm_mday = atol(d);
        if(t.tm_mday < 1 || t.tm_mday > 31)
        {
            // XXX add code to check the month for the max. day
            //     note that mktime() works with such, but the date
            //     will be changed to one of the following days
            throw memfile_exception_invalid("the day of the month must be between 1 and 31 inclusive");
        }
        --t.tm_mon;

        // XXX code breaks in 2068 when time_t exceeds a signed 32 bits
        info.set_mtime(mktime(&t));
    }
    for(; isspace(*l); ++l);

    // 5. Device
    if(l[0] == '-' && isspace(l[1]))
    {
        // no device specified
        l += 2;
    }
    else if(l[0] == '-' && l[1] == ',' && l[2] == '-' && isspace(l[3]))
    {
        // no devices specified
        l += 4;
    }
    else
    {
        const char *dev_major(l);
        for(; *l != ','; ++l) {
            if(*l == '\0' || isspace(*l))
            {
                throw memfile_exception_invalid("unterminated device identifier in \"" + line + "\"");
            }
        }
        if(l == dev_major)
        {
            throw memfile_exception_invalid("a major device identifier cannot be empty");
        }
        if(l - dev_major > 1 || *dev_major != '-')
        {
            info.set_dev_major(dev_major, static_cast<int>(l - dev_major), 10);
        }
        ++l;  // skip the comma
        const char *dev_minor(l);
        for(; !isspace(*l); ++l)
        {
            if(*l == '\0')
            {
                throw memfile_exception_invalid("file meta data must include all the columns, including the filename");
            }
        }
        if(dev_minor == l)
        {
            throw memfile_exception_invalid("a minor device identifier cannot be empty");
        }
        if(l - dev_minor > 1 || *dev_minor != '-')
        {
            info.set_dev_minor(dev_minor, static_cast<int>(l - dev_minor), 10);
        }
    }
    for(; isspace(*l); ++l);

    // 6. Filename
    const char *filename(l);
    for(; *l != '\0'; ++l);
    for(; l > filename && isspace(l[-1]); --l);
    if(l == filename)
    {
        throw memfile_exception_invalid("the filename in a file meta data cannot be empty");
    }
    // TODO: add support for symbolic link with the arrow syntax: '... -> ...'
    const char *softlink = strstr(filename, " -> ");
    if(softlink != NULL)
    {
        if(softlink - filename > 0 && l - softlink > 4)
        {
            info.set_link(softlink + 4, static_cast<int>(l - softlink - 4));
            l = softlink;
        }
        else
        {
            throw memfile_exception_invalid("invalid soft link specification");
        }
    }

    std::string filename_only(filename, l - filename);
    wpkg_filename::uri_filename cname(filename_only);
    std::string pattern(cname.path_only());
    if(pattern[0] == '+')
    {
        if(pattern.length() == 1)
        {
            throw memfile_exception_invalid("the filename cannot just be +");
        }
        if(pattern[1] != '/')
        {
            throw memfile_exception_invalid("the filename must start with / or +/");
        }
        // remove the '/', but keep the +
        pattern.erase(1, 1);
        if(pattern.length() == 1)
        {
            pattern += ".";
        }
    }
    else if(pattern[0] != '/')
    {
        throw memfile_exception_invalid("the filename must start with / or +/");
    }
    else
    {
        // ignore the starting '/' because it's not there in the output
        pattern.erase(0, 1);
        if(pattern.empty())
        {
            pattern = ".";
        }
    }
    info.set_filename(pattern);

    return true;
}

void memory_file::append_ar(const file_info& info, const memory_file& data)
{
    char buf[60];

    if(info.get_filename().length() > 15)
    {
        throw memfile_exception_parameter("the filename is too long to fit in an ar file (long filenames are not yet supported)");
    }

    // create the ar header and then write it to the file
    memset(buf, ' ', sizeof(buf));
    memcpy(buf, info.get_filename().c_str(), info.get_filename().size()); // char ar_name[16]
    // dpkg does NOT terminate filenames with '/' so we don't either
    //buf[info.get_filename().length()] = '/'; // end filename with '/'
    file_info::int_to_str(buf + 16, static_cast<int>(info.get_mtime()), 12, 10, ' '); // char ar_date[12]
    file_info::int_to_str(buf + 16 + 12, info.get_uid(), 6, 10, ' '); // char ar_uid[6]
    file_info::int_to_str(buf + 16 + 12 + 6, info.get_gid(), 6, 10, ' '); // char ar_gid[6]
    file_info::int_to_str(buf + 16 + 12 + 6 + 6, info.get_mode(), 8, 8, ' '); // char ar_mode[8]
    file_info::int_to_str(buf + 16 + 12 + 6 + 6 + 8, info.get_size(), 10, 10, ' '); // char ar_size[8]
    buf[58] = '`'; // char ar_fmag[2]
    buf[59] = '\n';
    write(buf, f_buffer.size(), sizeof(buf));

    // copy the file data
    int data_size(data.size());
    if(data_size > 0)
    {
        char d[block_manager::BLOCK_MANAGER_BUFFER_SIZE];
        int offset(f_buffer.size());
        int pos(0);
        while(data_size >= block_manager::BLOCK_MANAGER_BUFFER_SIZE)
        {
            data.read(d, pos, block_manager::BLOCK_MANAGER_BUFFER_SIZE);
            f_buffer.write(d, offset, block_manager::BLOCK_MANAGER_BUFFER_SIZE);
            pos += block_manager::BLOCK_MANAGER_BUFFER_SIZE;
            offset += block_manager::BLOCK_MANAGER_BUFFER_SIZE;
            data_size -= block_manager::BLOCK_MANAGER_BUFFER_SIZE;
        }
        if(data_size > 0)
        {
            data.read(d, pos, data_size);
            f_buffer.write(d, offset, data_size);
            if((data_size & 1) != 0)
            {
                // we need the size to always be even but we cannot read
                // it from the source which may not include such a byte
                d[0] = 0;
                f_buffer.write(d, offset + data_size, 1);
            }
        }
    }
}

void memory_file::append_tar(const file_info& info, const memory_file& data)
{
    // make a copy so we can change the filename and link
    file_info valid_info(info);

    // do we need to create a long link?
    const std::string link(info.get_link());
    if(link.length() > 100)
    {
        file_info symlink_info;
        memory_file name;
        name.create(file_format_other);
        name.write(&link[0], 0, static_cast<int>(link.length()));
        char eos(0);
        name.write(&eos, static_cast<int>(link.length()), 1); // NUL
        symlink_info.set_filename("././@LongLink");
        symlink_info.set_size(static_cast<int>(link.length() + 1));
        symlink_info.set_file_type(file_info::long_symlink);
        symlink_info.set_mode(0);
        symlink_info.set_mtime(0);
        append_tar_write(symlink_info, name);
        valid_info.set_link(link.substr(0, 100));
    }

    // can we fit the filename?
    bool filename_fits(true);
    const std::string filename(info.get_filename());
    const size_t l(filename.length());
    if(l > 100 + 155 + 1)
    {
        // surely way too long
        filename_fits = false;
    }
    else if(l > 100)
    {
        // filename is too large, extract a prefix
        size_t p(std::string::npos);
        size_t n(filename.find_last_of('/', p));
        for(;;)
        {
            if(n == std::string::npos || n == 0 || l - n > 100)
            {
                filename_fits = false;
                break;
            }
            if(n <= 155 && l - n <= 100)
            {
                break;
            }
            p = n - 1;
            n = filename.find_last_of('/', p);
        }
    }
    if(!filename_fits)
    {
        file_info filename_info;
        memory_file name;
        name.create(file_format_other);
        name.write(&filename[0], 0, static_cast<int>(filename.length()));
        char eos(0);
        name.write(&eos, static_cast<int>(filename.length()), 1); // NUL
        filename_info.set_filename("././@LongLink");
        filename_info.set_size(static_cast<int>(filename.length() + 1));
        filename_info.set_file_type(file_info::long_filename);
        filename_info.set_mode(0);
        filename_info.set_mtime(0);
        append_tar_write(filename_info, name);
        valid_info.set_filename(filename.substr(0, 100));
    }

    append_tar_write(valid_info, data);
}

void memory_file::append_tar_write(const file_info& info, const memory_file& data)
{
    std::vector<char> header;
    header.reserve( 512 );
    header.resize( 512, 0 );

    std::string fn(info.get_filename());
    size_t l(fn.length());
    if(l <= 100)
    {
        // the name fits without using a prefix
        std::copy( fn.begin(), fn.end(), header.begin() );
    }
    else
    {
        // way too long anyway?
        if(l > 100 + 155 + 1)
        {
            throw memfile_exception_parameter("the filename is too long to fit in a tar file");
        }

        // filename is too large, extract a prefix
        size_t p(std::string::npos);
        size_t n(fn.find_last_of('/', p));
        for(;;)
        {
            if(n == std::string::npos || n == 0)
            {
                throw memfile_exception_parameter("the filename cannot be broken up to fit in a tar file");
            }
            if(n <= 155 && l - n <= 100)
            {
                break;
            }
            p = n - 1;
            n = fn.find_last_of('/', p);
        }

        // note that we "lose" the '/' between the prefix and name
        std::copy( fn.begin() + n + 1, fn.end(), header.begin() );
        std::copy( fn.begin(), fn.begin() + n, header.begin() + 345 );
    }

    file_info::int_to_str(&header[100], info.get_mode(), 7, 8, '0');
    file_info::int_to_str(&header[108], info.get_uid(), 7, 8, '0');
    file_info::int_to_str(&header[116], info.get_gid(), 7, 8, '0');
    bool has_data(false);
    switch(info.get_file_type())
    {
    case file_info::regular_file:
    case file_info::continuous:
    case file_info::long_filename:
    case file_info::long_symlink:
        file_info::int_to_str(&header[124], info.get_size(), 11, 8, '0');
        has_data = true;
        break;

    case file_info::pax_header:
        // the user could have used that directly...
        throw memfile_exception_compatibility("the PaxHeader is not yet supported in the writer");

    default:
        file_info::int_to_str(&header[124], 0, 11, 8, '0');
        break;

    }
    file_info::int_to_str(&header[136], static_cast<int>(info.get_mtime()), 11, 8, '0');

    switch(info.get_file_type())
    {
    case file_info::regular_file:
        header[156] = '0';
        break;

    case file_info::hard_link:
        header[156] = '1';
        break;

    case file_info::symbolic_link:
        header[156] = '2';
        {
            const std::string link(info.get_link());
            if(link.length() > 100) {
                throw memfile_exception_compatibility("the symbolic link \"" + link + "\" is too long to fit in a tar file");
            }
            std::copy(link.begin(), link.end(), header.begin() + 157);
        }
        break;

    case file_info::character_special:
        header[156] = '3';
        break;

    case file_info::block_special:
        header[156] = '4';
        break;

    case file_info::directory:
        header[156] = '5';
        break;

    case file_info::fifo:
        header[156] = '6';
        break;

    case file_info::continuous:
        header[156] = '7';
        break;

    case file_info::long_symlink:
        header[156] = 'K';
        break;

    case file_info::long_filename:
        header[156] = 'L';
        break;

    default:
        throw memfile_exception_parameter("invalid file type for a tar file");

    }

    // magic
    std::string ustar( "ustar " );
    std::copy( ustar.begin(), ustar.end(), header.begin() + 257 );
    header[263] = ' '; // dpkg tar ball version: " \0"
    header[264] = '\0';

    // user name
    std::string user(info.get_user());
    if(user.length() > 32)
    {
        throw memfile_exception_compatibility("the user name \"" + user + "\" is too long to fit in a tar file");
    }
    std::copy(user.begin(), user.end(), header.begin() + 265);

    // group name
    std::string group(info.get_group());
    if(group.length() > 32)
    {
        throw memfile_exception_compatibility("the group name \"" + group + "\" is too long to fit in a tar file");
    }
    std::copy(group.begin(), group.end(), header.begin() + 297);

    switch(info.get_file_type())
    {
    case file_info::character_special:
    case file_info::block_special:
        // if not character or block special, keep '\0'
        file_info::int_to_str(&header[329], info.get_dev_major(), 7, 8, '0');
        file_info::int_to_str(&header[337], info.get_dev_minor(), 7, 8, '0');
        break;

    default:
        // only character and block special files have a minor/major identifier
        break;

    }

    // now we can compute the checksum properly and save it
    uint32_t sum(tar_check_sum(&header[0]));
    // Note: Linux tar (& dpkg) saves this field as 6 digits, '\0' and ' '
    // so we do the same even though that's not supposed to be like that
    if(sum > 32767)
    {
        // in the remote case we have a really large checksum...
        file_info::int_to_str(&header[148], sum, 7, 8, '0');
    }
    else
    {
        file_info::int_to_str(&header[148], sum, 6, 8, '0');
    }
    header[155] = ' ';

    memory_file::write(&header[0], f_buffer.size(), static_cast<int>(header.size()));

    // copy the file data
    if(has_data)
    {
        int data_size(data.size());
        int in_offset(0);
        int offset(f_buffer.size());
        while(data_size > 0)
        {
            char buf[block_manager::BLOCK_MANAGER_BUFFER_SIZE];
            int sz(std::min(data_size, block_manager::BLOCK_MANAGER_BUFFER_SIZE));
            data.read(buf, in_offset, sz);
            in_offset += sz;
            data_size -= sz;
            // make sure we are aligned to 512 bytes
            for(; (sz & 511) != 0; ++sz)
            {
                buf[sz] = 0;
            }
            f_buffer.write(buf, offset, sz);
            offset += sz;
        }
    }
}

void memory_file::append_wpkg(const file_info& info, const memory_file& data)
{
    wpkgar::wpkgar_block_t  header;

    if(sizeof(header) != 1024)
    {
        throw std::logic_error("the size of the wpkgar structure is expected to be exactly 1024");
    }
    std::string filename(info.get_filename());
    if(filename.length() > 65535)
    {
        throw memfile_exception_parameter("the filename is too long to fit in a wpkg archive file");
    }
    std::string link(info.get_link());
    if(link.length() > 65535)
    {
        throw memfile_exception_parameter("the symbolic link is too long to fit in a wpkg archive file");
    }

    memset(&header, 0, sizeof(header));

    header.f_magic = wpkgar::WPKGAR_MAGIC;
    memcpy(header.f_version, wpkgar::WPKGAR_VERSION_1_1, sizeof(header.f_version));

    switch(info.get_file_type())
    {
    case file_info::regular_file:
    case file_info::continuous: // no distinction in type for continuous
        if(filename.find_last_of('/') == std::string::npos)
        {
            // package files do not appear in a folder
            header.f_type = wpkgar::wpkgar_block_t::WPKGAR_TYPE_PACKAGE;
        }
        else
        {
            // all package files are in some folder (be it just /)
            header.f_type = wpkgar::wpkgar_block_t::WPKGAR_TYPE_REGULAR;
        }

        // for regular files, compute their md5sum
        if(data.f_created || data.f_loaded)
        {
            md5::raw_md5sum sum;
            data.raw_md5sum(sum);
            memcpy(header.f_md5sum, sum.f_sum, sizeof(header.f_md5sum));
        }
        else
        {
            memcpy(header.f_md5sum, info.get_raw_md5sum().f_sum, sizeof(header.f_md5sum));
        }
        break;

    case file_info::hard_link:
        header.f_type = wpkgar::wpkgar_block_t::WPKGAR_TYPE_HARD_LINK;
        break;

    case file_info::symbolic_link:
        header.f_type = wpkgar::wpkgar_block_t::WPKGAR_TYPE_SYMBOLIC_LINK;
        break;

    case file_info::character_special:
        header.f_type = wpkgar::wpkgar_block_t::WPKGAR_TYPE_CHARACTER_SPECIAL;
        break;

    case file_info::block_special:
        header.f_type = wpkgar::wpkgar_block_t::WPKGAR_TYPE_BLOCK_SPECIAL;
        break;

    case file_info::directory:
        header.f_type = wpkgar::wpkgar_block_t::WPKGAR_TYPE_DIRECTORY;
        break;

    case file_info::fifo:
        header.f_type = wpkgar::wpkgar_block_t::WPKGAR_TYPE_FIFO;
        break;

    default:
        throw std::logic_error("undefined file type in file info found in append_wpkg()");

    }

    header.f_original_compression = static_cast<uint8_t>(info.get_original_compression());
    header.f_use = wpkgar::wpkgar_block_t::WPKGAR_USAGE_UNKNOWN;
    header.f_status = wpkgar::wpkgar_block_t::WPKGAR_STATUS_UNKNOWN;

    header.f_uid = info.get_uid();
    header.f_gid = info.get_gid();
    header.f_mode = info.get_mode();
    if(data.f_created || data.f_loaded)
    {
        header.f_size = data.size();
    }
    else
    {
        header.f_size = info.get_size();
    }
    header.f_mtime = static_cast<int>(info.get_mtime());
    header.f_dev_major = info.get_dev_major();
    header.f_dev_minor = info.get_dev_minor();

    strncpy(reinterpret_cast<char *>(header.f_name), filename.c_str(), 300);
    strncpy(reinterpret_cast<char *>(header.f_link), link.c_str(), 300);

    strncpy(reinterpret_cast<char *>(header.f_user), info.get_user().c_str(), 32);
    strncpy(reinterpret_cast<char *>(header.f_group), info.get_group().c_str(), 32);

    if(filename.length() > 300)
    {
        header.f_name_size = static_cast<uint16_t>(filename.length());
    }
    if(link.length() > 300)
    {
        header.f_link_size = static_cast<uint16_t>(link.length());
    }

    header.f_checksum = wpkg_check_sum(reinterpret_cast<const uint8_t *>(&header));

    write(reinterpret_cast<const char *>(&header), f_buffer.size(), sizeof(header));

    if(filename.size() > 300)
    {
        write(filename.c_str(), f_buffer.size(), static_cast<int>(filename.length()));
        const int length(sizeof(wpkgar::wpkgar_block_t) - (f_buffer.size() & (sizeof(wpkgar::wpkgar_block_t) - 1)));
        if(length != sizeof(wpkgar::wpkgar_block_t))
        {
            // creating the pad with a dynamic size may not work with
            // all compilers, so create the largest
            char pad[sizeof(wpkgar::wpkgar_block_t)];
            memset(pad, 0, length);
            write(pad, f_buffer.size(), length);
        }
    }
    if(link.size() > 300)
    {
        write(link.c_str(), f_buffer.size(), static_cast<int>(link.length()));
        const int length(sizeof(wpkgar::wpkgar_block_t) - (f_buffer.size() & (sizeof(wpkgar::wpkgar_block_t) - 1)));
        if(length != sizeof(wpkgar::wpkgar_block_t))
        {
            // creating the pad with a dynamic size may not work with
            // all compilers, so create the largest
            char pad[sizeof(wpkgar::wpkgar_block_t)];
            memset(pad, 0, length);
            write(pad, f_buffer.size(), length);
        }
    }

    // package data is saved in the "database" (wpkg folder for this package)
    // the data.tar.gz files are not saved in the database however
    if(header.f_type == wpkgar::wpkgar_block_t::WPKGAR_TYPE_PACKAGE
    && (data.f_created || data.f_loaded))
    {
        if(f_package_path.empty())
        {
            throw memfile_exception_parameter("the f_package_path was not defined, call set_package_path()");
        }
        data.write_file(f_package_path.append_child(info.get_filename()), true);
    }
}

memory_file::file_info::file_info()
{
    reset();
}

void memory_file::file_info::reset()
{
    // reset the vector so all values are marked undefined
    f_defined.resize(0);
    f_defined.resize(field_name_max, false);

    f_package_name = ""; // TBD?
    f_filename = "";
    f_file_type = regular_file;
    f_link = "";
    f_user = "root";
    f_group = "root";
    f_uid = 0;
    f_gid = 0;
    f_mode = 0400;
    f_size = 0;
    f_mtime = static_cast<int>(time(NULL));
    f_dev_major = 0;
    f_dev_minor = 0;
    // f_raw_md5sum -- there isn't a clear for this one (necessary?)
    f_original_compression = wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_NONE;
}

bool memory_file::file_info::is_field_defined(field_name_t field) const
{
    return f_defined[field];
}

void memory_file::file_info::set_field(field_name_t field)
{
    f_defined[field] = true;
}

void memory_file::file_info::reset_field(field_name_t field)
{
    f_defined[field] = false;
}

wpkg_filename::uri_filename memory_file::file_info::get_uri() const
{
    return f_uri;
}

std::string memory_file::file_info::get_package_name() const
{
    return f_package_name;
}

std::string memory_file::file_info::get_filename() const
{
    return f_filename;
}

std::string memory_file::file_info::get_basename() const
{
    size_t n(f_filename.find_last_of('/'));
    if(n == std::string::npos) {
        return f_filename;
    }
    return f_filename.substr(n + 1);
}

memory_file::file_info::file_type_t memory_file::file_info::get_file_type() const
{
    return f_file_type;
}

std::string memory_file::file_info::get_link() const
{
    return f_link;
}

std::string memory_file::file_info::get_user() const
{
    return f_user;
}

std::string memory_file::file_info::get_group() const
{
    return f_group;
}

int memory_file::file_info::get_uid() const
{
    return f_uid;
}

int memory_file::file_info::get_gid() const
{
    return f_gid;
}

int memory_file::file_info::get_mode() const
{
    return f_mode;
}

std::string memory_file::file_info::get_mode_flags() const
{
    static const char *mode_flags[8] = {
        "---", "--x", "-w-", "-wx",
        "r--", "r-x", "rw-", "rwx"
    };

    std::string result;
    if(f_file_type == directory) {
        result += 'd';
    }
    else if(f_file_type == symbolic_link) {
        result += 'l';
    }
    else if(f_file_type == character_special) {
        result += 'c';
    }
    else if(f_file_type == block_special) {
        result += 'b';
    }
    else if(f_file_type == fifo) {
        result += 'p';
    }
    else {
        result += '-';
    }
    result += mode_flags[(f_mode >> 6) & 7];
    result += mode_flags[(f_mode >> 3) & 7];
    result += mode_flags[f_mode & 7];

    return result;
}

int memory_file::file_info::get_size() const
{
    return f_size;
}

time_t memory_file::file_info::get_mtime() const
{
    return f_mtime;
}

time_t memory_file::file_info::get_ctime() const
{
    return f_ctime;
}

time_t memory_file::file_info::get_atime() const
{
    return f_atime;
}

std::string memory_file::file_info::get_date() const
{
    time_t t(f_mtime);
    std::string d(ctime(&t));
    while(d.length() > 0 && (*d.rbegin() == '\n' || *d.rbegin() == '\r')) {
        d.resize(d.length() - 1);
    }
    return d;
}

int memory_file::file_info::get_dev_major() const
{
    return f_dev_major;
}

int memory_file::file_info::get_dev_minor() const
{
    return f_dev_minor;
}

const md5::raw_md5sum& memory_file::file_info::get_raw_md5sum() const
{
    return f_raw_md5sum;
}

wpkgar::wpkgar_block_t::wpkgar_compression_t memory_file::file_info::get_original_compression() const
{
    return f_original_compression;
}

void memory_file::file_info::set_uri(const wpkg_filename::uri_filename& uri)
{
    f_uri = uri;
}

void memory_file::file_info::set_package_name(const std::string& package_name)
{
    f_package_name = package_name;
    set_field(field_name_package_name);
}

void memory_file::file_info::set_filename(const std::string& filename)
{
    if(filename.length() <= 0) {
        throw memfile_exception_io("empty filename not allowed in a file info");
    }
    f_filename = filename;
    set_field(field_name_filename);
}

void memory_file::file_info::set_filename(const char *fn, int max_size)
{
    if(max_size <= 0) {
        throw memfile_exception_io("empty filename not allowed in a file info");
    }
    f_filename.assign(fn, strnlen(fn, max_size));
    set_field(field_name_filename);
}

void memory_file::file_info::set_file_type(file_type_t t)
{
    switch(t) {
    case regular_file:
    case hard_link:
    case symbolic_link:
    case character_special:
    case block_special:
    case directory:
    case fifo:
    case continuous:
    case long_filename:
    case long_symlink:
    case pax_header:
        f_file_type = t;
        break;

    default:
        throw memfile_exception_parameter("invalid file type in memory_file::file_info::set_file_type()");

    }
    set_field(field_name_file_type);
}

void memory_file::file_info::set_link(const std::string& link)
{
    f_link = link;
    set_field(field_name_link);
}

void memory_file::file_info::set_link(const char *lnk, int max_size)
{
    f_link.assign(lnk, strnlen(lnk, max_size));;
    set_field(field_name_link);
}

void memory_file::file_info::set_user(const std::string& user)
{
    f_user = user;
    set_field(field_name_user);
}

void memory_file::file_info::set_user(const char *user, int max_size)
{
    f_user.assign(user, strnlen(user, max_size));
    set_field(field_name_user);
}

void memory_file::file_info::set_group(const std::string& group)
{
    f_group = group;
    set_field(field_name_group);
}

void memory_file::file_info::set_group(const char *g, int max_size)
{
    f_group.assign(g, strnlen(g, max_size));
    set_field(field_name_group);
}

void memory_file::file_info::set_uid(int uid)
{
    f_uid = uid;
    set_field(field_name_uid);
}

void memory_file::file_info::set_uid(const char *u, int max_size, int base)
{
    f_uid = str_to_int(u, max_size, base);
    set_field(field_name_uid);
}

void memory_file::file_info::set_gid(int gid)
{
    f_gid = gid;
    set_field(field_name_gid);
}

void memory_file::file_info::set_gid(const char *g, int max_size, int base)
{
    f_gid = str_to_int(g, max_size, base);
    set_field(field_name_gid);
}

void memory_file::file_info::set_mode(int mode)
{
    f_mode = mode;
    set_field(field_name_mode);
}

void memory_file::file_info::set_mode(const char *m, int max_size, int base)
{
    f_mode = str_to_int(m, max_size, base);
    set_field(field_name_mode);
}

void memory_file::file_info::set_size(int size)
{
    f_size = size;
    set_field(field_name_size);
}

void memory_file::file_info::set_size(const char *s, int max_size, int base)
{
    f_size = str_to_int(s, max_size, base);
    set_field(field_name_size);
}

void memory_file::file_info::set_mtime(time_t mtime)
{
    f_mtime = mtime;
    set_field(field_name_mtime);
}

void memory_file::file_info::set_mtime(const char *t, int max_size, int base)
{
    f_mtime = str_to_int(t, max_size, base);
    set_field(field_name_mtime);
}

void memory_file::file_info::set_ctime(time_t ctime)
{
    f_ctime = ctime;
    set_field(field_name_ctime);
}

void memory_file::file_info::set_ctime(const char *t, int max_size, int base)
{
    f_ctime = str_to_int(t, max_size, base);
    set_field(field_name_ctime);
}

void memory_file::file_info::set_atime(time_t atime)
{
    f_atime = atime;
    set_field(field_name_atime);
}

void memory_file::file_info::set_atime(const char *t, int max_size, int base)
{
    f_atime = str_to_int(t, max_size, base);
    set_field(field_name_atime);
}

void memory_file::file_info::set_dev_major(int dev)
{
    f_dev_major = dev;
    set_field(field_name_dev_major);
}

void memory_file::file_info::set_dev_major(const char *d, int max_size, int base)
{
    f_dev_major = str_to_int(d, max_size, base);
    set_field(field_name_dev_major);
}

void memory_file::file_info::set_dev_minor(int dev)
{
    f_dev_minor = dev;
    set_field(field_name_dev_minor);
}

void memory_file::file_info::set_dev_minor(const char *d, int max_size, int base)
{
    f_dev_minor = str_to_int(d, max_size, base);
    set_field(field_name_dev_minor);
}

void memory_file::file_info::set_raw_md5sum(md5::raw_md5sum& raw)
{
    f_raw_md5sum = raw;
    set_field(field_name_raw_md5sum);
}

void memory_file::file_info::set_original_compression(wpkgar::wpkgar_block_t::wpkgar_compression_t original_compression)
{
    f_original_compression = original_compression;
    set_field(field_name_original_compression);
}

int memory_file::file_info::strnlen(const char *str, int n)
{
    // compute the string size bounded by the limit
    int len(0);
    for(const char *s(str); *s != '\0' && len < n; ++len, ++s);
    return len;
}

int memory_file::file_info::str_to_int(const char *s, int n, int base)
{
    const char *start(s);
    const int length(n);

    if(base != 10 && base != 8)
    {
        throw memfile_exception_parameter("str_to_int() only accepts a base of 8 or 10");
    }
    uint64_t result(0);
    char max_ch(base == 10 ? '9' : '7');
    // right aligned values (i.e. tar)
    // (note: pre-pended NULs are accepted by dpkg...)
    while(n > 0 && *s == ' ')
    {
        ++s;
        --n;
    }
    // could zero be defined with all spaces?!
    if(n == 0)
    {
        throw memfile_exception_compatibility("value string without any digits");
    }
    while(n > 0)
    {
        if(*s < '0' || *s > max_ch)
        {
            break;
        }
        result = result * base + *s - '0';
        ++s;
        --n;
    }
    while(n > 0 && (*s == ' ' || *s == '\0'))
    {
        ++s;
        --n;
    }
    if(n > 0)
    {
        throw memfile_exception_compatibility("spurious characters found in value string \"" + std::string(s, n) + "\" (part of \"" + std::string(start, length) + "\")");
    }
    if(result > 0x7FFFFFFF) {
        // this should never happen (because of size constraints)
        throw memfile_exception_compatibility("number too large in value string");
    }
    return static_cast<int>(result);
}

void memory_file::file_info::int_to_str(char *d, uint32_t value, int len, int base, char fill)
{
    if(base != 10 && base != 8) {
        throw memfile_exception_parameter("int_to_str() only accepts a base of 8 or 10");
    }

    // we need a buffer of 11 char's max. for a 32 bits unsigned value
    // in base 8; the base is expected to be 8 or 10
    // (ar & tar have a limit of 10 chars anyway)
    char buf[12], *s;
    s = buf;
    do {
        *s++ = static_cast<char>((value % base) + '0');
        value /= base;
    } while(value != 0);
    if(s - buf > len) {
        throw std::logic_error("resulting value larger than output buffer");
    }
    if(fill == '0') {
        int f(len - static_cast<int>(s - buf));
        while(f > 0 && len > 0) {
            *d++ = fill;
            --f;
            --len;
        }
    }
    while(len > 0 &&  s > buf) {
        *d++ = *--s;
        --len;
    }
    while(len > 0) {
        *d++ = fill;
        --len;
    }
}

} // namespace memfile
// vim: ts=4 sw=4 et
