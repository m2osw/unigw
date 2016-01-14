/*    integrationtest_package.cpp
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


#include "wpkg_tools.h"

#include "libdebpackages/debian_packages.h"
#include "libdebpackages/wpkg_util.h"

#include <catch.hpp>

#include <iostream>
#include <string.h>


namespace test_common
{


std::string wpkg_tools::f_tmp_dir;
std::string wpkg_tools::f_wpkg_tool;


obj_setenv::obj_setenv(const std::string& var)
    : f_copy(strdup(var.c_str()))
{
    putenv(f_copy);
    std::string::size_type p(var.find_first_of('='));
    f_name = var.substr(0, p);
}


obj_setenv::~obj_setenv()
{
    putenv(strdup((f_name + "=").c_str()));
    free(f_copy);
}


wpkg_tools::wpkg_tools()
{
    // make sure that the temporary directory is not empty, may be relative
    if(f_tmp_dir.empty())
    {
        fprintf(stderr, "\nerror:integrationtest_package: a temporary directory is required to run the package unit tests.\n");
        throw std::runtime_error("--tmp <directory> missing");
    }

    // path to the wpkg tool must not be empty either, may be relative
    if(f_wpkg_tool.empty())
    {
        fprintf(stderr, "\nerror:integrationtest_package: the path to the wpkg tool is required; we do not use chdir() so a relative path will do.\n");
        throw std::runtime_error("--wpkg <path-to-wpkg> missing");
    }

    wpkg_filename::uri_filename config1("/etc/wpkg/wpkg.conf");
    wpkg_filename::uri_filename config2("~/.config/wpkg/wpkg.conf");
    const char *env(getenv("WPKG_OPTIONS"));
    if(config1.exists() || config2.exists() || (env != nullptr && *env != '\0'))
    {
        fprintf(stderr, "\nerror:integrationtest_package: at least one of the wpkg.conf files or the WPKG_OPTIONS variable exist and could undermine this test. Please delete or rename configuration files (/etc/wpkg/wpkg.conf or ~/.config/wpkg/wpkg.conf) and unset  the WPKG_OPTIONS environment variable.\n");
        throw std::runtime_error("/etc/wpkg/wpkg.conf, ~/.config/wpkg/wpkg.conf, and WPKG_OPTIONS exist");
    }

    // delete everything before running ANY ONE TEST
    // (i.e. the setUp() function is called before each and every test)
    wpkg_filename::uri_filename root(f_tmp_dir);
    try
    {
        root.os_unlink_rf();
    }
    catch(const wpkg_filename::wpkg_filename_exception_io&)
    {
#ifdef MO_WINDOWS
        // at times MS-Windows needs a little pause...
        fprintf(stderr, "\n+++ Pause Between Package Tests +++ (%s)\n", root.os_filename().get_utf8().c_str());
        fflush(stderr);
        Sleep(200);
        root.os_unlink_rf();
#else
        // otherwise just rethrow
        throw;
#endif
    }

    std::cout << std::endl;
}


wpkg_tools::~wpkg_tools()
{
}


std::string wpkg_tools::escape_string( const std::string& orig_field )
{
#ifdef MO_WINDOWS
    std::string field;
    for( auto ch : orig_field )
    {
        switch( ch )
        {
            case '|':
            case '"':
            case '&':
                field.push_back( '^' );
                field.push_back( ch );
                break;

            default:
                field.push_back( ch );
        }
    }
    return field;
#else
    // There is nothing to "auto-escape" for now. Windows just needs
    // to translate stuff, but Linux has a saner method, IMHO.
    return orig_field;
#endif
}


/** \brief create a standard control file
 *
 * This function allocates a control file and creates 4 of the 5
 * mandatory fields. It does not create the Package field because
 * that's set when you want to create the package.
 *
 * \param[in] test_name  The name of the test, to recognize the package as coming from that specific test.
 */
