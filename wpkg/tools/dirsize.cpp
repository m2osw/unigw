/*    dirsize -- tool that can be used to compute the size of a directory
 *    Copyright (C) 2006-2014  Made to Order Software Corporation
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
 * \brief Implementation of the dirsize tool.
 *
 * The dirsize tool computes the size of the specified directory(ies).
 *
 * This tool was created because MS-Windows does not have a du command.
 * However, this functionality is now 100% part of the wpkg tool and
 * therefore it is not required here. Plus, because each system has a
 * different block size in their file system, it is really only very
 * partially useful.
 */
#include    "libdebpackages/advgetopt.h"
#include    "libdebpackages/memfile.h"
#include    "libdebpackages/debian_packages.h"
#include    "license.h"
#include    <stdio.h>
#include    <stdlib.h>





int main(int argc, char *argv[])
{
    static const advgetopt::getopt::option options[] = {
        {
            '\0',
            0,
            NULL,
            NULL,
            "Usage: dirsize [-<opt>] <package> ...",
            advgetopt::getopt::help_argument
        },
        {
            'h',
            0,
            "help",
            NULL,
            "print this help message",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "help-nobr",
            NULL,
            NULL,
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "version",
            NULL,
            "show the version of dirsize",
            advgetopt::getopt::no_argument
        },
        {
            'c',
            0,
            "total",
            NULL,
            "only output grand total",
            advgetopt::getopt::no_argument
        },
        {
            's',
            0,
            "sizeonly",
            NULL,
            "output the byte size only",
            advgetopt::getopt::no_argument
        },
        {
            'b',
            0,
            "blocksize",
            "512",
            "size of one block to compute the disk space",
            advgetopt::getopt::required_long
        },
        {
            'p',
            0,
            "package",
            NULL,
            NULL, // hidden argument in --help screen
            advgetopt::getopt::default_multiple_argument
        },
        {
            '\0',
            0,
            "license",
            NULL,
            "displays the license of this tool",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "licence", // French spelling
            NULL,
            NULL, // hidden argument in --help screen
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::end_of_options
        }
    };

    std::vector<std::string> configuration_files;
    advgetopt::getopt opt(argc, argv, options, configuration_files, "");

    if(opt.is_defined("help") || opt.is_defined("help-nobr")) {
        opt.usage(opt.is_defined("help-nobr") ? advgetopt::getopt::no_error_nobr : advgetopt::getopt::no_error, "Usage: dirsize [-<opt>] <package> ...");
        /*NOTREACHED*/
    }

    if(opt.is_defined("version")) {
        printf("%s\n", debian_packages_version_string());
        exit(1);
    }

    if(opt.is_defined("license") || opt.is_defined("licence")) {
        license::license();
        exit(1);
    }

    // get the size, if zero it's undefined
    int max(opt.size("package"));
    if(max == 0) {
        opt.usage(advgetopt::getopt::error, "package filename necessary");
        /*NOTREACHED*/
    }

    // user defined value or the default (512)
    int blocksize(static_cast<int>(opt.get_long("blocksize")));

    int total_size(0);
    int total_disk_size(0);
    memfile::memory_file m;
    for(int i(0); i < max; ++i) {
        std::string path(opt.get_string("package", i));
        int disk_size;
        int size(m.dir_size(path, disk_size, blocksize));
        total_size += size;
        total_disk_size += disk_size;
        if(!opt.is_defined("total")) {
            if(opt.is_defined("sizeonly")) {
                printf("%d\n", size);
            }
            else {
                printf("%s %d %d\n", path.c_str(), size, disk_size);
            }
        }
    }
    if(max > 1) {
        if(opt.is_defined("sizeonly")) {
            printf("%d\n", total_size);
        }
        else {
            printf("total %d %d\n", total_size, total_disk_size);
        }
    }

    return 0;
}


// vim: ts=4 sw=4 et
