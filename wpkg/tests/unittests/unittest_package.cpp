/*    unittest_package.cpp
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

#include "unittest_package.h"
#include "unittest_main.h"
#include "libdebpackages/debian_packages.h"
#include "libdebpackages/wpkg_control.h"
#include "libdebpackages/wpkg_architecture.h"
#include "libdebpackages/wpkg_util.h"
#include <stdexcept>
#include <cstring>

// for the WEXITSTATUS()
#ifdef __GNUC__
#	pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

CPPUNIT_TEST_SUITE_REGISTRATION( PackageUnitTests );


struct raii_tmp_dir_with_space
{
    raii_tmp_dir_with_space()
        : f_backup(unittest::tmp_dir)
    {
        unittest::tmp_dir += "/path with spaces";
    }

    ~raii_tmp_dir_with_space()
    {
        unittest::tmp_dir = f_backup;
    }

private:
    std::string f_backup;
};


void PackageUnitTests::setUp()
{
    // make sure that the temporary directory is not empty, may be relative
    if(unittest::tmp_dir.empty())
    {
        fprintf(stderr, "\nerror:unittest_package: a temporary directory is required to run the package unit tests.\n");
        throw std::runtime_error("--tmp <directory> missing");
    }

    // path to the wpkg tool must not be empty either, may be relative
    if(unittest::wpkg_tool.empty())
    {
        fprintf(stderr, "\nerror:unittest_package: the path to the wpkg tool is required; we do not use chdir() so a relative path will do.\n");
        throw std::runtime_error("--wpkg <path-to-wpkg> missing");
    }

    wpkg_filename::uri_filename config1("/etc/wpkg/wpkg.conf");
    wpkg_filename::uri_filename config2("~/.config/wpkg/wpkg.conf");
    const char *env(getenv("WPKG_OPTIONS"));
    if(config1.exists() || config2.exists() || (env != nullptr && *env != '\0'))
    {
        fprintf(stderr, "\nerror:unittest_package: at least one of the wpkg.conf files or the WPKG_OPTIONS variable exist and could undermine this test. Please delete or rename configuration files (/etc/wpkg/wpkg.conf or ~/.config/wpkg/wpkg.conf) and unset  the WPKG_OPTIONS environment variable.\n");
        throw std::runtime_error("/etc/wpkg/wpkg.conf, ~/.config/wpkg/wpkg.conf, and WPKG_OPTIONS exist");
    }

    // delete everything before running ANY ONE TEST
    // (i.e. the setUp() function is called before each and every test)
    wpkg_filename::uri_filename root(unittest::tmp_dir);
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

    printf("\n");
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//                                                                       //
//     MANY FUNCTIONS USED TO FACILITATE THE DEVELOPMENT OF TESTS        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

namespace
{
typedef std::vector<std::string> string_list;

/** \brief create a standard control file
 *
 * This function allocates a control file and creates 4 of the 5
 * mandatory fields. It does not create the Package field because
 * that's set when you want to create the package.
 *
 * \param[in] test_name  The name of the test, to recognize the package as coming from that specific test.
 */
std::shared_ptr<wpkg_control::control_file> get_new_control_file(const std::string& test_name)
{
    std::shared_ptr<wpkg_control::control_file> ctrl(new wpkg_control::binary_control_file(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::build_control_file_state_t)));

    //ctrl->set_field("Package", ...); -- this is set by the create_package() call
    ctrl->set_field("Description", "Test " + test_name);
    ctrl->set_field("Architecture", debian_packages_architecture());
    ctrl->set_field("Maintainer", "Alexis Wilke <alexis@m2osw.com>");
    ctrl->set_field("Version", "1.0");

    return ctrl;
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
void create_file(wpkg_control::file_list_t& files, wpkg_control::file_list_t::size_type idx, wpkg_filename::uri_filename path)
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
 * \li BUILD_PREOPTIONS -- command line options added before the --build
 * \li BUILD_POSTOPTIONS -- command line options added after the --build
 *
 * \param[in] name  The name of the of the package.
 * \param[in] ctrl  The control file for that package.
 * \param[in] reset_wpkg_dir  Whether the directory should be reset first.
 */
void create_package(const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, bool reset_wpkg_dir = true)
{
    wpkg_filename::uri_filename root(unittest::tmp_dir);
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

    std::string cmd(unittest::wpkg_tool);
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
        const int r(system(cmd.c_str()));
        const std::string expected_result(ctrl->get_variable("BUILD_RESULT"));
        const int expected_return_value(strtol(expected_result.c_str(), 0, 0));
        printf("  Build result = %d (expected %d)\n", WEXITSTATUS(r), expected_return_value);
        CPPUNIT_ASSERT(WEXITSTATUS(r) == expected_return_value);
    }
    else
    {
        CPPUNIT_ASSERT(system(cmd.c_str()) == 0);
    }
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
 * \li INSTALL_PREOPTIONS -- command line options added before the --install
 * \li INSTALL_POSTOPTIONS -- command line options added after the --install
 *
 * \param[in] name  The name of the package as passed to create_package().
 * \param[in] ctrl  The control file as passed to create_package().
 * \param[in] expected_return_value  The value we're expecting the command to return.
 */
void install_package(const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, int expected_return_value = 0)
{
    wpkg_filename::uri_filename root(unittest::tmp_dir);
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
        std::string core_cmd(unittest::wpkg_tool + " --root " + wpkg_util::make_safe_console_string(target_path.path_only())
                                                 + " --create-admindir " + wpkg_util::make_safe_console_string(core_ctrl_filename.path_only()));
        printf("Create AdminDir Command: \"%s\"\n", core_cmd.c_str());
        fflush(stdout);
        CPPUNIT_ASSERT(system(core_cmd.c_str()) == 0);
    }

    std::string cmd;
    if(ctrl->field_is_defined("PRE_COMMAND"))
    {
        cmd += ctrl->get_field("PRE_COMMAND");
        cmd += " && ";
    }
    cmd += unittest::wpkg_tool;
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
    int r(system(cmd.c_str()));
    printf("  Install result = %d (expected %d)\n", WEXITSTATUS(r), expected_return_value);
    CPPUNIT_ASSERT(WEXITSTATUS(r) == expected_return_value);
}

/** \brief Remove a package as the --remove command does.
 *
 * This function runs the wpkg --remove command with the specified package
 * name. The result should be the module removed from the target.
 *
 * \param[in] name  The name of the package to remove.
 * \param[in] ctrl  The control file fields.
 */
void remove_package(const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, int expected_return_value = 0)
{
    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));

    std::string cmd(unittest::wpkg_tool);
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
    int r(system(cmd.c_str()));
    printf("  Remove result = %d (expected %d)\n", WEXITSTATUS(r), expected_return_value);
    CPPUNIT_ASSERT(WEXITSTATUS(r) == expected_return_value);
}

/** \brief Purge a package as the --purge command does.
 *
 * This function runs the wpkg --purge command with the specified package
 * name. The result should be the module completely from the target (i.e.
 * all the files including the configuration files.)
 *
 * \param[in] name  The name of the package to remove.
 * \param[in] ctrl  The control file fields.
 */
void purge_package(const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, int expected_return_value = 0)
{
    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));

    std::string cmd(unittest::wpkg_tool);
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
    int r(system(cmd.c_str()));
    printf("  Purge result = %d (expected %d)\n", WEXITSTATUS(r), expected_return_value);
    CPPUNIT_ASSERT(WEXITSTATUS(r) == expected_return_value);
}

/** \brief Compare files from the build directories with those from the target.
 *
 * This function compares the files that were used to create a .deb
 * against those that were installed from that .deb in a target. It
 * ensures that the files are binary equal to each other (as they
 * should be as we do not process files at all.)
 *
 * This process works as long as the source package directory did
 * not get replaced (i.e. newer version replacing the older version
 * to test an upgrade, etc.)
 *
 * \param[in] name  The name of the package to check.
 */
void verify_installed_files(const std::string& name)
{
    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename build_path(root.append_child(name));
    memfile::memory_file dir;
    // this reads the directory used to build this package, so if you
    // created another version in between, it won't work!
    dir.dir_rewind(build_path);
    for(;;)
    {
        memfile::memory_file::file_info info;
        memfile::memory_file data;
        if(!dir.dir_next(info, &data))
        {
            break;
        }
        if(info.get_file_type() == memfile::memory_file::file_info::regular_file
        && strstr(info.get_filename().c_str(), "/WPKG/") == nullptr)
        {
            wpkg_filename::uri_filename installed_name(info.get_uri());
            installed_name = installed_name.remove_common_segments(build_path);
            installed_name = target_path.append_child(installed_name.path_only());
            memfile::memory_file target_data;
            target_data.read_file(installed_name);
            CPPUNIT_ASSERT_MESSAGE(installed_name.original_filename(), target_data.compare(data) == 0);
        }
    }
}


struct verify_file_t
{
    enum verify_modes
    {
        verify_deleted,
        verify_exists,
        verify_content,
        verify_text
    };
    typedef controlled_vars::limited_auto_init<verify_modes, verify_deleted, verify_text, verify_exists> verify_mode_t;
    void clear()
    {
        f_mode = static_cast<int>(verify_exists); // FIXME cast
        f_filename = "";
        f_data = "";
    }

    verify_mode_t               f_mode;
    std::string                 f_filename;
    std::string                 f_data;
};
typedef std::vector<verify_file_t> verify_file_vector_t;

/** \brief Compare files that scripts were expected to generate/delete.
 *
 * This function checks whether certain files are there or not there depending
 * on what the scripts are expected to do.
 *
 * The function accepts an array of verify_file_t structure. Each entry
 * has a relative filename starting at the root of the installation target.
 * The mode defines how the file will be tested:
 *
 * \li verify_deleted -- the file must not exist
 * \li verify_exists -- the file must exist
 * \li verify_content -- the file must exist and its content match one to one
 * \li verify_text -- the file must exist and its text content must match;
 *                    since this is viewed as text, new lines and carriage
 *                    returns are all checked as \n (so \n, \r\n, and \r
 *                    are all viewed as one \n.)
 *
 * The f_data parameter is a string (verify_text) or a binary buffer
 * (verify_content). In the former case, the string is taken as binary
 * and thus the size is used to determine the end of the content (i.e.
 * the buffer can include '\0'.)
 *
 * \param[in] name  The name of the package to check.
 */
void verify_generated_files(const verify_file_vector_t& files)
{
    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));

    for(verify_file_vector_t::const_iterator it(files.begin()); it != files.end(); ++it)
    {
        wpkg_filename::uri_filename filename(target_path.append_child(it->f_filename));
        switch(it->f_mode)
        {
        case verify_file_t::verify_deleted:
            CPPUNIT_ASSERT_MESSAGE("file is not expected to exist \"" + filename.original_filename() + "\"", !filename.exists());
            break;

        case verify_file_t::verify_exists:
            CPPUNIT_ASSERT_MESSAGE("file is expected to exist \"" + filename.original_filename() + "\"", filename.exists());
            break;

        case verify_file_t::verify_content:
            CPPUNIT_ASSERT_MESSAGE("file is expected to exist \"" + filename.original_filename() + "\"", filename.exists());
            {
                memfile::memory_file disk_data;
                disk_data.read_file(filename);
                CPPUNIT_ASSERT_MESSAGE("file content size does not match \"" + filename.original_filename() + "\"", static_cast<size_t>(disk_data.size()) == it->f_data.size());
                memfile::memory_file test_data;
                test_data.create(memfile::memory_file::file_format_other);
                test_data.write(it->f_data.c_str(), 0, static_cast<int>(it->f_data.size()));
                CPPUNIT_ASSERT_MESSAGE("file content does not match \"" + filename.original_filename() + "\"", disk_data.compare(test_data) == 0);
            }
            break;

        case verify_file_t::verify_text:
            CPPUNIT_ASSERT_MESSAGE("file is expected to exist " + filename.original_filename(), filename.exists());
            {
                memfile::memory_file disk_data;
                disk_data.read_file(filename);
                memfile::memory_file test_data;
                test_data.create(memfile::memory_file::file_format_other);
                test_data.write(it->f_data.c_str(), 0, static_cast<int>(it->f_data.size()));

                std::string disk_line, test_line;
                int disk_offset(0), test_offset(0);
                for(;;)
                {
                    bool disk_result(disk_data.read_line(disk_offset, disk_line));
                    bool test_result(test_data.read_line(test_offset, test_line));
                    CPPUNIT_ASSERT_MESSAGE("file content does not match \"" + filename.original_filename() + "\" (early EOF on one of the files)", disk_result == test_result);
                    if(!disk_result)
                    {
                        break;
                    }
                    // trim left and then right
                    // we assume that the test line is already clean
                    std::string::size_type p(disk_line.find_first_not_of(" \t\n\r\v\f"));
                    if(p != std::string::npos)
                    {
                        disk_line = disk_line.substr(p);
                    }
                    p = disk_line.find_last_not_of(" \t\n\r\v\f");
                    if(p != std::string::npos)
                    {
                        disk_line = disk_line.substr(0, p + 1);
                    }
                    CPPUNIT_ASSERT_MESSAGE("file lines \"" + disk_line + "\" and \"" + test_line + "\" do not match for \"" + filename.original_filename() + "\" (lines are invalid)", disk_line == test_line);
                }
            }
            break;

        }
        switch(it->f_mode)
        {
        case verify_file_t::verify_deleted:
            break;

        default:
            filename.os_unlink();
            break;

        }
    }
}

/** \brief Check that a package was properly removed.
 *
 * The name of the package that got removed.
 *
 * This function skips the package configuration files since a remove does
 * not delete those. It checks all the other files though. The \p ctrl object
 * is used to gather the list of configuration files. Remember that the list
 * of configuration files is removed when we create (--build) the package.
 * So before calling this function you have to redefine the field.
 *
 * \param[in] name  The name of the package that was purged.
 * \param[in] ctrl  The control file with fields.
 */
void verify_removed_files(const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl)
{
    wpkg_control::file_list_t conffiles("Conffiles");
    if(ctrl->field_is_defined("Conffiles"))
    {
        conffiles.set(ctrl->get_field("Conffiles"));
    }
    wpkg_control::file_list_t::size_type max(conffiles.size());
    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename build_path(root.append_child(name));
    memfile::memory_file dir;
    dir.dir_rewind(build_path);
    for(;;)
    {
        memfile::memory_file::file_info info;
        if(!dir.dir_next(info, nullptr))
        {
            break;
        }
        if(info.get_file_type() == memfile::memory_file::file_info::regular_file
        && strstr(info.get_filename().c_str(), "/WPKG/") == nullptr)
        {
            wpkg_filename::uri_filename installed_name(info.get_uri());
            installed_name = installed_name.remove_common_segments(build_path);
            std::string absolute_filename(installed_name.path_only());
            if(!installed_name.is_absolute())
            {
                absolute_filename = "/" + absolute_filename;
            }
            bool found(false);
            for(wpkg_control::file_list_t::size_type i(0); i < max; ++i)
            {
                std::string conf(conffiles[i].get_filename().c_str());

                std::string conf_filename(conffiles[i].get_filename());
                if(!conf_filename.empty() && conf_filename[0] != '/')
                {
                    conf_filename = "/" + conf_filename;
                }
                if(conf_filename == absolute_filename)
                {
                    found = true;
                    break;
                }
            }
            if(!found)
            {
                // not found as one of the configuration file so it must have
                // been deleted, verify
                installed_name = target_path.append_child(installed_name.path_only());
                if(installed_name.exists())
                {
                    fprintf(stderr, "error: file \"%s\" was expected to be removed, it is still present.\n", installed_name.path_only().c_str());
                    CPPUNIT_ASSERT(!"removed file still exists!?");
                }
            }
        }
    }
}

/** \brief Check that a package was properly purged.
 *
 * The name of the package that got purged.
 *
 * This function checks the package configuration files and all are removed
 * (i.e. the .wpkg-new, .wkpg-old, and .wpkg-user extensions are checked too.)
 *
 * The list of exceptions are paths to files that will not have been purged,
 * as expected. This happens when we try to install and it fails because
 * of files that would otherwise get overwritten.
 *
 * \param[in] name  The name of the package that was purged.
 * \param[in] ctrl  The control file with fields.
 * \param[in] exceptions  List of exceptions; files that are still there
 */
void verify_purged_files(const std::string& name, std::shared_ptr<wpkg_control::control_file> ctrl, string_list exceptions = string_list())
{
    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename build_path(root.append_child(name));
    memfile::memory_file dir;
    dir.dir_rewind(build_path);
    for(;;)
    {
        memfile::memory_file::file_info info;
        if(!dir.dir_next(info, nullptr))
        {
            break;
        }
        if(info.get_file_type() == memfile::memory_file::file_info::regular_file
        && strstr(info.get_filename().c_str(), "/WPKG/") == nullptr)
        {
            // in this case all files must be gone
            wpkg_filename::uri_filename installed_name(info.get_uri());
            installed_name = installed_name.remove_common_segments(build_path);
            bool found(false);
            for(string_list::size_type i(0); i < exceptions.size(); ++i)
            {
                if(installed_name.path_only() == exceptions[i])
                {
                    found = true;
                    break;
                }
            }
            installed_name = target_path.append_child(installed_name.path_only());
            if(found)
            {
                // exceptions happen when we test overwrite problems
                CPPUNIT_ASSERT(installed_name.exists());
            }
            else
            {
// this print useful if you're wondering why an exception fails (i.e. did you use an absolute path?)
//fprintf(stderr, "checking [%s]\n", installed_name.path_only().c_str());
                CPPUNIT_ASSERT(!installed_name.exists());
            }
        }
    }

    wpkg_control::file_list_t conffiles("Conffiles");
    if(ctrl->field_is_defined("Conffiles"))
    {
        conffiles.set(ctrl->get_field("Conffiles"));
        wpkg_control::file_list_t::size_type max(conffiles.size());
        for(wpkg_control::file_list_t::size_type i(0); i < max; ++i)
        {
            wpkg_filename::uri_filename conffile(target_path.append_child(conffiles[i].get_filename()));
            // assuming that the package was properly built, the next test is a repeat from the previous loop
            CPPUNIT_ASSERT(!conffile.exists());

            // different extensions
            wpkg_filename::uri_filename with_ext(conffile.path_only() + ".wpkg-new");
            CPPUNIT_ASSERT(!with_ext.exists());
            with_ext.set_filename(conffile.path_only() + ".wpkg-old");
            CPPUNIT_ASSERT(!with_ext.exists());
            with_ext.set_filename(conffile.path_only() + ".wpkg-user");
            CPPUNIT_ASSERT(!with_ext.exists());
        }
    }
}