wpkg_tools::control_file_pointer_t wpkg_tools::get_new_control_file(const std::string& test_name)
{
    std::shared_ptr<wpkg_control::control_file> ctrl(new wpkg_control::binary_control_file(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::build_control_file_state_t)));

    //ctrl->set_field("Package", ...); -- this is set by the create_package() call
    ctrl->set_field("Description", "Test " + test_name);
    ctrl->set_field("Architecture", debian_packages_architecture());
    ctrl->set_field("Maintainer", "Alexis Wilke <alexis@m2osw.com>");
    ctrl->set_field("Version", "1.0");

    return ctrl;
}


int wpkg_tools::execute_cmd( const std::string& cmd )
{
#ifdef MO_WINDOWS
    if( cmd.size() > 8191 )
    {
        std::cerr << "Error in command line '" << cmd << "'!" << std::endl;
        std::cerr << "Command string is greater than 8K, and will fail under MS Windows." << std::endl;
        CATCH_REQUIRE(false);
    }
#endif
    return system( cmd.c_str( ));
}


/** \brief Create a randomized file.
 *
 * To fill packages with actual files, we create them with random data
 * so they look real enough. These can then be used to check that the
 * --install, --unpack commands indeed install the files as expected.
 * We can also test that they do get removed too.
 *
 * The function makes use of the size as specified in the \p files
 * parameter list. If the size is zero ("undefined") then a random
 * size is choosen between 0 and 0x3FFFF (262143 bytes).
 *
 * Note that path is the directory name of the package, not the
 * exact path where the file is saved. This is because the \p files
 * filename may include a path too (i.e. /usr/share/doc/t1/copyright).
 *
 * \param[in] files  The list of files that we want created.
 * \param[in] idx  The index of the file to create on this call.
 * \param[in] path  The output directory where the file is to be saved.
 */
void wpkg_tools::create_file(wpkg_control::file_list_t& files, wpkg_control::file_list_t::size_type idx, wpkg_filename::uri_filename path)
{
    std::string filename(files[idx].get_filename());
    size_t size(files[idx].get_size());
    if(size == 0)
    {
        size = rand() & 0x3FFFF;
        files[idx].set_size(size);
    }
    memfile::memory_file file;
    file.create(memfile::memory_file::file_format_other);
    for(size_t i(0); i < size; ++i)
    {
        char c(rand() & 255);
        file.write(&c, static_cast<int>(i), static_cast<int>(sizeof(c)));
    }
    file.write_file(path.append_child(filename), true);

    files[idx].set_checksum(file.md5sum());
}


/** \brief Create (i.e. --build) a package.
 *
 * This function creates a package environment, randomized files, and
 * then build a package with the wpkg command line tool.
 *
 * The control file passed down will always have its Package field set
 * to the specified \p name parameter. It is also expected to have a
 * Files field, it is used to create all the files added to that package.
 * It also makes use of a few variables to add command line options to
 * the command:
 *
 * \li BUILD_PREOPTIONS  -- command line options added before the --build
 * \li BUILD_POSTOPTIONS -- command line options added after the --build
 * \li BUILD_RESULT      -- expected result [optional]
 *
 * \param[in] name  The name of the of the package.
 * \param[in] ctrl  The control file for that package.
 * \param[in] reset_wpkg_dir  Whether the directory should be reset first.
 */
