/*    rc-version.cpp -- generate a version.rc
 *    Copyright (C) 2012-2013  Made to Order Software Corporation
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
 * \brief Implementation of the rc-version tool.
 *
 * This tool is an attempt in having a tool generate a VERSION block in
 * a resource that can be linked against your MS-Windows tools so they
 * show a Debian compatible version.
 *
 * At this point this is not really a success. However, most of the
 * information is available in a complete project, so assuming you have
 * such a project, we'll be able to refine this tool then and generate
 * a good version.rc file.
 */
#include    "libdebpackages/wpkg_control.h"
#include    "libdebpackages/advgetopt.h"
#include    "libdebpackages/debian_packages.h"
#include    "license.h"
#include    <stdio.h>
#include    <stdlib.h>


namespace
{
const advgetopt::getopt::option rc_version_options[] =
{
    {
        '\0',
        0,
        NULL,
        NULL,
        "Usage: rc-version <control-file>",
        advgetopt::getopt::help_argument
    },
    {
        'h',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help",
        NULL,
        "print the help message about all the rc-version commands and options",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "help-nobr",
        NULL,
        NULL,
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        advgetopt::getopt::GETOPT_FLAG_SHOW_USAGE_ON_ERROR,
        "version",
        NULL,
        "show the version of rc-version",
        advgetopt::getopt::no_argument
    },
    {
        '\0',
        0,
        "filename",
        NULL,
        NULL, // hidden argument in --help screen
        advgetopt::getopt::default_multiple_argument
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

const char rc_version_header[] =
{
    "#include <windows.h>\n"
    "#ifndef _MAC\n"
    "VS_VERSION_INFO VERSIONINFO\n"
};

    //" FILEVERSION 1,2,3,4"
    //" PRODUCTVERSION 1,2,3,4"

const char rc_version_body[] =
{
    " FILEFLAGSMASK 0x3fL\n"
    "#ifdef _DEBUG\n"
    " FILEFLAGS 0x21L\n"
    "#else\n"
    " FILEFLAGS 0x20L\n"
    "#endif\n"
    " FILEOS 0x4L\n"
    " FILETYPE 0x2L\n"
    " FILESUBTYPE 0x0L\n"
    "BEGIN\n"
    "    BLOCK \"StringFileInfo\"\n"
    "    BEGIN\n"
    "        BLOCK \"040904b0\"\n"
    "        BEGIN\n"
};

    //"            VALUE \"Comments\",            COMMENTS",
    //"            VALUE \"CompanyName\",         COMPANYNAME",
    //"            VALUE \"FileDescription\",     DESCRIPTION",
    //"            VALUE \"FileVersion\",         VERSION",
    //"            VALUE \"InternalName\",        INTERNALNAME",
    //"            VALUE \"LegalCopyright\",      COPYRIGHT",
    //"            VALUE \"LegalTrademarks\",     TRADEMARKS",
    //"            VALUE \"OriginalFilename\",    FILENAME",
    //"            VALUE \"ProductName\",         PRODUCTNAME",
    //"            VALUE \"ProductVersion\",      VERSION",
    //"            VALUE \"SpecialBuild\",        SPECIALBLD",

const char rc_version_footer[] =
{
    "            VALUE \"OLESelfRegister\", \"\\0\"\n"
    "        END\n"
    "    END\n"
    "    BLOCK \"VarFileInfo\"\n"
    "    BEGIN\n"
    "        VALUE \"Translation\", 0x409, 1200\n"
    "    END\n"
    "END\n"
    "#endif // !_MAC\n"
};


std::string escape(const std::string& str)
{
    std::string result;
    for(const char *s(str.c_str()); *s != '\0'; ++s)
    {
        switch(*s)
        {
        case '"':
        case '\\':
            result += "\\";
            /*FALLTROUGH*/
        default:
            result += *s;
            break;

        }
    }

    return result;
}
} // no name namespace




int main(int argc, char *argv[])
{
    // we have a top try/catch to ensure stack unwinding and thus
    // have true RAII at all levels whatever the compiler.
    try
    {
        std::vector<std::string> configuration_files;
        advgetopt::getopt opt(argc, argv, rc_version_options, configuration_files, "");
        if(opt.is_defined("help") || opt.is_defined("help-nobr"))
        {
            opt.usage(opt.is_defined("help-nobr") ? advgetopt::getopt::no_error_nobr : advgetopt::getopt::no_error, "Usage: rc-version [-<opt>] <filename>");
            /*NOTREACHED*/
        }
        if(opt.is_defined("version"))
        {
            printf("%s\n", debian_packages_version_string());
        }

        if(opt.size("filename") != 1)
        {
            throw std::runtime_error("rc-version must be used with exactly one filename");
        }
        memfile::memory_file cf;
        const wpkg_filename::uri_filename filename(opt.get_string("filename", 0));
        cf.read_file(filename);
        wpkg_control::source_control_file ctrl;
        ctrl.set_input_file(&cf);
        ctrl.read();
        ctrl.set_input_file(NULL);

        // get all the fields we want to put in the VERSION block
        std::string package(ctrl.get_field("Package"));
        if(package.empty())
        {
            throw std::runtime_error("The package name cannot be an empty string.");
        }
        std::string version(ctrl.get_field("Version"));
        const char *s(version.c_str());
        if(*s == '\0')
        {
            throw std::runtime_error("The package version cannot be an empty string.");
        }
        char *version_buffer(new char[version.length() + 1]);
        const char *v[4];
        int idx(0);
        for(; idx < 4; ++idx)
        {
            v[idx] = version_buffer;
            for(; *s >= '0' && *s <= '9'; ++s, ++version_buffer)
            {
                *version_buffer = *s;
            }
            if(*s != '.')
            {
                if(*s == '\0')
                {
                    *version_buffer = '\0';
                    break;
                }
                throw std::runtime_error("The rc-version tool only supports numbers separated by periods for versions.");
            }
            *version_buffer++ = '\0';
            // skip the dot
            ++s;
        }
        for(; idx < 4; ++idx)
        {
            v[idx] = "0";
        }

        std::string comment;
        if(ctrl.field_is_defined("Comment"))
        {
            comment = ctrl.get_field("Comment");
        }

        std::string long_description;
        std::string description(ctrl.get_description("Description", long_description));
        if(!long_description.empty())
        {
            description = long_description;
        }

        std::string copyright;
        if(ctrl.field_is_defined("Copyright"))
        {
            copyright = ctrl.get_field("Copyright");
        }

        std::string trademark;
        if(ctrl.field_is_defined("Trademark"))
        {
            trademark = ctrl.get_field("Trademark");
        }

        // start output the VERSION block
        printf("%s", rc_version_header);

        //" FILEVERSION 1,2,3,4"
        printf(" FILEVERSION %s,%s,%s,%s\n", v[0], v[1], v[2], v[3]);
        //" PRODUCTVERSION 1,2,3,4"
        printf(" PRODUCTVERSION %s,%s,%s,%s\n", v[0], v[1], v[2], v[3]);

        printf("%s", rc_version_body);

        //"            VALUE \"Comments\",            COMMENTS",
        if(!comment.empty())
        {
            printf("            VALUE \"Comments\", \"%s\"\n", escape(comment).c_str());
        }
        //"            VALUE \"CompanyName\",         COMPANYNAME",
        //"            VALUE \"FileDescription\",     DESCRIPTION",
        printf("            VALUE \"FileDescription\", \"%s\"\n", escape(description).c_str());
        //"            VALUE \"FileVersion\",         VERSION",
        printf("            VALUE \"FileVersion\", \"%s.%s.%s.%s\"\n", v[0], v[1], v[2], v[3]);
        //"            VALUE \"InternalName\",        INTERNALNAME",
        printf("            VALUE \"InternalName\", \"%s\"\n", escape(package).c_str());
        //"            VALUE \"LegalCopyright\",      COPYRIGHT",
        if(!copyright.empty())
        {
            printf("            VALUE \"LegalCopyright\", \"%s\"\n", escape(copyright).c_str());
        }
        //"            VALUE \"LegalTrademarks\",     TRADEMARKS",
        if(!trademark.empty())
        {
            printf("            VALUE \"LegalTrademarks\", \"%s\"\n", escape(trademark).c_str());
        }
        //"            VALUE \"OriginalFilename\",    FILENAME",
        printf("            VALUE \"OriginalFilename\", \"%s\"\n", escape(package).c_str());
        //"            VALUE \"ProductName\",         PRODUCTNAME",
        printf("            VALUE \"ProductName\", \"%s\"\n", escape(package).c_str());
        //"            VALUE \"ProductVersion\",      VERSION",
        printf("            VALUE \"ProductVersion\", \"%s.%s.%s.%s\"\n", v[0], v[1], v[2], v[3]);
        //"            VALUE \"SpecialBuild\",        SPECIALBLD",
        printf("            VALUE \"SpecialBuild\", \"%s\"\n", "wpkg");

        printf("%s", rc_version_footer);
    }
#ifdef WIN32
    // under windows the default for an exception is to be silent
    catch(const std::exception& e)
    {
        fprintf(stderr, "exception: %s\n", e.what());
        throw e;
    }
#endif
    catch(...)
    {
        // nothing to do here, we're just making sure we
        // get a full stack unwinding effect for RAII
        throw;
    }

    return 0;
}

// vim: ts=4 sw=4 et
