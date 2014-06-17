// Copyright (c) 2007-2013  Made to Order Software Corporation
// This code is based on an MD5 library written by:
// WIDE Project, Poul-Henning Kamp, University of California, Matthias Kramm

/** \file
 * \brief An md5 implementation to compute an md5 checksum.
 *
 * This file includes all the necessary functions used to compute
 * an md5 checksum. These checksums are used to verify that a file
 * did or did not change while being transferred.
 *
 * The class allows for per file computations in binary or text hex forms.
 */
#include    "libdebpackages/md5.h"
#include    <stdexcept>
#include    <string.h>
#include    <stdio.h>

namespace md5
{


namespace {

/** \brief Sinus as a fixed 32 value.
 *
 * Table created with (assuming no overflow):
 *
 * \code
 *  // i is taken as a radian angle (0 is unused)
 *  for(i = 0; i < 64; ++i) {
 *      sin_fixed_32[i] = 4294967296 * fabs(sin(i + 1));
 *  }
 * \endcode
 */
const uint32_t sin_fixed_32[64] =
{
    0xd76aa478,     0xe8c7b756,     0x242070db,     0xc1bdceee,
    0xf57c0faf,     0x4787c62a,     0xa8304613,     0xfd469501,
    0x698098d8,     0x8b44f7af,     0xffff5bb1,     0x895cd7be,
    0x6b901122,     0xfd987193,     0xa679438e,     0x49b40821,

    0xf61e2562,     0xc040b340,     0x265e5a51,     0xe9b6c7aa,
    0xd62f105d,     0x02441453,     0xd8a1e681,     0xe7d3fbc8,
    0x21e1cde6,     0xc33707d6,     0xf4d50d87,     0x455a14ed,
    0xa9e3e905,     0xfcefa3f8,     0x676f02d9,     0x8d2a4c8a,

    0xfffa3942,     0x8771f681,     0x6d9d6122,     0xfde5380c,
    0xa4beea44,     0x4bdecfa9,     0xf6bb4b60,     0xbebfbc70,
    0x289b7ec6,     0xeaa127fa,     0xd4ef3085,     0x04881d05,
    0xd9d4d039,     0xe6db99e5,     0x1fa27cf8,     0xc4ac5665,

    0xf4292244,     0x432aff97,     0xab9423a7,     0xfc93a039,
    0x655b59c3,     0x8f0ccc92,     0xffeff47d,     0x85845dd1,
    0x6fa87e4f,     0xfe2ce6e0,     0xa3014314,     0x4e0811a1,
    0xf7537e82,     0xbd3af235,     0x2ad7d2bb,     0xeb86d391
};





/** \brief Do a rol on 32 bits values.
 *
 * This function rols an integer value, that means shifting with the bits
 * going out in one direction coming back in the other.
 *
 * Note that gcc figures out that this is a roll (rol to the left) under
 * i386.
 *
 * Note that the function expects the shift to be a value between 0 and 31.
 * It is not guaranteed to work with any other value.
 *
 * \param[in] value  The value to be rolled.
 * \param[in] shift  The number of bits we want to rol the value.
 *
 * \return The roll of value by shift.
 */
inline uint32_t rol(uint32_t value, uint32_t shift)
{
    return (value << shift) | (value >> (32 - shift));
}


inline uint32_t F(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
inline uint32_t G(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
inline uint32_t H(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
inline uint32_t I(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }


static const uint32_t g_Ra =  7;
static const uint32_t g_Rb = 12;
static const uint32_t g_Rc = 17;
static const uint32_t g_Rd = 22;

static const uint32_t g_Re =  5;
static const uint32_t g_Rf =  9;
static const uint32_t g_Rg = 14;
static const uint32_t g_Rh = 20;

static const uint32_t g_Ri =  4;
static const uint32_t g_Rj = 11;
static const uint32_t g_Rk = 16;
static const uint32_t g_Rl = 23;

static const uint32_t g_Rm =  6;
static const uint32_t g_Rn = 10;
static const uint32_t g_Ro = 15;
static const uint32_t g_Rp = 21;

void bin2hex(char *out, int in)
{
    static char hex[16] = {
        '0', '1', '2', '3', '4', '5', '6', '7',
        '8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
    };
    
    out[0] = hex[(in >> 4) & 15];
    out[1] = hex[in & 15];
}

}        // private namespace

/** \class md5sum
 * \brief The md5sum class allows for an in place computation of an MD5 checksum.
 *
 * This class allows for the computation of the md5sum of a data (generally
 * a file.) The class takes in data via its push_back() function.
 *
 * The checksum can be retrieved at any time with one of the following
 * functions:
 *
 * \li sum()
 * \li raw_sum()
 *
 * The sum() functions retrieve strings with the sum in hexadecimal ready
 * for printing.
 *
 * The raw_sum() returns the checksum as a raw 16 bytes of data. This is
 * useful if you just want to compare md5sums against each others. That way
 * you avoid the conversion to hexadecimal.
 */


/** \class raw_md5sum
 * \brief Handle raw MD5 checksums.
 *
 * In most cases uses want textual MD5 checksums. The fact is, to save in
 * checksum files and control files, that is what is required. However, to
 * quickly compute and compare md5sums, it is better to keep the raw value
 * instead. That way it is half the size and we avoid converting from binary
 * to ASCII before the comparison.
 *
 * Also, the wpkgar block format saves the md5sum of files in raw format.
 */


bool raw_md5sum::operator == (const raw_md5sum& rhs) const
{
    return memcmp(f_sum, rhs.f_sum, sizeof(f_sum)) == 0;
}

bool raw_md5sum::operator != (const raw_md5sum& rhs) const
{
    return !operator == (rhs);
}


/** \brief Initialize an md5sum object.
 *
 * This function initializes the md5sum object. This means a new md5sum can
 * be computed started from there. In other words the md5sum object is set to
 * the empty file md5 checksum.
 */
md5sum::md5sum()
{
    clear();
}


void md5sum::clear()
{
    f_size = 0;

    f_a = 0x67452301;
    f_b = 0xefcdab89;
    f_c = 0x98badcfe;
    f_d = 0x10325476;

    f_pos = 0;
    //memset(f_buffer, 0, sizeof(f_buffer)); -- f_pos marks the buffer as undefined already
}


bool md5sum::empty() const
{
    return f_size == 0;
}


uint64_t md5sum::size() const
{
    return f_size;
}


void md5sum::push_back(const uint8_t *data, size_t data_size)
{
    f_size += data_size;            // size in bytes

    while(data_size > 0) {
        // buffer ready at once whatever the endian
        uint32_t byte = f_pos & 3;
        if(byte == 0) {
            f_buffer[f_pos >> 2] = *data;
        }
        else {
            f_buffer[f_pos >> 2] |= *data << byte * 8;
        }

        // we're adding one byte at a time
        ++f_pos;
        --data_size;
        ++data;

        // buffer full?
        if(f_pos >= 64) {
            calc();
            f_pos = 0;
        }
    }
}


void md5sum::raw_sum(raw_md5sum& raw) const
{
    // 0x80 closes the stream, then all zeroes
    static const uint8_t pad[64] = {
        0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    };

    // Make a copy of this sum since it is a read-only
    // one and we want to be able to continue to enlarge
    // it more and more. (i.e. fully incremental!)
    md5sum copy(*this);

    if(f_pos >= 64 - 8) {
        // not enough room to close and include the bit size
        // so fill up the current buffer (which generates a calc)
        // and fill up the next buffer with zeros.
        copy.push_back(pad, 64 - f_pos);

        // this is faster than calling push_back()
        memset(copy.f_buffer, 0, 64 - 8);
        copy.f_pos = 64 - 8;
    }
    else {
        copy.push_back(pad, 64 - f_pos - 8);
    }

    // end the buffer with the size in bits
    uint64_t bit_size(f_size * 8);
    uint32_t encode_size(static_cast<uint32_t>(bit_size));
    copy.push_back(reinterpret_cast<uint8_t *>(&encode_size), 4);
    encode_size = static_cast<uint32_t>(bit_size >> 32);
    copy.push_back(reinterpret_cast<uint8_t *>(&encode_size), 4);
    // the last push_back() generates another calc()

    // return the raw md5sum
    // (the following works whatever the endian)
    raw.f_sum[ 0] = static_cast<uint8_t>(copy.f_a);
    raw.f_sum[ 1] = static_cast<uint8_t>(copy.f_a >> 8);
    raw.f_sum[ 2] = static_cast<uint8_t>(copy.f_a >> 16);
    raw.f_sum[ 3] = static_cast<uint8_t>(copy.f_a >> 24);
    raw.f_sum[ 4] = static_cast<uint8_t>(copy.f_b);
    raw.f_sum[ 5] = static_cast<uint8_t>(copy.f_b >> 8);
    raw.f_sum[ 6] = static_cast<uint8_t>(copy.f_b >> 16);
    raw.f_sum[ 7] = static_cast<uint8_t>(copy.f_b >> 24);
    raw.f_sum[ 8] = static_cast<uint8_t>(copy.f_c);
    raw.f_sum[ 9] = static_cast<uint8_t>(copy.f_c >> 8);
    raw.f_sum[10] = static_cast<uint8_t>(copy.f_c >> 16);
    raw.f_sum[11] = static_cast<uint8_t>(copy.f_c >> 24);
    raw.f_sum[12] = static_cast<uint8_t>(copy.f_d);
    raw.f_sum[13] = static_cast<uint8_t>(copy.f_d >> 8);
    raw.f_sum[14] = static_cast<uint8_t>(copy.f_d >> 16);
    raw.f_sum[15] = static_cast<uint8_t>(copy.f_d >> 24);
}


std::string md5sum::sum(const raw_md5sum& raw)
{
    // here we're ready to create the MD5 string
    char buf[raw_md5sum::MD5SUM_RAW_BUFSIZ * 2 + 1];
    for(int i(0); i < raw_md5sum::MD5SUM_RAW_BUFSIZ; ++i) {
        bin2hex(buf + i * 2, raw.f_sum[i]);
    }
    buf[sizeof(buf) - 1] = '\0'; // nul terminator

    return buf;
}



std::string md5sum::sum() const
{
    raw_md5sum raw;
    raw_sum(raw);
    return sum(raw);
}






uint32_t md5sum::round1(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t k, uint32_t s, uint32_t i)
{
    a += F(b, c, d) + f_buffer[k] + sin_fixed_32[i];
    return b + rol(a, s);
}

uint32_t md5sum::round2(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t k, uint32_t s, uint32_t i)
{
    a += G(b, c, d) + f_buffer[k] + sin_fixed_32[i];
    return b + rol(a, s);
}

uint32_t md5sum::round3(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t k, uint32_t s, uint32_t i)
{
    a += H(b, c, d) + f_buffer[k] + sin_fixed_32[i];
    return b + rol(a, s);
}

uint32_t md5sum::round4(uint32_t a, uint32_t b, uint32_t c, uint32_t d, uint32_t k, uint32_t s, uint32_t i)
{
    a += I(b, c, d) + f_buffer[k] + sin_fixed_32[i];
    return b + rol(a, s);
}




void md5sum::calc()
{
    uint32_t a = f_a;
    uint32_t b = f_b;
    uint32_t c = f_c;
    uint32_t d = f_d;

    a = round1(a, b, c, d,  0, g_Ra,  0); d = round1(d, a, b, c,  1, g_Rb,  1);
    c = round1(c, d, a, b,  2, g_Rc,  2); b = round1(b, c, d, a,  3, g_Rd,  3);
    a = round1(a, b, c, d,  4, g_Ra,  4); d = round1(d, a, b, c,  5, g_Rb,  5);
    c = round1(c, d, a, b,  6, g_Rc,  6); b = round1(b, c, d, a,  7, g_Rd,  7);
    a = round1(a, b, c, d,  8, g_Ra,  8); d = round1(d, a, b, c,  9, g_Rb,  9);
    c = round1(c, d, a, b, 10, g_Rc, 10); b = round1(b, c, d, a, 11, g_Rd, 11);
    a = round1(a, b, c, d, 12, g_Ra, 12); d = round1(d, a, b, c, 13, g_Rb, 13);
    c = round1(c, d, a, b, 14, g_Rc, 14); b = round1(b, c, d, a, 15, g_Rd, 15);

    a = round2(a, b, c, d,  1, g_Re, 16); d = round2(d, a, b, c,  6, g_Rf, 17);
    c = round2(c, d, a, b, 11, g_Rg, 18); b = round2(b, c, d, a,  0, g_Rh, 19);
    a = round2(a, b, c, d,  5, g_Re, 20); d = round2(d, a, b, c, 10, g_Rf, 21);
    c = round2(c, d, a, b, 15, g_Rg, 22); b = round2(b, c, d, a,  4, g_Rh, 23);
    a = round2(a, b, c, d,  9, g_Re, 24); d = round2(d, a, b, c, 14, g_Rf, 25);
    c = round2(c, d, a, b,  3, g_Rg, 26); b = round2(b, c, d, a,  8, g_Rh, 27);
    a = round2(a, b, c, d, 13, g_Re, 28); d = round2(d, a, b, c,  2, g_Rf, 29);
    c = round2(c, d, a, b,  7, g_Rg, 30); b = round2(b, c, d, a, 12, g_Rh, 31);

    a = round3(a, b, c, d,  5, g_Ri, 32); d = round3(d, a, b, c,  8, g_Rj, 33);
    c = round3(c, d, a, b, 11, g_Rk, 34); b = round3(b, c, d, a, 14, g_Rl, 35);
    a = round3(a, b, c, d,  1, g_Ri, 36); d = round3(d, a, b, c,  4, g_Rj, 37);
    c = round3(c, d, a, b,  7, g_Rk, 38); b = round3(b, c, d, a, 10, g_Rl, 39);
    a = round3(a, b, c, d, 13, g_Ri, 40); d = round3(d, a, b, c,  0, g_Rj, 41);
    c = round3(c, d, a, b,  3, g_Rk, 42); b = round3(b, c, d, a,  6, g_Rl, 43);
    a = round3(a, b, c, d,  9, g_Ri, 44); d = round3(d, a, b, c, 12, g_Rj, 45);
    c = round3(c, d, a, b, 15, g_Rk, 46); b = round3(b, c, d, a,  2, g_Rl, 47);

    a = round4(a, b, c, d,  0, g_Rm, 48); d = round4(d, a, b, c,  7, g_Rn, 49);
    c = round4(c, d, a, b, 14, g_Ro, 50); b = round4(b, c, d, a,  5, g_Rp, 51);
    a = round4(a, b, c, d, 12, g_Rm, 52); d = round4(d, a, b, c,  3, g_Rn, 53);
    c = round4(c, d, a, b, 10, g_Ro, 54); b = round4(b, c, d, a,  1, g_Rp, 55);
    a = round4(a, b, c, d,  8, g_Rm, 56); d = round4(d, a, b, c, 15, g_Rn, 57);
    c = round4(c, d, a, b,  6, g_Ro, 58); b = round4(b, c, d, a, 13, g_Rp, 59);
    a = round4(a, b, c, d,  4, g_Rm, 60); d = round4(d, a, b, c, 11, g_Rn, 61);
    c = round4(c, d, a, b,  2, g_Ro, 62); b = round4(b, c, d, a,  9, g_Rp, 63);

    f_a += a;
    f_b += b;
    f_c += c;
    f_d += d;
}



}    // namespace md5


// for test purposes; the first one is from OpenSSL.
#if 0
/* crypto/md5/md5test.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscape's SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the routines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publicly available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

{
static char *test[]={
    "",
    "a",
    "abc",
    "message digest",
    "abcdefghijklmnopqrstuvwxyz",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
    "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
    NULL,
    };

static char *ret[]={
    "d41d8cd98f00b204e9800998ecf8427e",
    "0cc175b9c0f1b6a831c399e269772661",
    "900150983cd24fb0d6963f7d28e17f72",
    "f96b697d7cb7938d525a2f31aaf161d0",
    "c3fcd3d76192e4007dfb496cca67e13b",
    "d174ab98d277d9f5a5611c2c9f419d9f",
    "57edf4a22be3c955ac49da2e2107b67a",
    };

{
    int i;
    char **P,**R;

    P=(char **)test;
    R=(char **)ret;
    i=1;
    while (*P != NULL)
    {
        //EVP_Digest(&(P[0][0]),strlen((char *)*P),md,NULL,EVP_md5(), NULL);

        md5::md5sum sum;
        sum.push_back((const uint8_t*)P[0], strlen(P[0]));
        std::string result = sum.sum();
        //std::cout << sum.sum() << " *" << argv[1] << std::endl;


        if(result != *R)
        {
            printf("error calculating MD5 on '%s'\n",*P);
            printf("got %s instead of %s\n",result.c_str(),*R);
        }
        else
            printf("test %d ok ('%s')\n",i,*P);
        i++;
        R++;
        P++;
    }

}


exit(1);
}
#endif

#if 0
{
FILE *f = fopen(argv[1], "r");
fseek(f, 0, SEEK_END);
int size = ftell(f);
fseek(f, 0, SEEK_SET);
uint8_t *buf = new uint8_t[size];
fread(buf, 1, size, f);
md5::md5sum sum;
sum.push_back(buf, size);
std::cout << sum.sum() << " *" << argv[1] << std::endl;
exit(1);
}
#endif

// vim: ts=4 sw=4 et
