/*    unittest_libutf8.cpp
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

#include "unittest_libutf8.h"
#include "libutf8/libutf8.h"

#include <cctype>

CPPUNIT_TEST_SUITE_REGISTRATION( LibUtf8UnitTests );



void LibUtf8UnitTests::setUp()
{
}



void LibUtf8UnitTests::conversions()
{
    std::string str;
    std::wstring wstr, back;
    int i;

    // create a string with all the characters defined in plane 1
    for(i = 1; i < 0x0FFFE; ++i)
    {
        // skip the surrogate because those are not encoded under
        // MS-Windows (these are not valid characters anyway)
        if(i < 0xD800 || i > 0xDFFF)
        {
            wstr += static_cast<wchar_t>(i);
        }
    }

    str = libutf8::wcstombs(wstr);

    // verify the UTF-8 string
    const char *s(str.c_str());
    for(i = 1; i < 0x080; ++i)
    {
        CPPUNIT_ASSERT(*s++ == static_cast<char>(i));
    }
    for(; i < 0x0800; ++i)
    {
        CPPUNIT_ASSERT(*s++ == static_cast<char>((i >> 6) | 0xC0));
        CPPUNIT_ASSERT(*s++ == static_cast<char>((i & 0x3F) | 0x80));
    }
    for(; i < 0x0FFFE; ++i)
    {
        if(i < 0xD800 || i > 0xDFFF)
        {
            CPPUNIT_ASSERT(*s++ == static_cast<char>((i >> 12) | 0xE0));
            CPPUNIT_ASSERT(*s++ == static_cast<char>(((i >> 6) & 0x3F) | 0x80));
            CPPUNIT_ASSERT(*s++ == static_cast<char>((i & 0x3F) | 0x80));
        }
    }

    // verify the UTF-8 to wchar_t
    back = libutf8::mbstowcs(str);
    CPPUNIT_ASSERT(back == wstr);
}



wchar_t rand_char()
{
    wchar_t wc;
    do
    {
        wc = rand() & 0x0FFFF;
    }
    while(wc == 0);
    return wc;
}

void LibUtf8UnitTests::compare()
{
    for(int i(1); i < 0x10000; ++i)
    {
        // as is
        std::wstring in;
        in += static_cast<wchar_t>(i);
        std::string mb(libutf8::wcstombs(in));
        CPPUNIT_ASSERT(libutf8::mbscasecmp(mb, mb) == 0);

        // as is against uppercase
        std::wstring uin;
        uin += std::toupper(static_cast<wchar_t>(i));
        std::string umb(libutf8::wcstombs(uin));
        CPPUNIT_ASSERT(libutf8::mbscasecmp(mb, umb) == 0);

        // as is against lowercase
        std::wstring lin;
        lin += std::tolower(static_cast<wchar_t>(i));
        std::string lmb(libutf8::wcstombs(lin));
        CPPUNIT_ASSERT(libutf8::mbscasecmp(mb, lmb) == 0);

        // random
        for(int j(0); j < 3; ++j)
        {
            wchar_t rwc(rand_char() & 0x0FFFF);
            in += rwc;
            uin += std::toupper(rwc);
            lin += std::tolower(rwc);

            std::string rmb(libutf8::wcstombs(in));
            CPPUNIT_ASSERT(libutf8::mbscasecmp(rmb, rmb) == 0);
            std::string rumb(libutf8::wcstombs(uin));
            CPPUNIT_ASSERT(libutf8::mbscasecmp(rmb, rumb) == 0);
            std::string rlmb(libutf8::wcstombs(lin));
            CPPUNIT_ASSERT(libutf8::mbscasecmp(rmb, rlmb) == 0);
        }

        wchar_t wc(rand_char() & 0x0FFFF);
        in += wc;
        std::string emb(libutf8::wcstombs(in));
        CPPUNIT_ASSERT(libutf8::mbscasecmp(emb, emb) == 0);
        CPPUNIT_ASSERT(libutf8::mbscasecmp(emb, umb) == 1);
        CPPUNIT_ASSERT(libutf8::mbscasecmp(emb, lmb) == 1);

        {
            wchar_t uwc(rand_char() & 0x0FFFF);
            uin += std::toupper(uwc);
            std::string eumb(libutf8::wcstombs(uin));
//printf("UPPER compare U+%04X/%04X with U+%04X/%04X\n", wc, std::toupper(wc), uwc, std::toupper(uwc));
//printf(" result: [%d]\n", libutf8::mbscasecmp(emb, eumb));
            if(std::toupper(wc) == std::toupper(uwc))
            {
                CPPUNIT_ASSERT(libutf8::mbscasecmp(emb, eumb) == 0);
            }
            else if(std::toupper(wc) < std::toupper(uwc))
            {
                CPPUNIT_ASSERT(libutf8::mbscasecmp(emb, eumb) == -1);
            }
            else
            {
                CPPUNIT_ASSERT(libutf8::mbscasecmp(emb, eumb) == 1);
            }
            CPPUNIT_ASSERT(libutf8::mbscasecmp(lmb, eumb) == -1);
        }

        // here we check with a lowercase character, but notice that the
        // compare uses uppercase!
        {
            wchar_t lwc(rand_char() & 0x0FFFF);
            lin += std::tolower(lwc);
            std::string elmb(libutf8::wcstombs(lin));
//printf("LOWER compare U+%04X/%04X with U+%04X/%04X\n", wc, std::toupper(wc), lwc, std::toupper(lwc));
//printf(" result: [%d]\n", libutf8::mbscasecmp(emb, elmb));
            if(std::toupper(wc) == std::toupper(lwc))
            {
                CPPUNIT_ASSERT(libutf8::mbscasecmp(emb, elmb) == 0);
            }
            else if(std::toupper(wc) < std::toupper(lwc))
            {
                CPPUNIT_ASSERT(libutf8::mbscasecmp(emb, elmb) == -1);
            }
            else
            {
                CPPUNIT_ASSERT(libutf8::mbscasecmp(emb, elmb) == 1);
            }
        }
    }
}


// With MS-Windows, we can check that our functions work the same way
// (return the expected value) as this Windows API function:
// 
// CompareStringOrdinal(L"This string", 11, L"That string", 11, TRUE);


// vim: ts=4 sw=4 et