/** \brief Generate a random filename.
 *
 * This function generates a long random filename composed of digits
 * and ASCII letters. The result is expected to be 100% compatible
 * with all operating systems (MS-Windows has a few special cases but
 * these are very short names.)
 *
 * The result of the function can immediately be used as a filename
 * although it is expected to be used in a sub-directory (i.e. the
 * function does not generate a sub-directory path.)
 *
 * The maximum \p limit is 136 because 135 + 120 = 255 which is the
 * maximum filename on ext[234] and NTFS. This will definitively
 * fail on a direct FAT32 file system, although with MS-Windows it
 * should still work.
 *
 * \param[in] limit  The variable limit (we add 120 to that value).
 *
 * \return The randomly generated long filename.
 */
std::string generate_long_filename(int limit)
{
    std::string long_filename;
    const int long_filename_length(rand() % limit + 120);
    for(int i(0); i < long_filename_length; ++i)
    {
        // we're not testing special characters or anything like that
        // so just digits and ASCII letters are used
        char c(rand() % 62);
        if(c < 10)
        {
            c += '0';
        }
        else if(c < 36)
        {
            c += 'A' - 10;
        }
        else
        {
            c += 'a' - 36;
        }
        long_filename += c;
    }

//fprintf(stderr, "ln %3ld [%s]\n", long_filename.length(), long_filename.c_str());
    return long_filename;
}


} // noname namespace



///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//                                                                       //
//        ACTUAL TESTS START HERE                                        //
//                                                                       //
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////



void PackageUnitTests::simple_package()
{
    // IMPORTANT: remember that all files are deleted between tests

    std::shared_ptr<wpkg_control::control_file> ctrl(get_new_control_file(__FUNCTION__));
    ctrl->set_field("Files", "conffiles\n"
        "/etc/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl);

    install_package("t1", ctrl);        // --install + --remove
    verify_installed_files("t1");
    remove_package("t1", ctrl);
    verify_removed_files("t1", ctrl);

    install_package("t1", ctrl);        // --install + --purge
    verify_installed_files("t1");
    purge_package("t1", ctrl);
    verify_purged_files("t1", ctrl);

    wpkg_filename::uri_filename root(unittest::tmp_dir);
    //wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    ctrl->set_variable("INSTALL_POSTOPTIONS", wpkg_util::make_safe_console_string(repository.append_child("/t1_" + ctrl->get_field("Version") + "_" + ctrl->get_field("Architecture") + ".deb").path_only()));
    install_package("t1", ctrl);        // --install + --remove + --purge
    verify_installed_files("t1");
    remove_package("t1", ctrl);
    verify_removed_files("t1", ctrl);
    purge_package("t1", ctrl);
    verify_purged_files("t1", ctrl);

    install_package("t1", ctrl);        // --install + --install ("restore") + --purge
    verify_installed_files("t1");
    install_package("t1", ctrl);
    verify_installed_files("t1");
    purge_package("t1", ctrl);
    verify_purged_files("t1", ctrl);
}

void PackageUnitTests::simple_package_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    simple_package();
}

void PackageUnitTests::admindir_package()
{
    // IMPORTANT: remember that all files are deleted between tests

    std::shared_ptr<wpkg_control::control_file> ctrl(get_new_control_file(__FUNCTION__));
    ctrl->set_field("Files", "conffiles\n"
        "/etc/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl);

    // in this special case we want to create the target directory to avoid
    // the --create-admindir in it; then create and run --create-admindir
    // in the separate administration directory
    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));
    wpkg_filename::uri_filename admindir(root.append_child("admin"));
    target_path.os_mkdir_p();
    admindir.os_mkdir_p();
    wpkg_filename::uri_filename core_ctrl_filename(repository.append_child("core.ctrl"));
    memfile::memory_file core_ctrl;
    core_ctrl.create(memfile::memory_file::file_format_other);
    core_ctrl.printf("Architecture: %s\nMaintainer: Alexis Wilke <alexis@m2osw.com>\n", debian_packages_architecture());
    core_ctrl.write_file(core_ctrl_filename);
    std::string core_cmd(unittest::wpkg_tool + " --admindir " + wpkg_util::make_safe_console_string(admindir.os_real_path().full_path()) + " --create-admindir " + wpkg_util::make_safe_console_string(core_ctrl_filename.path_only()));
    printf("  Specilized Create AdminDir Command: \"%s\"  ", core_cmd.c_str());
    fflush(stdout);
    CPPUNIT_ASSERT(system(core_cmd.c_str()) == 0);
    ctrl->set_variable("INSTALL_NOROOT", "Yes");
    ctrl->set_variable("INSTALL_PREOPTIONS", "--admindir " + wpkg_util::make_safe_console_string(admindir.os_real_path().full_path()) + " --instdir " + wpkg_util::make_safe_console_string(target_path.os_real_path().full_path()));
    ctrl->set_variable("REMOVE_NOROOT", "Yes");
    ctrl->set_variable("REMOVE_PREOPTIONS", "--admindir " + wpkg_util::make_safe_console_string(admindir.os_real_path().full_path()) + " --instdir " + wpkg_util::make_safe_console_string(target_path.os_real_path().full_path()));
    ctrl->set_variable("PURGE_NOROOT", "Yes");
    ctrl->set_variable("PURGE_PREOPTIONS", "--admindir " + wpkg_util::make_safe_console_string(admindir.os_real_path().full_path()) + " --instdir " + wpkg_util::make_safe_console_string(target_path.os_real_path().full_path()));

    install_package("t1", ctrl);        // --install + --remove
    verify_installed_files("t1");
    remove_package("t1", ctrl);
    verify_removed_files("t1", ctrl);

    install_package("t1", ctrl);        // --install + --purge
    verify_installed_files("t1");
    purge_package("t1", ctrl);
    verify_purged_files("t1", ctrl);

    install_package("t1", ctrl);        // --install + --remove + --purge
    verify_installed_files("t1");
    remove_package("t1", ctrl);
    verify_removed_files("t1", ctrl);
    purge_package("t1", ctrl);
    verify_purged_files("t1", ctrl);

    install_package("t1", ctrl);        // --install + --install ("restore") + --purge
    verify_installed_files("t1");
    install_package("t1", ctrl);
    verify_installed_files("t1");
    purge_package("t1", ctrl);
    verify_purged_files("t1", ctrl);
}

void PackageUnitTests::admindir_package_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    admindir_package();
}

void PackageUnitTests::upgrade_package()
{
    // IMPORTANT: remember that all files are deleted between tests

    std::shared_ptr<wpkg_control::control_file> ctrl(get_new_control_file(__FUNCTION__));
    ctrl->set_field("Files", "conffiles\n"
        "/etc/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/index..html 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl);

    install_package("t1", ctrl);        // --install
    verify_installed_files("t1");

    // replace /usr/bin/t1 with /usr/bin/t1-new
    ctrl->set_field("Version", "1.1");
    ctrl->set_field("Files", "conffiles\n"
        "/etc/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1-new 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl);

    install_package("t1", ctrl);        // --install ("upgrade")
    verify_installed_files("t1");

    // make sure that /usr/bin/t1 was removed
    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));
    CPPUNIT_ASSERT(!target_path.append_child("usr/bin/t1").exists());

    root.append_child("t1").os_rename(root.append_child("t1-save"));

    // now test a downgrade
    ctrl->set_field("Version", "0.9");
    ctrl->set_field("Files", "conffiles\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/info..save 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl);

    install_package("t1", ctrl, 1);        // --install ("upgrade")

    // restore the original t1 so we can verify that its files weren't modified
    root.append_child("t1").os_unlink_rf();
    root.append_child("t1-save").os_rename(root.append_child("t1"));
    verify_installed_files("t1");
}

void PackageUnitTests::upgrade_package_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    upgrade_package();
}

void PackageUnitTests::depends_with_simple_packages()
{
    // IMPORTANT: remember that all files are deleted between tests

    std::shared_ptr<wpkg_control::control_file> ctrl_t2(get_new_control_file(__FUNCTION__));
    ctrl_t2->set_field("Files", "conffiles\n"
        "/etc/t2/t2.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t2 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t2b 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t2/copyright 0123456789abcdef0123456789abcdef\n"
        // also test with a space in the path
        "\"/usr/share/other docs/t2/info\" 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t2", ctrl_t2);
    ctrl_t2->set_variable("INSTALL_POSTOPTIONS",
#ifdef MO_WINDOWS
        // here we assume that you're running with cmd.exe which system() does
        // we have to duplicate all the double quotes
        "--validate-fields \"getfield(\"\"Version\"\") > \"\"0.9\"\"\""
#else
        "--validate-fields 'getfield(\"Version\") > \"0.9\"'"
#endif
    );
    install_package("t2", ctrl_t2);
    verify_installed_files("t2");

    std::shared_ptr<wpkg_control::control_file> ctrl_t3(get_new_control_file(__FUNCTION__));
    // Conffiles
    ctrl_t3->set_field("Conffiles", "\n"
        "/etc/t3/setup.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t3->set_field("Files", "conffiles\n"
        "/etc/t3/setup.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t3 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t3/copyright 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t3->set_field("Depends", "t2 (>= 1.0)");
    create_package("t3", ctrl_t3);
    // Conffiles -- the create_package deletes this field
    ctrl_t3->set_field("Conffiles", "\n"
        "etc/t3/setup.conf 0123456789abcdef0123456789abcdef"
    );
    install_package("t3", ctrl_t3);
    verify_installed_files("t3");
    remove_package("t3", ctrl_t3);
    verify_removed_files("t3", ctrl_t3);

    // we couldn't have removed t2, t3 were still installed!
    remove_package("t2", ctrl_t3);
    verify_removed_files("t2", ctrl_t3);

    // now we can reinstall t2 and t3
    install_package("t2", ctrl_t2);
    verify_installed_files("t2");

    install_package("t3", ctrl_t3);
    verify_installed_files("t3");

    purge_package("t3", ctrl_t3);
    verify_purged_files("t3", ctrl_t3);

    purge_package("t2", ctrl_t2);
    verify_purged_files("t2", ctrl_t2);

    // test with the --repository option
    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename build_path_t2(root.append_child("t2"));
    wpkg_filename::uri_filename wpkg_path_t2(build_path_t2.append_child("WPKG"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));
    ctrl_t3->set_variable("INSTALL_PREOPTIONS", "--repository " + wpkg_util::make_safe_console_string(repository.path_only()));

    install_package("t3", ctrl_t3);
    verify_installed_files("t3");
    verify_installed_files("t2"); // t2 was auto-installed, we can check that!
    remove_package("t3", ctrl_t3);
    verify_removed_files("t3", ctrl_t3);

    purge_package("t3", ctrl_t3);
    verify_purged_files("t3", ctrl_t3);

    purge_package("t2", ctrl_t2);
    verify_purged_files("t2", ctrl_t2);

    // the next test checks that t2 gets installed before t3 even though t2
    // is specified first on the command line; to do so, we add a simple
    // shell script that checks whether t3's files exist just before t2
    // gets unpacked
    memfile::memory_file preinst;
    preinst.create(memfile::memory_file::file_format_other);
#ifdef MO_WINDOWS
    preinst.printf(
        "REM Test whether t3 is installed\n"
        "ECHO Running preinst of t2 package\n"
        "IF EXIST usr\\bin\\t3 (\n"
        "  ECHO t3 file already exists, order was not respected\n"
        "  EXIT 1\n"
        ") ELSE (\n"
        "  ECHO t3 file not present, test passed\n"
        "  EXIT 0\n"
        ")\n"
    );
    preinst.write_file(wpkg_path_t2.append_child("preinst.bat"));
#else
    preinst.printf(
        "#!/bin/sh\n"
        "# Test whether t3 is installed\n"
        "echo \"Running preinst of t2 package\"\n"
        "if test -f usr/bin/t3\n"
        "then\n"
        " echo \"t3 file already exists, order was not respected\"\n"
        " exit 1\n"
        "else\n"
        " echo \"t3 file not present, test passed\"\n"
        " exit 0\n"
        "fi\n"
    );
    preinst.write_file(wpkg_path_t2.append_child("preinst"));
#endif
    create_package("t2", ctrl_t2, false);

    ctrl_t3->set_variable("INSTALL_POSTOPTIONS",
        wpkg_util::make_safe_console_string(repository.append_child("t2_" + ctrl_t2->get_field("Version") + "_" + ctrl_t2->get_field("Architecture") + ".deb").path_only())
        + " -D 077777 "
#ifdef MO_WINDOWS
        // here we assume that you're running with cmd.exe which system() does
        // we have to duplicate all the double quotes
        "--validate-fields \"getfield(\"\"Version\"\") == \"\"1.0\"\"\""
#else
        "--validate-fields 'getfield(\"Version\") == \"1.0\"'"
#endif
    );
    install_package("t3", ctrl_t3);
    verify_installed_files("t3");
    verify_installed_files("t2"); // t2 was explicitly installed in this case

    purge_package("t3", ctrl_t3);
    verify_purged_files("t3", ctrl_t3);

    purge_package("t2", ctrl_t2);
    verify_purged_files("t2", ctrl_t2);

    // add t1 as a dependency of t2
    ctrl_t2->set_field("Depends", "t1 (<< 3.0)");
    create_package("t2", ctrl_t2);

    // the test a circular dependency now: t1 -> t3 -> t2 -> t1
    std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__));
    ctrl_t1->set_field("Files", "conffiles\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t1->set_field("Depends", "t3");
    create_package("t1", ctrl_t1);
    ctrl_t1->set_variable("INSTALL_POSTOPTIONS",
            wpkg_util::make_safe_console_string(repository.append_child("t2_" + ctrl_t2->get_field("Version") + "_" + ctrl_t2->get_field("Architecture") + ".deb").path_only())
            + " " +
            wpkg_util::make_safe_console_string(repository.append_child("t3_" + ctrl_t3->get_field("Version") + "_" + ctrl_t3->get_field("Architecture") + ".deb").path_only())
        );
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);
    verify_purged_files("t2", ctrl_t2);
    verify_purged_files("t3", ctrl_t3);

}

void PackageUnitTests::depends_with_simple_packages_with_spaces()
{
    // IMPORTANT: remember that all files are deleted between tests

    // run the simple packages with the path transformed to include a space
    raii_tmp_dir_with_space add_a_space;
    depends_with_simple_packages();
}

void PackageUnitTests::essential_package()
{
    // IMPORTANT: remember that all files are deleted between tests

    std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__));
    ctrl_t1->set_field("Files", "conffiles\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1b 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1c 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1d 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t1->set_field("Essential", "Yes");
    create_package("t1", ctrl_t1);
    install_package("t1", ctrl_t1);
    verify_installed_files("t1");

    // remove as is fails because essential package cannot be removed by default
    remove_package("t1", ctrl_t1, 1);
    verify_installed_files("t1");
    purge_package("t1", ctrl_t1, 1);
    verify_installed_files("t1");

    // remove as is fails because essential package cannot be removed by default
    ctrl_t1->set_variable("REMOVE_PREOPTIONS", "--force-remove-essential");
    remove_package("t1", ctrl_t1);
    verify_removed_files("t1", ctrl_t1);
    ctrl_t1->set_variable("PURGE_PREOPTIONS", "--force-remove-essential");
    purge_package("t1", ctrl_t1);
    verify_purged_files("t1", ctrl_t1);

    // now test that overwriting of an essential file is not possible
    // re-install t1
    install_package("t1", ctrl_t1);
    verify_installed_files("t1");

    // then create t2 which a file that will overwrite on in t1
    std::shared_ptr<wpkg_control::control_file> ctrl_t2(get_new_control_file(__FUNCTION__));
    ctrl_t2->set_field("Files", "conffiles\n"
        "/etc/t2/t2.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t2 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1b 0123456789abcdef0123456789abcdef\n" // same as t1 file!
        "/usr/bin/t2c 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t2d 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t2/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t2", ctrl_t2);

    // TBD -- how do we know that we are getting the correct errors?
    install_package("t2", ctrl_t2, 1); // simple overwrite error
    string_list exceptions;
    exceptions.push_back("usr/bin/t1b"); // exceptions are checked against relative paths
    verify_purged_files("t2", ctrl_t2, exceptions);

    // check with --force-overwrite and it fails again
    ctrl_t2->set_variable("INSTALL_PREOPTIONS", "--force-overwrite");
    install_package("t2", ctrl_t2, 1); // simple overwrite error
    verify_purged_files("t2", ctrl_t2, exceptions);
}

void PackageUnitTests::essential_package_with_spaces()
{
    raii_tmp_dir_with_space add_a_space;
    essential_package();
}

void PackageUnitTests::file_exists_in_admindir()
{
    // IMPORTANT: remember that all files are deleted between tests

    std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__));
    ctrl_t1->set_field("Files", "conffiles\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1);

    // create a file named "t1" in the admindir to prevent installation
    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename t1_file(target_path.append_child("var/lib/wpkg/t1"));
    memfile::memory_file t1_data;
    t1_data.create(memfile::memory_file::file_format_other);
    t1_data.printf("Some random data\n");
    t1_data.write_file(t1_file, true);

    // there should be no other reason why installing t1 would fail, try!
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);
}

void PackageUnitTests::file_exists_in_admindir_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    file_exists_in_admindir();
}

