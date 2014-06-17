/*    unittest_memfile.cpp
 *    Copyright (C) 2013  Made to Order Software Corporation
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

#include "unittest_memfile.h"
#include "libdebpackages/memfile.h"

#include <cppunit/config/SourcePrefix.h>
#include <string.h>
#include <time.h>


// seed: 1367790804

CPPUNIT_TEST_SUITE_REGISTRATION( MemfileUnitTests );

void MemfileUnitTests::setUp()
{
}


void MemfileUnitTests::compression(int zlevel)
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
        CPPUNIT_ASSERT( i.write(buf, 0, size) == size );
        i.compress(z, memfile::memory_file::file_format_gz, zlevel);
        z.decompress(t);
        memset(tst, 0, size);
        CPPUNIT_ASSERT( t.read(tst, 0, size) == size );
        CPPUNIT_ASSERT( memcmp(buf, tst, size) == 0 );

        CPPUNIT_ASSERT( i.write(buf, 0, size) == size );
//fprintf(stderr, "Try bz2 compression\n");
//i.write_file("last-file.raw");
        i.compress(z, memfile::memory_file::file_format_bz2, zlevel);
//fprintf(stderr, "Succeeded bz2 compression, expected size = %d\n", size);
        z.decompress(t);
        memset(tst, 0, size);
        CPPUNIT_ASSERT( t.read(tst, 0, size) == size );
        CPPUNIT_ASSERT( memcmp(buf, tst, size) == 0 );
    }
}

void MemfileUnitTests::compression1()
{
    compression(1);
}

void MemfileUnitTests::compression2()
{
    compression(2);
}

void MemfileUnitTests::compression3()
{
    compression(3);
}

void MemfileUnitTests::compression4()
{
    compression(4);
}

void MemfileUnitTests::compression5()
{
    compression(5);
}

void MemfileUnitTests::compression6()
{
    compression(6);
}

void MemfileUnitTests::compression7()
{
    compression(7);
}

void MemfileUnitTests::compression8()
{
    compression(8);
}

void MemfileUnitTests::compression9()
{
    compression(9);
}


// vim: ts=4 sw=4 et
