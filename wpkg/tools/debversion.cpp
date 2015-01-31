/*    debversion -- a tool to check a debian version or compare versions
 *    Copyright (C) 2006-2015  Made to Order Software Corporation
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
 * \brief Manage Debian versions.
 *
 * This file is the implementation of the Debian version tool used to:
 *
 * \li canonicalize versions;
 * \li compare versions between each others; and
 * \li verify that a version string is valid.
 */
#include    "libdebpackages/debian_version.h"
#include    "libdebpackages/debian_packages.h"
#include    "license.h"
#include    <string.h>
#include    <stdlib.h>
#include    <stdio.h>



void validate(const char *version)
{
    char    error_string[256];

    if(validate_debian_version(version, error_string, sizeof(error_string)) == 0) {
        // invalid
        fprintf(stderr, "debversion: error: %s\n", error_string);
        exit(1);
    }

    exit(0);
}


void canonicalize(const char *version)
{
    char error_string[256];
    char canon[256];

    debian_version_handle_t l(string_to_debian_version(version, error_string, sizeof(error_string)));
    if(l == 0) {
        fprintf(stderr, "debversion: error: %s\n", error_string);
        exit(1);
    }

    debian_version_to_string(l, canon, sizeof(canon));
    printf("%s\n", canon);

    exit(0);
}


void compare(const char *v1, const char *op, const char *v2)
{
    char error_string[256];

    debian_version_handle_t l(string_to_debian_version(v1, error_string, sizeof(error_string)));
    if(l == 0) {
        fprintf(stderr, "debversion: error: %s\n", error_string);
    }

    debian_version_handle_t p(string_to_debian_version(v2, error_string, sizeof(error_string)));
    if(p == 0) {
        fprintf(stderr, "debversion: error: %s\n", error_string);
    }

    if(l == 0 || p == 0) {
        exit(2);
    }

    int r = debian_versions_compare(l, p);

    if(strcmp(op, "-lt") == 0) {
        exit(r == -1 ? 0 : 1);
    }
    if(strcmp(op, "-le") == 0) {
        exit(r <= 0 ? 0 : 1);
    }
    if(strcmp(op, "-eq") == 0) {
        exit(r == 0 ? 0 : 1);
    }
    if(strcmp(op, "-ne") == 0) {
        exit(r != 0 ? 0 : 1);
    }
    if(strcmp(op, "-ge") == 0) {
        exit(r >= 0 ? 0 : 1);
    }
    if(strcmp(op, "-gt") == 0) {
        exit(r > 0 ? 0 : 1);
    }

    fprintf(stderr, "debversion: error: unknown operator '%s'\n", op);
    exit(3);
}


// convert to advgetopt at some point
void usage()
{
    printf("Usage of debversion v" DEBIAN_PACKAGES_VERSION_STRING "\n");
    printf("\n");
    printf("  debversion [--canonicalize|-c|--print|-p] <version>\n");
    printf("\n");
    printf("When <version> is valid, debversion returns 0; otherwise 1\n");
    printf("--canonicalize requires debversion to print the version back out\n");
    printf("in a canonical form (i.e. remove epoch 0:, sub-version .0, revision -1)\n");
    printf("--print prints the version as is in stdout\n");
    printf("\n");
    printf("  debversion <v1> -op <v2>       compare two versions\n");
    printf("\n");
    printf("Where -op is one of: -lt, -le, -eq, -ne, -ge or -gt\n");
    printf("In this case debversion returns 0 when the comparison is true;\n");
    printf("it returns 1 when the comparison is false;\n");
    printf("it returns 2 when one of the versions is invalid;\n");
    printf("and it returns 3 when the operator is not recognized\n");
    printf("\n");
}


int main(int argc, char *argv[])
{
    if(argc == 2)
    {
        if(strcmp("-h", argv[1]) == 0 || strcmp("--help", argv[1]) == 0 || strcmp("--help-nobr", argv[1]) == 0)
        {
            usage();
            exit(1);
        }
        if(strcmp("--license", argv[1]) == 0 || strcmp("--licence", argv[1]) == 0)
        {
            license::license();
            exit(1);
        }
        if(strcmp("-V", argv[1]) == 0 || strcmp("--version", argv[1]) == 0)
        {
            printf("%s\n", debian_packages_version_string());
            exit(1);
        }
        validate(argv[1]);
    }
    if(argc == 3 && (strcmp(argv[1], "-p") == 0 || strcmp(argv[1], "--print") == 0))
    {
        printf("%s\n", argv[2]);
        validate(argv[2]);
    }
    if(argc == 3 && (strcmp(argv[1], "-c") == 0 || strcmp(argv[1], "--canonicalize") == 0))
    {
        canonicalize(argv[2]);
    }
    if(argc == 4)
    {
        compare(argv[1], argv[2], argv[3]);
    }

    usage();
    exit(1);
}

// vim: ts=4 sw=4 et