void PackageUnitTests::depends_distribution_packages()
{
    // IMPORTANT: remember that all files are deleted between tests

    // first attempt to create a package without a Distribution field
    // we expect the installation to fail
    std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__));
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t1->set_field("Files", "conffiles\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1b 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1);
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t1->set_variable("INSTALL_EXTRACOREFIELDS", "Distribution: m2osw\n");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // re-create that same package, this time with the Distribution field,
    // but not the right distribution name
    ctrl_t1->set_field("Distribution", "wrong-name");
    create_package("t1", ctrl_t1);
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // okay, re-create with the correct distribution name this time
    ctrl_t1->set_field("Distribution", "m2osw");
    create_package("t1", ctrl_t1);
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );
    install_package("t1", ctrl_t1);
    verify_installed_files("t1");

    std::shared_ptr<wpkg_control::control_file> ctrl_t2(get_new_control_file(__FUNCTION__));
    // Conffiles
    ctrl_t2->set_field("Conffiles", "\n"
        "/etc/t2/setup.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t2->set_field("Files", "conffiles\n"
        "/etc/t2/setup.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t2 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t2/copyright 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t2->set_field("Depends", "t1 (<= 1.0)");
    ctrl_t2->set_field("Distribution", "m2osw");
    create_package("t2", ctrl_t2);
    // Conffiles -- the create_package deletes this field
    ctrl_t2->set_field("Conffiles", "\n"
        "etc/t2/setup.conf 0123456789abcdef0123456789abcdef"
    );
    install_package("t2", ctrl_t2);
    verify_installed_files("t2");
    remove_package("t2", ctrl_t2);
    verify_removed_files("t2", ctrl_t2);

    // we couldn't have removed t1, t2 were still installed!
    remove_package("t1", ctrl_t1);
    verify_removed_files("t1", ctrl_t1);

    // now we can reinstall t1 and t2
    install_package("t1", ctrl_t1);
    verify_installed_files("t1");

    install_package("t2", ctrl_t2);
    verify_installed_files("t2");

    purge_package("t2", ctrl_t2);
    verify_purged_files("t2", ctrl_t2);

    purge_package("t1", ctrl_t1);
    verify_purged_files("t1", ctrl_t1);

    ctrl_t1->set_variable("INSTALL_POSTOPTIONS",
#ifdef MO_WINDOWS
        // here we assume that you're running with cmd.exe which system() does
        // we have to duplicate all the double quotes
        "--validate-fields \"getfield(\"\"Package\"\") == \"\"t1\"\"\""
#else
        "--validate-fields 'getfield(\"Package\") == \"t1\"'"
#endif
    );
    ctrl_t2->set_variable("INSTALL_POSTOPTIONS",
#ifdef MO_WINDOWS
        // here we assume that you're running with cmd.exe which system() does
        // we have to duplicate all the double quotes
        "--validate-fields \"getfield(\"\"Package\"\") >= \"\"t1\"\"\""
#else
        "--validate-fields 'getfield(\"Package\") >= \"t1\"'"
#endif
    );

    // test with the --repository option
    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename repository(root.append_child("repository"));
    ctrl_t2->set_variable("INSTALL_PREOPTIONS", "--repository " + wpkg_util::make_safe_console_string(repository.path_only()));

    install_package("t2", ctrl_t2);
    verify_installed_files("t2");
    verify_installed_files("t1");

    purge_package("t2", ctrl_t2);
    verify_purged_files("t2", ctrl_t2);

    purge_package("t1", ctrl_t1);
    verify_purged_files("t1", ctrl_t1);

    // now try the --repository with the wrong distribution
    ctrl_t1->set_field("Distribution", "wong-name-again");
    create_package("t1", ctrl_t1);
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );

    // ?!?! WORKS WITH THE WRONG DISTRIBUTION ?!?!
    // This is because there is an index and all the validations count on the
    // index to be valid! (here we have a sync. problem too!)
    install_package("t2", ctrl_t2);
    verify_installed_files("t2");
    verify_installed_files("t1");

    purge_package("t2", ctrl_t2);
    verify_purged_files("t2", ctrl_t2);

    purge_package("t1", ctrl_t1);
    verify_purged_files("t1", ctrl_t1);

    // So now we reset the index and try again
    wpkg_filename::uri_filename index(repository.append_child("index.tar.gz"));
    index.os_unlink();

    install_package("t2", ctrl_t2, 1);
    verify_purged_files("t2", ctrl_t2);
    verify_purged_files("t1", ctrl_t1);

    // --force-distribution works even on implicit packages
    ctrl_t2->set_variable("INSTALL_POSTOPTIONS", "--force-distribution");
    install_package("t2", ctrl_t2);
    verify_installed_files("t2");
    verify_installed_files("t1");

    // cannot purge (or remove) because t2 depends on it
    purge_package("t1", ctrl_t1, 1);
    verify_installed_files("t1");

    // reset slate to test a Pre-Depends instead
    purge_package("t2", ctrl_t2);
    verify_purged_files("t2", ctrl_t2);

    purge_package("t1", ctrl_t1);
    verify_purged_files("t1", ctrl_t1);

    // when we change the architecture we get a new name,
    // make sure the old .deb is removed
    // we also have to delete the index because it still has a
    // reference to that old .deb file (and not the new file)
    {
        wpkg_filename::uri_filename t1(repository.append_child("/t1_" + ctrl_t1->get_field("Version") + "_" + ctrl_t1->get_field("Architecture") + ".deb"));
        t1.os_unlink();
    }
    index.os_unlink();

    // fix distribution + wrong architecture
    ctrl_t1->set_field("Distribution", "m2osw");
    ctrl_t1->set_field("Architecture", strcmp(debian_packages_architecture(), "win32-i386") == 0 ? "win64-amd64" : "win32-i386");
    create_package("t1", ctrl_t1);
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );

    install_package("t2", ctrl_t2, 1);
    verify_purged_files("t2", ctrl_t2);
    verify_purged_files("t1", ctrl_t1);

    // reset architecture
    ctrl_t1->set_field("Architecture", debian_packages_architecture());
    create_package("t1", ctrl_t1);
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );

    // replace the Depends with a Pre-Depends
    ctrl_t2->delete_field("Depends");
    ctrl_t2->set_field("Pre-Depends", "t1 (>> 0.9)");
    create_package("t2", ctrl_t2);
    ctrl_t2->set_field("Conffiles", "\n"
        "etc/t2/setup.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t2->delete_variable("INSTALL_POSTOPTIONS");

    // fails because t1 is a Pre-dependency
    install_package("t2", ctrl_t2, 1);
    verify_purged_files("t2", ctrl_t2);
    verify_purged_files("t1", ctrl_t1);

    install_package("t1", ctrl_t1);
    verify_installed_files("t1");
    install_package("t2", ctrl_t2);
    verify_installed_files("t2");
}

void PackageUnitTests::depends_distribution_packages_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    depends_distribution_packages();
}

void PackageUnitTests::conflicting_packages()
{
    // IMPORTANT: remember that all files are deleted between tests

    // create & install a package that doesn't like the other
    std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__));
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t1->set_field("Files", "conffiles\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t1->set_field("Conflicts", "t2");
    create_package("t1", ctrl_t1);
    // Conffiles -- the create_package deletes this field
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );
    install_package("t1", ctrl_t1);
    verify_installed_files("t1");

    // create that other package
    std::shared_ptr<wpkg_control::control_file> ctrl_t2(get_new_control_file(__FUNCTION__));
    ctrl_t2->set_field("Conffiles", "\n"
        "/etc/t2/setup.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t2->set_field("Files", "conffiles\n"
        "/etc/t2/setup.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t2 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t2/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t2", ctrl_t2);
    ctrl_t2->set_field("Conffiles", "\n"
        "etc/t2/setup.conf 0123456789abcdef0123456789abcdef"
    );
    install_package("t2", ctrl_t2, 1);
    verify_purged_files("t2", ctrl_t2);

    // try again with the force flag
    ctrl_t2->set_variable("INSTALL_POSTOPTIONS", "--force-conflicts");
    install_package("t2", ctrl_t2);
    verify_installed_files("t2");
    remove_package("t2", ctrl_t2);
    verify_removed_files("t2", ctrl_t2);

    purge_package("t2", ctrl_t2);
    verify_purged_files("t2", ctrl_t2);

    purge_package("t1", ctrl_t1);
    verify_purged_files("t1", ctrl_t1);

    // replace with a break which when packages are installed has the same effect
    ctrl_t1->delete_field("Conflicts");
    ctrl_t1->set_field("Breaks", "t2");
    create_package("t1", ctrl_t1);
    // Conffiles -- the create_package deletes this field
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );
    install_package("t1", ctrl_t1);
    verify_installed_files("t1");

    // t2 already exists so we can just try to install, it fails because of the Breaks
    ctrl_t2->delete_variable("INSTALL_POSTOPTIONS");
    install_package("t2", ctrl_t2, 1);
    verify_purged_files("t2", ctrl_t2);

    // try again with a force, this time it is expected to work
    ctrl_t2->set_variable("INSTALL_POSTOPTIONS", "--force-breaks");
    install_package("t2", ctrl_t2);
    verify_installed_files("t2");
    remove_package("t2", ctrl_t2);
    verify_removed_files("t2", ctrl_t2);

    purge_package("t2", ctrl_t2);
    verify_purged_files("t2", ctrl_t2);

    purge_package("t1", ctrl_t1);
    verify_purged_files("t1", ctrl_t1);
}

void PackageUnitTests::conflicting_packages_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    conflicting_packages();
}

void PackageUnitTests::sorted_packages_run(int precreate_index)
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename const root(unittest::tmp_dir);
    wpkg_filename::uri_filename const repository(root.append_child("repository"));

    // *** CREATION ***
    // create 50 to 70 packages and install them in random order
    // then upgrade different packages in a random order
    const int max_packages = rand() % 21 + 50;
    std::vector<bool> has_conf;
    has_conf.resize(max_packages + 1);
    std::vector<bool> has_dependents;
    has_dependents.resize(max_packages + 1);
    std::vector<int> order;
    order.resize(max_packages + 1);
    for(int i = 1; i <= max_packages; ++i)
    {
        order[i] = i;
        std::stringstream strname;
        strname << "t" << i;
        std::string name(strname.str());
        std::shared_ptr<wpkg_control::control_file> ctrl(get_new_control_file(__FUNCTION__));
        has_conf[i] = (rand() & 1) != 0;
        if(has_conf[i])
        {
            ctrl->set_field("Conffiles", "\n"
                "/etc/" + name + "/" + name + ".conf 0123456789abcdef0123456789abcdef"
            );
        }
        ctrl->set_field("Files", "conffiles\n"
            "/etc/" + name + "/" + name + ".conf 0123456789abcdef0123456789abcdef\n"
            "/usr/bin/" + name + " 0123456789abcdef0123456789abcdef\n"
            "/usr/share/doc/" + name + "/copyright 0123456789abcdef0123456789abcdef\n"
        );
        int depend(rand() % (max_packages * 2) + 1);
        if(depend <= max_packages && depend != i && !has_dependents[depend])
        {
            std::stringstream strdepend;
            strdepend << "t" << depend;
            ctrl->set_field("Depends", strdepend.str());
            has_dependents[i] = true;
        }
        create_package(name, ctrl);
        if(has_conf[i])
        {
            ctrl->set_field("Conffiles", "\n"
                "/etc/" + name + "/" + name + ".conf 0123456789abcdef0123456789abcdef"
            );
        }
    }

    // the installation will automatically create the index, however,
    // if we let it do that we miss on the potential to test validation
    // against field only; however, we want to test the automatic
    // mechanism too once in a while so we randomize the use of that
    if(precreate_index)
    {
        std::string cmd(unittest::wpkg_tool);
        cmd += " --create-index " + wpkg_util::make_safe_console_string(repository.full_path()) + "/index.tar.gz --repository " + wpkg_util::make_safe_console_string(repository.full_path());
        printf("Create packages index: \"%s\"\n", cmd.c_str());
        fflush(stdout);
        CPPUNIT_ASSERT(system(cmd.c_str()) == 0);
    }

    // *** INSTALLATION ***
    // randomize the order in which we'll be installing these
    for(int i = 1; i <= max_packages; ++i)
    {
        const int j = rand() % max_packages + 1;
        std::swap(order[i], order[j]);
    }
    for(int i = 1; i <= max_packages; ++i)
    {
        // some random control file is required
        // we need the proper architecture and version which we have not changed from the default
        std::shared_ptr<wpkg_control::control_file> ctrl(get_new_control_file(__FUNCTION__));
        std::stringstream strname;
        strname << "t" << order[i];
        ctrl->set_variable("INSTALL_PREOPTIONS", "--repository " + wpkg_util::make_safe_console_string(repository.path_only()));
        ctrl->set_variable("INSTALL_POSTOPTIONS",
#ifdef MO_WINDOWS
                // here we assume that you're running with cmd.exe which system() does
                // we have to duplicate all the double quotes
                "--validate-fields \"getfield(\"\"Version\"\") >= \"\"0.9\"\"\""
#else
                "--validate-fields 'getfield(\"Version\") >= \"0.9\"'"
#endif
            );
        install_package(strname.str(), ctrl);
    }

    // *** UPGRADE ***
    // randomize the order in which we'll be upgrading these
    for(int i = 1; i <= max_packages; ++i)
    {
        const int j = rand() % max_packages + 1;
        std::swap(order[i], order[j]);
    }
    std::vector<int> version;
    version.resize(max_packages + 1);
    for(int i = 1; i <= max_packages; ++i)
    {
        // recreate a valid control file
        std::shared_ptr<wpkg_control::control_file> ctrl(get_new_control_file(__FUNCTION__));
        std::stringstream strname;
        strname << "t" << order[i];
        std::string name(strname.str());
        if(has_conf[order[i]])
        {
            ctrl->set_field("Conffiles", "\n"
                "/etc/" + name + "/" + name + ".conf 0123456789abcdef0123456789abcdef"
            );
        }
        ctrl->set_field("Files", "conffiles\n"
            "/etc/" + name + "/" + name + ".conf 0123456789abcdef0123456789abcdef\n"
            "/usr/bin/" + name + " 0123456789abcdef0123456789abcdef\n"
            "/usr/share/doc/" + name + "/copyright 0123456789abcdef0123456789abcdef\n"
        );
        // bump version up (or not, one in 20 will still be 1.0)
        version[order[i]] = rand() % 20;
        std::stringstream strversion;
        strversion << "1." << version[order[i]];
        ctrl->set_field("Version", strversion.str());
        create_package(name, ctrl);
        // no need to recreate the Conffiles field here
        install_package(name, ctrl);
    }

    // *** REFRESH ***
    // randomize the order in which we'll be refreshing these
    for(int i = 1; i <= max_packages; ++i)
    {
        const int j = rand() % max_packages + 1;
        std::swap(order[i], order[j]);
    }
    std::shared_ptr<wpkg_control::control_file> ctrl_refresh(get_new_control_file(__FUNCTION__));
    std::stringstream name_list;
    for(int i = 2; i <= max_packages; ++i)
    {
        name_list << " " << wpkg_util::make_safe_console_string(repository.path_only()) << "/t" << order[i] << "_1." << version[order[i]] << "_" << debian_packages_architecture() << ".deb";
    }
    std::stringstream strfirst_version;
    strfirst_version << "1." << version[order[1]];
    ctrl_refresh->set_field("Version", strfirst_version.str());
    ctrl_refresh->set_variable("INSTALL_POSTOPTIONS", name_list.str());
    std::stringstream name_refresh;
    name_refresh << "t" << order[1];
    install_package(name_refresh.str(), ctrl_refresh);

    // with all those .deb files, we can create an impressive md5sums.txt file
    // so let's do that and then run a check
    wpkg_filename::os_dir debs(repository);
    const std::string debs_filenames(debs.read_all("*.deb"));
    {
        std::string cmd(unittest::wpkg_tool);
        //repository.append_child("/*.deb").full_path(true)
        cmd += " --md5sums " + debs_filenames + " >" + wpkg_util::make_safe_console_string(root.append_child("/md5sums.txt").full_path(true)) + " -v";
        printf("Create md5sums: \"%s\"\n", cmd.c_str());
        fflush(stdout);
        CPPUNIT_ASSERT(system(cmd.c_str()) == 0);
    }
    {
        std::string cmd(unittest::wpkg_tool);
        cmd += " --md5sums-check " + wpkg_util::make_safe_console_string(root.append_child("/md5sums.txt").full_path(true)) + " " + debs_filenames + " -v";
        printf("  check valid md5sums: \"%s\"\n", cmd.c_str());
        fflush(stdout);
        CPPUNIT_ASSERT(system(cmd.c_str()) == 0);
    }
    {
        // modify an md5 checksum
        FILE *f(fopen((root.full_path() + "/md5sums.txt").c_str(), "r+"));
        CPPUNIT_ASSERT(f != nullptr);
        char o;
#ifdef __GNUC__
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wunused-result"
#endif
        fseek(f, 0, SEEK_SET);
        fread(&o, 1, 1, f);
        char c(o == 'f' ? 'a' : 'f');
        fseek(f, 0, SEEK_SET);
        fwrite(&c, 1, 1, f);
        fclose(f);
#ifdef __GNUC__
#   pragma GCC diagnostic pop
#endif
        // try again and this time we MUST get an error
        std::string cmd(unittest::wpkg_tool);
        cmd += " --md5sums-check " + wpkg_util::make_safe_console_string(root.full_path()) + "/md5sums.txt " + debs_filenames + " -v";
        printf("  check invalid md5sums: \"%s\"\n", cmd.c_str());
        fflush(stdout);
        int r(system(cmd.c_str()));
        CPPUNIT_ASSERT(WEXITSTATUS(r) == 1);
    }
}

void PackageUnitTests::sorted_packages_auto_index()
{
    sorted_packages_run(false);
}

void PackageUnitTests::sorted_packages_auto_index_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    sorted_packages_auto_index();
}

void PackageUnitTests::sorted_packages_ready_index()
{
    sorted_packages_run(true);
}

void PackageUnitTests::sorted_packages_ready_index_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    sorted_packages_ready_index();
}

