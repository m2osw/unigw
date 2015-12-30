/*    unittest_main.cpp
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

// Tell catch we want it to add the runner code in this file.
#define CATCH_CONFIG_RUNNER

#include <catch.hpp>

#include "tools/license.h"
#include "libdebpackages/advgetopt.h"
#include "libdebpackages/debian_packages.h"
#include "libdebpackages/compatibility.h"
#include "time.h"
#if defined(MO_LINUX) || defined(MO_DARWIN) || defined(MO_SUNOS) || defined(MO_FREEBSD)
#   include <sys/types.h>
#   include <unistd.h>
#endif


namespace unittest
{
    std::string   tmp_dir;
}


namespace
{
    struct UnitTestCLData
    {
        bool        help;
        bool        license;
        int         seed;
        std::string tmp;
        bool        version;

        UnitTestCLData()
            : help(false)
            , license(false)
            , seed(0)
            //, tmp
            , version(false)
        {
        }
    };

    void remove_from_args( std::vector<std::string>& vect, const std::string& long_opt, const std::string& short_opt )
    {
        auto iter = std::find_if( vect.begin(), vect.end(), [long_opt, short_opt]( const std::string& arg )
        {
            return arg == long_opt || arg == short_opt;
        });
        if( iter != vect.end() )
        {
            auto next_iter = iter;
            vect.erase( ++next_iter );
            vect.erase( iter );
        }
    }
}
// namespace


int unittest_main(int argc, char *argv[])
{
    UnitTestCLData configData;
    Clara::CommandLine<UnitTestCLData> cli;

    cli.bind( &UnitTestCLData::help )
        .describe( "display usage information" )
        .shortOpt( "?")
        .shortOpt( "h")
        .longOpt( "help" );
    cli.bind( &UnitTestCLData::license )
        .describe( "prints out the license of the tests" )
        .shortOpt( "l")
        .longOpt( "license" )
        .longOpt( "licence" );
    cli.bind( &UnitTestCLData::seed )
        .describe( "value to seed the randomizerd" )
        .shortOpt( "S")
        .longOpt( "seed" )
        .hint("the_seed");
    cli.bind( &UnitTestCLData::tmp )
        .describe( "path to a temporary directory" )
        .shortOpt( "t")
        .longOpt( "tmp" )
        .hint( "path" );
    cli.bind( &UnitTestCLData::version )
        .describe( "print out the wpkg project version these unit tests pertain to" )
        .shortOpt( "V")
        .longOpt( "version" );
    cli.parseInto( argc, argv, configData );

    if( configData.help )
    {
        cli.usage( std::cout, argv[0] );
        Catch::Session().run(argc, argv);
        exit(1);
    }

    if( configData.version )
    {
        std::cout << debian_packages_version_string() << std::endl;
        exit(1);
    }

    if( configData.license )
    {
        license::license();
        exit(1);
    }

    std::vector<std::string> arg_list;
    for( int i = 0; i < argc; ++i )
    {
        arg_list.push_back( argv[i] );
    }

    // by default we get a different seed each time; that really helps
    // in detecting errors! (I know, I wrote loads of tests before)
    unsigned int seed(static_cast<unsigned int>(time(NULL)));
    if( configData.seed != 0 )
    {
        seed = static_cast<unsigned int>(configData.seed);
        remove_from_args( arg_list, "--seed", "-s" );
    }
    srand(seed);
    std::cout << "wpkg[" << getpid() << "]:unittest: seed is " << seed << std::endl;

    // we can only have one of those for ALL the tests that directly
    // access the library...
    // (because the result is cached and thus cannot change)
#if defined(MO_WINDOWS)
    _putenv_s("WPKG_SUBST", "f=/opt/wpkg|/m2osw/packages:h=usr/local/bin/wpkg");
#else
    putenv(const_cast<char *>("WPKG_SUBST=f=/opt/wpkg|/m2osw/packages:h=usr/local/bin/wpkg"));
#endif

    if( !configData.tmp.empty() )
    {
        unittest::tmp_dir = configData.tmp;
        remove_from_args( arg_list, "--tmp", "-t" );
    }

    std::vector<char *> new_argv;
    std::for_each( arg_list.begin(), arg_list.end(), [&new_argv]( const std::string& arg )
    {
        new_argv.push_back( const_cast<char *>(arg.c_str()) );
    });

    return Catch::Session().run( static_cast<int>(new_argv.size()), &new_argv[0] );
}


int main(int argc, char *argv[])
{
    return unittest_main(argc, argv);
}

// vim: ts=4 sw=4 et