void wpkg_tools::create_package(const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, bool reset_wpkg_dir )
{
    wpkg_filename::uri_filename root(f_tmp_dir);
    wpkg_filename::uri_filename build_path(root.append_child(name));
    wpkg_filename::uri_filename wpkg_path(build_path.append_child("WPKG"));

    // clean up the directory
    if(reset_wpkg_dir)
    {
        build_path.os_unlink_rf();
    }

    ctrl->set_field("Package", name);

    // handle the files before saving the control file so we can fix the md5sum
    wpkg_control::file_list_t files(ctrl->get_files("Files"));
    wpkg_control::file_list_t::size_type max(files.size());
    for(wpkg_control::file_list_t::size_type i(0); i < max; ++i)
    {
        create_file(files, i, build_path);
    }
    ctrl->set_field("Files", files.to_string());

    if(ctrl->field_is_defined("Conffiles"))
    {
        wpkg_control::file_list_t conffiles(ctrl->get_files("Conffiles"));
        memfile::memory_file conffiles_output;
        conffiles_output.create(memfile::memory_file::file_format_other);
        conffiles_output.printf("%s\n", conffiles.to_string(wpkg_control::file_item::format_list, false).c_str());
        wpkg_filename::uri_filename conffiles_filename(wpkg_path.append_child("conffiles"));
        conffiles_output.write_file(conffiles_filename, true);
        ctrl->delete_field("Conffiles");
    }

    memfile::memory_file ctrl_output;
    ctrl->write(ctrl_output, wpkg_field::field_file::WRITE_MODE_FIELD_ONLY);
    ctrl_output.write_file(wpkg_path.append_child("control"), true);

    wpkg_filename::uri_filename repository(root.append_child("repository"));
    repository.os_mkdir_p();

    std::string cmd(f_wpkg_tool);
    if(ctrl->variable_is_defined("BUILD_PREOPTIONS"))
    {
        cmd += " " + ctrl->get_variable("BUILD_PREOPTIONS");
    }
    cmd += " --output-dir " + wpkg_util::make_safe_console_string(repository.path_only());
    cmd += " --build " + wpkg_util::make_safe_console_string(build_path.path_only());
    if(ctrl->variable_is_defined("BUILD_POSTOPTIONS"))
    {
        cmd += " " + ctrl->get_variable("BUILD_POSTOPTIONS");
    }
    printf("Build Command: \"%s\"\n", cmd.c_str());
    fflush(stdout);

    if(ctrl->variable_is_defined("BUILD_RESULT"))
    {
        const int r(execute_cmd(cmd));
        const std::string expected_result(ctrl->get_variable("BUILD_RESULT"));
        const int expected_return_value(strtol(expected_result.c_str(), 0, 0));
        printf("  Build result = %d (expected %d)\n", WEXITSTATUS(r), expected_return_value);
        CATCH_REQUIRE(WEXITSTATUS(r) == expected_return_value);
    }
    else
    {
        CATCH_REQUIRE(execute_cmd(cmd.c_str()) == 0);
    }
}


namespace
{
    std::string int_to_string( const int val )
    {
        std::stringstream ss;
        ss << val;
        return ss.str();
    }
}


void wpkg_tools::create_package( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, int expected_return_value, bool reset_wpkg_dir )
{
    ctrl->set_variable( "BUILD_RESULT", int_to_string(expected_return_value) );
    create_package( name, ctrl, reset_wpkg_dir );
}


/** \brief Install a package that you previously created.
 *
 * This function runs wpkg --install to install a .deb file as
 * generated by the create_package() function. The .deb is
 * expected to be in the repository and have a version and
 * architecture specification (TODO: check if architecture
 * is "src"/"source" and do not add it in that case.)
 *
 * We take the control file as a parameter so we can make use
 * of some variables:
 *
 * \li INSTALL_PREOPTIONS  -- command line options added before the --install
 * \li INSTALL_POSTOPTIONS -- command line options added after the --install
 * \li INSTALL_RESULT      -- expected result [optional]
 *
 * \param[in] name  The name of the package as passed to create_package().
 * \param[in] ctrl  The control file as passed to create_package().
 */