void PackageUnitTests::choices_packages()
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    // Failing tree because pb and pc require two different versions of pd
    // pa: pb pc
    // pb: pd1
    // pc: pd2
    // pd1: pe
    // pd2: pe pf
    // pe:
    // pf:

    // package pa
    std::shared_ptr<wpkg_control::control_file> ctrl_pa(get_new_control_file(__FUNCTION__));
    ctrl_pa->set_field("Conffiles", "\n"
        "/etc/pa/pa.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_pa->set_field("Files", "conffiles\n"
        "/etc/pa/pa.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/pa 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/pa/copyright 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_pa->set_field("Depends", "pb, pc");
    create_package("pa", ctrl_pa);
    ctrl_pa->set_field("Conffiles", "\n"
        "/etc/pa/pa.conf 0123456789abcdef0123456789abcdef"
    );

    // package pb
    std::shared_ptr<wpkg_control::control_file> ctrl_pb(get_new_control_file(__FUNCTION__));
    ctrl_pb->set_field("Conffiles", "\n"
        "/etc/pb/pb.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_pb->set_field("Files", "conffiles\n"
        "/etc/pb/pb.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/pb 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/pb/copyright 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_pb->set_field("Depends", "pd (= 1.0)");
    create_package("pb", ctrl_pb);
    ctrl_pb->set_field("Conffiles", "\n"
        "/etc/pb/pb.conf 0123456789abcdef0123456789abcdef"
    );

    // package pc
    std::shared_ptr<wpkg_control::control_file> ctrl_pc(get_new_control_file(__FUNCTION__));
    ctrl_pc->set_field("Conffiles", "\n"
        "/etc/pc/pc.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_pc->set_field("Files", "conffiles\n"
        "/etc/pc/pc.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/pc 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/pc/copyright 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_pc->set_field("Depends", "pd (= 2.0)");
    create_package("pc", ctrl_pc);
    ctrl_pc->set_field("Conffiles", "\n"
        "/etc/pc/pc.conf 0123456789abcdef0123456789abcdef"
    );

    // package pd1 (version 1.0)
    std::shared_ptr<wpkg_control::control_file> ctrl_pd1(get_new_control_file(__FUNCTION__));
    ctrl_pd1->set_field("Conffiles", "\n"
        "/etc/pd/pd.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_pd1->set_field("Files", "conffiles\n"
        "/etc/pd/pd.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/pd 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/pd/copyright 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_pd1->set_field("Depends", "pe");
    create_package("pd", ctrl_pd1);
    ctrl_pd1->set_field("Conffiles", "\n"
        "/etc/pd/pd.conf 0123456789abcdef0123456789abcdef"
    );

    // package pd2 (version 2.0)
    std::shared_ptr<wpkg_control::control_file> ctrl_pd2(get_new_control_file(__FUNCTION__));
    ctrl_pd2->set_field("Version", "2.0");
    ctrl_pd2->set_field("Conffiles", "\n"
        "/etc/pd/pd.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_pd2->set_field("Files", "conffiles\n"
        "/etc/pd/pd.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/pd 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/pd/copyright 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_pd2->set_field("Depends", "pe, pf");
    create_package("pd", ctrl_pd2);
    ctrl_pd2->set_field("Conffiles", "\n"
        "/etc/pd/pd.conf 0123456789abcdef0123456789abcdef"
    );

    // package pe
    std::shared_ptr<wpkg_control::control_file> ctrl_pe(get_new_control_file(__FUNCTION__));
    ctrl_pe->set_field("Conffiles", "\n"
        "/etc/pe/pe.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_pe->set_field("Files", "conffiles\n"
        "/etc/pe/pe.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/pe 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/pe/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("pe", ctrl_pe);
    ctrl_pe->set_field("Conffiles", "\n"
        "/etc/pe/pe.conf 0123456789abcdef0123456789abcdef"
    );

    // package pf
    std::shared_ptr<wpkg_control::control_file> ctrl_pf(get_new_control_file(__FUNCTION__));
    ctrl_pf->set_field("Conffiles", "\n"
        "/etc/pf/pf.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_pf->set_field("Files", "conffiles\n"
        "/etc/pf/pf.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/pf 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/pf/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("pf", ctrl_pf);
    ctrl_pf->set_field("Conffiles", "\n"
        "/etc/pf/pf.conf 0123456789abcdef0123456789abcdef"
    );




    ctrl_pa->set_variable("INSTALL_PREOPTIONS", "--repository " + wpkg_util::make_safe_console_string(repository.path_only()));
    install_package("pa", ctrl_pa, 1);

    verify_purged_files("pa", ctrl_pa);
    verify_purged_files("pb", ctrl_pb);
    verify_purged_files("pc", ctrl_pc);
    //verify_purged_files("pd", ctrl_pd1); -- this was overwritten by pd2
    verify_purged_files("pd", ctrl_pd2);
    verify_purged_files("pe", ctrl_pe);
    verify_purged_files("pf", ctrl_pf);
}

void PackageUnitTests::choices_packages_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    choices_packages();
}

void PackageUnitTests::same_package_two_places_errors()
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename repository(root.append_child("repository"));
    wpkg_filename::uri_filename rep2(root.append_child("rep2"));
    rep2.os_mkdir_p();

    // create two packages with the exact same name (in two different directories)
    std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__));
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t1->set_field("Files", "conffiles\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1);
    // Conffiles -- the create_package deletes this field
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );

    // move t1 to rep2
    wpkg_filename::uri_filename t1_filename(repository.append_child("/t1_" + ctrl_t1->get_field("Version") + "_" + ctrl_t1->get_field("Architecture") + ".deb"));
    wpkg_filename::uri_filename t1_file2(rep2.append_child("/t1_" + ctrl_t1->get_field("Version") + "_" + ctrl_t1->get_field("Architecture") + ".deb"));
    t1_filename.os_rename(t1_file2);

    // create another t1 (t1b variables) in repository
    std::shared_ptr<wpkg_control::control_file> ctrl_t1b(get_new_control_file(__FUNCTION__));
    ctrl_t1b->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t1b->set_field("Files", "conffiles\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1b);
    // Conffiles -- the create_package deletes this field
    ctrl_t1b->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );

    ctrl_t1->set_variable("INSTALL_POSTOPTIONS", wpkg_util::make_safe_console_string(rep2.append_child("/t1_" + ctrl_t1->get_field("Version") + "_" + ctrl_t1->get_field("Architecture") + ".deb").path_only()));
    install_package("t1", ctrl_t1, 1);

    verify_purged_files("t1", ctrl_t1);
}

void PackageUnitTests::same_package_two_places_errors_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    same_package_two_places_errors();
}


void PackageUnitTests::self_upgrade()
{
    // IMPORTANT: remember that all files are deleted between tests

    // create a package with the name "wpkg"
    std::shared_ptr<wpkg_control::control_file> ctrl_wpkg(get_new_control_file(__FUNCTION__));
    ctrl_wpkg->set_field("Priority", "required");
    ctrl_wpkg->set_field("Conffiles", "\n"
        "/etc/wpkg/wpkg.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_wpkg->set_field("Files", "conffiles\n"
        "/etc/wpkg/wpkg.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/wpkg 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/wpkg/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("wpkg", ctrl_wpkg);
    // Conffiles -- the create_package deletes this field
    ctrl_wpkg->set_field("Conffiles", "\n"
        "/etc/wpkg/wpkg.conf 0123456789abcdef0123456789abcdef"
    );

    // the first install call is expected to work as is, no problems
    install_package("wpkg", ctrl_wpkg, 0);
    verify_installed_files("wpkg");

    // the second install works too, only this time we were upgrading
    // (IMPORTANT NOTE: Under MS-Windows we lose control and the 2nd instance
    // of wpkg.exe may generate errors and we won't know it!)
    install_package("wpkg", ctrl_wpkg, 0);
#ifdef MO_WINDOWS
    printf("Sleeping 20 seconds to give wpkg a chance to finish its work... [1]\n");
    fflush(stdout);
    Sleep(20000);
#endif
    verify_installed_files("wpkg");

    // wpkg does not allow removal (i.e. we marked it as required)
    remove_package("wpkg", ctrl_wpkg, 1);
    verify_installed_files("wpkg");
    purge_package("wpkg", ctrl_wpkg, 1);
    verify_installed_files("wpkg");


    // try again, this time we remove the Priority field...
    std::shared_ptr<wpkg_control::control_file> ctrl_wpkg2(get_new_control_file(__FUNCTION__));
    ctrl_wpkg2->set_field("Version", "1.4.3");
    ctrl_wpkg2->set_field("Conffiles", "\n"
        "/etc/wpkg/wpkg.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_wpkg2->set_field("Files", "conffiles\n"
        "/etc/wpkg/wpkg.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/wpkg 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/wpkg/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("wpkg", ctrl_wpkg2);
    // Conffiles -- the create_package deletes this field
    ctrl_wpkg2->set_field("Conffiles", "\n"
        "/etc/wpkg/wpkg.conf 0123456789abcdef0123456789abcdef"
    );

    // the first install call is expected to work as is, no problems
    install_package("wpkg", ctrl_wpkg2, 0);
#ifdef MO_WINDOWS
    printf("Sleeping 20 seconds to give wpkg a chance to finish its work... [2]\n");
    fflush(stdout);
    Sleep(20000);
#endif
    verify_installed_files("wpkg");

    // the second install works too, only this time we were upgrading
    // (IMPORTANT NOTE: Under MS-Windows we lose control and the 2nd instance
    // of wpkg.exe may generate errors and we won't know it!)
    install_package("wpkg", ctrl_wpkg2, 0);
#ifdef MO_WINDOWS
    printf("Sleeping 20 seconds to give wpkg a chance to finish its work... [3]\n");
    fflush(stdout);
    Sleep(20000);
#endif
    verify_installed_files("wpkg");

    // wpkg does not allow removal (i.e. we marked it as required)
    remove_package("wpkg", ctrl_wpkg2, 1);
    verify_installed_files("wpkg");
    purge_package("wpkg", ctrl_wpkg2, 1);
    verify_installed_files("wpkg");
}

void PackageUnitTests::self_upgrade_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    self_upgrade();
}

void PackageUnitTests::scripts_order()
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename repository(root.append_child("repository"));

////////////////////// t1 -- upgrade from full scripts to full scripts
    wpkg_filename::uri_filename build_path_t1(root.append_child("t1"));
    wpkg_filename::uri_filename wpkg_path_t1(build_path_t1.append_child("WPKG"));

    // create a first version of the package
    {
        std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__));
        ctrl_t1->set_field("Files", "conffiles\n"
            "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
            "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
        );

        memfile::memory_file preinst;
        preinst.create(memfile::memory_file::file_format_other);
        memfile::memory_file postinst;
        postinst.create(memfile::memory_file::file_format_other);
        memfile::memory_file prerm;
        prerm.create(memfile::memory_file::file_format_other);
        memfile::memory_file postrm;
        postrm.create(memfile::memory_file::file_format_other);

#ifdef MO_WINDOWS
        preinst.printf(
            "REM Test to know that t1 preinst ran\n"
            "ECHO preinst: called with: [%%*]\n"
            "ECHO pre-inst ctrl_t1 > preinst.txt\n"
            "ECHO arguments: [%%*] >> preinst.txt\n"
        );
        preinst.write_file(wpkg_path_t1.append_child("preinst.bat"), true);
        postinst.printf(
            "REM Test to know that t1 postinst ran\n"
            "ECHO postinst: called with: [%%*]\n"
            "ECHO post-inst ctrl_t1 > postinst.txt\n"
            "ECHO arguments: [%%*] >> postinst.txt\n"
            "IF EXIST preinst.txt (\n"
            "  ECHO t1 preinst ran as expected\n"
            "  EXIT 0\n"
            ") ELSE (\n"
            "  ECHO t1 preinst.txt file not present, test failed\n"
            "  EXIT 1\n"
            ")\n"
        );
        postinst.write_file(wpkg_path_t1.append_child("postinst.bat"), true);
        prerm.printf(
            "REM Test to know that t1 prerm ran\n"
            "ECHO pre-rm: called with: [%%*]\"\n"
            "ECHO pre-rm ctrl_t1 > prerm.txt\n"
            "ECHO arguments: [%%*] >> prerm.txt\n"
        );
        prerm.write_file(wpkg_path_t1.append_child("prerm.bat"), true);
        postrm.printf(
            "REM Test to know that t1 postrm ran\n"
            "ECHO post-rm: called with: [%%*]\"\n"
            "ECHO post-rm ctrl_t1 > postrm.txt\n"
            "ECHO arguments: [%%*] >> postrm.txt\n"
        );
        postrm.write_file(wpkg_path_t1.append_child("postrm.bat"), true);
#else
        preinst.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t1 preinst ran\n"
            "echo \"preinst: called with: [$*]\"\n"
            "if test -f *.txt\n"
            "then\n"
            "  echo \"  preinst: t1 preinst found unexpected .txt files\"\n"
            "  exit 1\n"
            "fi\n"
            "echo \"pre-inst ctrl_t1\" > preinst.txt\n"
            "echo \"arguments: [$*]\" >> preinst.txt\n"
        );
        preinst.write_file(wpkg_path_t1.append_child("preinst"), true);
        postinst.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t1 postinst ran\n"
            "echo \"postinst: called with: [$*]\"\n"
            "echo \"post-inst ctrl_t1\" > postinst.txt\n"
            "echo \"arguments: [$*]\" >> postinst.txt\n"
            "if test -f preinst.txt\n"
            "then\n"
            "  echo \"  postinst: t1 preinst ran as expected\"\n"
            "  exit 0\n"
            "else\n"
            "  echo \"  postinst: t1 preinst file not present, test failed\"\n"
            "  exit 1\n"
            "fi\n"
        );
        postinst.write_file(wpkg_path_t1.append_child("postinst"), true);
        prerm.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t1 prerm ran\n"
            "echo \"prerm: called with: [$*]\"\n"
            "if test -f *.txt\n"
            "then\n"
            "  echo \"  prerm: t1 prerm found unexpected .txt files\"\n"
            "  exit 1\n"
            "fi\n"
            "echo \"pre-rm ctrl_t1\" > prerm.txt\n"
            "echo \"arguments: [$*]\" >> prerm.txt\n"
        );
        prerm.write_file(wpkg_path_t1.append_child("prerm"), true);
        postrm.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t1 postrm ran\n"
            "echo \"postrm: called with: [$*]\"\n"
            "echo \"post-rm ctrl_t1\" > postrm.txt\n"
            "echo \"arguments: [$*]\" >> postrm.txt\n"
            "if test -f preinst-b.txt -a -f prerm.txt\n"
            "then\n"
            "  echo \"  postinst: t1 preinst ran as expected\"\n"
            "  exit 0\n"
            "else\n"
            "  echo \"  postinst: t1 preinst file not present, test failed\"\n"
            "  exit 1\n"
            "fi\n"
        );
        postrm.write_file(wpkg_path_t1.append_child("postrm"), true);
#endif
        create_package("t1", ctrl_t1, false);
        install_package("t1", ctrl_t1);
        verify_installed_files("t1");

        // verify that each script created the file we expect
        verify_file_vector_t files;
        verify_file_t f;
        f.f_filename = "preinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "pre-inst ctrl_t1\n"
                   "arguments: [install]";
        files.push_back(f);
        f.f_filename = "postinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "post-inst ctrl_t1\n"
                   "arguments: [configure 1.0]";
        files.push_back(f);
        f.f_filename = "prerm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postrm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        verify_generated_files(files);
    }

    // create an upgrade
    {
        std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__));
        ctrl_t1->set_field("Version", "1.1");
        ctrl_t1->set_field("Files", "conffiles\n"
            "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
            "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
        );
        // destroy the previous version
        create_package("t1", ctrl_t1);

        memfile::memory_file preinst;
        preinst.create(memfile::memory_file::file_format_other);
        memfile::memory_file postinst;
        postinst.create(memfile::memory_file::file_format_other);
        memfile::memory_file prerm;
        prerm.create(memfile::memory_file::file_format_other);
        memfile::memory_file postrm;
        postrm.create(memfile::memory_file::file_format_other);

#ifdef MO_WINDOWS
        preinst.printf(
            "REM Test to know whether t1(b) preinst ran\n"
            "ECHO preinst(b): called with: [%%*]\n"
            "ECHO pre-inst ctrl_t1 (b) > preinst-b.txt\n"
            "ECHO arguments: [%%*] >> preinst-b.txt\n"
        );
        preinst.write_file(wpkg_path_t1.append_child("preinst.bat"), true);
        postinst.printf(
            "REM Test to know that t1 postinst ran\n"
            "ECHO postinst(b): called with: [%%*]\n"
            "ECHO post-inst ctrl_t1 (b) > postinst-b.txt\n"
            "ECHO arguments: [%%*] >> postinst-b.txt\n"
            "IF EXIST preinst-b.txt (\n"
            "  ECHO \"t1(b) preinst ran as expected\"\n"
            "  EXIT 0\n"
            ") ELSE (\n"
            "  ECHO \"t1(b) preinst-b.txt file not present, test failed\"\n"
            "  EXIT 1\n"
            ")\n"
        );
        postinst.write_file(wpkg_path_t1.append_child("postinst.bat"), true);
        prerm.printf(
            "REM Test to know that t1 prerm ran\n"
            "ECHO prerm(b): called with: [%%*]\n"
            "ECHO pre-rm ctrl_t1 (b) > prerm-b.txt\n"
            "ECHO arguments: [%%*] >> prerm-b.txt\n"
        );
        prerm.write_file(wpkg_path_t1.append_child("prerm.bat"), true);
        postrm.printf(
            "REM Test to know that t1 postrm ran\n"
            "ECHO postrm(b): called with: [%%*]\n"
            "ECHO post-rm ctrl_t1 (b) > postrm-b.txt\n"
            "ECHO arguments: [%%*] >> postrm-b.txt\n"
        );
        postrm.write_file(wpkg_path_t1.append_child("postrm.bat"), true);
#else
        preinst.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t1 (b) preinst ran\n"
            "echo \"preinst(b): called with: [$*]\"\n"
            "echo \"pre-inst ctrl_t1 (b)\" > preinst-b.txt\n"
            "echo \"arguments: [$*]\" >> preinst-b.txt\n"
            "if test -f prerm.txt\n"
            "then\n"
            "  echo \"  preinst(b): t1 prerm ran as expected\"\n"
            "  exit 0\n"
            "else\n"
            "  echo \"  preinst(b): t1 prerm.txt file not present, test failed\"\n"
            "  exit 1\n"
            "fi\n"
        );
        preinst.write_file(wpkg_path_t1.append_child("preinst"), true);
        postinst.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t1 postinst ran\n"
            "echo \"postinst(b): called with: [$*]\"\n"
            "echo \"post-inst ctrl_t1 (b)\" > postinst-b.txt\n"
            "echo \"arguments: [$*]\" >> postinst-b.txt\n"
            "if test -f preinst-b.txt\n"
            "then\n"
            "  echo \"  postinst: t1(b) preinst ran as expected\"\n"
            "  exit 0\n"
            "else\n"
            "  echo \"  postinst: t1(b) preinst file not present, test failed\"\n"
            "  exit 1\n"
            "fi\n"
        );
        postinst.write_file(wpkg_path_t1.append_child("postinst"), true);
        prerm.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t1(b) prerm ran\n"
            "echo \"prerm(b): called with: [$*]\"\n"
            "if test -f *.txt\n"
            "then\n"
            "  echo \"  prerm: t1(b) prerm found unexpected .txt files\"\n"
            "  exit 1\n"
            "fi\n"
            "echo \"pre-rm ctrl_t1 (b)\" > prerm-b.txt\n"
            "echo \"arguments: [$*]\" >> prerm-b.txt\n"
        );
        prerm.write_file(wpkg_path_t1.append_child("prerm"), true);
        postrm.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t1(b) postrm ran\n"
            "echo \"postrm(b): called with: [$*]\"\n"
            "echo \"post-rm ctrl_t1 (b)\" > postrm-b.txt\n"
            "echo \"arguments: [$*]\" >> postrm-b.txt\n"
            "if test -f prerm-b.txt\n"
            "then\n"
            "  echo \"  postrm: t1(b) prerm ran as expected\"\n"
            "  exit 0\n"
            "else\n"
            "  echo \"  postrm: t1(b) prerm file not present, test failed\"\n"
            "  exit 1\n"
            "fi\n"
        );
        postrm.write_file(wpkg_path_t1.append_child("postrm"), true);
#endif
        create_package("t1", ctrl_t1, false);
        install_package("t1", ctrl_t1);
        verify_installed_files("t1");

        // verify that each script created the file we expect
        verify_file_vector_t files;
        verify_file_t f;
        f.f_filename = "preinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "preinst-b.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "pre-inst ctrl_t1 (b)\n"
                   "arguments: [upgrade 1.0]";
        files.push_back(f);
        f.f_filename = "postinst-b.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "post-inst ctrl_t1 (b)\n"
                   "arguments: [configure 1.1]";
        files.push_back(f);
        f.f_filename = "prerm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "pre-rm ctrl_t1\n"
                   "arguments: [upgrade 1.1]";
        files.push_back(f);
        f.f_filename = "postrm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "post-rm ctrl_t1\n"
                   "arguments: [upgrade 1.1]";
        files.push_back(f);
        verify_generated_files(files);

        // remove the result
        remove_package("t1", ctrl_t1);
        verify_removed_files("t1", ctrl_t1);

        // verify that each script created the file we expect
        files.clear();
        f.clear();
        f.f_filename = "preinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "prerm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postrm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "preinst-b.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postinst-b.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);

        f.f_filename = "prerm-b.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "pre-rm ctrl_t1 (b)\n"
                   "arguments: [remove]";
        files.push_back(f);
        f.f_filename = "postrm-b.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "post-rm ctrl_t1 (b)\n"
                   "arguments: [remove]";
        files.push_back(f);
        verify_generated_files(files);
    }

