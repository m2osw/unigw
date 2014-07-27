/*    unittest_main.cpp
 *    Copyright (C) 2013-2014  Made to Order Software Corporation
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

#include <cppunit/BriefTestProgressListener.h>
#include <cppunit/CompilerOutputter.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TestRunner.h>
#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include "tools/license.h"
#include "libdebpackages/advgetopt.h"
#include "libdebpackages/debian_packages.h"
#include "libdebpackages/compatibility.h"
#include "time.h"
#if defined(MO_LINUX) || defined(MO_DARWIN) || defined(MO_SUNOS) || defined(MO_FREEBSD)
#   include <sys/types.h>
#   include <unistd.h>
#endif
#ifdef HAVE_QT4
#   include <qxcppunit/testrunner.h>
#   include <QApplication>
#endif


namespace unittest
{

std::string   tmp_dir;
std::string   wpkg_tool;

}


// Recursive dumps the given Test heirarchy to cout
namespace
{
void dump(CPPUNIT_NS::Test *test, std::string indent)
{
    if(test)
    {
        std::cout << indent << test->getName() << std::endl;

        // recursive for children
        indent += "  ";
        int max(test->getChildTestCount());
        for(int i = 0; i < max; ++i)
        {
            dump(test->getChildTestAt(i), indent);
        }
    }
}

template<class R>
void add_tests(const advgetopt::getopt& opt, R& runner)
{
    CPPUNIT_NS::Test *root(CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest());
    int max(opt.size("filename"));
    if(max == 0 || opt.is_defined("all"))
    {
        if(max != 0)
        {
            fprintf(stderr, "unittest: named tests on the command line will be ignored since --all was used.\n");
        }
        CPPUNIT_NS::Test *all_tests(root->findTest("All Tests"));
        if(all_tests == NULL)
        {
            // this should not happen because cppunit throws if they do not find
            // the test you specify to the findTest() function
            std::cerr << "error: no tests were found." << std::endl;
            exit(1);
        }
        runner.addTest(all_tests);
    }
    else
    {
        for(int i = 0; i < max; ++i)
        {
            std::string test_name(opt.get_string("filename", i));
            CPPUNIT_NS::Test *test(root->findTest(test_name));
            if(test == NULL)
            {
                // this should not happen because cppunit throws if they do not find
                // the test you specify to the findTest() function
                std::cerr << "error: test \"" << test_name << "\" was not found." << std::endl;
                exit(1);
            }
            runner.addTest(test);
        }
    }
}
}

int unittest_main(int argc, char *argv[])
{
    static const advgetopt::getopt::option options[] = {
        {
            '\0',
            0,
            NULL,
            NULL,
            "Usage: unittest [--opt] [test-name]",
            advgetopt::getopt::help_argument
        },
        {
            'a',
            0,
            "all",
            NULL,
            "run all the tests in the console (default)",
            advgetopt::getopt::no_argument
        },
        {
            'g',
            0,
            "gui",
            NULL,
            "start the GUI version if available",
            advgetopt::getopt::no_argument
        },
        {
            'h',
            0,
            "help",
            NULL,
            "print out this help screen",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "license",
            NULL,
            "prints out the license of the tests",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "licence",
            NULL,
            NULL, // hide this one from the help screen
            advgetopt::getopt::no_argument
        },
        {
            'l',
            0,
            "list",
            NULL,
            "list all the available tests",
            advgetopt::getopt::no_argument
        },
        {
            'S',
            0,
            "seed",
            NULL,
            "value to seed the randomizer",
            advgetopt::getopt::required_argument
        },
        {
            't',
            0,
            "tmp",
            NULL,
            "path to a temporary directory",
            advgetopt::getopt::required_argument
        },
        {
            'w',
            0,
            "wpkg",
            NULL,
            "path to the wpkg executable",
            advgetopt::getopt::required_argument
        },
        {
            'V',
            0,
            "version",
            NULL,
            "print out the wpkg project version these unit tests pertain to",
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

    std::vector<std::string> configuration_files;
    advgetopt::getopt opt(argc, argv, options, configuration_files, "UNITTEST_OPTIONS");

    if(opt.is_defined("help"))
    {
        opt.usage(advgetopt::getopt::no_error, "Usage: unittest [--opt] [test-name]");
        /*NOTREACHED*/
    }

    if(opt.is_defined("version"))
    {
        printf("%s\n", debian_packages_version_string());
        exit(1);
    }

    if(opt.is_defined("license") || opt.is_defined("licence"))
    {
        license::license();
        exit(1);
    }

    if(opt.is_defined("list"))
    {
        CPPUNIT_NS::Test *all = CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest();
        dump(all, "");
        exit(1);
    }

    // by default we get a different seed each time; that really helps
    // in detecting errors! (I know, I wrote loads of tests before)
    unsigned int seed(static_cast<unsigned int>(time(NULL)));
    if(opt.is_defined("seed"))
    {
        seed = static_cast<unsigned int>(opt.get_long("seed"));
    }
    srand(seed);
    printf("wpkg[%d]:unittest: seed is %d\n", getpid(), seed);

    // we can only have one of those for ALL the tests that directly
    // access the library...
    // (because the result is cached and thus cannot change)
#if defined(MO_WINDOWS)
    _putenv_s("WPKG_SUBST", "f=/opt/wpkg|/m2osw/packages:h=usr/local/bin/wpkg");
#else
    putenv(const_cast<char *>("WPKG_SUBST=f=/opt/wpkg|/m2osw/packages:h=usr/local/bin/wpkg"));
#endif

    if(opt.is_defined("tmp"))
    {
        unittest::tmp_dir = opt.get_string("tmp");
    }
    if(opt.is_defined("wpkg"))
    {
        unittest::wpkg_tool = opt.get_string("wpkg");
    }

    if(opt.is_defined("gui"))
    {
#ifdef HAVE_QT4
        QApplication app(argc, argv);
        QxCppUnit::TestRunner runner;
        //runner.addTest(CPPUNIT_NS::TestFactoryRegistry::getRegistry().makeTest());
        add_tests(opt, runner);
        runner.run();
#else
        fprintf(stderr, "error: no GUI compiled in this test, you cannot use the --gui option.\n");
        exit(1);
#endif
    }
    else
    {
        // Create the event manager and test controller
        CPPUNIT_NS::TestResult controller;

        // Add a listener that colllects test result
        CPPUNIT_NS::TestResultCollector result;
        controller.addListener(&result);        

        // Add a listener that print dots as test run.
        CPPUNIT_NS::BriefTestProgressListener progress;
        controller.addListener(&progress);      

        CPPUNIT_NS::TestRunner runner;

        add_tests(opt, runner);

        runner.run(controller);

        // Print test in a compiler compatible format.
        CPPUNIT_NS::CompilerOutputter outputter(&result, CPPUNIT_NS::stdCOut());
        outputter.write(); 

        if(result.testFailuresTotal())
        {
            return 1;
        }
    }

    return 0;
}


int main(int argc, char *argv[])
{
    return unittest_main(argc, argv);
}

// vim: ts=4 sw=4 et
