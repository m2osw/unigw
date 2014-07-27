/*    wpkg_architecture.cpp -- verify and compare architectures
 *    Copyright (C) 2012-2014  Made to Order Software Corporation
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
 * \brief Parse and compare architectures.
 *
 * This file is the implementation of the wpkg_architecture class. It is
 * capable of parsing a string and transforming it in a canonicalized
 * architecture.
 *
 * An architecture is a triplet including the operating system, the
 * vendor, and the processor information. Note that the functions also
 * support a duet with just the operating system and the processor
 * information (i.e. in most cases the vendor is optional.)
 *
 * The class can be used to compare two architectures against each others.
 * The == and != operators actually support patterns (architectures that
 * make use of the word "any" as one of its components.)
 */
#include    "libdebpackages/wpkg_architecture.h"

#include    <iostream>


/** \brief The architecture namespace.
 *
 * This namespace includes the different declarations and implementations
 * to support advance features in regard to the architecture.
 */
namespace wpkg_architecture
{


namespace
{


/** \brief List of supported abbreviations.
 *
 * The Debian system accepts the name of a processor as an architecture
 * abbreviation (although really "i386" is not equivalent to "linux-i386"
 * it may be equivalent to "i486-linux-gnu"...)
 *
 * Here we support different names that are rather common that we can
 * transform to an operating system. A processor name is accepted as
 * is to represent the type of computer that commonly used that processor.
 * For example, for wpkg "i386" is viewed as "linux-i386" and "powerpc"
 * is viewed as "darwin-powerpc".
 *
 * Similarly, an operating system name can be used as an abbreviation for
 * the most common architecture of that operating system. For example the
 * name "win32" is viewed as "win32-i386" and "darwin" is viewed as
 * "darwin-i386".
 *
 * wpkg has a command line option, --list-abbreviations, to get a list
 * of these abbreviation in your console.
 */
const architecture::abbreviation_t arch_abbreviation[] =
{
    // all and any patterns are special cases but understood as abbreviations
    {
        "all",
        "all", ""
    },
    {
        "any",
        "any", "any"
    },
    // Linux
    {
        "i386",
        "linux", "i386"
    },
    {
        "amd64",
        "linux", "amd64"
    },
    // MS-Windows
    {
        "win32",
        "mswindows", "i386"
    },
    {
        "win64",
        "mswindows", "amd64"
    },
    // Darwin
    {
        "darwin",
        "darwin", "i386"
    },
    {
        "darwin64",
        "darwin", "amd64"
    },
    {
        "darwinppc",
        "darwin", "powerpc"
    },
    // Solaris
    {
        "sunos",
        "solaris", "i386"
    },
    {
        "solaris",
        "solaris", "i386"
    },
    {
        "sunos64",
        "solaris", "amd64"
    },
    {
        "solaris64",
        "solaris", "amd64"
    },
    {
        "sparc",
        "solaris", "sparc"
    },
    {
        "sparc64",
        "solaris", "sparc64"
    },
    // FreeBSD
    {
        "freebsd",
        "freebsd", "i386"
    },
    {
        "freebsd64",
        "freebsd", "amd64"
    },
    {
        nullptr,
        nullptr, nullptr
    }
};

/** \brief List of supported operating systems.
 *
 * The following is a complete list of operating systems that are supported
 * by Debian and we also support them in wpkg. However, this is NOT the list
 * of operating system that wpkg can be compiled on. That is, we did not test
 * all of those systems and even less validated them.
 *
 * At this point we only support Ubuntu Linux 64 bit, Windows 32 and 64
 * bit, and Darwin 32 bit. Other operating systems are likely to be capable
 * of compiling and running wpkg without too much trouble but we do not
 * support them. We did compile and run wpkg with success on Fedora Linux,
 * FreeBSD, and Solaris.
 *
 * This list should allow all our users to create wpkg packages on any one
 * of those operating systems without having to edit this file, though.
 *
 * \note
 * The Debian list of operating systems includes two names such as gnu-linux.
 * What that means is not exactly clear to me. Darwin is marked as bsd-darwin,
 * and solaris as sysv-solaris. So we can see that it shows a basic flavor
 * and then a specific "implementation". However, I'm not too sure whether
 * this is used when specifying an architecture. The triplettable file
 * may be the way to go to understand how to clean up the triplet. For
 * example, gnu-linux-\<cpu> is transformed to \<cpu> only. So "amd64" means
 * "gnu-linux-amd64". Similarly "bsd-freebsd-\<cpu>" would mean
 * "freebsd-\<cpu>". Strangely enough, we place the vendor in second place,
 * but it feels like Debian would be putting it in the first place (but it
 * is clearly documented as \<os>-\<vendor>-\<cpu>.)
 *
 * \todo
 * Add other names to the list of operating in order to support names defined
 * by GNU and other sources.
 */
const architecture::os_t arch_os[] =
{
    { "any" },
    { "linux" },
    { "kfreebsd" },
    { "knetbsd" },
    { "kopensolaris" },
    { "hurd" },
    { "darwin" },
    { "freebsd" },
    { "mswindows" },
    { "netbsd" },
    { "openbsd" },
    { "solaris" },
    { "uclinux" },
    { NULL }
};


/** \brief The list of known processors.
 *
 * The arch_processor list includes all the processors that Debian
 * supports. We accept them all, although we really only worked on
 * little endian processors so far:
 *
 * \li i386
 * \li amd64
 *
 * However, we tested under Ubuntu, Fedora, FreeBSD, Solaris, Windows,
 * and Darwin which means many different operating systems and different
 * versions of two distinct compilers.
 *
 * You can list all the supported processors in your shell using
 * the wpkg --list-processors command.
 */
const architecture::processor_t arch_processor[] =
{
    // note that divers processors support both endianess at run time (bi-endian)
    // however, this table follows the Debian definitions
    //
    // WARNING: the order is IMPORTANT (and not exactly alphabetical) because
    //          the glob expressions need to be checked in that order
    //          (note that it is an "enhanced" glob since we support the | operator)
    { "any",     NULL,                          0, architecture::PROCESSOR_ENDIAN_UNKNOWN },
    { "alpha",   "alpha*",                     64, architecture::PROCESSOR_ENDIAN_LITTLE  },
    { "amd64",   NULL,                         64, architecture::PROCESSOR_ENDIAN_LITTLE  }, // note that "x86_64" is not valid ('_' is forbidden in an architecture name)
    { "arm64",   "aarch64",                    64, architecture::PROCESSOR_ENDIAN_LITTLE  },
    { "armeb",   "arm*b",                      32, architecture::PROCESSOR_ENDIAN_BIG     },
    { "arm",     "arm*",                       32, architecture::PROCESSOR_ENDIAN_LITTLE  },
    { "avr32",   NULL,                         32, architecture::PROCESSOR_ENDIAN_BIG     },
    { "i386",    "i[4-6]86|pentium",           32, architecture::PROCESSOR_ENDIAN_LITTLE  },
    { "ia64",    NULL,                         64, architecture::PROCESSOR_ENDIAN_LITTLE  },
    { "hppa",    "hppa*",                      32, architecture::PROCESSOR_ENDIAN_BIG     },
    { "m32r",    NULL,                         32, architecture::PROCESSOR_ENDIAN_BIG     },
    { "m68k",    NULL,                         32, architecture::PROCESSOR_ENDIAN_BIG     },
    { "mips",    "mipseb",                     32, architecture::PROCESSOR_ENDIAN_BIG     },
    { "mipsel",  NULL,                         32, architecture::PROCESSOR_ENDIAN_LITTLE  },
    { "powerpc", "ppc",                        32, architecture::PROCESSOR_ENDIAN_BIG     },
    { "ppc64",   NULL,                         64, architecture::PROCESSOR_ENDIAN_BIG     },
    { "s390",    NULL,                         32, architecture::PROCESSOR_ENDIAN_BIG     },
    { "s390x",   NULL,                         64, architecture::PROCESSOR_ENDIAN_BIG     },
    { "sh3",     NULL,                         32, architecture::PROCESSOR_ENDIAN_LITTLE  },
    { "sh3eb",   NULL,                         32, architecture::PROCESSOR_ENDIAN_BIG     },
    { "sh4",     NULL,                         32, architecture::PROCESSOR_ENDIAN_LITTLE  },
    { "sh4eb",   NULL,                         32, architecture::PROCESSOR_ENDIAN_BIG     },
    { "sparc",   NULL,                         32, architecture::PROCESSOR_ENDIAN_BIG     },
    { "sparc64", NULL,                         64, architecture::PROCESSOR_ENDIAN_BIG     },
    { NULL,      NULL,                          0, architecture::PROCESSOR_ENDIAN_UNKNOWN }
};



} // no name namespace


/** \class wpkg_architecture_exception
 * \brief Basic exception for all architecture exceptions.
 *
 * The basic exception for any exception in the implementation of the
 * architecture type.
 */


/** \class wpkg_architecture_exception_invalid
 * \brief Exception representing an invalid architecture.
 *
 * This exception is generally raised when an invalid character, value,
 * string, etc. is passed to an architecture function.
 */


/** \class architecture
 * \brief The parser, comparator, and canonicalization of architectures.
 *
 * This class defines the functions used to parse, compare and canonicalize
 * architectures as understood by the Debian environment.
 *
 * Architectures are composed of three parts:
 *
 * \li Operating System
 * \li Vendor
 * \li Processor
 *
 * The Vendor is most often omitted, but it can be useful to enforce only
 * having packages from one vendor (otherwise one can mix packages from
 * different origins which is not always something wanted.)
 *
 * The lists of operating systems and processors are both limited to what
 * Debian defines. Although we added "mswindows" to the list of operating
 * systems.
 *
 * At this point, we really only support:
 *
 * \li Operating Systems: linux and mswindows
 * \li Processor: i386 and amd64
 *
 * The canonicalization will transform other names such as "win32" into
 * what we currently support: "mswindows" in this example.
 */


/** \struct architecture::abbreviation_t
 * \brief Definition of an abbreviation in terms of operating system and processor.
 *
 * This structure defines an abbreviation which represents a specific set
 * of operating system and processor. For example, "win32" means operating
 * system "mswindows" and "i386" processor.
 *
 * You can retrieve a list of abbreviations with a call to the
 * architecture::abbreviation_list() function.
 */


/** \struct architecture::os_t
 * \brief Name of a supported operating system.
 *
 * This structure is used to create a list of supported operating system.
 * The list can be obtained with the architecture::os_list(). The list is
 * NULL terminated (meaning that the last entry has its f_name parameter
 * set to NULL.)
 */


/** \struct architecture::processor_t
 * \brief Name of a processor, aliases, size, and endian.
 *
 * This structure defines a processor as supported by wpkg (and Debian.)
 *
 * The aliases are defined as a list of names separated by a pipe (|)
 * character and written as uri_filename::glob() compatible patterns.
 * When parsing an architecture, any one of those names are considered
 * valid. When writing an architecture, only the main name is used which
 * renders the output canonicalized.
 *
 * The size represents the number of bits the processor supports by
 * default (32 or 64).
 *
 * The endian parameter defines how the processor handles groups of
 * bytes (words, quad words, etc.)
 *
 * You can retrieve a list of the currently supported processors by calling
 * the architecture::processor_list() function.
 */


/** \brief The name of the unknown vendor.
 *
 * When setting up an architecture, the vendor segment is set to this
 * UNKNOWN_VENDOR value by default. In other words, an undefined
 * vendor entry is viewed as "unknown" and not "".
 *
 * An empty architecture, however, has "" as its vendor string.
 */
const std::string architecture::UNKNOWN_VENDOR = "unknown";


/** \brief Check whether a vendor string is valid.
 *
 * This function verifies the characters of a vendor string for validity.
 *
 * A vendor name has the same restriction as the package name and it cannot
 * include a dash (-) because the different parts of an architecture are
 * already separated by dashes.
 *
 * This means the following are accepted in a vendor string:
 *
 * \li Digits (0-9)
 * \li Lowercase letters (a-z)
 * \li Period (.)
 * \li Plus (+)
 *
 * Any other characters will generate an error.
 *
 * \param[in] vendor  The vendor to be validated.
 *
 * \return true if the vendor name is considered valid.
 */
bool architecture::valid_vendor(const std::string& vendor)
{
    for(const char *s(vendor.c_str()); *s != '\0'; ++s)
    {
        if((*s < '0' || *s > '9')
        && (*s < 'a' || *s > 'z')
        && *s != '.' && *s != '+')
        {
            return false;
        }
    }

    return true;
}


/** \brief Check whether \p abbreviation is valid.
 *
 * This function verifies that the specified \p abbreviation parameter
 * is a valid abbreviation.
 *
 * \param[in] abbreviation  The abbreviation to validate.
 *
 * \return A pointer to the matching abbreviation or NULL.
 */
const architecture::abbreviation_t *architecture::find_abbreviation(const std::string& abbreviation)
{
    for(const abbreviation_t *a(arch_abbreviation); a->f_abbreviation != NULL; ++a)
    {
        if(abbreviation == a->f_abbreviation)
        {
            return a;
        }
    }

    // unknown abbreviation
    return NULL;
}


/** \brief Check whether \p os is recognized as valid.
 *
 * wpkg understands a certain number of operating systems and this
 * function returns true if the \p os parameter is one of those
 * accepted operating systems.
 *
 * The operating system name can also be set to the special value
 * "any" when specifying a pattern. This function returns true if
 * the value is "any". It is up to you to check in the event you
 * do not support patterns.
 *
 * \param[in] os  The name of the operating system to be checked.
 *
 * \return A pointer to the matching operating system (os_t) or NULL.
 */
const architecture::os_t *architecture::find_os(const std::string& os)
{
    std::string operating_system(os);
    if(os == "win32" || os == "win64")
    {
        // backward compatibility
        operating_system = "mswindows";
    }
    for(const os_t *o(arch_os); o->f_name != NULL; ++o)
    {
        if(operating_system == o->f_name)
        {
            return o;
        }
    }

    // unknown operating system
    return NULL;
}


/** \brief Validate the processor.
 *
 * This function ensures that the specified \p processor is defined
 * in the list of accepted processors supported by wpkg.
 *
 * Note that the function accepts the special processor name "any".
 * If you do not support patterns, then make sure to add a test
 * to prevent "any" as a valid processor.
 *
 * \param[in] processor  The processor to validate.
 * \param[in] extended  Also check whether it matches the glob patterns.
 *
 * \return the pointer of the processor that match or NULL when there is no match
 */
const architecture::processor_t *architecture::find_processor(const std::string& processor, bool extended)
{
    if(extended)
    {
        // in this case the processor name may include invalid characters
        // that would not be caught by the '*' in a pattern
        if(!valid_vendor(processor))
        {
            return NULL;
        }
    }

    // we use a "filename" so that way we can test with glob() patterns
    // (or should we move the pattern check to wpkg_util?)
    wpkg_filename::uri_filename processor_filename(processor);
    for(const processor_t *p(arch_processor); p->f_name != NULL; ++p)
    {
        if(processor == p->f_name)
        {
            return p;
        }
        if(extended && p->f_other_names != NULL)
        {
            const char *start(p->f_other_names);
            for(const char *s(start);; ++s)
            {
                // end of string or pipe indicate the end of the current pattern
                if(*s == '\0' || *s == '|')
                {
                    std::string pattern(start, s - start);
                    if(processor_filename.glob(pattern.c_str()))
                    {
                        return p;
                    }
                    if(*s == '\0')
                    {
                        break;
                    }
                }
            }
        }
    }

    // unknown processor
    return NULL;
}


/** \brief Retrieve a pointer to the supported list of abbreviations.
 *
 * This function returns a pointer to the internal list of supported
 * abbreviations. The list of abbreviations are constant and cannot
 * be modified. You may use it to print the list to users so they
 * can choose one of the abbreviations if so they wish.
 *
 * The list ends with a NULL entry meaning that the f_abbreviation
 * of the last item in the list is a NULL pointer.
 *
 * The f_os and f_processor correspond the the operating system and
 * processor that this abbreviation represents.
 *
 * To find an abbreviation from a string, use the find_abbreviation()
 * function instead.
 *
 * \return A constant pointer to a null terminated list of abbreviations.
 */
const architecture::abbreviation_t *architecture::abbreviation_list()
{
    return arch_abbreviation;
}


/** \brief Return the list of operating systems.
 *
 * This function returns a constant pointer to read-only memory with
 * the list of currently supported operating systems. This list represents
 * the name of the operating systems for which wpkg can build and then
 * manage packages for.
 *
 * The list ends with one NULL entry meaning that the f_name of the last
 * item in the list is a NULL pointer.
 *
 * To find an operating system from a string, use the find_os() function
 * instead.
 *
 * \return A pointer to the list of operating systems.
 */
const architecture::os_t *architecture::os_list()
{
    return arch_os;
}


/** \brief Return the list of processors.
 *
 * This function returns a constant pointer to read-only memory with
 * the list of currently supported operating systems. This list
 * represents the name of the processor, other acceptable names,
 * the number of bits on the architecture (32 or 64) and the
 * endianess of the architecture.
 *
 * The list ends with one NULL entry meaning that the f_name of the last
 * item in the list is a NULL pointer.
 *
 * To find a processor from a string, use the find_processor() function
 * instead.
 *
 * \return A pointer to the list of processors.
 */
const architecture::processor_t *architecture::processor_list()
{
    return arch_processor;
}


/** \brief Initialize an empty architecture.
 *
 * This constructor initializes an empty architecture. This is a special
 * case of an architecture that is considered "empty" (may be used as
 * an undefined architecture.)
 */
architecture::architecture()
    //: f_os("") -- auto-init
    //, f_vendor("") -- auto-init
    //, f_processor("") -- auto-init
    //, f_ignore_vendor(false) -- auto-init
{
}


/** \brief Initialize the architecture with arch.
 *
 * This constructor creates an architecture object and defines the
 * architecture triplet using the arch parameter.
 *
 * \exception wpkg_architecture_exception_invalid
 * If the architecture is invalid, then this exception is raised.
 *
 * \param[in] arch  The architecture used to initialize this instance.
 * \param[in] ignore_vendor_field  Whether the vendor field will be ignored
 *                                 while comparing two architectures.
 */
architecture::architecture(const std::string& arch, bool ignore_vendor_field)
    //: f_os("") -- auto-init
    //, f_vendor("") -- auto-init
    //, f_processor("") -- auto-init
    : f_ignore_vendor(ignore_vendor_field)
{
    if(!set(arch))
    {
        throw wpkg_architecture_exception_invalid("\"" + arch + "\" is an invalid architecture.");
    }
}


/** \brief Copy an architecture in a new object.
 *
 * This constructor creates a copy of another architecture.
 *
 * \param[in] arch  The source architecture.
 */
architecture::architecture(const architecture& arch)
    : f_os(arch.f_os)
    , f_vendor(arch.f_vendor)
    , f_processor(arch.f_processor)
    , f_ignore_vendor(arch.f_ignore_vendor)
{
}


/** \brief Check whether an architecture is empty.
 *
 * An empty architecture may be used in places where an undefined architecture
 * is necessary. It also allows us to have vectors and maps of architectures.
 *
 * \return true if the architecture represent the empty architecture.
 */
bool architecture::empty() const
{
    return f_os.empty() && f_vendor.empty() && f_processor.empty();
}


/** \brief Check whether this is a pattern.
 *
 * An architecture is considered a pattern if any one of its triplet members
 * is set to the special name "any". For example, to find all the architectures
 * that match the amd64 processor one can write:
 *
 * \code
 * "any-any-amd64"
 * \endcode
 *
 * \return true if this architecture is considered a pattern.
 */
bool architecture::is_pattern() const
{
    return f_os == "any" || f_vendor == "any" || f_processor == "any";
}


/** \brief Check whether the architecture represents source files.
 *
 * When the processor name is set to "src" or "source" then the files
 * in that package are assumed to be source files. In that case this
 * function returns true.
 *
 * \return true if the processor is "source".
 */
bool architecture::is_source() const
{
    return f_processor == "source";
}


/** \brief Detect whether the operating system is a Unix compatible system.
 *
 * This function checks whether the operating system of this architecture
 * object represents a Unix compatible system. This includes all forms
 * of operating systems such as Linux, Darwin, FreeBSD, Solaris, etc.
 *
 * Note that if the operating system is set to "all" then this function
 * returns false because "all" could represent a non-Unix system.
 *
 * Note that "any" also returns false, although that represents a pattern
 * and not what I would call a valid architecture name.
 *
 * \note
 * At this point we only support MS-Windows which is not a Unix compatible
 * operating system.
 *
 * \return True if the operating system is a Unix compatible system.
 */
bool architecture::is_unix() const
{
    return f_os != "mswindows" && f_os != "any" && f_os != "all" && !f_os.empty();
}


/** \brief Detect whether the operating system is a MS-Windows compatible system.
 *
 * This function checks whether the operating system of this architecture
 * object represents a MS-Windows compatible system. This includes all forms
 * of operating systems such as MS-Windows, Wine (although we do not support
 * that one), etc.
 *
 * Note that if the operating system is set to "any" then this function
 * returns false because "any" could represent a non-MS-Windows system.
 *
 * \return True if the operating system is "mswindows".
 */
bool architecture::is_mswindows() const
{
    return f_os == "mswindows";
}


/** \brief Retrieve the operating system part of this architecture object.
 *
 * This function returns the operating system part of this architecture
 * object.
 *
 * Example of operating systems: linux, mswindows, darwin.
 *
 * \return A string representing the operating system of this architecture.
 */
const std::string& architecture::get_os() const
{
    return f_os;
}


/** \brief Retrieve the vendor name of this architecture object.
 *
 * This function returns the vendor name of this architecture
 * object.
 *
 * Example of vendor: m2osw.
 *
 * \return A string representing the vendor name in this architecture.
 */
const std::string& architecture::get_vendor() const
{
    return f_vendor;
}


/** \brief Retrieve the processor (CPU) of this architecture object.
 *
 * This function returns the processor name (also often called the CPU)
 * of this architecture object.
 *
 * Processors examples: i386, amd64, powerpc.
 *
 * \return A string representing the processor of this architecture.
 */
const std::string& architecture::get_processor() const
{
    return f_processor;
}


/** \brief Parse an architecture string and defines the triplet accordingly.
 *
 * This function parses the \p arch parameter in an architecture triplet.
 * If the architecture information is invalid, then the function returns
 * false and this architecture object is not modified.
 *
 * The input is an architecture triplet where the \<vendor> part is optional.
 * Also the system understands a certain number of architectures that are
 * one word abbreviations such as "linux" which means "linux-i386", and
 * "win64" which means "win64-amd64".
 *
 * The list of operating systems and processors supported can be listed
 * using the os_list() and the processor_list() functions.
 *
 * When a vendor is specified, the operating system and the processor must
 * also be present or the function returns false (error.)
 *
 * You may set the architecture to the empty architecture by using the
 * empty string as the \p arch parameter.
 *
 * \param[in] arch  The architecture triplet defined as: \<os>-\<vendor>-\<processor>.
 *
 * \return true if the \p arch parameter was a valid architecture string.
 */
bool architecture::set(const std::string& arch)
{
    // the empty architecture
    if(arch.empty())
    {
        f_os = "";
        f_vendor = "";
        f_processor = "";
        return true;
    }

    // valid on any architecture (but not a pattern)
    if(arch == "all")
    {
        f_os = "all";
        f_vendor = "all";
        f_processor = "all";
        return true;
    }

    // special case of a source package
    if(arch == "src" || arch == "source")
    {
        f_os = "all";
        f_vendor = "all";
        f_processor = "source";
        return true;
    }

    // any triplet
    if(arch == "any")
    {
        f_os = "any";
        f_vendor = "any";
        f_processor = "any";
        return true;
    }

    std::string os;
    std::string vendor(UNKNOWN_VENDOR);
    std::string processor;

    const std::string::size_type p(arch.find_first_of('-'));
    if(p == std::string::npos)
    {
        // <abbreviation>
        const abbreviation_t *abbr(find_abbreviation(arch));
        if(abbr == NULL)
        {
            // unknown abbreviation
            return false;
        }
        os = abbr->f_os;
        processor = abbr->f_processor;
    }
    else
    {
        // an architecture name cannot start with a '-'
        if(p == 0)
        {
            // same as testing 'os.empty()'
            return false;
        }

        os = arch.substr(0, p);
        const std::string::size_type q(arch.find_first_of('-', p + 1));
        if(q == std::string::npos)
        {
            // <os>-<processor>
            processor = arch.substr(p + 1);

            if(processor.empty())
            {
                return false;
            }
        }
        else
        {
            // <os>-<vendor>-<processor>
            vendor = arch.substr(p + 1, q - p - 1);
            processor = arch.substr(q + 1);

            // should we allow for an empty vendor?
            if(vendor.empty() || processor.empty())
            {
                return false;
            }

            if(!valid_vendor(vendor))
            {
                return false;
            }
        }
    }

    // here we have a semi-valid triplet in os, vendor, and processor
    // variables now we want to verify that these are valid (supported)
    // architecture parameters as found in our lists and as we are at
    // it we canonicalize
    const os_t *co(find_os(os));
    const processor_t *cp(find_processor(processor, true));
    if(co == NULL || cp == NULL)
    {
        return false;
    }

    f_os = co->f_name;
    f_vendor = vendor;
    f_processor = cp->f_name;

    return true;
}


/** \brief Create a canonicalized string of the architecture.
 *
 * This function returns the canonicalized version of the architecture
 * from the operating system, vendor, and processor currently defined
 * in this architecture object.
 *
 * If all three strings are empty, then the empty string is returned
 * (i.e. the empty architecture.)
 *
 * If all the strings are "any", or the vendor is empty and both others
 * are "any", then "any" is returned.
 *
 * If the vendor says "any" and the other two strings are not, then
 * the \<os>-\<processor> architecture syntax is returned.
 *
 * In all other cases, the \<os>-\<vendor>-\<processor> syntax is used.
 */
std::string architecture::to_string() const
{
    // the unknown vendor is nearly the same as "any" in our case
    if(f_vendor == UNKNOWN_VENDOR || f_vendor == "")
    {
        if(f_os == "" && f_processor == "")
        {
            // empty
            return "";
        }
        if(f_os == "any" && f_processor == "any")
        {
            // any pattern
            return "any";
        }
        // standard <os>-<processor>
        return f_os + "-" + f_processor;
    }

    if(f_os == "any" && f_vendor == "any" && f_processor == "any")
    {
        return "any";
    }

    // full <os>-<vendor>-<processor>
    return f_os + "-" + f_vendor + "-" + f_processor;
}


/** \brief Change whether the vendor string is ignored.
 *
 * By default the vendor string is checked each time two architectures are
 * compared. By setting this value to true, then the vendor strings are
 * ignored while comparing architectures.
 *
 * Note that only one architecture object needs to have that flag set to
 * true for the vendor string to be ignored although it is customary to
 * set both architecture objects to the same value.
 *
 * \param[in] ignore_vendor_field  Whether the vendor string should be ignored.
 */
void architecture::set_ignore_vendor(bool ignore_vendor_field)
{
    f_ignore_vendor = ignore_vendor_field;
}


/** \brief Get whether the vendor string is ignored.
 *
 * This function returns true if the vendor string is ignored while comparing
 * this architecture with another.
 *
 * \return true if the vendor string is to be ignored.
 */
bool architecture::ignore_vendor() const
{
    return f_ignore_vendor;
}


/** \brief Copy an architecture in another.
 *
 * This function copies the right hand side (rhs) architecture values
 * in this architecture object.
 *
 * \param[in] rhs  The right hand side architecture to copy.
 *
 * \return A reference to this architecture.
 */
architecture& architecture::operator = (const architecture& rhs)
{
    if(this != &rhs)
    {
        f_os = rhs.f_os;
        f_vendor = rhs.f_vendor;
        f_processor = rhs.f_processor;
        f_ignore_vendor = rhs.f_ignore_vendor;
    }

    return *this;
}


/** \brief Transform the architecture back to a string.
 *
 * This cast operator is the same as calling the to_string() function.
 * It gets you the canonicalized string representing this architecture.
 *
 * \return The architecture as a string.
 */
architecture::operator std::string () const
{
    return to_string();
}


/** \brief Check whether the architecture is not empty.
 *
 * An undefined architecture, or setting the architecture to the empty
 * string (""), can be detected with the empty() function or the bool
 * cast operator or the logical not (!) operator.
 *
 * \param[in] rhs  The architecture to test against, usually a pattern.
 *
 * \return true if architecture is considered NOT empty.
 */
architecture::operator bool () const
{
    return !empty();
}


/** \brief Check whether the architecture is empty.
 *
 * An undefined architecture, or setting the architecture to the empty
 * string (""), can be detected with the empty() function or the bool
 * cast operator or the logical not (!) operator.
 *
 * \param[in] rhs  The architecture to test against, usually a pattern.
 *
 * \return true if the architecture is considered empty.
 */
bool architecture::operator ! () const
{
    return empty();
}


/** \brief Compare two architectures for equality.
 *
 * Compare two architectures and return true when they are considered
 * equal. This test checks whether one or both of the architectures
 * are patterns. Note that when comparing patterns against each others,
 * the "any" entries must match the corresponding entries as "any" in
 * the other architecture (in other words, any-amd64 is not equal to
 * any-any.)
 *
 * So, an architecture such as linux-amd64 will match any-amd64 and thus
 * this function will return true in that case.
 *
 * \param[in] rhs  The architecture to test against, usually a pattern.
 *
 * \return true if both architectures are considered equal.
 */
bool architecture::operator == (const architecture& rhs) const
{
    return ! operator != (rhs);
}


/** \brief Compare two architectures for inequality.
 *
 * Compare two architectures and return true when they are considered
 * unequal. This test checks whether one or both of the architectures
 * are patterns. Note that when comparing patterns against each others,
 * the "any" entries must match the corresponding entries as "any" in
 * the other architecture (in other words, any-amd64 is not equal to
 * any-any.)
 *
 * So, an architecture such as linux-amd64 will match any-amd64 and thus
 * this function will return false in that case.
 *
 * \param[in] rhs  The architecture to test against, usually a pattern.
 *
 * \return true if both architectures are considered unequal.
 */
bool architecture::operator != (const architecture& rhs) const
{
    const bool pa(is_pattern());
    const bool pb(rhs.is_pattern());
    const bool iv(f_ignore_vendor || rhs.f_ignore_vendor);

//printf("%d/%d => %s-%s-%s vs %s-%s-%s\n", pa, pb, f_os.c_str(), f_vendor.c_str(), f_processor.c_str(), rhs.f_os.c_str(), rhs.f_vendor.c_str(), rhs.f_processor.c_str());
    if(pa ^ pb)
    {
        // compare pattern against architecture
        const architecture *a(pa ? &rhs : this);
        const architecture *p(pa ? this : &rhs);
        if(p->f_os != "any" && p->f_os != a->f_os)
        {
            return true;
        }
        if(!iv && p->f_vendor != "any" && p->f_vendor != UNKNOWN_VENDOR && p->f_vendor != a->f_vendor)
        {
            return true;
        }
        if(p->f_processor != "any" && p->f_processor != a->f_processor)
        {
            return true;
        }
    }
    else
    {
        // compare architecture against architecture
        //      or pattern against pattern
        if(f_os != rhs.f_os
        || (!iv && f_vendor != rhs.f_vendor && f_vendor != UNKNOWN_VENDOR && rhs.f_vendor != UNKNOWN_VENDOR)
        || f_processor != rhs.f_processor)
        {
            return true;
        }
    }

    return false;
}


/** \brief Determine whether this is smaller than rhs.
 *
 * This function compares this architecture against the \p rhs architecture
 * and returns true if this is considered smaller.
 *
 * This is just so one can sort architectures for faster processing. This is
 * no real order for architectures otherwise.
 *
 * \param[in] rhs  The other architecture to compare against.
 *
 * \return true if this is smaller than rhs.
 */
bool architecture::operator < (const architecture& rhs) const
{
    if(f_ignore_vendor || rhs.f_ignore_vendor)
    {
        return f_os < rhs.f_os
            || (f_os == rhs.f_os && f_processor < rhs.f_processor);
    }
    return f_os < rhs.f_os
        || (f_os == rhs.f_os && f_vendor < rhs.f_vendor)
        || (f_os == rhs.f_os && f_vendor == rhs.f_vendor && f_processor < rhs.f_processor);
}


/** \brief Determine whether this is smaller or equal to rhs.
 *
 * This function compares this architecture against the \p rhs architecture
 * and returns true if this is considered smaller or equal.
 *
 * This is just so one can sort architectures for faster processing. This is
 * no real order for architectures otherwise.
 *
 * \param[in] rhs  The other architecture to compare against.
 *
 * \return true if this is smaller or equal to rhs.
 */
bool architecture::operator <= (const architecture& rhs) const
{
    if(f_ignore_vendor || rhs.f_ignore_vendor)
    {
        return f_os < rhs.f_os
            || (f_os == rhs.f_os && f_processor <= rhs.f_processor);
    }
    return f_os < rhs.f_os
        || (f_os == rhs.f_os && f_vendor < rhs.f_vendor)
        || (f_os == rhs.f_os && f_vendor == rhs.f_vendor && f_processor <= rhs.f_processor);
}


/** \brief Determine whether this is larger than rhs.
 *
 * This function compares this architecture against the \p rhs architecture
 * and returns true if this is considered larger.
 *
 * This is just so one can sort architectures for faster processing. This is
 * no real order for architectures otherwise.
 *
 * \param[in] rhs  The other architecture to compare against.
 *
 * \return true if this is larger than rhs.
 */
bool architecture::operator > (const architecture& rhs) const
{
    if(f_ignore_vendor || rhs.f_ignore_vendor)
    {
        return f_os > rhs.f_os
            || (f_os == rhs.f_os && f_processor > rhs.f_processor);
    }
    return f_os > rhs.f_os
        || (f_os == rhs.f_os && f_vendor > rhs.f_vendor)
        || (f_os == rhs.f_os && f_vendor == rhs.f_vendor && f_processor > rhs.f_processor);
}


/** \brief Determine whether this is larger or equal to rhs.
 *
 * This function compares this architecture against the \p rhs architecture
 * and returns true if this is considered larger or equal.
 *
 * This is just so one can sort architectures for faster processing. This is
 * no real order for architectures otherwise.
 *
 * \param[in] rhs  The other architecture to compare against.
 *
 * \return true if this is larger or equal to rhs.
 */
bool architecture::operator >= (const architecture& rhs) const
{
    if(f_ignore_vendor || rhs.f_ignore_vendor)
    {
        return f_os > rhs.f_os
            || (f_os == rhs.f_os && f_processor >= rhs.f_processor);
    }
    return f_os > rhs.f_os
        || (f_os == rhs.f_os && f_vendor > rhs.f_vendor)
        || (f_os == rhs.f_os && f_vendor == rhs.f_vendor && f_processor >= rhs.f_processor);
}



}   // namespace wpkg_architecture

// vim: ts=4 sw=4 et