////////////////////// t2 -- upgrade from a package without any scripts to a package with full scripts
    wpkg_filename::uri_filename build_path_t2(root.append_child("t2"));
    wpkg_filename::uri_filename wpkg_path_t2(build_path_t2.append_child("WPKG"));

    // create a first version of the package
    {
        std::shared_ptr<wpkg_control::control_file> ctrl_t2(get_new_control_file(__FUNCTION__));
        ctrl_t2->set_field("Version", "2.0");
        ctrl_t2->set_field("Files", "conffiles\n"
            "/usr/bin/t2 0123456789abcdef0123456789abcdef\n"
            "/usr/share/doc/t2/copyright 0123456789abcdef0123456789abcdef\n"
        );

        create_package("t2", ctrl_t2, false);
        install_package("t2", ctrl_t2);
        verify_installed_files("t2");

        // verify that each script created the file we expect
        verify_file_vector_t files;
        verify_file_t f;
        f.f_filename = "preinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "prerm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postrm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        verify_generated_files(files);
    }

    // create an upgrade
    {
        std::shared_ptr<wpkg_control::control_file> ctrl_t2(get_new_control_file(__FUNCTION__));
        ctrl_t2->set_field("Version", "2.1");
        ctrl_t2->set_field("Files", "conffiles\n"
            "/usr/bin/t2 0123456789abcdef0123456789abcdef\n"
            "/usr/share/doc/t2/copyright 0123456789abcdef0123456789abcdef\n"
        );
        // destroy the previous version
        create_package("t2", ctrl_t2);

        memfile::memory_file preinst;
        preinst.create(memfile::memory_file::file_format_other);
        memfile::memory_file postinst;
        postinst.create(memfile::memory_file::file_format_other);
        memfile::memory_file prerm;
        prerm.create(memfile::memory_file::file_format_other);
        memfile::memory_file postrm;
        postrm.create(memfile::memory_file::file_format_other);

#ifdef MO_WINDOWS
        preinst.printf(
            "REM Test to know whether t2(b) preinst ran\n"
            "ECHO preinst(b): t2 called with: [%%*]\n"
            "ECHO pre-inst ctrl_t2 (b) > preinst-b.txt\n"
            "ECHO arguments: [%%*] >> preinst-b.txt\n"
        );
        preinst.write_file(wpkg_path_t2.append_child("preinst.bat"), true);
        postinst.printf(
            "REM Test to know that t2 postinst ran\n"
            "ECHO postinst(c): called with: [%%*]\n"
            "ECHO post-inst ctrl_t2 (c) > postinst-c.txt\n"
            "ECHO arguments: [%%*] >> postinst-c.txt\n"
            "IF EXIST preinst-b.txt (\n"
            "  ECHO \"t2(c) preinst ran as expected\"\n"
            "  EXIT 0\n"
            ") ELSE (\n"
            "  ECHO \"t2(c) preinst.txt file not present, test failed\"\n"
            "  EXIT 1\n"
            ")\n"
        );
        postinst.write_file(wpkg_path_t2.append_child("postinst.bat"), true);
        prerm.printf(
            "REM Test to know that t2(b) prerm ran\n"
            "ECHO prerm(b): called with: [%%*]\n"
            "ECHO pre-rm ctrl_t2 (b) > prerm-b.txt\n"
            "ECHO arguments: [%%*] >> prerm-b.txt\n"
        );
        prerm.write_file(wpkg_path_t2.append_child("prerm.bat"), true);
        postrm.printf(
            "REM Test to know that t2 postrm ran\n"
            "ECHO postrm(b): called with: [%%*]\n"
            "ECHO post-rm ctrl_t2 (b) > postrm-b.txt\n"
            "ECHO arguments: [%%*] >> postrm-b.txt\n"
        );
        postrm.write_file(wpkg_path_t2.append_child("postrm.bat"), true);
#else
        preinst.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t2 (b) preinst ran\n"
            "echo \"preinst(b): t2 called with: [$*]\"\n"
            "if test -f *.txt\n"
            "then\n"
            "  echo \"  preinst(b): t2 preinst file detected other unexpected files\"\n"
            "  exit 1\n"
            "else\n"
            "  echo \"  preinst(b): t2 preinst ran first as expected\"\n"
            "fi\n"
            "echo \"pre-inst ctrl_t2 (b)\" > preinst-b.txt\n"
            "echo \"arguments: [$*]\" >> preinst-b.txt\n"
        );
        preinst.write_file(wpkg_path_t2.append_child("preinst"), true);
        postinst.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t2 postinst ran\n"
            "echo \"postinst(c): called with: [$*]\"\n"
            "echo \"post-inst ctrl_t2 (c)\" > postinst-c.txt\n"
            "echo \"arguments: [$*]\" >> postinst-c.txt\n"
            "if test -f preinst-b.txt\n"
            "then\n"
            "  echo \"  postinst: t2(c) preinst ran as expected\"\n"
            "  exit 0\n"
            "else\n"
            "  echo \"  postinst: t2(c) preinst.txt file not present, test failed\"\n"
            "  exit 1\n"
            "fi\n"
        );
        postinst.write_file(wpkg_path_t2.append_child("postinst"), true);
        prerm.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t2(b) prerm ran\n"
            "echo \"prerm(b): called with: [$*]\"\n"
            "if test -f *.txt\n"
            "then\n"
            "  echo \"  prerm: t2(b) prerm found unexpected .txt files\"\n"
            "  exit 1\n"
            "fi\n"
            "echo \"pre-rm ctrl_t2 (b)\" > prerm-b.txt\n"
            "echo \"arguments: [$*]\" >> prerm-b.txt\n"
        );
        prerm.write_file(wpkg_path_t2.append_child("prerm"), true);
        postrm.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t2(b) postrm ran\n"
            "echo \"postrm(b): called with: [$*]\"\n"
            "echo \"post-rm ctrl_t2 (b)\" > postrm-b.txt\n"
            "echo \"arguments: [$*]\" >> postrm-b.txt\n"
            "if test -f prerm-b.txt\n"
            "then\n"
            "  echo \"  postrm: t2(b) prerm ran as expected\"\n"
            "  exit 0\n"
            "else\n"
            "  echo \"  postrm: t2(b) prerm file not present, test failed\"\n"
            "  exit 1\n"
            "fi\n"
        );
        postrm.write_file(wpkg_path_t2.append_child("postrm"), true);
#endif
        create_package("t2", ctrl_t2, false);
        install_package("t2", ctrl_t2);
        verify_installed_files("t2");

        // verify that each script created the file we expect
        verify_file_vector_t files;
        verify_file_t f;
        f.f_filename = "preinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "preinst-b.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "pre-inst ctrl_t2 (b)\n"
                   "arguments: [upgrade 2.0]";
        files.push_back(f);
        f.f_filename = "postinst-c.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "post-inst ctrl_t2 (c)\n"
                   "arguments: [configure 2.1]";
        files.push_back(f);
        f.f_filename = "prerm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postrm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        verify_generated_files(files);

        // remove the result
        remove_package("t2", ctrl_t2);
        verify_removed_files("t2", ctrl_t2);

        // verify that each script created the file we expect
        files.clear();
        f.clear();
        f.f_filename = "preinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "prerm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postrm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "preinst-b.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postinst-b.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);

        f.f_filename = "prerm-b.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "pre-rm ctrl_t2 (b)\n"
                   "arguments: [remove]";
        files.push_back(f);
        f.f_filename = "postrm-b.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "post-rm ctrl_t2 (b)\n"
                   "arguments: [remove]";
        files.push_back(f);
        verify_generated_files(files);
    }

////////////////////// t3 -- upgrade from a package without any scripts to a package with full scripts
    wpkg_filename::uri_filename build_path_t3(root.append_child("t3"));
    wpkg_filename::uri_filename wpkg_path_t3(build_path_t3.append_child("WPKG"));

    // create a first version of the package
    {
        std::shared_ptr<wpkg_control::control_file> ctrl_t3(get_new_control_file(__FUNCTION__));
        ctrl_t3->set_field("Version", "3.0");
        ctrl_t3->set_field("Files", "conffiles\n"
            "/usr/bin/t3 0123456789abcdef0123456789abcdef\n"
            "/usr/share/doc/t3/copyright 0123456789abcdef0123456789abcdef\n"
        );

        memfile::memory_file preinst;
        preinst.create(memfile::memory_file::file_format_other);
        memfile::memory_file postinst;
        postinst.create(memfile::memory_file::file_format_other);
        memfile::memory_file prerm;
        prerm.create(memfile::memory_file::file_format_other);
        memfile::memory_file postrm;
        postrm.create(memfile::memory_file::file_format_other);

#ifdef MO_WINDOWS
        preinst.printf(
            "REM Test to know whether t3 preinst ran\n"
            "ECHO preinst: t3 called with: [%%*]\n"
            "ECHO pre-inst ctrl_t3 > preinst.txt\n"
            "ECHO arguments: [%%*] >> preinst.txt\n"
        );
        preinst.write_file(wpkg_path_t3.append_child("preinst.bat"), true);
        postinst.printf(
            "REM Test to know that t3 postinst ran\n"
            "ECHO postinst: called with: [%%*]\n"
            "ECHO post-inst ctrl_t3 > postinst.txt\n"
            "ECHO arguments: [%%*] >> postinst.txt\n"
            "IF EXIST preinst.txt (\n"
            "  ECHO   postinst: t3 preinst ran as expected\n"
            "  EXIT 0\n"
            ") ELSE (\n"
            "  ECHO   postinst: t3 preinst file not present, test failed\n"
            "  EXIT 1\n"
            ")\n"
        );
        postinst.write_file(wpkg_path_t3.append_child("postinst.bat"), true);
        prerm.printf(
            "REM Test to know that t3 prerm ran\n"
            "ECHO prerm: called with: [%%*]\n"
            "ECHO pre-rm ctrl_t3 > prerm-b.txt\n"
            "ECHO arguments: [%%*] >> prerm-b.txt\n"
        );
        prerm.write_file(wpkg_path_t3.append_child("prerm.bat"), true);
        postrm.printf(
            "REM Test to know that t3 postrm ran\n"
            "ECHO postrm: called with: [%%*]\n"
            "ECHO post-rm ctrl_t3 > postrm-b.txt\n"
            "ECHO arguments: [%%*] >> prerm-b.txt\n"
        );
        postrm.write_file(wpkg_path_t3.append_child("postrm.bat"), true);
#else
        preinst.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t3 preinst ran\n"
            "echo \"preinst: t3 called with: [$*]\"\n"
            "if test -f *.txt\n"
            "then\n"
            "  echo \"  preinst: t3 preinst file detected other unexpected files\"\n"
            "  exit 1\n"
            "else\n"
            "  echo \"  preinst: t3 preinst ran first as expected\"\n"
            "fi\n"
            "echo \"pre-inst ctrl_t3\" > preinst.txt\n"
            "echo \"arguments: [$*]\" >> preinst.txt\n"
        );
        preinst.write_file(wpkg_path_t3.append_child("preinst"), true);
        postinst.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t3 postinst ran\n"
            "echo \"postinst: called with: [$*]\"\n"
            "echo \"post-inst ctrl_t3\" > postinst.txt\n"
            "echo \"arguments: [$*]\" >> postinst.txt\n"
            "if test -f preinst.txt\n"
            "then\n"
            "  echo \"  postinst: t3 preinst ran as expected\"\n"
            "  exit 0\n"
            "else\n"
            "  echo \"  postinst: t3 preinst file not present, test failed\"\n"
            "  exit 1\n"
            "fi\n"
        );
        postinst.write_file(wpkg_path_t3.append_child("postinst"), true);
        prerm.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t3 prerm ran\n"
            "echo \"prerm: called with: [$*]\"\n"
            "if test -f *.txt\n"
            "then\n"
            "  echo \"  prerm: t3 prerm found unexpected .txt files\"\n"
            "  exit 1\n"
            "fi\n"
            "echo \"pre-rm ctrl_t3\" > prerm-b.txt\n"
            "echo \"arguments: [$*]\" >> prerm-b.txt\n"
        );
        prerm.write_file(wpkg_path_t3.append_child("prerm"), true);
        postrm.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t3 postrm ran\n"
            "echo \"postrm: called with: [$*]\"\n"
            "echo \"post-rm ctrl_t3\" > postrm-b.txt\n"
            "echo \"arguments: [$*]\" >> postrm-b.txt\n"
            "if test -f prerm-b.txt\n"
            "then\n"
            "  echo \"  postrm: t3 prerm ran as expected\"\n"
            "  exit 0\n"
            "else\n"
            "  echo \"  postrm: t3 prerm file not present, test failed\"\n"
            "  exit 1\n"
            "fi\n"
        );
        postrm.write_file(wpkg_path_t3.append_child("postrm"), true);
#endif
        create_package("t3", ctrl_t3, false);
        install_package("t3", ctrl_t3);
        verify_installed_files("t3");

        // verify that each script created the file we expect
        verify_file_vector_t files;
        verify_file_t f;
        f.f_filename = "preinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "pre-inst ctrl_t3\n"
                   "arguments: [install]";
        files.push_back(f);
        f.f_filename = "postinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_text); // FIXME cast
        f.f_data = "post-inst ctrl_t3\n"
                   "arguments: [configure 3.0]";
        files.push_back(f);
        f.f_filename = "prerm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postrm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        verify_generated_files(files);
    }

    // create an upgrade
    {
        std::shared_ptr<wpkg_control::control_file> ctrl_t3(get_new_control_file(__FUNCTION__));
        ctrl_t3->set_field("Version", "3.1");
        ctrl_t3->set_field("Files", "conffiles\n"
            "/usr/bin/t3 0123456789abcdef0123456789abcdef\n"
            "/usr/share/doc/t3/copyright 0123456789abcdef0123456789abcdef\n"
        );
        // destroy the previous version
        create_package("t3", ctrl_t3);
        install_package("t3", ctrl_t3);
        verify_installed_files("t3");

        // verify that each script created the file we expect
        verify_file_vector_t files;
        verify_file_t f;
        f.f_filename = "preinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "prerm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postrm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        verify_generated_files(files);

        // remove the result
        remove_package("t3", ctrl_t3);
        verify_removed_files("t3", ctrl_t3);

        // verify that each script created the file we expect
        files.clear();
        f.clear();
        f.f_filename = "preinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postinst.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "prerm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        f.f_filename = "postrm.txt";
        f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
        files.push_back(f);
        verify_generated_files(files);
    }
}

void PackageUnitTests::scripts_order_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    scripts_order();
}

void PackageUnitTests::compare_versions()
{
    struct version_t
    {
        const char * const  f_left;
        const char * const  f_right;
        int                 f_results[10];
    };
    const version_t versions[] = {
        //    l         r        << -nl  <= -nl  ==  !=  >= -nl  >> -nl
        { "",          "",      { 1,  1,  0,  0,  0,  1,  0,  0,  1,  1 } },
        { "",       "0.9",      { 0,  1,  0,  1,  1,  0,  1,  0,  1,  0 } },
        { "1.0",       "",      { 1,  0,  1,  0,  1,  0,  0,  1,  0,  1 } },
        { "1.0",    "0.9",      { 1,  1,  1,  1,  1,  0,  0,  0,  0,  0 } },
        { "0.9",    "1.0",      { 0,  0,  0,  0,  1,  0,  1,  1,  1,  1 } },
        { "1.0",    "1.0",      { 1,  1,  0,  0,  0,  1,  0,  0,  1,  1 } },
        { "1b",     "1a",       { 1,  1,  1,  1,  1,  0,  0,  0,  0,  0 } },
        { "1a",     "1b",       { 0,  0,  0,  0,  1,  0,  1,  1,  1,  1 } },
        { "1a~",    "1a~",      { 1,  1,  0,  0,  0,  1,  0,  0,  1,  1 } },
        { "1a",     "1a~",      { 1,  1,  1,  1,  1,  0,  0,  0,  0,  0 } },
        { "1a~",    "1a",       { 0,  0,  0,  0,  1,  0,  1,  1,  1,  1 } },
        { "1.0",    "1.a",      { 0,  0,  0,  0,  1,  0,  1,  1,  1,  1 } },
        { "1.0",    "1.+",      { 0,  0,  0,  0,  1,  0,  1,  1,  1,  1 } },
        { "1.0",    "1.--0",    { 0,  0,  0,  0,  1,  0,  1,  1,  1,  1 } },
        { "1.+",    "1.--0",    { 0,  0,  0,  0,  1,  0,  1,  1,  1,  1 } },
        { "1+",     "1--0",     { 0,  0,  0,  0,  1,  0,  1,  1,  1,  1 } },
        { "1.3a+",  "1.3a--0",  { 0,  0,  0,  0,  1,  0,  1,  1,  1,  1 } },
        { "3.5-10", "3.5-5",    { 1,  1,  1,  1,  1,  0,  0,  0,  0,  0 } },
        { "3.5-20", "3.5-15",   { 1,  1,  1,  1,  1,  0,  0,  0,  0,  0 } },
        { "3.5-2",  "3.5-15",   { 0,  0,  0,  0,  1,  0,  1,  1,  1,  1 } },
        { "3:5.2",  "3:5.02",   { 1,  1,  0,  0,  0,  1,  0,  0,  1,  1 } },
        { "3:5.9",  "3:5.09",   { 1,  1,  0,  0,  0,  1,  0,  0,  1,  1 } },
        { "2:5.9",  "3:5.09",   { 0,  0,  0,  0,  1,  0,  1,  1,  1,  1 } },
        { "4:5.9",  "3:5.09",   { 1,  1,  1,  1,  1,  0,  0,  0,  0,  0 } },
        { "7:5.9",  "7:5:9",    { 0,  0,  0,  0,  1,  0,  1,  1,  1,  1 } },
    };
    const size_t versions_size = sizeof(versions) / sizeof(versions[0]);
    const char *ops[3][10] = {
        { "<<", "lt-nl", "<=", "le-nl",  "=", "!=", ">=", "ge-nl", ">>", "gt-nl" },
        { "lt", "lt-nl", "le", "le-nl", "eq", "ne", "ge", "ge-nl", "gt", "gt-nl" },
        {  "<", "lt-nl", "<=", "le-nl", "==", "<>", ">=", "ge-nl",  ">", "gt-nl" }
    };

#if defined(MO_WINDOWS)
    const char quote('\"');
#else
    const char quote('\'');
#endif
    for(size_t i(0); i < versions_size; ++i)
    {
        for(size_t j(0); j < 3; ++j)
        {
            for(size_t k(0); k < 10; ++k)
            {
                std::string cmd(unittest::wpkg_tool);
                cmd += " --compare-versions ";
                if(*versions[i].f_left == '\0')
                {
                    cmd += quote;
                    cmd += quote;
                }
                else
                {
                    cmd += versions[i].f_left;
                }
                cmd += " ";
                cmd += quote;
                cmd += ops[j][k];
                cmd += quote;
                cmd += " ";
                if(*versions[i].f_right == '\0')
                {
                    cmd += quote;
                    cmd += quote;
                }
                else
                {
                    cmd += versions[i].f_right;
                }

                int r(system(cmd.c_str()));
                int result(WEXITSTATUS(r));
                CPPUNIT_ASSERT_MESSAGE(cmd + " completely failed", result == 0 || result == 1);
                std::stringstream msg;
                msg << cmd << " result: " << result << " (expected: " << versions[i].f_results[k] << ")";
                CPPUNIT_ASSERT_MESSAGE(msg.str(), versions[i].f_results[k] == result);
            }
        }
    }
}