void wpkg_tools::install_package( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl )
{
    wpkg_filename::uri_filename root(f_tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    if(!target_path.is_dir() || !target_path.append_child("var/lib/wpkg/core").exists())
    {
        target_path.os_mkdir_p();
        wpkg_filename::uri_filename core_ctrl_filename(repository.append_child("core.ctrl"));
        memfile::memory_file core_ctrl;
        core_ctrl.create(memfile::memory_file::file_format_other);
        if(ctrl->variable_is_defined("INSTALL_ARCHITECTURE"))
        {
            core_ctrl.printf("Architecture: %s\n", ctrl->get_variable("INSTALL_ARCHITECTURE").c_str());
        }
        else
        {
            core_ctrl.printf("Architecture: %s\n", debian_packages_architecture());
        }
        core_ctrl.printf("Maintainer: Alexis Wilke <alexis@m2osw.com>\n");
        if(ctrl->variable_is_defined("INSTALL_EXTRACOREFIELDS"))
        {
            core_ctrl.printf("%s", ctrl->get_variable("INSTALL_EXTRACOREFIELDS").c_str());
        }
        core_ctrl.write_file(core_ctrl_filename);
        std::string core_cmd(f_wpkg_tool
                + " --root " + wpkg_util::make_safe_console_string(target_path.path_only())
                + " --create-admindir " + wpkg_util::make_safe_console_string(core_ctrl_filename.path_only()));
        printf("Create AdminDir Command: \"%s\"\n", core_cmd.c_str());
        fflush(stdout);
        CATCH_REQUIRE(execute_cmd(core_cmd.c_str()) == 0);
    }
    else
    {
        // In case we are running after creation of root and repository, update the index
        //
        wpkg_filename::uri_filename index_file(repository.append_child("index.tar.gz"));
        std::string cmd;
        cmd = f_wpkg_tool;
        cmd += " --create-index " + wpkg_util::make_safe_console_string(index_file.path_only());
        cmd += " --repository "   + wpkg_util::make_safe_console_string(repository.path_only());
        std::cout << "Build index command: \"" << cmd << "\"" << std::endl << std::flush;
        const int r(execute_cmd(cmd.c_str()));
        std::cout << "  Build index result = " << WEXITSTATUS(r) << std::endl << std::flush;
        CATCH_REQUIRE(WEXITSTATUS(r) == 0);
    }


    // The command string (empty to start)
    //
    std::string cmd;

    if(ctrl->field_is_defined("WPKG_SUBST"))
    {
        const std::string field( escape_string( ctrl->get_field("WPKG_SUBST") ) );
#ifdef MO_WINDOWS
        cmd = "set WPKG_SUBST=" + field + " && ";
#else
        cmd = "WPKG_SUBST='" + field + "' ";
#endif
    }

    if(ctrl->field_is_defined("PRE_COMMAND"))
    {
        cmd += ctrl->get_field("PRE_COMMAND") + " ";
    }
    cmd += f_wpkg_tool;
    if(ctrl->variable_is_defined("INSTALL_PREOPTIONS"))
    {
        cmd += " " + ctrl->get_variable("INSTALL_PREOPTIONS");
    }
    if(!ctrl->variable_is_defined("INSTALL_NOROOT"))
    {
        cmd += " --root " + wpkg_util::make_safe_console_string(target_path.path_only());
    }
    cmd += " --install " + wpkg_util::make_safe_console_string(repository.append_child("/" + name + "_" + ctrl->get_field("Version") + "_" + ctrl->get_field("Architecture") + ".deb").path_only());
    if(ctrl->variable_is_defined("INSTALL_POSTOPTIONS"))
    {
        cmd += " " + ctrl->get_variable("INSTALL_POSTOPTIONS");
    }
    printf("Install Command: \"%s\"\n", cmd.c_str());
    fflush(stdout);

    if(ctrl->variable_is_defined("INSTALL_RESULT"))
    {
        const int r(execute_cmd(cmd.c_str()));
        const std::string expected_result(ctrl->get_variable("INSTALL_RESULT"));
        const int expected_return_value(strtol(expected_result.c_str(), 0, 0));
        printf("  Install result = %d (expected %d)\n", WEXITSTATUS(r), expected_return_value);
        CATCH_REQUIRE(WEXITSTATUS(r) == expected_return_value);
    }
    else
    {
        CATCH_REQUIRE(execute_cmd(cmd.c_str()) == 0);
    }
}


void wpkg_tools::install_package( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, int expected_return_value )
{
    ctrl->set_variable( "INSTALL_RESULT", int_to_string(expected_return_value) );
    install_package( name, ctrl );
}


/** \brief Remove a package as the --remove command does.
 *
 * This function runs the wpkg --remove command with the specified package
 * name. The result should be the module removed from the target.
 *
 * \li REMOVE_RESULT      -- expected result [optional]
 *
 * \param[in] name  The name of the package to remove.
 * \param[in] ctrl  The control file fields.
 */
void wpkg_tools::remove_package( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl )
{
    wpkg_filename::uri_filename root(f_tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));

    std::string cmd(f_wpkg_tool);
    if(ctrl->variable_is_defined("REMOVE_PREOPTIONS"))
    {
        cmd += " " + ctrl->get_variable("REMOVE_PREOPTIONS");
    }
    if(!ctrl->variable_is_defined("REMOVE_NOROOT"))
    {
        cmd += " --root " + wpkg_util::make_safe_console_string(target_path.path_only());
    }
    cmd += " --remove " + name;
    if(ctrl->variable_is_defined("REMOVE_POSTOPTIONS"))
    {
        cmd += " " + ctrl->get_variable("REMOVE_POSTOPTIONS");
    }
    printf("Remove Command: \"%s\"\n", cmd.c_str());
    fflush(stdout);

    if(ctrl->variable_is_defined("REMOVE_RESULT"))
    {
        const int r(execute_cmd(cmd.c_str()));
        const std::string expected_result(ctrl->get_variable("REMOVE_RESULT"));
        const int expected_return_value(strtol(expected_result.c_str(), 0, 0));
        printf("  Remove result = %d (expected %d)\n", WEXITSTATUS(r), expected_return_value);
        CATCH_REQUIRE(WEXITSTATUS(r) == expected_return_value);
    }
    else
    {
        CATCH_REQUIRE(execute_cmd(cmd.c_str()) == 0);
    }
}


