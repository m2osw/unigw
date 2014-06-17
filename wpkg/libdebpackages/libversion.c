/*    debian_packages -- the version of the library at runtime
 *    Copyright (C) 2006-2013  Made to Order Software Corporation
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
 * \brief Implementation of the library version information.
 *
 * This file defines the version information as well as a few more details
 * about the library such as the date and time when the library was compiled
 * and the machine the library was compiled for and the machine it is
 * currently running on.
 *
 * \note
 * A note to programmers: to change most of the these parameters, look at the
 * CMakeLists.txt file in the root directory of the wpkg project. You should
 * find them all there (i.e. version information, vendor, etc.)
 */
#include "libdebpackages/debian_packages.h"

#include <string.h>
#if defined(MO_WINDOWS)
#include <stdlib.h>
#else
#include <sys/utsname.h>
#endif


int debian_packages_version_major()
{
    return DEBIAN_PACKAGES_VERSION_MAJOR;
}

int debian_packages_version_minor()
{
    return DEBIAN_PACKAGES_VERSION_MINOR;
}

int debian_packages_version_patch()
{
    return DEBIAN_PACKAGES_VERSION_PATCH;
}

const char *debian_packages_version_string()
{
    return DEBIAN_PACKAGES_VERSION_STRING;
}

const char *debian_packages_build_time()
{
    return __DATE__ " " __TIME__;
}

const char *debian_packages_architecture()
{
    return DEBIAN_PACKAGES_ARCHITECTURE;
}

const char *debian_packages_processor()
{
    return DEBIAN_PACKAGES_PROCESSOR;
}

const char *debian_packages_os()
{
    return DEBIAN_PACKAGES_OS;
}

const char *debian_packages_vendor()
{
    return DEBIAN_PACKAGES_VENDOR;
}

const char *debian_packages_triplet()
{
    return DEBIAN_PACKAGES_OS "-" DEBIAN_PACKAGES_VENDOR "-" DEBIAN_PACKAGES_PROCESSOR;
}

const char *debian_packages_machine()
{
    static char machine[256] = { 0 };
    if(machine[0] == '\0')
    {
#if defined(MO_WINDOWS)
        const char *processor = getenv("PROCESSOR_ARCHITEW6432");
        if(processor == NULL)
        {
            processor = getenv("PROCESSOR_ARCHITECTURE");
            if(processor == NULL)
            {
                processor = getenv("PROCESSOR_IDENTIFIER");
            }
        }
        strncpy(machine, processor == NULL ? "unknown" : processor, sizeof(machine) - 1);
#else
        static struct utsname info;
        if(uname(&info) == 0)
        {
            strncpy(machine, info.machine, sizeof(machine) - 1);
        }
        else
        {
            strncpy(machine, "unknown", sizeof(machine) - 1);
        }
#endif
        // truncate in case the value was too long
        machine[sizeof(machine) - 1] = '\0';
    }
    return machine;
}

// vim: ts=4 sw=4 et