void PackageUnitTests::compare_versions_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    compare_versions();
}



void PackageUnitTests::auto_upgrade()
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename root(unittest::tmp_dir);
    //wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    // create a package that will be auto-upgraded
    std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__ + std::string(" t1")));
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t1->set_field("Files", "conffiles\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1);
    // Conffiles -- the create_package deletes this field
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );

    // the first install call is expected to work as is, no problems
    install_package("t1", ctrl_t1, 0);
    verify_installed_files("t1");

    // create a package that we'll mark for hold
    std::shared_ptr<wpkg_control::control_file> ctrl_t1_15(get_new_control_file(__FUNCTION__ + std::string(" t1 v1.5")));
    ctrl_t1_15->set_field("Version", "1.5");
    ctrl_t1_15->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t1_15->set_field("Files", "conffiles\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/changes_in_15 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1_15);
    // Conffiles -- the create_package deletes this field
    ctrl_t1_15->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );

    // Now create t2 with t1 as a dependency that needs to be auto-upgraded
    std::shared_ptr<wpkg_control::control_file> ctrl_t2(get_new_control_file(__FUNCTION__ + std::string(" t2")));
    ctrl_t2->set_field("Depends", "t1 (= 1.5)");
    ctrl_t2->set_field("Conffiles", "\n"
        "/etc/t2/t2.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t2->set_field("Files", "conffiles\n"
        "/etc/t2/t2.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t2 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t2/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t2", ctrl_t2);
    // Conffiles -- the create_package deletes this field
    ctrl_t2->set_field("Conffiles", "\n"
        "/etc/t2/t2.conf 0123456789abcdef0123456789abcdef"
    );

    // the first install call is expected to work as is, no problems
    ctrl_t2->set_variable("INSTALL_POSTOPTIONS", "--repository " + wpkg_util::make_safe_console_string(repository.path_only()));
    install_package("t2", ctrl_t2, 0);
    verify_installed_files("t2");
}

void PackageUnitTests::auto_upgrade_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    auto_upgrade();
}

void PackageUnitTests::auto_downgrade()
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename root(unittest::tmp_dir);
    //wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    // create a package that will be viewed as an auto-downgrad
    std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__ + std::string(" t1")));
    ctrl_t1->set_field("Version", "1.9");
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t1->set_field("Files", "conffiles\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1);
    // Conffiles -- the create_package deletes this field
    ctrl_t1->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );

    // the first install call is expected to work as is, no problems
    install_package("t1", ctrl_t1, 0);
    verify_installed_files("t1");

    // create a package that we'll mark for hold
    std::shared_ptr<wpkg_control::control_file> ctrl_t1_12(get_new_control_file(__FUNCTION__ + std::string(" t1 v1.2")));
    ctrl_t1_12->set_field("Version", "1.2");
    ctrl_t1_12->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t1_12->set_field("Files", "conffiles\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/changes_in_15 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1_12);
    // Conffiles -- the create_package deletes this field
    ctrl_t1_12->set_field("Conffiles", "\n"
        "/etc/t1/t1.conf 0123456789abcdef0123456789abcdef"
    );

    // Now create t2 with t1 as a dependency that needs to be auto-upgraded
    std::shared_ptr<wpkg_control::control_file> ctrl_t2(get_new_control_file(__FUNCTION__ + std::string(" t2")));
    ctrl_t2->set_field("Depends", "t1 (= 1.2)");
    ctrl_t2->set_field("Conffiles", "\n"
        "/etc/t2/t2.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_t2->set_field("Files", "conffiles\n"
        "/etc/t2/t2.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t2 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t2/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t2", ctrl_t2);
    // Conffiles -- the create_package deletes this field
    ctrl_t2->set_field("Conffiles", "\n"
        "/etc/t2/t2.conf 0123456789abcdef0123456789abcdef"
    );

    // the first install call is expected to work as is, no problems
    ctrl_t2->set_variable("INSTALL_POSTOPTIONS", "--repository " + wpkg_util::make_safe_console_string(wpkg_util::make_safe_console_string(repository.path_only())));
    install_package("t2", ctrl_t2, 1);
    verify_purged_files("t2", ctrl_t2);
}

void PackageUnitTests::auto_downgrade_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    auto_downgrade();
}


void PackageUnitTests::test_hold()
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    // create a package that we'll mark for hold
    std::shared_ptr<wpkg_control::control_file> ctrl_held(get_new_control_file(__FUNCTION__));
    ctrl_held->set_field("Conffiles", "\n"
        "/etc/held/held.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_held->set_field("Files", "conffiles\n"
        "/etc/held/held.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/held 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/held/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("held", ctrl_held);
    // Conffiles -- the create_package deletes this field
    ctrl_held->set_field("Conffiles", "\n"
        "/etc/held/held.conf 0123456789abcdef0123456789abcdef"
    );

    // the first install call is expected to work as is, no problems
    install_package("held", ctrl_held, 0);
    verify_installed_files("held");

    // now we want to mark the package for hold
    std::string cmd(unittest::wpkg_tool);
    cmd += " --root " + wpkg_util::make_safe_console_string(target_path.path_only());
    cmd += " --set-selection hold held";
    printf("Set Selection Command: \"%s\"\n", cmd.c_str());
    fflush(stdout);
    int r(system(cmd.c_str()));
    printf("  Set selection result = %d (expected 0)\n", WEXITSTATUS(r));
    CPPUNIT_ASSERT(WEXITSTATUS(r) == 0);

    // create a package that we'll mark for hold
    std::shared_ptr<wpkg_control::control_file> ctrl_held15(get_new_control_file(__FUNCTION__));
    ctrl_held15->set_field("Version", "1.5");
    ctrl_held15->set_field("Conffiles", "\n"
        "/etc/held/held.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_held15->set_field("Files", "conffiles\n"
        "/etc/held/held.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/held 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/held/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/held/changes_in_15 0123456789abcdef0123456789abcdef\n"
    );
    create_package("held", ctrl_held15);
    // Conffiles -- the create_package deletes this field
    ctrl_held15->set_field("Conffiles", "\n"
        "/etc/held/held.conf 0123456789abcdef0123456789abcdef"
    );

    // the first install call is expected to work as is, no problems
    install_package("held", ctrl_held15, 1);

    //verify_installed_files("held"); -- the install of 1.5 fails, but the
    // files of 1.0 are still installed... instead we use the
    // verify_generated_files() since it has not side effects over
    // non-existing files:
    verify_file_vector_t files;
    verify_file_t f;
    f.f_filename = "usr/share/doc/held/changes_in_15";
    f.f_mode = static_cast<int>(verify_file_t::verify_deleted); // FIXME cast
    files.push_back(f);
    verify_generated_files(files);

    // Now try again with held 1.5 as an implicit package
    std::shared_ptr<wpkg_control::control_file> ctrl_friend(get_new_control_file(__FUNCTION__));
    ctrl_friend->set_field("Depends", "held (= 1.5)");
    ctrl_friend->set_field("Conffiles", "\n"
        "/etc/friend/friend.conf 0123456789abcdef0123456789abcdef"
    );
    ctrl_friend->set_field("Files", "conffiles\n"
        "/etc/friend/friend.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/friend 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/friend/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("friend", ctrl_friend);
    // Conffiles -- the create_package deletes this field
    ctrl_friend->set_field("Conffiles", "\n"
        "/etc/friend/friend.conf 0123456789abcdef0123456789abcdef"
    );

    // this install does not work because of the selection that's on Hold
    ctrl_friend->set_variable("INSTALL_POSTOPTIONS", "--repository " + wpkg_util::make_safe_console_string(wpkg_util::make_safe_console_string(repository.path_only())));
    install_package("friend", ctrl_friend, 1);
    verify_purged_files("friend", ctrl_friend);

    // the --force-hold does NOT help installing friend because the problem
    // is with the implicit dependency
    ctrl_friend->set_variable("INSTALL_PREOPTIONS", "--force-hold");
    install_package("friend", ctrl_friend, 1);
    verify_purged_files("friend", ctrl_friend);

    // the --force-hold on the held package itself works, however
    ctrl_held15->set_variable("INSTALL_PREOPTIONS", "--force-hold");
    install_package("held", ctrl_held15, 0);
    verify_installed_files("held");

    // now we can install friend without any addition parameters
    ctrl_friend->delete_variable("INSTALL_PREOPTIONS");
    ctrl_friend->delete_variable("INSTALL_POSTOPTIONS");
    install_package("friend", ctrl_friend, 0);
    verify_installed_files("friend");

    // now we can do a recursive remove,
    // but without the --force-hold it will fail
    ctrl_held15->set_variable("REMOVE_PREOPTIONS", "--recursive");
    remove_package("held", ctrl_held15, 1);
    verify_installed_files("held");
    verify_installed_files("friend");

    // try again with the --force-hold
    ctrl_held15->set_variable("REMOVE_POSTOPTIONS", "--force-hold");
    remove_package("held", ctrl_held15, 0);
    verify_removed_files("held", ctrl_held15);
    verify_removed_files("friend", ctrl_friend);
}

void PackageUnitTests::test_hold_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    test_hold();
}


void PackageUnitTests::minimum_upgradable_version()
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename root(unittest::tmp_dir);
    //wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    // create a package with a very old version (1.0)
    std::shared_ptr<wpkg_control::control_file> ctrl_t1_10(get_new_control_file(__FUNCTION__ + std::string(" t1 v1.0")));
    ctrl_t1_10->set_field("Files", "conffiles\n"
        "/usr/bin/minimum 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/minimum/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1_10);

    // the first install call is expected to work as is, no problems
    install_package("t1", ctrl_t1_10, 0);
    verify_installed_files("t1");

    // create a package with a new version that require a minimum version
    // in the old world to do the upgrade (i.e. need version 1.51 for upgrade)
    std::shared_ptr<wpkg_control::control_file> ctrl_t1_20(get_new_control_file(__FUNCTION__ + std::string(" t1 v2.0")));
    ctrl_t1_20->set_field("Version", "2.0");
    ctrl_t1_20->set_field("Minimum-Upgradable-Version", "1.51");
    ctrl_t1_20->set_field("Files", "conffiles\n"
        "/usr/bin/minimum2 123456789abcdef0123456789abcdef0\n"
        "/usr/share/doc/minimum2/copyright 123456789abcdef0123456789abcdef0\n"
    );
    create_package("t1", ctrl_t1_20);

    // installing this version now fails
    install_package("t1", ctrl_t1_20, 1);
    verify_purged_files("t1", ctrl_t1_20);

    // so create a package version 1.51 and install it first
    std::shared_ptr<wpkg_control::control_file> ctrl_t1_151(get_new_control_file(__FUNCTION__ + std::string(" t1 v1.51")));
    ctrl_t1_151->set_field("Version", "1.51");
    ctrl_t1_151->set_field("Files", "conffiles\n"
        "/usr/bin/minimum 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/minimum/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1_151);

    // install that 1.51 version
    install_package("t1", ctrl_t1_151, 0);
    verify_installed_files("t1");

    // now we expect this install to succeed
    install_package("t1", ctrl_t1_20, 0);
    //verify_installed_files("t1"); -- 2.0 files got overwritten in tmp/t1/...
    verify_purged_files("t1", ctrl_t1_10);

    // try again, but this time with the --force-upgrade-any-version
    purge_package("t1", ctrl_t1_20, 0);
    install_package("t1", ctrl_t1_10, 0);
    install_package("t1", ctrl_t1_20, 1); // fail again!
    ctrl_t1_20->set_variable("INSTALL_PREOPTIONS", "--force-upgrade-any-version");
    install_package("t1", ctrl_t1_20, 0); // forced, shown a warning only
}

void PackageUnitTests::minimum_upgradable_version_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    minimum_upgradable_version();
}

void PackageUnitTests::check_drive_subst()
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename root(unittest::tmp_dir);

    // create a package
    std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__ + std::string(" t1")));
    ctrl_t1->set_field("Files", "conffiles\n"
        "/usr/bin/subst-test 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/subst/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1);

    // invalid pipe (we support only one)
#if defined(MO_WINDOWS)
    ctrl_t1->set_field("PRE_COMMAND", "set WPKG_SUBST=\"f=/opt/wpkg|/m2osw/packages|/only/one/pipe/allowed:h=usr/local/bin/wpkg\"");
#else
    ctrl_t1->set_field("PRE_COMMAND", "export WPKG_SUBST='f=/opt/wpkg|/m2osw/packages|/only/one/pipe/allowed:h=usr/local/bin/wpkg'");
#endif
    ctrl_t1->set_variable("INSTALL_PREOPTIONS", "--repository f:this-file");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // invalid character in directory path (*)
#if defined(MO_WINDOWS)
    ctrl_t1->set_field("PRE_COMMAND", "set WPKG_SUBST=\"f=/opt/wpkg|/m2osw*/packages:h=usr/local/bin/wpkg\"");
#else
    ctrl_t1->set_field("PRE_COMMAND", "export WPKG_SUBST='f=/opt/wpkg|/m2osw*/packages:h=usr/local/bin/wpkg'");
#endif
    ctrl_t1->set_variable("INSTALL_PREOPTIONS", "--repository f:this-file");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // invalid character in subst path (*)
#if defined(MO_WINDOWS)
    ctrl_t1->set_field("PRE_COMMAND", "set WPKG_SUBST=\"f=/opt/wpkg*|/m2osw/packages:h=usr/local/bin/wpkg\"");
#else
    ctrl_t1->set_field("PRE_COMMAND", "export WPKG_SUBST='f=/opt/wpkg*|/m2osw/packages:h=usr/local/bin/wpkg'");
#endif
    ctrl_t1->set_variable("INSTALL_PREOPTIONS", "--repository f:this-file");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // invalid character in directory path (?)
#if defined(MO_WINDOWS)
    ctrl_t1->set_field("PRE_COMMAND", "set WPKG_SUBST=\"f=/opt/wpkg///|/m2osw/pack?ages:h=usr/local/bin/wpkg\"");
#else
    ctrl_t1->set_field("PRE_COMMAND", "export WPKG_SUBST='f=/opt/wpkg/\\/|/m2osw/pack?ages:h=usr/local/bin/wpkg'");
#endif
    ctrl_t1->set_variable("INSTALL_PREOPTIONS", "--repository f:this-file");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // invalid character in subst path (?)
#if defined(MO_WINDOWS)
    ctrl_t1->set_field("PRE_COMMAND", "set WPKG_SUBST=\"f=///opt///wp?kg|/m2osw/packages:h=usr/local/bin/wpkg\"");
#else
    ctrl_t1->set_field("PRE_COMMAND", "export WPKG_SUBST='f=/opt/wp?kg|/m2osw/packages:h=usr/local/bin/wpkg'");
#endif
    ctrl_t1->set_variable("INSTALL_PREOPTIONS", "--repository f:this-file");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // invalid character in directory path (")
#if defined(MO_WINDOWS)
    ctrl_t1->set_field("PRE_COMMAND", "set WPKG_SUBST=\"f=/opt/wpkg|/m2osw\\\\packages\"\":h=usr/local/bin/wpkg\"");
#else
    ctrl_t1->set_field("PRE_COMMAND", "export WPKG_SUBST='f=/opt/wpkg|/m2osw\\\\packages\":h=usr/local/bin/wpkg'");
#endif
    ctrl_t1->set_variable("INSTALL_PREOPTIONS", "--repository f:this-file");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // invalid character in subst path (")
#if defined(MO_WINDOWS)
    ctrl_t1->set_field("PRE_COMMAND", "set WPKG_SUBST=\"f=/opt\\\\wpkg\"\"|/m2osw/packages:h=usr/local/bin/wpkg\"");
#else
    ctrl_t1->set_field("PRE_COMMAND", "export WPKG_SUBST='f=/opt\\\\wpkg\"|/m2osw/packages:h=usr/local/bin/wpkg'");
#endif
    ctrl_t1->set_variable("INSTALL_PREOPTIONS", "--repository f:this-file");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // invalid character in directory path (<)
#if defined(MO_WINDOWS)
    ctrl_t1->set_field("PRE_COMMAND", "set WPKG_SUBST=\"f=/opt/wpkg|</m2osw/packages:h=usr/local/bin/wpkg\"");
#else
    ctrl_t1->set_field("PRE_COMMAND", "export WPKG_SUBST='f=/opt/wpkg|</m2osw/packages:h=usr/local/bin/wpkg'");
#endif
    ctrl_t1->set_variable("INSTALL_PREOPTIONS", "--repository f:this-file");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // invalid character in subst path (<)
#if defined(MO_WINDOWS)
    ctrl_t1->set_field("PRE_COMMAND", "set WPKG_SUBST=\"f=</opt/wpkg|/m2osw/packages:h=usr/local/bin/wpkg\"");
#else
    ctrl_t1->set_field("PRE_COMMAND", "export WPKG_SUBST='f=</opt/wpkg|/m2osw/packages:h=usr/local/bin/wpkg'");
#endif
    ctrl_t1->set_variable("INSTALL_PREOPTIONS", "--repository f:this-file");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // invalid character in directory path (>)
#if defined(MO_WINDOWS)
    ctrl_t1->set_field("PRE_COMMAND", "set WPKG_SUBST=\"f=/opt//wpkg|/>m2osw/packages:h=usr/local/bin/wpkg\"");
#else
    ctrl_t1->set_field("PRE_COMMAND", "export WPKG_SUBST='f=/opt//wpkg|/>m2osw/packages:h=usr/local/bin/wpkg'");
#endif
    ctrl_t1->set_variable("INSTALL_PREOPTIONS", "--repository f:this-file");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // invalid character in subst path (>)
#if defined(MO_WINDOWS)
    ctrl_t1->set_field("PRE_COMMAND", "set WPKG_SUBST=\"F=/>opt/wpkg|/m2osw/packages:h=usr/local/bin/wpkg\"");
#else
    ctrl_t1->set_field("PRE_COMMAND", "export WPKG_SUBST='F=/>opt/wpkg|/m2osw/packages:h=usr/local/bin/wpkg'");
#endif
    ctrl_t1->set_variable("INSTALL_PREOPTIONS", "--repository f:this-file");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // no equal sign (=)
#if defined(MO_WINDOWS)
    ctrl_t1->set_field("PRE_COMMAND", "set WPKG_SUBST=\"g=/valid/path/|good/dir:::f:/opt/wpkg\"");
#else
    ctrl_t1->set_field("PRE_COMMAND", "export WPKG_SUBST='g=/valid/path/|good/dir:::f:/opt/wpkg'");
#endif
    ctrl_t1->set_variable("INSTALL_PREOPTIONS", "--repository f:this-file");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);

    // letter drive
