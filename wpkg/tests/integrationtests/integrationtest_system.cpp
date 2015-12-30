/*    integrationtest_system.cpp
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

#include "integrationtest_main.h"
#include "libdebpackages/debian_packages.h"
#include "libdebpackages/wpkg_control.h"
#include <stdexcept>
#include <string.h>
#include <catch.hpp>

// for the WEXITSTATUS()
#ifdef __GNUC__
#	pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

//
// System tests are used to test the full monty of the --build process:
//
// Simple build of one project
//    cd project/path
//    wpkg --build
//    wpkg --build project-src_version.deb
//
// Build of a repository of source projects
//    cd project/path
//    wpkg --build --output-dir repository
//    ... [repeat for each project]
//    wpkg --build repository
//
// Also to finalize the test we want to make sure that the tests succeeded
// by installing a "special" package with an integration test.
//

namespace
{

struct project_info
{
    const char *        f_name;
    const char *        f_version;
    const char *        f_component;
};

typedef std::vector<project_info> project_list_t;

const char * const g_projects[] =
{
    // PROJECT ONE
    ">project one",
    ">version 1.5.2",
    ">component main/admin",
    ">file CMakeLists.txt",
        "set(ONE_VERSION_MAJOR 1)",
        "set(ONE_VERSION_MINOR 5)",
        "set(ONE_VERSION_PATCH 2)",
        "cmake_minimum_required(VERSION 2.8.4)",
        "project(one)",
        "add_executable(${PROJECT_NAME} a.cpp)",
        "install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin COMPONENT runtime)",
        "project(extension_one)",
        "add_library(${PROJECT_NAME} SHARED extension_one.cpp)",
        "install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib COMPONENT runtime)",
        "project(extension_one_static)",
        "add_library(${PROJECT_NAME} STATIC extension_one.cpp)",
        "install(TARGETS ${PROJECT_NAME} ARCHIVE DESTINATION lib COMPONENT development)",
        "install(DIRECTORY ${PROJECT_SOURCE_DIR}/. DESTINATION include COMPONENT development FILES_MATCHING PATTERN *.h)",
        "add_custom_target(run_unit_tests",
        "COMMAND ./one",
        "WORKING_DIRECTORY ${CMAKE_BINARY_DIR}",
        "DEPENDS ${PROJECT_NAME})",
        "set(CPACK_PACKAGE_NAME \"one\")",
        "set(CPACK_PACKAGE_DESCRIPTION_SUMMARY \"Test One\")",
        "set(CPACK_PACKAGE_VENDOR \"Made to Order Software Corporation\")",
        "set(CPACK_PACKAGE_CONTACT \"contact@m2osw.com\")",
        "set(CPACK_RESOURCE_FILE_LICENSE \"${one_SOURCE_DIR}/COPYING\")",
        "set(CPACK_SOURCE_GENERATOR \"TGZ\")",
        "set(CPACK_PACKAGE_VERSION \"${ONE_VERSION_MAJOR}.${ONE_VERSION_MINOR}.${ONE_VERSION_PATCH}\")",
        "set(CPACK_PACKAGE_VERSION_MAJOR \"${ONE_VERSION_MAJOR}\")",
        "set(CPACK_PACKAGE_VERSION_MINOR \"${ONE_VERSION_MINOR}\")",
        "set(CPACK_PACKAGE_VERSION_PATCH \"${ONE_VERSION_PATCH}\")",
        "set(CPACK_SOURCE_PACKAGE_FILE_NAME \"one_${ONE_VERSION_MAJOR}.${ONE_VERSION_MINOR}.${ONE_VERSION_PATCH}\")",
        "set(CPACK_COMPONENTS_ALL runtime development)",
        "set(CPACK_DEB_COMPONENT_INSTALL ON)",
        "include(CPack)",
    ">file a.cpp",
        "#include <stdio.h>",
        "#include <stdlib.h>",
        "int main()",
        "{printf(\"a.cpp ran\\n\");exit(0);}",
    ">file extension_one.h",
        "#pragma once",
        "extern int ext_special();",
    ">file extension_one.cpp",
        "int ext_special()",
        "{return 5;}",
    ">file wpkg/build_number",
        "0",
    ">file wpkg/control.info",
        "Sub-Packages: runtime*, development",
        "Package/runtime: one",
        "Package/development: one-dev",
        "Architecture: $(architecture())",
        "Homepage: http://windowspackager.org/",
        "Description: Project one",
        "Component: main/admin",
    ">file wpkg/copyright",
        "Format: http://www.debian.org/doc/packaging-manuals/copyright-format/1.0/",
        "Upstream-Name: wpkg",
        "Upstream-Contact: Alexis Wilke <alexis@m2osw.com>",
        " http://windowspackager.org/contact",
        "Source: http://windowspackager.org/",
        "License: GPL2",
        "Disclaimer: This package is part of the wpkg unit test environment.",
        "Copyright:",
        " Copyright (c) 2013-2015 Made to Order Software Corporation",
    ">file wpkg/changelog",
        "one (1.5.2) unstable; urgency=low",
        "",
        "  * Very first version, really...",
        "",
        " -- Alexis Wilke <alexis@m2osw.com>  Tue, 02 Jul 2013 02:21:09 -0800",
    ">file wpkg/one.pc",
        "prefix=${instdir}${install_prefix}",
        "exec_prefix=${prefix}",
        "includedir=${prefix}/include",
        "libdir=${exec_prefix}/lib",
        "",
        "Name: ${name}",
        "Cflags: -I${includedir}/wpkg -I${includedir}",
        "Libs: -L${libdir} -ldebpackages",
        "Version: ${version}",
        "Description: ${description}",
        "URL: ${homepage}",
    ">file README",
        "Detailed info about package ONE.",
    ">file INSTALL.TXT",
        "Installation instructions: wpkg --install one.deb",
    ">file COPYING",
        "GPL2 License",

    // PROJECT TWO
    ">project two",
    ">version 0.5.9",
    ">component main/tools",
    ">file CMakeLists.txt",
        "set(TWO_VERSION_MAJOR 0)",
        "set(TWO_VERSION_MINOR 5)",
        "set(TWO_VERSION_PATCH 9)",
        "cmake_minimum_required(VERSION 2.8.4)",
        "project(two)",
        "add_executable(${PROJECT_NAME} b.cpp)",
        "set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_FLAGS \"-I$ENV{WPKG_INSTDIR}/include\" LINK_FLAGS \"-L$ENV{WPKG_INSTDIR}/lib\")",
        "target_link_libraries(${PROJECT_NAME} extension_one)",
        "add_custom_target(run_unit_tests",
        "COMMAND set",
        "COMMAND ./two",
        "COMMAND one", // we can also run "one" because we depend on it
        "WORKING_DIRECTORY ${CMAKE_BINARY_DIR}",
        "DEPENDS ${PROJECT_NAME})",
        "install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin COMPONENT runtime)",
        "set(CPACK_PACKAGE_NAME \"two\")",
        "set(CPACK_PACKAGE_DESCRIPTION_SUMMARY \"Test Two\")",
        "set(CPACK_PACKAGE_VENDOR \"Made to Order Software Corporation\")",
        "set(CPACK_PACKAGE_CONTACT \"contact@m2osw.com\")",
        "set(CPACK_RESOURCE_FILE_LICENSE \"${two_SOURCE_DIR}/COPYING\")",
        "set(CPACK_SOURCE_GENERATOR \"TGZ\")",
        "set(CPACK_PACKAGE_VERSION \"${TWO_VERSION_MAJOR}.${TWO_VERSION_MINOR}.${TWO_VERSION_PATCH}\")",
        "set(CPACK_PACKAGE_VERSION_MAJOR \"${TWO_VERSION_MAJOR}\")",
        "set(CPACK_PACKAGE_VERSION_MINOR \"${TWO_VERSION_MINOR}\")",
        "set(CPACK_PACKAGE_VERSION_PATCH \"${TWO_VERSION_PATCH}\")",
        "set(CPACK_SOURCE_PACKAGE_FILE_NAME \"two_${TWO_VERSION_MAJOR}.${TWO_VERSION_MINOR}.${TWO_VERSION_PATCH}\")",
        "set(CPACK_COMPONENTS_ALL runtime)",
        "set(CPACK_DEB_COMPONENT_INSTALL ON)",
        "include(CPack)",
    ">file b.cpp",
        "#include <stdio.h>",
        "#include <stdlib.h>",
        "#include <extension_one.h>",
        "int main()",
        "{printf(\"b.cpp ran: %d\\n\", ext_special());exit(0);}",
    ">file wpkg/build_number",
    ">file wpkg/control.info",
        "Sub-Packages: runtime*",
        "Package/runtime: two",
        "Build-Depends: one-dev",
        "Architecture: $(architecture())",
        "Homepage: http://windowspackager.org/",
        "Description: Project two",
        "Depends: one",
        "Component: main/tools",
    ">file wpkg/copyright",
        "Format: http://www.debian.org/doc/packaging-manuals/copyright-format/1.0/",
        "Upstream-Name: wpkg",
        "Upstream-Contact: Alexis Wilke <alexis@m2osw.com>",
        " http://windowspackager.org/contact",
        "Source: http://windowspackager.org/",
        "License: GPL2",
        "Disclaimer: This package is part of the wpkg unit test environment.",
        "Copyright:",
        " Copyright (c) 2013-2015 Made to Order Software Corporation",
    ">file wpkg/changelog",
        "two (0.5.9) unstable; urgency=low",
        "",
        "  * First version of project two, really...",
        "",
        " -- Alexis Wilke <alexis@m2osw.com>  Tue, 02 Jul 2013 02:21:09 -0800",
    ">file README.txt",
        "Detailed info about package TWO.",
    ">file INSTALL",
        "Installation instructions: wpkg --install two.deb",
    ">file COPYING",
        "GPL2 License",

    // PROJECT THREE
    ">project three",
    ">version 2.1.7",
    ">component optional/gui", // verify that sub-section gets lost
    ">file CMakeLists.txt",
        "set(THREE_VERSION_MAJOR 2)",
        "set(THREE_VERSION_MINOR 1)",
        "set(THREE_VERSION_PATCH 7)",
        "cmake_minimum_required(VERSION 2.8.4)",
        "project(three)",
        "add_executable(${PROJECT_NAME} c.cpp)",
        "add_custom_target(run_unit_tests",
        "COMMAND ./three",
        "WORKING_DIRECTORY ${CMAKE_BINARY_DIR}",
        "DEPENDS ${PROJECT_NAME})",
        "install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin COMPONENT runtime)",
        "set(CPACK_PACKAGE_NAME \"three\")",
        "set(CPACK_PACKAGE_DESCRIPTION_SUMMARY \"Test Three\")",
        "set(CPACK_PACKAGE_VENDOR \"Made to Order Software Corporation\")",
        "set(CPACK_PACKAGE_CONTACT \"contact@m2osw.com\")",
        "set(CPACK_RESOURCE_FILE_LICENSE \"${three_SOURCE_DIR}/COPYING\")",
        "set(CPACK_SOURCE_GENERATOR \"TGZ\")",
        "set(CPACK_PACKAGE_VERSION \"${THREE_VERSION_MAJOR}.${THREE_VERSION_MINOR}.${THREE_VERSION_PATCH}\")",
        "set(CPACK_PACKAGE_VERSION_MAJOR \"${THREE_VERSION_MAJOR}\")",
        "set(CPACK_PACKAGE_VERSION_MINOR \"${THREE_VERSION_MINOR}\")",
        "set(CPACK_PACKAGE_VERSION_PATCH \"${THREE_VERSION_PATCH}\")",
        "set(CPACK_SOURCE_PACKAGE_FILE_NAME \"three_${THREE_VERSION_MAJOR}.${THREE_VERSION_MINOR}.${THREE_VERSION_PATCH}\")",
        "set(CPACK_COMPONENTS_ALL runtime)",
        "set(CPACK_DEB_COMPONENT_INSTALL ON)",
        "include(CPack)",
    ">file c.cpp",
        "#include <stdio.h>",
        "#include <stdlib.h>",
        "int main()",
        "{printf(\"c.cpp ran\\n\");exit(0);}",
    ">file wpkg/build_number",
        "5",
    ">file wpkg/control.info",
        "Sub-Packages: runtime*",
        "Package/runtime: three",
        "Architecture: $(architecture())",
        "Homepage: http://windowspackager.org/",
        "Description: Project three",
        "Depends: two (>= 0.4.3-2)",
        "Component: optional/gui/image-editor",
    ">file wpkg/copyright",
        "Format: http://www.debian.org/doc/packaging-manuals/copyright-format/1.0/",
        "Upstream-Name: wpkg",
        "Upstream-Contact: Alexis Wilke <alexis@m2osw.com>",
        " http://windowspackager.org/contact",
        "Source: http://windowspackager.org/",
        "License: GPL2",
        "Disclaimer: This package is part of the wpkg unit test environment.",
        "Copyright:",
        " Copyright (c) 2013-2015 Made to Order Software Corporation",
    ">file wpkg/changelog",
        "three (2.1.7) unstable; urgency=low",
        "",
        "  * First version of project three, really...",
        "",
        " -- Alexis Wilke <alexis@m2osw.com>  Tue, 02 Jul 2013 02:21:09 -0800",
    ">file README.txt",
        "Detailed info about package THREE.",
    ">file INSTALL.txt",
        "Installation instructions: wpkg --install three.deb",
    ">file COPYING",
        "GPL2 License",

    NULL
};


}
// namespace


class SystemUnitTests
{
    public:
        SystemUnitTests();

        void manual_builds();
        void automated_builds();

    private:
        void create_projects(project_list_t& list);
        void create_target();
};

SystemUnitTests::SystemUnitTests()
{
    // make sure that the temporary directory is not empty, may be relative
    if(integrationtest::tmp_dir.empty())
    {
        fprintf(stderr, "\nerror:integrationtest_system: a temporary directory is required to run the system unit tests.\n");
        throw std::runtime_error("--tmp <directory> missing");
    }

    // path to the wpkg tool must not be empty either, may be relative
    if(integrationtest::wpkg_tool.empty())
    {
        fprintf(stderr, "\nerror:integrationtest_system: the path to the wpkg tool is required; we do not use chdir() so a relative path will do.\n");
        throw std::runtime_error("--wpkg <path-to-wpkg> missing");
    }

    // delete everything before running ANY ONE TEST
    // (i.e. the setUp() function is called before each and every test)
    wpkg_filename::uri_filename root(integrationtest::tmp_dir);
    try
    {
        root.os_unlink_rf();
    }
    catch(const wpkg_filename::wpkg_filename_exception_io&)
    {
#ifdef MO_WINDOWS
        // at times MS-Windows needs a little pause...
        fprintf(stderr, "\n+++ Pause Between Package Tests +++\n");
        Sleep(200);
        root.os_unlink_rf();
#else
        // otherwise just rethrow
        throw;
#endif
    }

    printf("\n");
    fflush(stdout);
}


void SystemUnitTests::create_projects(project_list_t& list)
{
    wpkg_filename::uri_filename root(integrationtest::tmp_dir);

    wpkg_filename::uri_filename project;
    wpkg_filename::uri_filename filename;
    memfile::memory_file file;
    bool has_file(false);
    bool has_project(false);
    bool new_project(false);
    project_info project_details;
    for(const char * const *p = g_projects; *p != NULL; ++p)
    {
        const char *line(*p);
        if(*line == '>')
        {
            if(strncmp(line, ">project ", 9) == 0)
            {
                project = root.append_child("projects").append_child(line + 9);
                project_details.f_name = line + 9;
                new_project = true;
                has_project = true;
            }
            else if(strncmp(line, ">version ", 9) == 0)
            {
                project_details.f_version = line + 9;
            }
            else if(strncmp(line, ">component ", 11) == 0)
            {
                project_details.f_component = line + 11;
            }
            else if(strncmp(line, ">file ", 6) == 0)
            {
                if(!has_project)
                {
                    throw std::logic_error("you cannot define a file before defining a project");
                }
                if(new_project)
                {
                    new_project = false;
                    list.push_back(project_details);
                }
                if(has_file)
                {
                    file.write_file(filename, true);
                }
                filename = project.append_child(line + 6);
                file.create(memfile::memory_file::file_format_other);
                has_file = true;
            }
        }
        else
        {
            if(!has_file)
            {
                throw std::logic_error("you cannot define content before you defined a file");
            }
            file.printf("%s\n", line);
        }
    }
    if(has_file)
    {
        file.write_file(filename, true);
    }
}


void SystemUnitTests::create_target()
{
    wpkg_filename::uri_filename root(integrationtest::tmp_dir);
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    wpkg_filename::uri_filename target_path(root.append_child("target"));
    target_path.os_mkdir_p();

    // create a core.ctrl file
    wpkg_filename::uri_filename core_ctrl_filename(repository.append_child("core.ctrl"));
    memfile::memory_file core_ctrl;
    core_ctrl.create(memfile::memory_file::file_format_other);
    core_ctrl.printf("Architecture: %s\n", debian_packages_architecture());
    core_ctrl.printf("Maintainer: Alexis Wilke <alexis@m2osw.com>\n");
    core_ctrl.write_file(core_ctrl_filename, true);

    // install the core.ctrl file in the target system
    const std::string core_cmd(integrationtest::wpkg_tool + " --root " + target_path.path_only() + " --create-admindir " + core_ctrl_filename.path_only());
    printf("Run --create-admindir command: \"%s\"\n", core_cmd.c_str());
    fflush(stdout);
    CATCH_REQUIRE(system(core_cmd.c_str()) == 0);
}


void SystemUnitTests::manual_builds()
{
    wpkg_filename::uri_filename root(integrationtest::tmp_dir);
    root.os_mkdir_p();
    root = root.os_real_path();

    wpkg_filename::uri_filename wpkg(integrationtest::wpkg_tool);
    wpkg = wpkg.os_real_path();

    wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    create_target();

    // IMPORTANT: remember that all files are deleted between tests

    project_list_t project_list;
    create_projects(project_list);

    for( auto project : project_list )
    {
        // to build projects, we need to be inside the project directory
        std::string cd_cmd("cd ");
        cd_cmd += root.append_child("projects").append_child(project.f_name).full_path();
        cd_cmd += " && ";

        // build source package
        {
            std::string cmd = cd_cmd + wpkg.full_path();
            cmd += " --build --output-repository-dir ";
            cmd += repository.full_path();
            cmd += " --create-index index.tar.gz";
            cmd += " --repository ";
            cmd += repository.full_path();
            cmd += " -D 0100";
            std::cout << "***" << std::endl
                      << "*** Build source package command: " << cmd.c_str() << std::endl
                      << "***" << std::endl;
            fflush(stdout);
            int r(system(cmd.c_str()));
            std::cout << " Build command returned " << WEXITSTATUS(r) << " (expected 0)" << std::endl;
            CATCH_REQUIRE(WEXITSTATUS(r) == 0);
        }

        // build binary package
        {
            wpkg_filename::uri_filename source_path(repository);
            source_path = source_path.append_child("sources").append_child(project.f_component).append_child(project.f_name);
            std::string cmd = cd_cmd + wpkg.full_path();
            cmd += " --root ";
            cmd += target_path.full_path();
            cmd += " --build ";
            cmd += source_path.full_path();
            cmd += "-src_";
            cmd += project.f_version;
            cmd += ".deb";
            cmd += " --output-repository-dir ";
            cmd += repository.full_path();
            cmd += " -D 0100 --force-file-info";
            cmd += " --repository ";
            cmd += repository.full_path();
            cmd += " --run-unit-tests";
            std::cout << "***" << std::endl
                      << "*** Build binary package command: " << cmd.c_str() << std::endl
                      << "***" << std::endl;
            fflush(stdout);
            int r(system(cmd.c_str()));
            std::cout << " Build command returned " << WEXITSTATUS(r) << " (expected 0)" << std::endl;
            CATCH_REQUIRE(WEXITSTATUS(r) == 0);
        }
    }
}



void SystemUnitTests::automated_builds()
{
    wpkg_filename::uri_filename root(integrationtest::tmp_dir);
    root.os_mkdir_p();
    root = root.os_real_path();

    wpkg_filename::uri_filename wpkg(integrationtest::wpkg_tool);
    wpkg = wpkg.os_real_path();

    wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    create_target();

    // IMPORTANT: remember that all files are deleted between tests

    project_list_t project_list;
    create_projects(project_list);

    for( auto project : project_list )
    {
        // to build projects, we need to be inside the project directory
        std::string cd_cmd("cd ");
        cd_cmd += root.append_child("projects").append_child(project.f_name).full_path();
        cd_cmd += " && ";

        // build source package
        {
            std::string cmd = cd_cmd + wpkg.full_path();
            cmd += " --build --output-repository-dir ";
            cmd += repository.full_path();
            cmd += " --create-index index.tar.gz";
            cmd += " --repository ";
            cmd += repository.full_path();
            cmd += " -D 0100";
            printf("***\n*** Build source package command: %s\n***\n", cmd.c_str());
            fflush(stdout);
            int r(system(cmd.c_str()));
            printf(" Build command returned %d (expected 0)\n", WEXITSTATUS(r));
            CATCH_REQUIRE(WEXITSTATUS(r) == 0);
        }
    }

    // build all binary packages at once
    {
        std::string cmd = wpkg.full_path();
        cmd += " --root ";
        cmd += target_path.full_path();
        cmd += " --build ";
        cmd += repository.full_path();
        cmd += " --output-repository-dir ";
        cmd += repository.full_path();
        cmd += " -D 0100 --force-file-info";
        cmd += " --repository ";
        cmd += repository.full_path();
        cmd += " --run-unit-tests";
        printf("***\n*** Build binary package command: %s\n***\n", cmd.c_str());
        fflush(stdout);
        int r(system(cmd.c_str()));
        printf(" Build command returned %d (expected 0)\n", WEXITSTATUS(r));
        CATCH_REQUIRE(WEXITSTATUS(r) == 0);
    }
}


CATCH_TEST_CASE("SystemUnitTests::manual_builds","SystemUnitTests")
{
    SystemUnitTests sut;
    sut.manual_builds();
}


CATCH_TEST_CASE("SystemUnitTests::automated_builds","SystemUnitTests")
{
    SystemUnitTests sut;
    sut.automated_builds();
}



// vim: ts=4 sw=4 et