void wpkg_tools::remove_package( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, int expected_return_value )
{
    ctrl->set_variable( "REMOVE_RESULT", int_to_string(expected_return_value) );
    remove_package( name, ctrl );
}


/** \brief Purge a package as the --purge command does.
 *
 * This function runs the wpkg --purge command with the specified package
 * name. The result should be the module completely from the target (i.e.
 * all the files including the configuration files.)
 *
 * \li PURGE_RESULT      -- expected result [optional]
 *
 * \param[in] name  The name of the package to remove.
 * \param[in] ctrl  The control file fields.
 */
void wpkg_tools::purge_package( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl )
{
    wpkg_filename::uri_filename root(f_tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));

    std::string cmd(f_wpkg_tool);
    if(ctrl->variable_is_defined("PURGE_PREOPTIONS"))
    {
        cmd += " " + ctrl->get_variable("PURGE_PREOPTIONS");
    }
    if(!ctrl->variable_is_defined("PURGE_NOROOT"))
    {
        cmd += " --root " + wpkg_util::make_safe_console_string(target_path.path_only());
    }
    cmd += " --purge " + name;
    if(ctrl->variable_is_defined("PURGE_POSTOPTIONS"))
    {
        cmd += " " + ctrl->get_variable("PURGE_POSTOPTIONS");
    }
    printf("Purge Command: \"%s\"\n", cmd.c_str());
    fflush(stdout);

    if(ctrl->variable_is_defined("PURGE_RESULT"))
    {
        const int r(execute_cmd(cmd.c_str()));
        const std::string expected_result(ctrl->get_variable("PURGE_RESULT"));
        const int expected_return_value(strtol(expected_result.c_str(), 0, 0));
        printf("  Purge result = %d (expected %d)\n", WEXITSTATUS(r), expected_return_value);
        CATCH_REQUIRE(WEXITSTATUS(r) == expected_return_value);
    }
    else
    {
        CATCH_REQUIRE(execute_cmd(cmd.c_str()) == 0);
    }
}


void wpkg_tools::purge_package( const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, int expected_return_value )
{
    ctrl->set_variable( "PURGE_RESULT", int_to_string(expected_return_value) );
    purge_package( name, ctrl );
}


}
// namespace test_common

// vim: ts=4 sw=4 et