#if defined(MO_WINDOWS)
    ctrl_t1->set_field("PRE_COMMAND", "set WPKG_SUBST=\"f=/valid/path/:3=/opt/wpkg\"");
#else
    ctrl_t1->set_field("PRE_COMMAND", "export WPKG_SUBST='f=/valid/path/:3=/opt/wpkg'");
#endif
    ctrl_t1->set_variable("INSTALL_PREOPTIONS", "--repository f:this-file");
    install_package("t1", ctrl_t1, 1);
    verify_purged_files("t1", ctrl_t1);
}

void PackageUnitTests::check_drive_subst_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    check_drive_subst();
}


void PackageUnitTests::check_architecture_vendor()
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename root(unittest::tmp_dir);
    //wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    // create a package with an architecture including a vendor
    std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__ + std::string(" t1")));
    wpkg_architecture::architecture arch("linux-m2osw-i386");
    ctrl_t1->set_field("Architecture", arch.to_string());
    ctrl_t1->set_field("Files", "conffiles\n"
        "/usr/bin/vendor 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/vendor/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1);
    ctrl_t1->set_variable("INSTALL_ARCHITECTURE", arch);

    // the first install call is expected to work as is, no problems
    install_package("t1", ctrl_t1, 0);
    verify_installed_files("t1");

    // test with a package without a vendor
    std::shared_ptr<wpkg_control::control_file> ctrl_t2(get_new_control_file(__FUNCTION__ + std::string(" t2")));
    ctrl_t2->set_field("Architecture", "linux-i386");
    ctrl_t2->set_field("Files", "conffiles\n"
        "/usr/bin/no-vendor 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/no-vendor/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t2", ctrl_t2);

    // this is accepted because no vendor is equivalent to "any"
    install_package("t2", ctrl_t2, 0);
    verify_installed_files("t2");

    // test with a package with the wrong vendor
    std::shared_ptr<wpkg_control::control_file> ctrl_t3(get_new_control_file(__FUNCTION__ + std::string(" t3")));
    ctrl_t3->set_field("Architecture", "linux-ubuntu-i386");
    ctrl_t3->set_field("Files", "conffiles\n"
        "/usr/bin/bad-vendor 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/bad-vendor/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t3", ctrl_t3);

    // the first install call is expected to work as is, no problems
    install_package("t3", ctrl_t3, 1);
    verify_purged_files("t3", ctrl_t3);
}

void PackageUnitTests::check_architecture_vendor_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    check_architecture_vendor();
}

void PackageUnitTests::check_architecture_vendor2()
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename root(unittest::tmp_dir);
    //wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    // create a package with an architecture including a vendor
    // but do not include that vendor in the install target
    std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__ + std::string(" t1")));
    wpkg_architecture::architecture arch("linux-m2osw-i386");
    ctrl_t1->set_field("Architecture", arch.to_string());
    ctrl_t1->set_field("Files", "conffiles\n"
        "/usr/bin/vendor 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/vendor/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/vendor/long-filename/" + generate_long_filename(120) + " 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1);
    ctrl_t1->set_variable("INSTALL_ARCHITECTURE", "linux-i386");

    // the first install call is expected to work as is, no problems
    install_package("t1", ctrl_t1, 0);
    verify_installed_files("t1");

    // test with a package without a vendor
    std::shared_ptr<wpkg_control::control_file> ctrl_t2(get_new_control_file(__FUNCTION__ + std::string(" t2")));
    ctrl_t2->set_field("Architecture", "linux-i386");
    ctrl_t2->set_field("Files", "conffiles\n"
        "/usr/bin/no-vendor 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/no-vendor/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/no-vendor/a-long-filename/" + generate_long_filename(135) + " 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t2", ctrl_t2);

    // this is accepted because no vendor is equivalent to "any"
    install_package("t2", ctrl_t2, 0);
    verify_installed_files("t2");

    // test with a package with the wrong vendor
    std::shared_ptr<wpkg_control::control_file> ctrl_t3(get_new_control_file(__FUNCTION__ + std::string(" t3")));
    ctrl_t3->set_field("Architecture", "linux-ubuntu-i386");
    ctrl_t3->set_field("Files", "conffiles\n"
        "/usr/bin/bad-vendor 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/bad-vendor/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/bad-vendor/another-long-filename/which-was/breaking-wpkg/archives/" + generate_long_filename(135) + "/" + generate_long_filename(135) + " 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t3", ctrl_t3);

    // the first install call is expected to work as is, no problems
    install_package("t3", ctrl_t3, 0);
    verify_installed_files("t3");
}

void PackageUnitTests::check_architecture_vendor2_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    check_architecture_vendor2();
}

void PackageUnitTests::install_hooks()
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    // ******* kernel package so things get initialized
    std::shared_ptr<wpkg_control::control_file> ctrl_kernel(get_new_control_file(__FUNCTION__ + std::string(" kernel")));
    ctrl_kernel->set_field("Files", "conffiles\n"
        "/bin/init 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/kernel/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("kernel", ctrl_kernel, false);
    install_package("kernel", ctrl_kernel, 0);
    verify_installed_files("kernel");


    // +++++++ list hooks while still empty +++++++
    // (one day we'll have a popen() and compare output feature...)
    {
        // this would fail because the hooks directory does not exist
        std::string cmd(unittest::wpkg_tool);
        cmd += " --root " + wpkg_util::make_safe_console_string(target_path.path_only());
        cmd += " --list-hooks";
        printf("List Hooks Command: \"%s\"\n", cmd.c_str());
        fflush(stdout);
        CPPUNIT_ASSERT(system(cmd.c_str()) == 0);
    }


    // ******* global hook test (user defined)
    // define a global hook and install it with wpkg --add-hooks
    {
        memfile::memory_file hook_validate;
        hook_validate.create(memfile::memory_file::file_format_other);
        wpkg_filename::uri_filename hook_validate_filename;
#ifdef MO_WINDOWS
        hook_validate.printf(
            "REM Test to know that the global hook/validate ran\n"
            "ECHO hooks/core_global_validate: called with: [%%*]\n"
            "ECHO hooks/core_global_validate >> global_validate.txt\n"
            "ECHO arguments: [%%*] >> global_validate.txt\n"
        );
        hook_validate_filename = repository.append_child("global_validate.bat");
        hook_validate.write_file(hook_validate_filename, true);
#else
        hook_validate.printf(
            "#!/bin/sh -e\n"
            "# Test to know that the global hook/validate ran\n"
            "echo \"hooks/core_global_validate: called with: [$*]\"\n"
            "echo \"hooks/core_global_validate\" >> global_validate.txt\n"
            "echo \"arguments: [$*]\" >> global_validate.txt\n"
        );
        hook_validate_filename = repository.append_child("global_validate");
        hook_validate.write_file(hook_validate_filename, true);
#endif
        std::string cmd(unittest::wpkg_tool);
        cmd += " --root " + wpkg_util::make_safe_console_string(target_path.path_only());
        cmd += " --add-hooks " + wpkg_util::make_safe_console_string(hook_validate_filename.path_only());
        printf("Add Hooks Command: \"%s\"\n", cmd.c_str());
        fflush(stdout);
        CPPUNIT_ASSERT(system(cmd.c_str()) == 0);
    }
    // adding a global hook does not run it!
    wpkg_filename::uri_filename global_validate_file(target_path.append_child("global_validate.txt"));
    CPPUNIT_ASSERT_MESSAGE("global_validate.txt file (" + global_validate_file.full_path() + ") was created on installation???", !global_validate_file.exists());



    // ******* t1 test (with global hooks)
    wpkg_filename::uri_filename build_path_t1(root.append_child("t1"));
    wpkg_filename::uri_filename wpkg_path_t1(build_path_t1.append_child("WPKG"));

    // create a package with hooks
    std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__ + std::string(" t1")));
    ctrl_t1->set_field("Files", "conffiles\n"
        "/usr/bin/hooks 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/hooks/copyright 0123456789abcdef0123456789abcdef\n"
    );
    {
        memfile::memory_file hook_validate;
        hook_validate.create(memfile::memory_file::file_format_other);
#ifdef MO_WINDOWS
        hook_validate.printf(
            "REM Test to know that t1 hook/validate ran\n"
            "ECHO hooks/t1_validate: called with: [%%*]\n"
            "ECHO hooks/t1_validate > t1_validate.txt\n"
            "ECHO arguments: [%%*] >> t1_validate.txt\n"
        );
        hook_validate.write_file(wpkg_path_t1.append_child("t1_validate.bat"), true);
#else
        hook_validate.printf(
            "#!/bin/sh -e\n"
            "# Test to know that t1 hook/validate ran\n"
            "echo \"hooks/t1_validate: called with: [$*]\"\n"
            "echo \"hooks/t1_validate\" > t1_validate.txt\n"
            "echo \"arguments: [$*]\" >> t1_validate.txt\n"
        );
        hook_validate.write_file(wpkg_path_t1.append_child("t1_validate"), true);
#endif
    }
    create_package("t1", ctrl_t1, false);

    // creating a package has no hook side effects
    CPPUNIT_ASSERT_MESSAGE("global_validate.txt file (" + global_validate_file.full_path() + ") was created on a package build???", !global_validate_file.exists());

    // the install call is expected to work as is
    ctrl_t1->set_variable("INSTALL_POSTOPTIONS", "--verbose");
    install_package("t1", ctrl_t1, 0);
    verify_installed_files("t1");

    wpkg_filename::uri_filename t1_validate_file(target_path.append_child("t1_validate.txt"));
    CPPUNIT_ASSERT_MESSAGE("t1_validate.txt file already exists", !t1_validate_file.exists());
    CPPUNIT_ASSERT_MESSAGE("global_validate.txt file (" + global_validate_file.full_path() + ") was not created on installation? (1)", global_validate_file.exists());
    global_validate_file.os_unlink();

    // on second installation the hook gets executed
    install_package("t1", ctrl_t1, 0);
    CPPUNIT_ASSERT_MESSAGE("t1_validate.txt file (" + t1_validate_file.full_path() + ") is missing when it should exist", t1_validate_file.exists());
    // get rid of it
    t1_validate_file.os_unlink();
    CPPUNIT_ASSERT_MESSAGE("global_validate.txt file (" + global_validate_file.full_path() + ") was not created on installation? (2)", global_validate_file.exists());
    global_validate_file.os_unlink();


    // +++++++ list hooks +++++++
    // (one day we'll have a popen() and compare output feature...)
    {
        std::string cmd(unittest::wpkg_tool);
        cmd += " --root " + wpkg_util::make_safe_console_string(target_path.path_only());
        cmd += " --list-hooks";
        printf("List Hooks Command: \"%s\"\n", cmd.c_str());
        fflush(stdout);
        CPPUNIT_ASSERT(system(cmd.c_str()) == 0);
    }


    // when we remove a package, its hooks get removed
    remove_package("t1", ctrl_t1);
    // the hook gets removed AFTER validation so the file exists!
    CPPUNIT_ASSERT_MESSAGE("t1_validate.txt file (" + t1_validate_file.full_path() + ") is missing when it should exist after the first remove", t1_validate_file.exists());
    // get rid of it
    t1_validate_file.os_unlink();
    CPPUNIT_ASSERT_MESSAGE("global_validate.txt file (" + global_validate_file.full_path() + ") was not created on removal? (1)", global_validate_file.exists());
    global_validate_file.os_unlink();

    // the 2nd time the package is already gone, so no hook
    remove_package("t1", ctrl_t1);
    CPPUNIT_ASSERT_MESSAGE("t1_validate.txt file (" + t1_validate_file.full_path() + ") was re-created on the second remove?!", !t1_validate_file.exists());
    CPPUNIT_ASSERT_MESSAGE("global_validate.txt file (" + global_validate_file.full_path() + ") was not created on removal? (2)", global_validate_file.exists());
    global_validate_file.os_unlink();



    // ******* global hook test (user defined)
    // remove the global hooks
    {
        memfile::memory_file hook_validate;
        hook_validate.create(memfile::memory_file::file_format_other);
        wpkg_filename::uri_filename hook_validate_filename;
#ifdef MO_WINDOWS
        hook_validate_filename = "global_validate.bat";
#else
        hook_validate_filename = "global_validate";
#endif
        std::string cmd(unittest::wpkg_tool);
        cmd += " --root " + wpkg_util::make_safe_console_string(target_path.path_only());
        cmd += " --remove-hooks " + wpkg_util::make_safe_console_string(hook_validate_filename.path_only());
        printf("Remove Hooks Command: \"%s\"\n", cmd.c_str());
        fflush(stdout);
        CPPUNIT_ASSERT(system(cmd.c_str()) == 0);
    }
}

void PackageUnitTests::install_hooks_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    install_hooks();
}

