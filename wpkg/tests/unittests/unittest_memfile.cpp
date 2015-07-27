/*    unittest_memfile.cpp
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

#include "libdebpackages/memfile.h"

#include <string.h>
#include <time.h>
#include <catch.hpp>


// seed: 1367790804


namespace
{
    void compression(int zlevel)
    {
        memfile::memory_file i;         // input
        memfile::memory_file z;         // compressed
        memfile::memory_file t;         // test version (must be == to input)

        // Test 1
        // create a file of 0 to block_size bytes with all z levels
        const int block_size = 145 * 1024;
        char buf[block_size + 1];
        char tst[block_size + 1];
        for(int pos = 0; pos < block_size; ++pos)
        {
            buf[pos] = rand();
        }

        i.create(memfile::memory_file::file_format_other);
        for(int size = 0; size <= block_size; size += rand() & 0x03FF)
        {
            CATCH_REQUIRE( i.write(buf, 0, size) == size );
            i.compress(z, memfile::memory_file::file_format_gz, zlevel);
            z.decompress(t);
            memset(tst, 0, size);
            CATCH_REQUIRE( t.read(tst, 0, size) == size );
            CATCH_REQUIRE( memcmp(buf, tst, size) == 0 );

            CATCH_REQUIRE( i.write(buf, 0, size) == size );
            //fprintf(stderr, "Try bz2 compression\n");
            //i.write_file("last-file.raw");
            i.compress(z, memfile::memory_file::file_format_bz2, zlevel);
            //fprintf(stderr, "Succeeded bz2 compression, expected size = %d\n", size);
            z.decompress(t);
            memset(tst, 0, size);
            CATCH_REQUIRE( t.read(tst, 0, size) == size );
            CATCH_REQUIRE( memcmp(buf, tst, size) == 0 );
        }
    }
}

CATCH_TEST_CASE("MemfileUnitTests::buffer1","MemfileUnitTests")
{
    //memfile::memory_file::block_manager::buffer_t buf;
    // TODO:
    //
    // Write out some data.
    // Read it back in, make sure it's correct.
    // Test the swapfile, make sure it has sane contents.
    // Make sure that we can swap in and out of memory, and the data stays
    // consistent.
    // Make sure the swap file is deleted when the object is destroyed.
}

CATCH_TEST_CASE("MemfileUnitTests::compression1","MemfileUnitTests")
{
    compression(1);
}

CATCH_TEST_CASE("MemfileUnitTests::compression2","MemfileUnitTests")
{
    compression(2);
}

CATCH_TEST_CASE("MemfileUnitTests::compression3","MemfileUnitTests")
{
    compression(3);
}

CATCH_TEST_CASE("MemfileUnitTests::compression4","MemfileUnitTests")
{
    compression(4);
}

CATCH_TEST_CASE("MemfileUnitTests::compression5","MemfileUnitTests")
{
    compression(5);
}

CATCH_TEST_CASE("MemfileUnitTests::compression6","MemfileUnitTests")
{
    compression(6);
}

CATCH_TEST_CASE("MemfileUnitTests::compression7","MemfileUnitTests")
{
    compression(7);
}

CATCH_TEST_CASE("MemfileUnitTests::compression8","MemfileUnitTests")
{
    compression(8);
}

CATCH_TEST_CASE("MemfileUnitTests::compression9","MemfileUnitTests")
{
    compression(9);
}


// vim: ts=4 sw=4 et