void PackageUnitTests::auto_remove()
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename target_path(root.append_child("target"));
    wpkg_filename::uri_filename repository(root.append_child("repository"));

    // to test the auto-remove we want to add several packages and make sure
    // that full installed (non-implicit) packages do not get removed
    // automatically. So at this point we create the following setup:
    //
    //      create t1
    //      install t1
    //      auto-remove, nothing happens
    //      create t2 which depends on t1
    //      create t3 which depends on t2
    //      create t4 which depends on t3
    //      install t4 which auto-installs t3 and t2
    //      auto-remove, nothing happens
    //      create t5
    //      install t5
    //      auto-remove, nothing happens
    //      remove t4
    //      auto-remove, t3 and t2 are auto-removed
    //      t1 and t5 are still installed
    //

    // create packages
    std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__ + std::string(" t1")));
    ctrl_t1->set_field("Files", "conffiles\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t1", ctrl_t1);

    std::shared_ptr<wpkg_control::control_file> ctrl_t2(get_new_control_file(__FUNCTION__ + std::string(" t2")));
    ctrl_t2->set_field("Files", "conffiles\n"
        "/usr/bin/t2 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t2/copyright 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t2->set_field("Depends", "t1");
    create_package("t2", ctrl_t2);

    std::shared_ptr<wpkg_control::control_file> ctrl_t3(get_new_control_file(__FUNCTION__ + std::string(" t3")));
    ctrl_t3->set_field("Files", "conffiles\n"
        "/usr/bin/t3 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t3/copyright 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t3->set_field("Depends", "t2");
    create_package("t3", ctrl_t3);

    std::shared_ptr<wpkg_control::control_file> ctrl_t4(get_new_control_file(__FUNCTION__ + std::string(" t4")));
    ctrl_t4->set_field("Files", "conffiles\n"
        "/usr/bin/t4 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t4/copyright 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t4->set_field("Depends", "t3");
    create_package("t4", ctrl_t4);
    ctrl_t4->set_variable("INSTALL_PREOPTIONS", "--repository " + wpkg_util::make_safe_console_string(repository.path_only()));

    std::shared_ptr<wpkg_control::control_file> ctrl_t5(get_new_control_file(__FUNCTION__ + std::string(" t5")));
    ctrl_t5->set_field("Files", "conffiles\n"
        "/usr/bin/t5 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t5/copyright 0123456789abcdef0123456789abcdef\n"
    );
    create_package("t5", ctrl_t5);

    // start installation and such
    install_package("t1", ctrl_t1, 0);
    verify_installed_files("t1");

    {
        std::string cmd(unittest::wpkg_tool);
        cmd += " --root " + wpkg_util::make_safe_console_string(target_path.path_only());
        cmd += " --autoremove ";
        printf("Auto-Remove Command: \"%s\"\n", cmd.c_str());
        fflush(stdout);
        CPPUNIT_ASSERT(system(cmd.c_str()) == 0);
    }

    // t1 still installed
    verify_installed_files("t1");

    // installing t4 auto-installs t2 and t3
    install_package("t4", ctrl_t4, 0);
    verify_installed_files("t1");
    verify_installed_files("t2");
    verify_installed_files("t3");
    verify_installed_files("t4");

    {
        std::string cmd(unittest::wpkg_tool);
        cmd += " --root " + wpkg_util::make_safe_console_string(target_path.path_only());
        cmd += " --autoremove ";
        printf("Auto-Remove Command: \"%s\"\n", cmd.c_str());
        fflush(stdout);
        CPPUNIT_ASSERT(system(cmd.c_str()) == 0);
    }

    // still all there!
    verify_installed_files("t1");
    verify_installed_files("t2");
    verify_installed_files("t3");
    verify_installed_files("t4");

    // install t5 now
    install_package("t5", ctrl_t5, 0);
    verify_installed_files("t1");
    verify_installed_files("t2");
    verify_installed_files("t3");
    verify_installed_files("t4");
    verify_installed_files("t5");

    {
        std::string cmd(unittest::wpkg_tool);
        cmd += " --root " + wpkg_util::make_safe_console_string(target_path.path_only());
        cmd += " --autoremove ";
        printf("Auto-Remove Command: \"%s\"\n", cmd.c_str());
        fflush(stdout);
        CPPUNIT_ASSERT(system(cmd.c_str()) == 0);
    }

    // still all there!!!
    verify_installed_files("t1");
    verify_installed_files("t2");
    verify_installed_files("t3");
    verify_installed_files("t4");
    verify_installed_files("t5");

    // remove t4 to allow t2/t3 to be auto-removed
    remove_package("t4", ctrl_t4);
    verify_installed_files("t1");
    verify_installed_files("t2");
    verify_installed_files("t3");
    verify_removed_files("t4", ctrl_t4);
    verify_installed_files("t5");

    {
        std::string cmd(unittest::wpkg_tool);
        cmd += " --root " + wpkg_util::make_safe_console_string(target_path.path_only());
        cmd += " --autoremove ";
        printf("Auto-Remove Command: \"%s\"\n", cmd.c_str());
        fflush(stdout);
        CPPUNIT_ASSERT(system(cmd.c_str()) == 0);
    }

    // this time the auto-remove had an effect!
    verify_installed_files("t1");
    verify_removed_files("t2", ctrl_t2);
    verify_removed_files("t3", ctrl_t3);
    verify_removed_files("t4", ctrl_t4);
    verify_installed_files("t5");
}

void PackageUnitTests::auto_remove_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    auto_remove();
}


void PackageUnitTests::scripts_selection()
{
    // IMPORTANT: remember that all files are deleted between tests

    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename repository(root.append_child("repository"));

////////////////////// t1 -- make sure only Unix or MS-Windows scripts get in the package
    wpkg_filename::uri_filename build_path_t1(root.append_child("t1"));
    wpkg_filename::uri_filename wpkg_path_t1(build_path_t1.append_child("WPKG"));

    // create a first version of the package
    struct test_archs
    {
        char const *    f_name;
        int             f_flags;
    };

    test_archs archs_info[] =
    {
        {
            /* f_name  */ "linux-m2osw-i386",
            /* f_flags */ 0x001F
        },
        {
            /* f_name  */ "linux-m2osw-amd64",
            /* f_flags */ 0x001F
        },
        {
            /* f_name  */ "linux-i386",
            /* f_flags */ 0x001F
        },
        {
            /* f_name  */ "linux-amd64",
            /* f_flags */ 0x001F
        },
        {
            /* f_name  */ "linux-powerpc",
            /* f_flags */ 0x001F
        },
        {
            /* f_name  */ "i386",
            /* f_flags */ 0x001F
        },
        {
            /* f_name  */ "amd64",
            /* f_flags */ 0x001F
        },
        {
            /* f_name  */ "mswindows-m2osw-i386",
            /* f_flags */ 0x03E0
        },
        {
            /* f_name  */ "mswindows-m2osw-amd64",
            /* f_flags */ 0x03E0
        },
        {
            /* f_name  */ "mswindows-i386",
            /* f_flags */ 0x03E0
        },
        {
            /* f_name  */ "mswindows-amd64",
            /* f_flags */ 0x03E0
        },
        {
            /* f_name  */ "win32",
            /* f_flags */ 0x03E0
        },
        {
            /* f_name  */ "win64",
            /* f_flags */ 0x03E0
        },
        {
            /* f_name  */ "win32-m2osw-i386",
            /* f_flags */ 0x03E0
        },
        {
            /* f_name  */ "win64-m2osw+11-amd64",
            /* f_flags */ 0x03E0
        },
        {
            /* f_name  */ "win64-m2osw.com-mips", // yes, there was a MIPS version!
            /* f_flags */ 0x03E0
        },
        {
            /* f_name  */ "all",
            /* f_flags */ 0x03FF
        },
        {
            /* f_name  */ "source",
            /* f_flags */ 0x0000
        }
    };

    size_t const max_archs(sizeof(archs_info) / sizeof(archs_info[0]));
    for(size_t idx(0); idx < max_archs; ++idx)
    {
        std::shared_ptr<wpkg_control::control_file> ctrl_t1(get_new_control_file(__FUNCTION__));
        ctrl_t1->set_field("Architecture", archs_info[idx].f_name);
        ctrl_t1->set_field("Files", "conffiles\n"
            "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
            "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
        );

        // MS-Windows
        {
            memfile::memory_file validate;
            validate.create(memfile::memory_file::file_format_other);
            memfile::memory_file preinst;
            preinst.create(memfile::memory_file::file_format_other);
            memfile::memory_file postinst;
            postinst.create(memfile::memory_file::file_format_other);
            memfile::memory_file prerm;
            prerm.create(memfile::memory_file::file_format_other);
            memfile::memory_file postrm;
            postrm.create(memfile::memory_file::file_format_other);

            validate.printf(
                "REM Script showing that MS-Windows scripts were selected (validate)\n"
            );
            validate.write_file(wpkg_path_t1.append_child("validate.bat"), true);
            preinst.printf(
                "REM Script showing that MS-Windows scripts were selected (preinst)\n"
            );
            preinst.write_file(wpkg_path_t1.append_child("preinst.bat"), true);
            postinst.printf(
                "REM Script showing that MS-Windows scripts were selected (postinst)\n"
            );
            postinst.write_file(wpkg_path_t1.append_child("postinst.bat"), true);
            prerm.printf(
                "REM Script showing that MS-Windows scripts were selected (prerm)\n"
            );
            prerm.write_file(wpkg_path_t1.append_child("prerm.bat"), true);
            postrm.printf(
                "REM Script showing that MS-Windows scripts were selected (postrm)\n"
            );
            postrm.write_file(wpkg_path_t1.append_child("postrm.bat"), true);
        }

        {
            memfile::memory_file validate;
            validate.create(memfile::memory_file::file_format_other);
            memfile::memory_file preinst;
            preinst.create(memfile::memory_file::file_format_other);
            memfile::memory_file postinst;
            postinst.create(memfile::memory_file::file_format_other);
            memfile::memory_file prerm;
            prerm.create(memfile::memory_file::file_format_other);
            memfile::memory_file postrm;
            postrm.create(memfile::memory_file::file_format_other);

            validate.printf(
                "#!/bin/sh\n"
                "# Script showing that Unix scripts were selected (validate)\n"
            );
            validate.write_file(wpkg_path_t1.append_child("validate"), true);
            preinst.printf(
                "#!/bin/sh\n"
                "# Script showing that Unix scripts were selected (preinst)\n"
            );
            preinst.write_file(wpkg_path_t1.append_child("preinst"), true);
            postinst.printf(
                "#!/bin/sh\n"
                "# Script showing that Unix scripts were selected (postinst)\n"
            );
            postinst.write_file(wpkg_path_t1.append_child("postinst"), true);
            prerm.printf(
                "#!/bin/sh\n"
                "# Script showing that Unix scripts were selected (prerm)\n"
            );
            prerm.write_file(wpkg_path_t1.append_child("prerm"), true);
            postrm.printf(
                "#!/bin/sh\n"
                "# Script showing that Unix scripts were selected (postrm)\n"
            );
            postrm.write_file(wpkg_path_t1.append_child("postrm"), true);
        }

        create_package("t1", ctrl_t1, false);

        // load the result and verify which files are present in the .deb
        std::string architecture(ctrl_t1->get_field("Architecture"));
        if(architecture == "source")
        {
            architecture = "";
        }
        else
        {
            architecture = "_" + architecture;
        }
        wpkg_filename::uri_filename package_filename(repository.append_child("/t1_1.0" + architecture + ".deb"));
        memfile::memory_file package_file;
        package_file.read_file(package_filename);
        package_file.dir_rewind();
        for(;;)
        {
            memfile::memory_file::file_info info;
            memfile::memory_file data;
            // assert here because the control.tar.gz MUST be present
            CPPUNIT_ASSERT(package_file.dir_next(info, &data));

            if(info.get_filename() == "control.tar.gz")
            {
                // we can reuse the info parameter since the previous level
                // info does not interest us anymore
                int flags(0);
                memfile::memory_file control_file;
                data.decompress(control_file);
                control_file.dir_rewind();
                for(;;)
                {
                    memfile::memory_file::file_info ctrl_info;
                    if(!control_file.dir_next(ctrl_info, nullptr))
                    {
                        break;
                    }
                    if(ctrl_info.get_filename() == "validate")
                    {
                        flags |= 0x0001;
                    }
                    else if(ctrl_info.get_filename() == "preinst")
                    {
                        flags |= 0x0002;
                    }
                    else if(ctrl_info.get_filename() == "postinst")
                    {
                        flags |= 0x0004;
                    }
                    else if(ctrl_info.get_filename() == "prerm")
                    {
                        flags |= 0x0008;
                    }
                    else if(ctrl_info.get_filename() == "postrm")
                    {
                        flags |= 0x0010;
                    }
                    else if(ctrl_info.get_filename() == "validate.bat")
                    {
                        flags |= 0x0020;
                    }
                    else if(ctrl_info.get_filename() == "preinst.bat")
                    {
                        flags |= 0x0040;
                    }
                    else if(ctrl_info.get_filename() == "postinst.bat")
                    {
                        flags |= 0x0080;
                    }
                    else if(ctrl_info.get_filename() == "prerm.bat")
                    {
                        flags |= 0x0100;
                    }
                    else if(ctrl_info.get_filename() == "postrm.bat")
                    {
                        flags |= 0x0200;
                    }
                }
                if(archs_info[idx].f_flags != flags)
                {
                    std::cerr << "error: found flags 0x" << std::hex << flags << ", expected flags 0x" << archs_info[idx].f_flags << "\n";
                }
                CPPUNIT_ASSERT(archs_info[idx].f_flags == flags);
                break;
            }
        }
    }
}

void PackageUnitTests::scripts_selection_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    scripts_selection();
}


void PackageUnitTests::complex_tree_in_repository()
{
    // IMPORTANT: remember that all files are deleted between tests

    ////////////////////////// cpp-utils
    // t1       version 1.0 //
    //////////////////////////
    std::shared_ptr<wpkg_control::control_file> ctrl_t1_0(get_new_control_file(__FUNCTION__));
    ctrl_t1_0->set_field("Files", "conffiles\n"
        "/etc/t1.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t1/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t1_0->set_field("Version", "1.0");
    ctrl_t1_0->set_field("Depends", "t5 (= 1.3), t3 (= 1.2), t4 (= 1.1), t7 (= 1.1)");
    create_package("t1", ctrl_t1_0);


    ////////////////////////// lp-utils-workspace
    // t2       version 1.0 //
    //////////////////////////
    std::shared_ptr<wpkg_control::control_file> ctrl_t2(get_new_control_file(__FUNCTION__));
    ctrl_t2->set_field("Files", "conffiles\n"
        "/etc/t2.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t2 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t2/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t2/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t2->set_field("Version", "1.0");
    ctrl_t2->set_field("Depends", "t1 (= 1.0), t5 (= 1.3), t10 (= 1.1), t4 (= 1.1), t11 (= 1.0)");
    create_package("t2", ctrl_t2);


    ////////////////////////////////////
    // t3       version 1.0, 1.1, 1.2 //
    ////////////////////////////////////
    std::shared_ptr<wpkg_control::control_file> ctrl_t3_0(get_new_control_file(__FUNCTION__));
    ctrl_t3_0->set_field("Files", "conffiles\n"
        "/etc/t3.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t3 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t3/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t3/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t3_0->set_field("Version", "1.0");
    create_package("t3", ctrl_t3_0);

    std::shared_ptr<wpkg_control::control_file> ctrl_t3_1(get_new_control_file(__FUNCTION__));
    ctrl_t3_1->set_field("Files", "conffiles\n"
        "/etc/t3.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t3 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t3/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t3/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t3_1->set_field("Version", "1.1");
    create_package("t3", ctrl_t3_1);

    std::shared_ptr<wpkg_control::control_file> ctrl_t3_2(get_new_control_file(__FUNCTION__));
    ctrl_t3_2->set_field("Files", "conffiles\n"
        "/etc/t3.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t3 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t3/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t3/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t3_2->set_field("Version", "1.2");
    create_package("t3", ctrl_t3_2);

    /////////////////////////////// liblog4cplus
    // t4       version 1.0, 1.1 //
    ///////////////////////////////
    std::shared_ptr<wpkg_control::control_file> ctrl_t4_0(get_new_control_file(__FUNCTION__));
    ctrl_t4_0->set_field("Files", "conffiles\n"
        "/etc/t4.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t4 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t4/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t4/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t4_0->set_field("Version", "1.0");
    create_package("t4", ctrl_t4_0);

    std::shared_ptr<wpkg_control::control_file> ctrl_t4_1(get_new_control_file(__FUNCTION__));
    ctrl_t4_1->set_field("Files", "conffiles\n"
        "/etc/t4.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t4 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t4/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t4/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t4_1->set_field("Version", "1.1");
    create_package("t4", ctrl_t4_1);


    /////////////////////////////// libboost
    // t5       version 1.2, 1.3 //
    ///////////////////////////////
    std::shared_ptr<wpkg_control::control_file> ctrl_t5_2(get_new_control_file(__FUNCTION__));
    ctrl_t5_2->set_field("Files", "conffiles\n"
        "/etc/t5.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t5 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t5/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t5/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t5_2->set_field("Version", "1.2");
    ctrl_t5_2->set_field("Depends", "t8 (= 1.2)");
    create_package("t5", ctrl_t5_2);

    std::shared_ptr<wpkg_control::control_file> ctrl_t5_3(get_new_control_file(__FUNCTION__));
    ctrl_t5_3->set_field("Files", "conffiles\n"
        "/etc/t5.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t5 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t5/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t5/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t5_3->set_field("Version", "1.3");
    ctrl_t5_3->set_field("Depends", "t8 (= 1.3)");
    create_package("t5", ctrl_t5_3);


    /////////////////////////////// libboost-log
    // t6       version 1.2, 1.3 //
    ///////////////////////////////
    std::shared_ptr<wpkg_control::control_file> ctrl_t6_2(get_new_control_file(__FUNCTION__));
    ctrl_t6_2->set_field("Files", "conffiles\n"
        "/etc/t6.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t6 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t6/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t6/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t6_2->set_field("Version", "1.2");
    ctrl_t6_2->set_field("Depends", "t8 (= 1.2), t5 (= 1.2)");
    create_package("t6", ctrl_t6_2);

    std::shared_ptr<wpkg_control::control_file> ctrl_t6_3(get_new_control_file(__FUNCTION__));
    ctrl_t6_3->set_field("Files", "conffiles\n"
        "/etc/t6.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t6 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t6/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t6/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t6_3->set_field("Version", "1.3");
    ctrl_t6_3->set_field("Depends", "t8 (= 1.3), t5 (= 1.3)");
    create_package("t6", ctrl_t6_3);


    /////////////////////////////// libgdal
    // t7       version 1.0, 1.1 //
    ///////////////////////////////
    std::shared_ptr<wpkg_control::control_file> ctrl_t7_0(get_new_control_file(__FUNCTION__));
    ctrl_t7_0->set_field("Files", "conffiles\n"
        "/etc/t7.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t7 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t7/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t7/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t7_0->set_field("Version", "1.0");
    ctrl_t7_0->set_field("Depends", "t9 (= 1.1)");
    create_package("t7", ctrl_t7_0);

    std::shared_ptr<wpkg_control::control_file> ctrl_t7_1(get_new_control_file(__FUNCTION__));
    ctrl_t7_1->set_field("Files", "conffiles\n"
        "/etc/t7.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t7 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t7/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t7/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t7_1->set_field("Version", "1.1");
    ctrl_t7_1->set_field("Depends", "t9 (= 1.2)");
    create_package("t7", ctrl_t7_1);


    /////////////////////////////// libboost-headers
    // t8       version 1.2, 1.3 //
    ///////////////////////////////
    std::shared_ptr<wpkg_control::control_file> ctrl_t8_2(get_new_control_file(__FUNCTION__));
    ctrl_t8_2->set_field("Files", "conffiles\n"
        "/etc/t8.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t8 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t8/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t8/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t8_2->set_field("Version", "1.2");
    create_package("t8", ctrl_t8_2);

    std::shared_ptr<wpkg_control::control_file> ctrl_t8_3(get_new_control_file(__FUNCTION__));
    ctrl_t8_3->set_field("Files", "conffiles\n"
        "/etc/t8.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t8 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t8/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t8/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t8_3->set_field("Version", "1.3");
    create_package("t8", ctrl_t8_3);


    //////////////////////////////////// libgeos
    // t9       version 1.0, 1.1, 1.2 //
    ////////////////////////////////////
    std::shared_ptr<wpkg_control::control_file> ctrl_t9_0(get_new_control_file(__FUNCTION__));
    ctrl_t9_0->set_field("Files", "conffiles\n"
        "/etc/t9.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t9 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t9/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t9/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t9_0->set_field("Version", "1.0");
    create_package("t9", ctrl_t9_0);

    std::shared_ptr<wpkg_control::control_file> ctrl_t9_1(get_new_control_file(__FUNCTION__));
    ctrl_t9_1->set_field("Files", "conffiles\n"
        "/etc/t9.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t9 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t9/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t9/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t9_1->set_field("Version", "1.1");
    create_package("t9", ctrl_t9_1);

    std::shared_ptr<wpkg_control::control_file> ctrl_t9_2(get_new_control_file(__FUNCTION__));
    ctrl_t9_2->set_field("Files", "conffiles\n"
        "/etc/t9.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t9 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t9/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t9/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t9_2->set_field("Version", "1.2");
    create_package("t9", ctrl_t9_2);


    /////////////////////////////// mongoose-2001
    // t10      version 1.0, 1.1 //
    ///////////////////////////////
    std::shared_ptr<wpkg_control::control_file> ctrl_t10_0(get_new_control_file(__FUNCTION__));
    ctrl_t10_0->set_field("Files", "conffiles\n"
        "/etc/t10.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t10 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t10/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t10/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t10_0->set_field("Version", "1.0");
    create_package("t10", ctrl_t10_0);

    std::shared_ptr<wpkg_control::control_file> ctrl_t10_1(get_new_control_file(__FUNCTION__));
    ctrl_t10_1->set_field("Files", "conffiles\n"
        "/etc/t10.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t10 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t10/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t10/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t10_1->set_field("Version", "1.1");
    create_package("t10", ctrl_t10_1);


    /////////////////////////////// wpkg-venv
    // t11           version 1.0 //
    ///////////////////////////////
    std::shared_ptr<wpkg_control::control_file> ctrl_t11_0(get_new_control_file(__FUNCTION__));
    ctrl_t11_0->set_field("Files", "conffiles\n"
        "/etc/t11.conf 0123456789abcdef0123456789abcdef\n"
        "/usr/bin/t11 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t11/copyright 0123456789abcdef0123456789abcdef\n"
        "/usr/share/doc/t11/info 0123456789abcdef0123456789abcdef\n"
    );
    ctrl_t11_0->set_field("Version", "1.0");
    create_package("t11", ctrl_t11_0);


    // Installing t2 without --repository fails
    install_package("t2", ctrl_t2, 1);

    // Installing t2 with --repository works
    wpkg_filename::uri_filename root(unittest::tmp_dir);
    wpkg_filename::uri_filename repository(root.append_child("repository"));
    ctrl_t2->set_variable("INSTALL_PREOPTIONS", "--repository " + wpkg_util::make_safe_console_string(repository.path_only()) + " -D 07777");
    install_package("t2", ctrl_t2, 0);
}

void PackageUnitTests::complex_tree_in_repository_with_spaces()
{
    raii_tmp_dir_with_space add_spaces;
    complex_tree_in_repository();
}


void PackageUnitTests::unacceptable_filename()
{
    {
        // filename ending with a period
        std::shared_ptr<wpkg_control::control_file> ctrl_t1_0(get_new_control_file(__FUNCTION__));
        ctrl_t1_0->set_field("Files", "conffiles\n"
            "/usr/bin/t1 0123456789abcdef0123456789abcdef\n"
            "/usr/bin/bad. 0123456789abcdef0123456789abcdef\n"
            "/usr/share/doc/t1/copyright 0123456789abcdef0123456789abcdef\n"
            "/usr/share/doc/t1/info 0123456789abcdef0123456789abcdef\n"
        );
        ctrl_t1_0->set_field("Version", "1.0");
        ctrl_t1_0->set_variable("BUILD_RESULT", "1");
        create_package("t1", ctrl_t1_0);
    }
}


// vim: ts=4 sw=4 et
