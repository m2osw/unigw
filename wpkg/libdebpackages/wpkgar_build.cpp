/*    wpkgar_build.cpp -- create debian packages
 *    Copyright (C) 2012-2015  Made to Order Software Corporation
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
 * \brief Implementation of the --build command of wpkg.
 *
 * The following is the implementation of the build command in the wpkg.
 * The build is relatively simple as it involves creating only one package
 * at a time, although if you specify an info file multiple packages will
 * be created. Yet each is created separately.
 *
 * By default, we expect one directory as the input. This directory is
 * expected to be a Debian like directory.
 *
 * You can also specify a file in which case it is taken as an info file.
 * Info files can be used to create multiple packages at once. This is
 * useful to create the runtime, source, documentation, and other packages
 * defining most of the control file fields in one place.
 *
 * List of valid architectures: http://www.debian.org/ports/
 */
#include    "libdebpackages/wpkgar_build.h"
#include    "libdebpackages/wpkgar_install.h"
#include    "libdebpackages/wpkgar_tracker.h"
#include    "libdebpackages/compatibility.h"
#include    "libdebpackages/wpkg_util.h"
#include    "libdebpackages/wpkg_changelog.h"
#include    "libdebpackages/wpkg_copyright.h"
#include    "libdebpackages/wpkg_architecture.h"
#include    "libdebpackages/debian_packages.h"
#include    <stdlib.h>
#include    <sstream>
#include    <iostream>
#include    <time.h>

namespace wpkgar
{


/** \class wpkgar_build
 * \brief The archive manager to build packages.
 *
 * This class is used to create packages. It handles all the validations
 * over the existing project to create source packages, is capable to
 * run cmake / make to build projects and then build binary packages.
 */


/** \class wpkgar_build::source_validation
 * \brief Structure used to track the validation status.
 *
 * While validating the source of a project, this structure is used to
 * record the status of all the data. This is useful to better help the
 * user in fixing potential problems.
 *
 * For example, a GUI application could run the process in the background
 * and then decide to open a wizard and ask the user to fix the different
 * problems found from within that wizard. If no problems were found,
 * then the user can be offered to actually create the source package or
 * not.
 */


/** \class wpkgar_build::source_validation::source_property
 * \brief Class used to represent one validation property.
 *
 * Each source validation step is called a validation property. Each
 * property object is used to document the steps to take in case the
 * validation fails.
 *
 * This list is retrieved from your source_validation object once you
 * went through the source validation process.
 */


namespace
{

/** \brief List of fields to be removed from source packages.
 *
 * The fields listed here will be removed from source packages since they
 * do not make sense in such packages.
 */
const char * const non_source_fields[] =
{
    "Essential",
    NULL
};


/** \brief List of fields to be removed from binary packages.
 *
 * The fields listed here will be removed from all binary packages because
 * they are only for source packages.
 */
const char * const non_binary_fields[] =
{
    "Build-Conflicts",
    "Build-Conflicts-Arch",
    "Build-Conflicts-Indep",
    "Build-Depends",
    "Build-Depends-Arch",
    "Build-Depends-Indep",
    NULL
};

} // no name namespace


/** \brief Initialize the build manager.
 *
 * This function initializes a build manager with a main manager pointer and
 * a build directory. The build directory (or filename) must be defined here
 * as it is otherwise a constant in the class.
 *
 * To finish initialization of the class you may want to call some or all
 * of the following functions:
 *
 * \li set_parameter()
 * \li set_zlevel()
 * \li set_compressor()
 * \li set_extra_path()
 * \li set_output_dir()
 * \li set_output_repository_dir()
 * \li set_filename()
 * \li add_repository()
 * \li add_exception()
 * \li is_exception()
 *
 * Note that the build object expects the \p build_directory to be one of the
 * following:
 *
 * \li empty string -- to build a project source package
 * \li source-package.deb -- the name of a source package from which
 * binary packages get generated
 * \li control.info filename -- this requires an extra path to the directory
 * with all the installed components; it can be defined with the
 * set_extra_path() or the ROOT_TREE variable in the control.info file
 * \li directory name -- a wpkg directory which includes a WPKG sub-directory
 * with the control file and all the directories that are to be installed by
 * that binary package
 *
 * \param[in] manager  The manager to link with this build object.
 * \param[in] build_directory  The build directory or file.
 */
wpkgar_build::wpkgar_build(wpkgar_manager *manager, const std::string& build_directory)
    : f_manager(manager)
    //, f_zlevel(9) -- auto-init
    //, f_ignore_empty_packages(false) -- auto-init
    //, f_run_tests(false) -- auto-init
    //, f_rename_changelog(false) -- auto-init
    //, f_rename_copyright(false) -- auto-init
    //, f_rename_controlinfo(false) -- auto-init
    //, f_changelog_filename("") -- auto-init
    //, f_copyright_filename("") -- auto-init
    //, f_controlinfo_filename("") -- auto-init
    //, f_package_source_path("") -- auto-init
    //, f_install_prefix("") -- auto-init
    , f_compressor(memfile::memory_file::file_format_gz)
    , f_build_directory(build_directory)
    //, f_output_dir("") -- auto-init
    //, f_filename("") -- auto-init
    //, f_package_name("") -- auto-init
    //, f_extra_path("") -- auto-init
    , f_build_number_filename("wpkg/build_number")
    //, f_exceptions() -- auto-init
    //, f_repository() -- auto-init
    //, f_flags() -- auto-init
    //, f_cmake_generator("")
    , f_make_tool("make")
{
    f_exceptions.push_back("RCS");
    f_exceptions.push_back("SCCS");
    f_exceptions.push_back("CVS");
    f_exceptions.push_back(".svn");
    f_exceptions.push_back(".git");
    f_exceptions.push_back("*.bak");
    f_exceptions.push_back("*~");
    f_exceptions.push_back("*.swp");

    // Wed Mar  4 18:49:37 PST 2015 RDB:
    // I'm removing these from the exception list. This makes it impossible to build a package
    // for boost 1.53, which has a subdirectory under the include folders called "bimap/tags".
    //
    //f_exceptions.push_back("TAGS");
    //f_exceptions.push_back("tags");

    // The following causes a problem with boost which has a sub-directory
    // named "core"; many other systems have such too...
    //f_exceptions.push_back("core");
}


/** \brief Set one of the build parameters.
 *
 * This function saves the specified integer value as a parameter of the
 * build object. The \p flag parameter defines which flag is set. The value
 * represents the value of that parameter.
 *
 * To retrieve that value, use the get_parameter().
 *
 * It is most often used for things such as --recursive which is just a
 * Boolean flag.
 *
 * \param[in] flag  The name of the parameter to set.
 * \param[in] value  The new value of this parameter.
 */
void wpkgar_build::set_parameter(parameter_t flag, int value)
{
    f_flags[flag] = value;
}


/** \brief Retrieve the parameter value.
 *
 * This function returns the value of a parameter as defined by the
 * set_parameter() function.
 *
 * \todo
 * The default is defined in this call which is certainly not correct
 * because when you have many calls used to read the same parameter,
 * some calls could make use of a different value as the default value!
 * This will be corrected later.
 *
 * \param[in] flag  The parameter to retrieve.
 * \param[in] default_value  Use this default value if the parameter was never set.
 *
 * \return The current value of the parameter, if undefined return default_value.
 */
int wpkgar_build::get_parameter(parameter_t flag, int default_value) const
{
    wpkgar_flags_t::const_iterator it(f_flags.find(flag));
    if(it == f_flags.end())
    {
        // This line is not currently used from wpkg because all the
        // parameters are always all defined from command line arguments
        return default_value; // LCOV_EXCL_LINE
    }
    return it->second;
}


/** \brief Set the level of compression.
 *
 * By default the level of compression used by the system to the maximum
 * level, which is 9. The parameter can be set with the --zlevel option
 * of the wpkg command.
 *
 * In most cases you do not want to use a lower level. However, it could
 * be used when testing so the compression goes faster. However, it probably
 * will not help much in regard to decompression speed or amount of memory
 * used while compressing or decompressing the data.
 *
 * For packages that you offer to other people, you should always use the
 * highest available compression level.
 *
 * \param[in] zlevel  The level of compression to use for the package data.
 */
void wpkgar_build::set_zlevel(int zlevel)
{
    if(zlevel < 1 || zlevel > 9)
    {
        throw wpkgar_exception_parameter("the compression level must be between 1 and 9 inclusive");
    }
    f_zlevel = zlevel;
}


/** \brief Define the compressor to use to compress the data.tar file.
 *
 * This function defines the compressor as the file format to use to compress
 * the data.tar file of a binary package. By default this is set to:
 *
 * \code
 * memfile::memory_file::file_format_best
 * \endcode
 *
 * which means the best compression scheme will be used. This makes use of
 * a lot of memory because it will keep two compressed version of the
 * data.tar file in memory until we discover which one is the smallest.
 * However, with servers now having a lot of memory, it should not be a
 * problem. However, if you are working with a model that does not have
 * as much memory, forcing one compressor will definitively help.
 *
 * Also, to quickly test packages, you may avoid the compression by using
 * this format:
 *
 * \code
 * memfile::memory_file::file_format_other
 * \endcode
 *
 * This is legal, even in a dpkg package.
 */
void wpkgar_build::set_compressor(memfile::memory_file::file_format_t compressor)
{
    switch(compressor)
    {
    case memfile::memory_file::file_format_other: // no compression
    case memfile::memory_file::file_format_gz:
    case memfile::memory_file::file_format_bz2:
    case memfile::memory_file::file_format_lzma:
    case memfile::memory_file::file_format_xz:
    case memfile::memory_file::file_format_best: // try them all and keep the smallest
        f_compressor = compressor;
        break;

    default:
        throw wpkgar_exception_parameter("invalid compressor specification");

    }
}


/** \brief Set the maximum length of a path.
 *
 * This function is used to define the maximum length of a path in an archive.
 * By default the size is set to 1024. Note that the packager only generates
 * a warning because of length limits by default. To get an error, use a
 * negative size (i.e. -4096 to limit file length that work on Linux and
 * MS-Windows.)
 *
 * Note that the function adjusts the limit to a minimum of 64 characters
 * to avoid potential problems with limits that are too small.
 *
 * \exception controlled_vars_error_out_of_bounds
 * The maximum length accepted is 64Kb (65536). A large value generates an
 * exception.
 *
 * \param[in] limit  The limit of the filename, if positive, emit a warning,
 *                   if negative, emit an error.
 */
void wpkgar_build::set_path_length_limit(int limit)
{
    // minimum is 256 characters
    if(labs(limit) < 64)
    {
        limit = limit < 0 ? -64 : 64;
    }
    f_path_length_limit = limit;
}


/** \brief Add an extra directory path to the build environment.
 *
 * This function is used to define an extra path to the build system. This
 * is necessary when we build binary packages from a control.info file.
 * Note though that the control.info file may include a ROOT_TREE instead.
 *
 * If the path is left empty, then it has to be defined in the ROOT_TREE
 * variable. Note that if the extra path is defined this way, then the
 * ROOT_TREE variable is completely ignored.
 *
 * \param[in] extra_path  The extra path to use to build packages from a
 *                        .info file
 */
void wpkgar_build::set_extra_path(const wpkg_filename::uri_filename& extra_path)
{
    f_extra_path = extra_path;
}


/** \brief Define the filename of the build number file.
 *
 * By default the build number file is found in the wpkg directory:
 *
 * \code
 * wpkg/build_number
 * \endcode
 *
 * This function gives the user the ability to change the filename. The
 * wpkg command line option is --build-number-filename ...
 *
 * \param[in] filename  The name of the build number file.
 */
void wpkgar_build::set_build_number_filename(const wpkg_filename::uri_filename& filename)
{
    f_build_number_filename = filename;
}


/** \brief Increase the build number from the build number file.
 *
 * This function loads a file that is expected to represent the build
 * number of this project. The file is expected to be composed of only
 * digits (0-9) and a newline character.
 *
 * If the file does not exist, nothing happens.
 *
 * If the file is empty, the number is considered to be zero.
 *
 * \todo
 * We may want to offer a way to find the build number in a generic text
 * file so someone could put that number in a header or some similar
 * file. However, it is always somewhat dangerous to have a third party
 * tool modify one of your files... We could for example have a Build-Number
 * field in the control.info file instead.
 *
 * \return true if it worked and the build number was incremented.
 */
bool wpkgar_build::increment_build_number()
{
    int build_number(0);
    const bool result(load_build_number(build_number, false));
    if(result)
    {
        ++build_number;
        memfile::memory_file output;
        output.create(memfile::memory_file::file_format_other);
        output.printf("%d\n", build_number);
        output.write_file(f_build_number_filename);
    }
    return result;
}


/** \brief Load the build number.
 *
 * This function loads the build number from the build number file.
 *
 * This function returns false if the build file cannot be read in which
 * case the build number is set to zero.
 *
 * \param[out] build_number  The build number value, zero if undefined or
 *                           not yet incremented to 1 or more.
 * \param[in] quiet  If true, generate errors if the input file is invalid.
 *
 * \return true when the build number was read as expected, false if the
 *         build number is not available.
 */
bool wpkgar_build::load_build_number(int& build_number, bool quiet) const
{
    build_number = 0;

    if(!f_build_number_filename.exists())
    {
        return true;
    }

    memfile::memory_file file;
    file.read_file(f_build_number_filename);
    int offset(0);
    std::string line;
    if(file.read_line(offset, line))
    {
        std::string ln;
        if(file.read_line(offset, ln))
        {
            if(!quiet)
            {
                wpkg_output::log("specified build number file %1 has more than one line, which is not valid.")
                        .quoted_arg(f_build_number_filename)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_build_info)
                    .action("build-source");
            }
            return false;
        }
        // transform the line into a build number
        for(const char *l(line.c_str()); *l != '\0' && *l != '\n'; ++l)
        {
            if(*l < '0' || *l > '9')
            {
                if(!quiet)
                {
                    build_number = 0;
                    wpkg_output::log("specified build number file %1 is not just a number.")
                            .quoted_arg(f_build_number_filename)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_build_info)
                        .action("build-source");
                }
                return false;
            }
            build_number = build_number * 10 + *l - '0';
        }
    }

    return true;
}


/** \brief Define the output directory.
 *
 * By default the new packages get saved in the current directory which
 * on a build system is likely not the right place. This is often a
 * repository directory.
 *
 * Note that the package is saved at once, pretty quickly, at the end of
 * the process. So in theory, it is unlikely to cause a problem to save a
 * package directly to a live repository directory, however, it is still
 * not a good idea. You should have a script that moves the resulting
 * file to your repository the package was saved, with the move being
 * done between two directories on the same hard drive partition.
 */
void wpkgar_build::set_output_dir(const wpkg_filename::uri_filename& output_dir)
{
    f_output_dir = output_dir;
}


/** \brief Define the output repository directory.
 *
 * By default new packages are saved in the current directory.
 * If you are managing a repository, you probably want those packages to
 * directory be saved in your repository instead.
 *
 * This parameter defines the root of the repository directory. The system
 * automatically adds the path of the Distribution field (in case of a
 * source package, the current distribution being built) and then the
 * Component field.
 *
 * \code
 * <output repository directory>/<distribution>/<component>
 * \endcode
 *
 * If the component is a single segment, main is added first:
 *
 * \code
 * <output repository directory>/<distribution>/main/<component>
 * \endcode
 *
 * \param[in] output_dir  The output directory where the file is saved.
 */
void wpkgar_build::set_output_repository_dir(const wpkg_filename::uri_filename& output_dir)
{
    f_output_repository_dir = output_dir;
}


/** \brief Set the output filename.
 *
 * This function can be used to setup the filename of the package file
 * although it is not recommended that you do so because package files
 * have very specific names defined as:
 *
 * \code
 * <package name>_<version>_<architecture>.deb
 * \endcode
 *
 * The \<architecture> is not added for source packages. Instead we have
 * a "-src" appended to the package name as in:
 *
 * \code
 * <package name>-src_<version>.deb
 * \endcode
 *
 * This scheme is followed by all the parts of wpkg such as the automatic
 * updates, upgrades, builds, so changing the filename here is considered
 * dangerous, yet at times it is required.
 *
 * \param[in] filename  The filename used to save the package.
 */
void wpkgar_build::set_filename(const wpkg_filename::uri_filename& filename)
{
    if(!filename.dirname().empty())
    {
        throw wpkgar_exception_parameter("the filename of a package cannot include a directory (" + filename.original_filename() + "); use --output-dir for the directory part");
    }
    if(filename.msdos_drive() != wpkg_filename::uri_filename::uri_no_msdos_drive)
    {
        throw wpkgar_exception_parameter("the filename of a package cannot include a drive specification (" + filename.original_filename() + "); use --output-dir for the drive part");
    }
    f_filename = filename;
}


/** \brief Define an installation prefix for the project.
 *
 * In most cases, a project to be installed on a Linux system (Unix in general)
 * will get an installation prefix of "/usr". Under MS-Windows, it is common
 * to leave the prefix empty. It is also possible to make use of other
 * prefixes. For example, we may use "/opt/usys" for our entire usys
 * environment so as to make sure it is properly separate from the standard
 * operating system environment.
 *
 * The default is the empty string (like MS-Windows.)
 *
 * \param[in] install_prefix  The installation prefix to use when building
 *                            source and binary packages from a project or
 *                            a source package respectively.
 */
void wpkgar_build::set_install_prefix(const wpkg_filename::uri_filename& install_prefix)
{
    f_install_prefix = install_prefix;
}


/** \brief Define the name of the generator to use with cmake.
 *
 * This parameter is used with cmake to ensure the proper generator. By
 * default no generator is specified to cmake.
 *
 * In most cases, "Unix Makefiles" is used for Linux and other Unix systems.
 * Under MS-Windows, we suggest the "NMake Makefiles" generator and then
 * use of the NMake tool to compile.
 *
 * \param[in] generator  Name of the generator as shown by "cmake --help"
 */
void wpkgar_build::set_cmake_generator(const std::string& generator)
{
    f_cmake_generator = generator;
}


/** \brief Define the name of the make tool.
 *
 * Once cmake generated a set of files to use to compile a project, it is
 * necessary to run a tool. The name of that tool is defined by this
 * function. It defaults to "make".
 *
 * \param[in] make  Name of the make tool.
 */
void wpkgar_build::set_make_tool(const std::string& make)
{
    f_make_tool = make;
}


/** \brief Define the full name to the wpkg program.
 *
 * This function is used to define the full program name to the wpkg
 * program to be used when building binary packages from source
 * repositories.
 *
 * This parameter is generally set using argv[0] from the main()
 * function.
 *
 * \param[in] program_name  The wpkg program path and file name.
 */
void wpkgar_build::set_program_fullname(const std::string& program_name)
{
    f_program_fullname = program_name;
}


/** \brief Add a filename an exception.
 *
 * This function is used to add patterns to check against each filename
 * to be added to the data.tar archive. If it matches, then the file
 * does not get added to the archive.
 *
 * There are some default patterns, such as *.bak, added by the constructor.
 * These can be cleared out calling this function with \p filename set to
 * the empty string. Actually, doing so clears all the patterns added so
 * far, including the default and user defined patterns.
 *
 * \note
 * The pattern cannot include an MS-DOS drive specification.
 *
 * \note
 * The pattern cannot include a path, it must just be a pattern that can be
 * used against one basename.
 *
 * \param[in] pattern  The name or pattern of a filename to exclude from
 *                      the data.tar file.
 */
void wpkgar_build::add_exception(const wpkg_filename::uri_filename& pattern)
{
    if(pattern.empty())
    {
        f_exceptions.clear();
    }
    else
    {
        if(pattern.segment_size() != 1)
        {
            // this is a filename only, not a path!
            throw wpkgar_exception_parameter("an exception filename cannot include a slash");
        }
        if(pattern.msdos_drive() != wpkg_filename::uri_filename::uri_no_msdos_drive)
        {
            throw wpkgar_exception_parameter("an exception filename cannot include a drive specification (" + pattern.original_filename() + ")");
        }
        f_exceptions.push_back(pattern);
    }
}


/** \brief Is filename an exception?
 *
 * This function checks whether any segment of the filename is a match against
 * any of the exceptions. So if an exception is ".svn" and that appears
 * anywhere in filename, that file is viewed as an exception.
 *
 * \param[in] filename  The filename to check against the exceptions.
 *
 * \return true if any segment of \p filename matched any exception.
 */
bool wpkgar_build::is_exception(const wpkg_filename::uri_filename& filename) const
{
    // the vector of exceptions checks each part of the filename
    // either individually or as a whole, but in most cases we
    // want to avoid things such as CVS, .svn, *.swp which are
    // better matched against one individual part of the filename
    int max_segments(filename.segment_size());
    for(int i(0); i < max_segments; ++i)
    {
        wpkg_filename::uri_filename name(filename.segment(i));
        for(exception_vector_t::const_iterator it(f_exceptions.begin());
                    it != f_exceptions.end(); ++it)
        {
            if(name.glob(it->path_only().c_str()))
            {
                return true;
            }
        }
    }

    return false;
}


/** \brief Retrieve the name of the package built.
 *
 * The build object is expected to be used to create packages. By default,
 * the package name is expected to be generated by the system because it
 * makes use of parameters from the control file of the package to generate
 * the exact name necessary for that package.
 *
 * This function returns that generated name once available, which is at
 * the very end of the build() command. This means the build worked as
 * expected. Before then the package name is set to the empty string.
 *
 * \return The name of the package once available, the empty string otherwise.
 */
wpkg_filename::uri_filename wpkgar_build::get_package_name() const
{
    return f_package_name;
}


/** \brief Build packages.
 *
 * This function is the one used to build packages from whatever information
 * was given to the system. The function is capable of building:
 *
 * \li Source Packages
 *
 * When creating a wpkgar_build object with an empty build directory, this
 * function assumes that the current working directory is a project directory
 * that needs to be packed in a source package.
 *
 * This process includes a validation of the project used to ensure that the
 * project is going to be fully package-able.
 *
 * \li Binary Package from Source Package
 *
 * The package generation starts from a source package, which gets installed
 * in a target system, the packager then compiles the source, installs each
 * component to generate the corresponding binary package which gets saved
 * in a repository directory or the current directory.
 *
 * \li One Binary Package from a control file
 *
 * Using one control file, it is possible to create one binary package. This
 * is the default method of creating binary package without first starting
 * with a source package. This method requires you to create an installation
 * environment that is compatible with the final system where the package will
 * be installed. Also, the directory specified in this case has to include
 * a WPKG (or DEBIAN) directory with the control file.
 *
 * \li Many Binary Packages from a control.info file
 *
 * This method makes use of a control.info file and an extra path possibly
 * defined in the control.info as the ROOT_TREE variable. That extra path
 * defines the location of all the directories that are to be used to generate
 * the packages. This method to generate binary packages is actually used by
 * the source package method which needs a valid control.info file and uses
 * the list of Sub-Packages as the list of components to install and
 * package, just like with this method.
 */
void wpkgar_build::build()
{
    if(f_build_directory.empty())
    {
        build_source();
        return;
    }

    wpkg_filename::uri_filename::file_stat s;
    if(f_build_directory.os_stat(s) != 0)
    {
        throw wpkgar_exception_io("build directory \"" + f_build_directory.original_filename() + "\" does not exist or is not accessible");
    }

    switch(s.get_mode() & S_IFMT)
    {
    case S_IFDIR:
        if(f_build_directory.append_child("WPKG").exists()
        || f_build_directory.append_child("DEBIAN").exists())
        {
            // simple binary package build process
            build_deb(f_build_directory);
        }
        else if(f_build_directory.append_child("sources").exists())
        {
            // build all binary packages from a source repository
            build_repository();
        }
        else
        {
            // TODO: should we check some more stuff to see what
            //       sub-directory is missing?
            throw wpkgar_exception_io("the WPKG package sub-directory is missing");
        }
        break;

    case S_IFREG:
        {
#ifdef WINDOWS
            case_insensitive::case_insensitive_string ext(f_build_directory.extension());
#else
            std::string ext(f_build_directory.extension());
#endif
            if(ext == "deb")
            {
                build_packages();
            }
            else
            {
                build_info();
            }
        }
        break;

    default:
        throw wpkgar_exception_io("the specified filename is neither a build directory nor an info file");

    }
}


/** \brief Append a file to a control.tar or data.tar tarball.
 *
 * There are two main reason for checking the length of the path in a package:
 *
 * \li Assuming the resulting package is expected to be installed on multiple
 * destination, or a destination that may have different file systems, then
 * having a maximum set to the smallest number of accepted characters is
 * generally wise. For example, if you use a file system that accepts at most
 * 1Kb filenames, then letting a user on a file system accepting 4Kb
 * filenames to create a package with 2Kb filenames will result in errors at
 * time of installation of the package.
 *
 * \li The packages may get installed in a directory other than the root
 * directory meaning that the parent path eliminates that many characters
 * from the total supported by the operating system. For example, if you
 * create a package with a filename of 3Kb total and a user attempts to
 * install that package under a path which is 1.5Kb, it will not work under
 * Linux which in general limits the length of the path to 4Kb and 4.5Kb is
 * too much.
 *
 * In most cases, paths are quite limited and this function is not going to
 * generate anything. Frankly, the default limit of 1,024 should rarely be
 * reached. This being said, some tools, such as Doxygen, generate very long
 * filenames, so it could reach such length fairly quickly.
 *
 * Note that the limit is against the full filename (path + basename). The
 * basename itself is generally limited to 255 or 256 characters by the
 * file system that you are using.
 *
 * \param[in] archive  The archive where file is to be added.
 * \param[in] info  The information about the file being happened.
 * \param[in] file  The file to be appended.
 */
void wpkgar_build::append_file(memfile::memory_file& archive, memfile::memory_file::file_info& info, memfile::memory_file& file)
{
    const std::string::size_type length(info.get_filename().length());

    if(f_path_length_limit < 0)
    {
        // limit is negative, if too long it's an error
        if(length > static_cast<std::string::size_type>(-f_path_length_limit))
        {
            wpkg_output::log("full filename %1 is too long for the package.")
                    .quoted_arg(info.get_filename())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .action("build-source");
        }
    }
    else
    {
        // limit is positive, too long is just a warning
        if(length > static_cast<std::string::size_type>(f_path_length_limit))
        {
            wpkg_output::log("full filename %1 is quite long for this package.")
                    .quoted_arg(info.get_filename())
                .level(wpkg_output::level_warning)
                .module(wpkg_output::module_build_package)
                .action("build-source");
        }
    }

    archive.append_file(info, file);
}


/** \brief Save a package (Debian .ar file)
 *
 * This function saves the debian_ar file to the package file.
 *
 * We have a separate function because the determination of the filename
 * includes the use of many different parameters defined in the build class
 * and the control file.
 *
 * The function has the side effect of setting up the f_package_name field
 * which can later be retrieved with the get_package_name() function.
 *
 * \param[in] debian_ar  The Debian package to save.
 * \param[in] fields  The control file of the package.
 *
 * \sa get_package_name()
 */
void wpkgar_build::save_package(memfile::memory_file& debian_ar, const wpkg_control::control_file& fields)
{
    // now generate the output filename and save the result
    const std::string package(fields.get_field(wpkg_control::control_file::field_package_factory_t::canonicalized_name()));
    const std::string arch_value(fields.get_field(wpkg_control::control_file::field_architecture_factory_t::canonicalized_name()));
    const wpkg_architecture::architecture arch(arch_value);
    const bool is_source(arch.is_source());
    if(f_filename.empty())
    {
        const std::string version(fields.get_field(wpkg_control::control_file::field_version_factory_t::canonicalized_name()));
        std::string package_name = package + "_" + wpkg_util::canonicalize_version_for_filename(version);
        if(!is_source)
        {
            package_name += "_" + arch_value;
        }
        package_name += ".deb";
        f_package_name = package_name;
    }
    else
    {
        f_package_name = f_filename;
    }
    // note that at this point f_package_name cannot be a full path so
    // appending it as a child will always work

    // the output directory is defined from the Distribution + Component
    // fields if these and the repository directory are both defined
    if(fields.field_is_defined(wpkg_control::control_file::field_distribution_factory_t::canonicalized_name())
    && !f_output_repository_dir.empty())
    {
        wpkg_filename::uri_filename output_dir(f_output_repository_dir);
        if(is_source)
        {
            // for source packages, always use "sources" here
            output_dir = output_dir.append_child("sources");
        }
        else
        {
            // the Distribution has to be a valid path
            const std::string path(fields.get_field(wpkg_control::control_file::field_distribution_factory_t::canonicalized_name()));
            if(path.find_first_of(" \n") != std::string::npos)
            {
                // to bad... so close!
                const std::shared_ptr<wpkg_field::field_file::field_t> path_info(fields.get_field_info(wpkg_control::control_file::field_distribution_factory_t::canonicalized_name()));
                wpkg_output::log("control:%1:%2: the Distribution field path %3 cannot include spaces or new lines when defined in a binary package")
                        .arg(path_info->get_filename())
                        .arg(path_info->get_line())
                        .quoted_arg(path)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_build_package)
                    .action("build-source");
                return;
            }
            output_dir = output_dir.append_child(path);
        }
        if(fields.field_is_defined(wpkg_control::control_file::field_component_factory_t::canonicalized_name()))
        {
            wpkg_filename::uri_filename component(fields.get_field(wpkg_control::control_file::field_component_factory_t::canonicalized_name()));
            if(component.segment_size() > 2)
            {
                output_dir = output_dir.append_child(component.segment(0)).append_child(component.segment(1));
            }
            else
            {
                output_dir = output_dir.append_child(component.original_filename());
            }
        }
        f_package_name = output_dir.append_child(f_package_name.path_only());
    }
    else if(!f_output_dir.empty())
    {
        f_package_name = f_output_dir.append_child(f_package_name.path_only());
    }
    debian_ar.write_file(f_package_name, true);
}


/** \brief Check for a set of filenames.
 *
 * This function checks for a set of filenames and if it finds it, returns
 * that name. The function expects the very first name to be the most
 * \em expected name for this file. If that name is found, then the
 * \p rename parameter is set to false. If any other value is returned, the
 * rename parameter will be true.
 *
 * \param[in] filenames  The list of filenames to check for existence.
 * \param[out] rename  Whether the file will be renamed in the source data.tar
 *                     archive. true if the first filename matches, false in
 *                     all other cases.
 *
 * \return The first file that matches, the empty string if no files matches.
 */
wpkg_filename::uri_filename wpkgar_build::find_source_file(const char **filenames, controlled_vars::fbool_t& rename)
{
    rename = false;
    for(; *filenames != NULL; ++filenames)
    {
        wpkg_filename::uri_filename test(*filenames);
        if(test.exists())
        {
            wpkg_output::log("found copyright file %1")
                    .quoted_arg(*filenames)
                .debug(wpkg_output::debug_flags::debug_files)
                .module(wpkg_output::module_build_package);
            return *filenames;
        }
        rename = true;
    }

    return "";
}


/** \brief Validate a project.
 *
 * This function goes through the files found in a project to validate its
 * content for inclusion in a wpkg source package (which are binary like
 * packages.)
 *
 * The validation is used to increase wpkg's chances to be able to create
 * valid binary packages, although when creating the source we do not test
 * whether the project compiles, or its tests run, etc. (which would not
 * automatically be useful since the source package may not be created on
 * a computer that is setup to do the full compile.)
 *
 * The list of validations appears in a table named g_source_validation_property
 * which one can see from the wpkg command line using the --help command
 * like this:
 *
 * \code
 * wpkg --help build-validations [--verbose]
 * \endcode
 *
 * \param[in] validate_status  The validation list of properties to be checked.
 * \param[out] controlinfo_fields  The control.info file we're dealing with.
 *
 * \return true if all the validations passed.
 */
bool wpkgar_build::validate_source(source_validation& validate_status, wpkg_control::control_file& controlinfo_fields)
{
    const uint32_t err_count(wpkg_output::get_output_error_count());

    // We assume that the current directory 'get_cwd()' is the project
    // directory, so it must have the changelog, control.info, and
    // CMakeLists.txt files, among others. The following loop checks
    // for all those files

    wpkg_filename::uri_filename cwd(wpkg_filename::uri_filename::get_cwd());
    wpkg_output::log("validating project directory %1")
            .quoted_arg(cwd)
        .level(wpkg_output::level_info)
        .module(wpkg_output::module_build_package)
        .action("build-source");

    wpkg_filename::uri_filename cmakeliststxt;
    wpkg_filename::uri_filename license;
    wpkg_filename::uri_filename readme;
    wpkg_filename::uri_filename install;

    // first check files that we want to rename/move in the source package
    const char *changelog_filenames[] =
    {
        "wpkg/changelog",
        "debian/changelog",
        "changelog",
        "Changelog",
        "ChangeLog",
        NULL
    };
    f_changelog_filename = find_source_file(changelog_filenames, f_rename_changelog);
    const bool has_changelog(!f_changelog_filename.empty());

    const char *copyright_filenames[] =
    {
        "wpkg/copyright",
        "debian/copyright",
        NULL
    };
    f_copyright_filename = find_source_file(copyright_filenames, f_rename_copyright);

    const char *controlinfo_filenames[] =
    {
        "wpkg/control.info",
        "control.info",
        NULL
    };
    f_controlinfo_filename = find_source_file(controlinfo_filenames, f_rename_controlinfo);

    memfile::memory_file project_dir;
    project_dir.dir_rewind(".", true);
    for(;;)
    {
        f_manager->check_interrupt();

        memfile::memory_file::file_info info;
        if(!project_dir.dir_next(info, NULL))
        {
            break;
        }
        // define a name that is not case sensitive for some of the files
        // that we are looking for; also avoid the path in that one
        case_insensitive::case_insensitive_string basename(info.get_basename());
        if(info.get_filename() == "CMakeLists.txt")
        {
            cmakeliststxt = info.get_uri();

            wpkg_output::log("found CMakeLists.txt file %1")
                    .quoted_arg(cmakeliststxt)
                .debug(wpkg_output::debug_flags::debug_files)
                .module(wpkg_output::module_build_package);
        }
        //else if(info.get_filename() == "debian/copyright"
        //     || info.get_filename() == "wpkg/copyright")
        //{
        //    // The copyright file is a special Debian file with a computer
        //    // readable format. We want to check it and also make sure it
        //    // points to the right places.
        //    if(f_copyright_filename.empty())
        //    {
        //        // we keep the URI as defined in the source info
        //        f_copyright_filename = info.get_uri();

        //        wpkg_output::log("found copyright file %1")
        //                .quoted_arg(f_copyright_filename)
        //            .debug(wpkg_output::debug_flags::debug_files)
        //            .module(wpkg_output::module_build_package);
        //    }
        //    else
        //    {
        //        wpkg_output::log("we found more than one copyright file (%1 and %2)")
        //                .quoted_arg(f_copyright_filename)
        //                .quoted_arg(info.get_filename())
        //            .level(wpkg_output::level_error)
        //            .module(wpkg_output::module_build_package)
        //            .action("build-source");
        //    }
        //}
        else if(basename == "changelog" && !has_changelog)
        {
            // Any ChangeLog file that we discover is viewed as the change
            // log; there can be only one although we do not care too much
            // about the case used to spell the name
            if(f_changelog_filename.empty())
            {
                // we keep the URI as defined in the source info
                f_changelog_filename = info.get_uri();

                wpkg_output::log("found changelog file %1")
                        .quoted_arg(f_changelog_filename)
                    .debug(wpkg_output::debug_flags::debug_files)
                    .module(wpkg_output::module_build_package);
            }
            else
            {
                wpkg_output::log("we found more than one changelog file (%1 and %2)")
                        .quoted_arg(f_changelog_filename)
                        .quoted_arg(info.get_filename())
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_build_package)
                    .action("build-source");
            }
        }
        //else if(info.get_filename() == "debian/control.info"
        //     || info.get_filename() == "wpkg/control.info")
        //{
        //    // we found the .info file
        //    if(f_controlinfo_filename.empty())
        //    {
        //        f_controlinfo_filename = info.get_uri();

        //        wpkg_output::log("found control.info file %1")
        //                .quoted_arg(f_controlinfo_filename)
        //            .debug(wpkg_output::debug_flags::debug_files)
        //            .module(wpkg_output::module_build_package);
        //    }
        //    else
        //    {
        //        // This cannot happen since we check as is and not case insensitively
        //        wpkg_output::log("we found more than one control.info file")
        //            .level(wpkg_output::level_error)
        //            .module(wpkg_output::module_build_info)
        //            .action("build-source");
        //    }
        //}
        else if(basename == "COPYING" || basename == "COPYING.txt"
             || basename == "LICENSE" || basename == "LICENSE.txt")
        {
            // we require a license, it may be written in one of many
            // different ways; we save that to check with the copyright
            // notice too; but we require an explicit license in all
            // projects
            if(license.empty())
            {
                license = info.get_uri();

                wpkg_output::log("found a license file %1")
                        .quoted_arg(license)
                    .debug(wpkg_output::debug_flags::debug_files)
                    .module(wpkg_output::module_build_package);
            }
            else
            {
                wpkg_output::log("we found more than one license file (%1 and %2); which is fine as long as you specify the proper one in the copyright file")
                        .quoted_arg(license)
                        .quoted_arg(info.get_filename())
                    .level(wpkg_output::level_warning)
                    .module(wpkg_output::module_build_info)
                    .action("build-source");
            }
        }
        else if(basename == "README" || basename == "README.txt")
        {
            if(readme.empty())
            {
                readme = info.get_uri();

                wpkg_output::log("found readme file %1")
                        .quoted_arg(readme)
                    .debug(wpkg_output::debug_flags::debug_files)
                    .module(wpkg_output::module_build_package);
            }
            else
            {
                wpkg_output::log("we found more than one README file (%1 and %2); which is fine although you may want to remove one of them to avoid confusion")
                        .quoted_arg(readme)
                        .quoted_arg(info.get_filename())
                    .level(wpkg_output::level_warning)
                    .module(wpkg_output::module_build_info)
                    .action("build-source");
            }
        }
        else if(basename == "INSTALL" || basename == "INSTALL.txt")
        {
            if(install.empty())
            {
                install = info.get_uri();

                wpkg_output::log("found an INSTALL file %1")
                        .quoted_arg(install)
                    .debug(wpkg_output::debug_flags::debug_files)
                    .module(wpkg_output::module_build_package);
            }
            else
            {
                wpkg_output::log("we found more than one INSTALL file (%1 and %2); which is fine although you may want to remove one of them to avoid confusion")
                        .quoted_arg(install)
                        .quoted_arg(info.get_filename())
                    .level(wpkg_output::level_warning)
                    .module(wpkg_output::module_build_info)
                    .action("build-source");
            }
        }
    }

    // *** wpkg/control.info ***
    std::string package;
    if(f_controlinfo_filename.empty())
    {
        wpkg_output::log("we could not find the wpkg/control.info file, we cannot create a valid source package from this project")
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_info)
            .action("build-source");
        validate_status.done("control.info", source_validation::source_property::SOURCE_VALIDATION_STATUS_MISSING);
    }
    else
    {
        validate_status.set_value("control.info", f_controlinfo_filename.original_filename());

        // loading the info file must work
        memfile::memory_file data;
        data.read_file(f_controlinfo_filename);
        controlinfo_fields.set_input_file(&data);
        if(controlinfo_fields.read())
        {
            validate_status.done("control.info", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
            // it is valid in the sense that we could load the control file
            // it may not be valid if the Sub-Packages is not properly
            // defined (i.e. has a default Package field that match one to
            // one with the name of all the sub-packages Package field.)
            if(!controlinfo_fields.field_is_defined(wpkg_control::control_file::field_subpackages_factory_t::canonicalized_name()))
            {
                wpkg_output::log("the %1 file does not include a Sub-Packages field; use \"Sub-Packages: runtime*\" by default if needed")
                        .quoted_arg(f_controlinfo_filename)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_build_info)
                    .action("build-source");
                validate_status.done("control.info", source_validation::source_property::SOURCE_VALIDATION_STATUS_INCOMPLETE);
            }
            else
            {
                // *** Package ***
                // We check that we have a main package definition and that
                // all the sub-packages have a name that starts like the main
                // package followed by a dash (i.e. wpkg and wpkg-doc)
                wpkg_field::field_file::list_t sub_packages = controlinfo_fields.get_field_list(wpkg_control::control_file::field_subpackages_factory_t::canonicalized_name());
                for(wpkg_control::control_file::field_file::list_t::const_iterator it(sub_packages.begin()); it != sub_packages.end(); ++it)
                {
                    // get the sub-package name
                    std::string sub_name(*it);
                    if(!sub_name.empty() && *sub_name.rbegin() == '*')
                    {
                        sub_name = sub_name.substr(0, sub_name.length() - 1);
                        validate_status.set_value("Sub-Packages", sub_name);
                        std::string field_name(wpkg_control::control_file::field_package_factory_t::canonicalized_name() + ("/" + sub_name));
                        if(!controlinfo_fields.field_is_defined(field_name, true))
                        {
                            wpkg_output::log("Mandatory field %1 is not defined")
                                    .quoted_arg(field_name)
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_build_info)
                                .action("build-source");
                            validate_status.done("Package", source_validation::source_property::SOURCE_VALIDATION_STATUS_INCOMPLETE);
                        }
                        else
                        {
                            package = controlinfo_fields.get_field(field_name);
                            validate_status.done("Package", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
                            validate_status.set_value("Package", field_name);
                        }
                        break;
                    }
                    // we do not need to test more because the subpackages
                    // field defined in the control_file already does that
                }
                // we did not find a name with an asterisk
                if(package.empty())
                {
                    wpkg_output::log("\"Sub-Packages: %1\" does not include a hidden name (a name that ends with *)")
                            .arg(controlinfo_fields.get_field(wpkg_control::control_file::field_subpackages_factory_t::canonicalized_name()))
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_build_info)
                        .action("build-source");
                    validate_status.done("Package", source_validation::source_property::SOURCE_VALIDATION_STATUS_INCOMPLETE);
                }
                else
                {
                    // *** Sub-Package ***
                    // now test the names again to make sure they all starts with "<package>-..."
                    std::map<std::string, bool> found;
                    std::string introducer(package + "-");
                    for(wpkg_control::control_file::field_file::list_t::const_iterator it(sub_packages.begin()); it != sub_packages.end(); ++it)
                    {
                        // get the sub-package name
                        std::string sub_name(*it);
                        if(!sub_name.empty() && *sub_name.rbegin() != '*')
                        {
                            std::string field_name(wpkg_control::control_file::field_package_factory_t::canonicalized_name() + ("/" + sub_name));
                            if(!controlinfo_fields.field_is_defined(field_name, true))
                            {
                                wpkg_output::log("Mandatory field %1 is not defined")
                                        .quoted_arg(field_name)
                                    .level(wpkg_output::level_error)
                                    .module(wpkg_output::module_build_info)
                                    .action("build-source");
                                validate_status.done("Sub-Packages", source_validation::source_property::SOURCE_VALIDATION_STATUS_INCOMPLETE);
                            }
                            else
                            {
                                std::string sub_package_name = controlinfo_fields.get_field(field_name);
                                if(sub_package_name.substr(0, introducer.length()) != introducer)
                                {
                                    wpkg_output::log("%1 has an invalid value (%2), it must start with %3")
                                            .quoted_arg(field_name)
                                            .arg(sub_package_name)
                                            .quoted_arg(introducer)
                                        .level(wpkg_output::level_error)
                                        .module(wpkg_output::module_build_info)
                                        .action("build-source");
                                    validate_status.done("Sub-Packages", source_validation::source_property::SOURCE_VALIDATION_STATUS_INVALID);
                                }
                                else if(found.find(sub_package_name) != found.end())
                                {
                                    wpkg_output::log("The control.info file of %1 has two package names that are identical: %2")
                                            .quoted_arg(package)
                                            .quoted_arg(sub_package_name)
                                        .level(wpkg_output::level_error)
                                        .module(wpkg_output::module_build_info)
                                        .action("build-source");
                                    validate_status.done("Sub-Packages", source_validation::source_property::SOURCE_VALIDATION_STATUS_INVALID);
                                }
                                else
                                {
                                    found[sub_package_name] = true;
                                }
                            }
                        }
                    }
                    if(validate_status.get_status("Sub-Packages") == source_validation::source_property::SOURCE_VALIDATION_STATUS_UNKNOWN)
                    {
                        validate_status.done("Sub-Packages", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
                    }
                }
            }

            // *** Architecture ***
            // Force the architecture to source; whatever the control.info defines
            // is not important until we try to build the binaries (although we
            // may be able to add a check later; but remember that if we're here
            // the architecture specified is valid.)
            //if(controlinfo_fields.field_is_defined(wpkg_control::control_file::field_architecture_factory_t::canonicalized_name()))
            //{
            //    ... implement tests ...
            //}
        }
        else
        {
            validate_status.done("control.info", source_validation::source_property::SOURCE_VALIDATION_STATUS_INVALID);
        }
        controlinfo_fields.set_input_file(NULL);
    }

    // check that we found the CMakeLists.txt file
    if(cmakeliststxt.empty())
    {
        wpkg_output::log("a source package requires a CMakeLists.txt file in the root directory of the project")
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_package)
            .package(package)
            .action("build-source");
        validate_status.done("CMakeLists.txt", source_validation::source_property::SOURCE_VALIDATION_STATUS_MISSING);
    }
    else
    {
        // note: we do not really know whether it is valid at this point,
        // although we could test before returning to see where all the
        // necessary targets exist (i.e. to install of each component,
        // to run all the tests, etc.)
        validate_status.done("CMakeLists.txt", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
    }

    // *** wpkg/changelog ***
    wpkg_changelog::changelog_file changelog_file;
    if(f_changelog_filename.empty())
    {
        wpkg_output::log("we could not find a wpkg/changelog file, we cannot create a valid source package from this project")
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_info)
            .package(package)
            .action("build-source");
        validate_status.done("changelog", source_validation::source_property::SOURCE_VALIDATION_STATUS_MISSING);
    }
    else
    {
        validate_status.set_value("changelog", f_changelog_filename.original_filename());

        // loading the changelog file must work
        memfile::memory_file data;
        data.read_file(f_changelog_filename);
        if(changelog_file.read(data))
        {
            validate_status.done("changelog", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
        }
        else
        {
            // TODO: find a way to determine whether it is INCOMPLETE instead
            //       of INVALID
            validate_status.done("changelog", source_validation::source_property::SOURCE_VALIDATION_STATUS_INVALID);
        }
    }

    // *** wpkg/copyright ***
    wpkg_copyright::copyright_info copyright_file;
    if(f_copyright_filename.empty())
    {
        wpkg_output::log("we could not find a wpkg/copyright file, we cannot create a valid source package from this project")
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_info)
            .package(package)
            .action("build-source");
        validate_status.done("copyright", source_validation::source_property::SOURCE_VALIDATION_STATUS_MISSING);
    }
    else
    {
        validate_status.set_value("copyright", f_copyright_filename.original_filename());

        // loading the copyright file must work
        memfile::memory_file data;
        data.read_file(f_copyright_filename);
        if(copyright_file.read(&data))
        {
            validate_status.done("copyright", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
        }
        else
        {
            // TODO: find a way to determine whether it is INCOMPLETE instead
            //       of INVALID
            validate_status.done("copyright", source_validation::source_property::SOURCE_VALIDATION_STATUS_INVALID);
        }
    }

    // *** README ***
    if(readme.empty())
    {
        wpkg_output::log("a source package should have a README (or README.txt) file with a long description of the package")
            .level(wpkg_output::level_warning)
            .module(wpkg_output::module_build_package)
            .action("build-source");
        validate_status.done("README", source_validation::source_property::SOURCE_VALIDATION_STATUS_MISSING);
    }
    else
    {
        validate_status.done("README", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);

        // TODO: check that it is a text file?
    }

    // *** INSTALL ***
    if(install.empty())
    {
        wpkg_output::log("a source package should have an INSTALL (or INSTALL.txt) file with easy to follow steps to compile your project")
            .level(wpkg_output::level_warning)
            .module(wpkg_output::module_build_package)
            .action("build-source");
        validate_status.done("INSTALL", source_validation::source_property::SOURCE_VALIDATION_STATUS_MISSING);
    }
    else
    {
        validate_status.done("INSTALL", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);

        // TODO: check that it is a text file?
    }

    // if errors (already) occurred then we do not go on
    // (it would be too complicated to test everything again to do the
    // additional validations)
    if(err_count != wpkg_output::get_output_error_count())
    {
        return false;
    }

    if(changelog_file.get_version_count() == 0)
    {
        // our algorithm requires at least one entry
        throw std::logic_error("changelog_file is empty even though we did not detect any errors");
    }
    wpkg_changelog::changelog_file::version v(changelog_file.get_version(0));

    // *** ChangeLog: Package ***
    // verify the package name between the changelog and control.info files
    std::string package_name(v.get_package());
    if(package != package_name)
    {
        wpkg_output::log("the name of the package (%1) does not match with the name found in %3 (%2)")
                .arg(package)
                .arg(package_name)
                .arg(f_changelog_filename)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_package)
            .package(package)
            .action("build-source");
        validate_status.done("Package", source_validation::source_property::SOURCE_VALIDATION_STATUS_INVALID);
    }
    else
    {
        // the name is valid, but this is a source package and it needs to
        // have a different name (so we can install the source package and
        // the main binary package!)
        controlinfo_fields.set_field(wpkg_control::control_file::field_package_factory_t::canonicalized_name(), package + "-src");
        validate_status.done("Package", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
    }

    // *** ChangeLog: Version ***
    // verify the version between the changelog and control.info files
    std::string version(v.get_version());
    if(controlinfo_fields.field_is_defined(wpkg_control::control_file::field_version_factory_t::canonicalized_name()))
    {
        std::string package_version(controlinfo_fields.get_field(wpkg_control::control_file::field_version_factory_t::canonicalized_name()));

        if(wpkg_util::versioncmp(version, package_version) != 0)
        {
            wpkg_output::log("the version of the package (%1) does not match with the version found in %3 (%2)")
                    .arg(package_version)
                    .arg(version)
                    .arg(f_changelog_filename)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .package(package)
                .action("build-source");
            validate_status.done("Version", source_validation::source_property::SOURCE_VALIDATION_STATUS_INVALID);
        }
        else
        {
            // here it looks like the package version is already set, but
            // it may be an expression and we do not want to keep it that
            // way...
            validate_status.done("Version", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
        }
    }
    else
    {
        // the version is valid in the changelog file
        validate_status.done("Version", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
    }

    if(validate_status.get_status("Version") == source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID)
    {
        // save the parsed version
        // (i.e. the version string in the Version field may be an expression
        // and we do not want to keep it that way in the final source file.)
        controlinfo_fields.set_field(wpkg_control::control_file::field_version_factory_t::canonicalized_name(), version);
    }

    // *** ChangeLog: Distributions ***
    wpkg_changelog::changelog_file::version::distributions_t distributions(v.get_distributions());
    if(distributions.size() > 0)
    {
        bool valid(true);

        // +++ Distribution +++
        if(controlinfo_fields.field_is_defined(wpkg_control::control_file::field_distribution_factory_t::canonicalized_name()))
        {
            // since it is defined in both places, we have to make 100% sure
            // that it is an exact match knowing that the order is not
            // important (which makes the following a little more complicated)
            std::map<std::string, bool> found;
            for(wpkg_changelog::changelog_file::version::distributions_t::const_iterator it(distributions.begin()); it != distributions.end(); ++it)
            {
                found[*it] = false;
            }
            std::string distro(controlinfo_fields.get_field(wpkg_control::control_file::field_distribution_factory_t::canonicalized_name()));
            const char *s(NULL);
            for(const char *start(distro.c_str()); *start != '\0'; start = s)
            {
                for(s = start; !isspace(*s) && *s != '\0'; ++s);
                std::string name(start, s - start);
                if(found.find(name) == found.end())
                {
                    wpkg_output::log("distribution %1 defined in your control.info file is not defined in %2")
                            .quoted_arg(name)
                            .quoted_arg(f_changelog_filename)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_build_package)
                        .package(package)
                        .action("build-source");
                    valid = false;
                }
                else
                {
                    found[name] = true;
                }
                for(; *s != '\0' && isspace(*s); ++s);
            }
            for(std::map<std::string, bool>::const_iterator it(found.begin()); it != found.end(); ++it)
            {
                if(!it->second)
                {
                    wpkg_output::log("distribution %1 defined in %2 is not defined your control.info file")
                            .quoted_arg(it->first)
                            .quoted_arg(f_changelog_filename)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_build_package)
                        .package(package)
                        .action("build-source");
                    valid = false;
                }
            }
        }
        else
        {
            std::string distribution;
            for(wpkg_changelog::changelog_file::version::distributions_t::const_iterator it(distributions.begin()); it != distributions.end(); ++it)
            {
                if(!distribution.empty())
                {
                    distribution += " ";
                }
                distribution += *it;
            }
            controlinfo_fields.set_field(wpkg_control::control_file::field_distribution_factory_t::canonicalized_name(), distribution);
        }
        validate_status.done("Distributions", valid ? source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID
                                                    : source_validation::source_property::SOURCE_VALIDATION_STATUS_INVALID);
    }

    // *** ChangeLog: Urgency ***
    if(v.parameter_is_defined("urgency"))
    {
        case_insensitive::case_insensitive_string urgency(v.get_parameter("urgency"));
        std::string urgency_only;
        std::string urgency_comment;
        if(!wpkg_control::control_file::field_urgency_t::is_valid(urgency, urgency_only, urgency_comment))
        {
            wpkg_output::log("the urgency %1 parameter is not valid")
                    .arg(urgency)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .package(package)
                .action("build-source");
            validate_status.done("Urgency", source_validation::source_property::SOURCE_VALIDATION_STATUS_INVALID);
        }
        else if(controlinfo_fields.field_is_defined(wpkg_control::control_file::field_urgency_factory_t::canonicalized_name()))
        {
            case_insensitive::case_insensitive_string package_urgency(controlinfo_fields.get_field(wpkg_control::control_file::field_urgency_factory_t::canonicalized_name()));

            if(urgency != package_urgency)
            {
                wpkg_output::log("the urgency of the package (%1) does not match with the urgency found in %3 (%2)")
                        .arg(package_urgency)
                        .arg(urgency)
                        .arg(f_changelog_filename)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_build_package)
                    .package(package)
                    .action("build-source");
                validate_status.done("Urgency", source_validation::source_property::SOURCE_VALIDATION_STATUS_INVALID);
            }
            else
            {
                validate_status.done("Urgency", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
            }
        }
        else
        {
            // the urgency is valid in the changelog file
            controlinfo_fields.set_field(wpkg_control::control_file::field_urgency_factory_t::canonicalized_name(), urgency);
            validate_status.done("Urgency", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
        }
    }

    // *** ChangeLog: Maintainer ***
    case_insensitive::case_insensitive_string maintainer(v.get_maintainer());
    if(controlinfo_fields.field_is_defined(wpkg_control::control_file::field_maintainer_factory_t::canonicalized_name()))
    {
        case_insensitive::case_insensitive_string package_maintainer(controlinfo_fields.get_field(wpkg_control::control_file::field_maintainer_factory_t::canonicalized_name()));

        if(maintainer != package_maintainer)
        {
            wpkg_output::log("the maintainer of the package (%1) does not match with the maintainer found in %3 (%2)")
                    .arg(package_maintainer)
                    .arg(maintainer)
                    .arg(f_changelog_filename)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .package(package)
                .action("build-source");
            validate_status.done("Maintainer", source_validation::source_property::SOURCE_VALIDATION_STATUS_INVALID);
        }
        else
        {
            validate_status.done("Maintainer", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
        }
    }
    else
    {
        // the maintainer is valid in the changelog file
        controlinfo_fields.set_field(wpkg_control::control_file::field_maintainer_factory_t::canonicalized_name(), maintainer);
        validate_status.done("Maintainer", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
    }

    // *** ChangeLog: Changes-Date ***
    case_insensitive::case_insensitive_string date(v.get_date());
    if(controlinfo_fields.field_is_defined(wpkg_control::control_file::field_changesdate_factory_t::canonicalized_name()))
    {
        case_insensitive::case_insensitive_string package_date(controlinfo_fields.get_field(wpkg_control::control_file::field_changesdate_factory_t::canonicalized_name()));

        if(date != package_date)
        {
            wpkg_output::log("the changes date of the package (%1) does not match with the changes date found in %3 (%2)")
                    .arg(package_date)
                    .arg(date)
                    .arg(f_changelog_filename)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .package(package)
                .action("build-source");
            validate_status.done("Changes-Date", source_validation::source_property::SOURCE_VALIDATION_STATUS_INVALID);
        }
        else
        {
            validate_status.done("Changes-Date", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
        }
    }
    else
    {
        // the maintainer is valid in the changelog file
        controlinfo_fields.set_field(wpkg_control::control_file::field_changesdate_factory_t::canonicalized_name(), date);
        validate_status.done("Changes-Date", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
    }

    // *** ChangeLog: Changes ***
    if(controlinfo_fields.field_is_defined(wpkg_control::control_file::field_changes_factory_t::canonicalized_name()))
    {
        wpkg_output::log("the %1 field cannot be defined in your %2 file")
                .arg(wpkg_control::control_file::field_changes_factory_t::canonicalized_name())
                .arg(f_controlinfo_filename)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_package)
            .package(package)
            .action("build-source");
        validate_status.done("Changes", source_validation::source_property::SOURCE_VALIDATION_STATUS_INVALID);
    }
    else
    {
        // the maintainer is valid in the changelog file
        wpkg_changelog::changelog_file::version::log_list_t logs(v.get_logs());
        std::string changes;
        for(wpkg_changelog::changelog_file::version::log_list_t::const_iterator i(logs.begin()); i != logs.end(); ++i)
        {
            if(i->is_group() && !changes.empty())
            {
                changes += "\n";
            }
            changes += "\n" + i->get_log();
        }
        controlinfo_fields.set_field(wpkg_control::control_file::field_changes_factory_t::canonicalized_name(), changes);
        validate_status.done("Changes", source_validation::source_property::SOURCE_VALIDATION_STATUS_VALID);
    }

    return err_count == wpkg_output::get_output_error_count();
}


/** \brief Prepare the shell environment.
 *
 * This function readies a command by adding a cd to a directory and a set
 * of environment variables that are useful, or even necessary to run in
 * a target system.
 */
void wpkgar_build::prepare_cmd(std::string& cmd, const wpkg_filename::uri_filename& dir)
{
#if defined(MO_WINDOWS)
    cmd = "cd /d ";
#else
    cmd = "cd ";
#endif
    cmd += wpkg_util::make_safe_console_string(dir.full_path());
#if defined(MO_WINDOWS)
    cmd += " && set PATH=";
    cmd += wpkg_util::make_safe_console_string(f_manager->get_inst_path().append_safe_child(f_install_prefix).append_child("bin").full_path());
    cmd += ";%PATH%";
    cmd += " && set WPKG_ROOTDIR=";
    cmd += wpkg_util::make_safe_console_string(f_manager->get_root_path().full_path());
    cmd += " && set WPKG_INSTDIR=";
    cmd += wpkg_util::make_safe_console_string(f_manager->get_inst_path().full_path());
    cmd += " && set WPKG_ADMINDIR=";
    cmd += wpkg_util::make_safe_console_string(f_manager->get_database_path().full_path());
#else
    cmd += " && export PATH=";
    cmd += wpkg_util::make_safe_console_string(f_manager->get_inst_path().append_safe_child(f_install_prefix).append_child("bin").full_path());
    cmd += ":$PATH";
    cmd += " && export LD_LIBRARY_PATH=";
    cmd += wpkg_util::make_safe_console_string(f_manager->get_inst_path().append_safe_child(f_install_prefix).append_child("lib").full_path());
    if(getenv("LD_LIBRARY_PATH") != NULL)
    {
        cmd += ":$LD_LIBRARY_PATH";
    }
    cmd += " && export WPKG_ROOTDIR=";
    cmd += wpkg_util::make_safe_console_string(f_manager->get_root_path().full_path());
    cmd += " && export WPKG_INSTDIR=";
    cmd += wpkg_util::make_safe_console_string(f_manager->get_inst_path().full_path());
    cmd += " && export WPKG_ADMINDIR=";
    cmd += wpkg_util::make_safe_console_string(f_manager->get_database_path().full_path());
#endif
    cmd += " && ";
}


/** \brief Run cmake to ready a development environment.
 *
 * This function readies a development environment by running cmake in
 * a temporary build directory.
 *
 * \param[in] package_name  The name of the concerned package/project.
 * \param[in] build_tmpdir  The build temporary directory.
 * \param[in] sourcedir  The source directory.
 */
bool wpkgar_build::run_cmake(const std::string& package_name, const wpkg_filename::uri_filename& build_tmpdir, const wpkg_filename::uri_filename& sourcedir)
{
    std::string cmd;
    prepare_cmd(cmd, build_tmpdir);

    cmd += "cmake ";
    if(!f_cmake_generator.empty())
    {
        cmd += "-G " + wpkg_util::make_safe_console_string(f_cmake_generator) + " ";
    }
    cmd += wpkg_util::make_safe_console_string(sourcedir.full_path());

    wpkg_output::log("system(%1).")
            .quoted_arg(cmd)
        .level(wpkg_output::level_info)
        .module(wpkg_output::module_run_script)
        .package(package_name)
        .action("execute-script");

    const int r(system(cmd.c_str()));
    if(r != 0)
    {
        wpkg_output::log("system(%1) called returned %2")
                .quoted_arg(cmd)
                .arg(r)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_run_script)
            .package(package_name)
            .action("build-source");
        return false;
    }

    return true;
}


/** \brief Build a source package.
 *
 * This function is used to build a source package. This is expected to
 * run from the root directory of a project. The result is a binary
 * source package that can be specified on the command line of wpkg to
 * generate all the binary packages of that project.
 */
void wpkgar_build::build_source()
{
    // this is a wpkg specific build feature that creates a wpkg source
    // package; wpkg source packages are "binary" files (ar/tar) created
    // from the source of a package and a source specific control file

    // first run a validation
    source_validation sv;
    wpkg_control::source_control_file controlinfo_fields;
    f_manager->set_control_variables(controlinfo_fields);
    if(!validate_source(sv, controlinfo_fields))
    {
        return;
    }

    int build_number(0);
    bool has_build_number(false);
    if(increment_build_number())
    {
        has_build_number = load_build_number(build_number, true);
    }

    // the package name must be defined with a sub-package specification
    // so we make use of the name as saved in the source_validation
    std::string package(controlinfo_fields.get_field(wpkg_control::control_file::field_package_factory_t::canonicalized_name()));

    wpkg_control::source_control_file fields;
    f_manager->set_control_variables(fields);
    wpkg_field::field_file::list_t excluded;
    excluded.push_back(wpkg_control::control_file::field_subpackages_factory_t::canonicalized_name());
    controlinfo_fields.copy(fields, sv.get_value(wpkg_control::control_file::field_subpackages_factory_t::canonicalized_name()), excluded);
    fields.set_field(wpkg_control::control_file::field_package_factory_t::canonicalized_name(), package);
    fields.set_field(wpkg_control::control_file::field_packagerversion_factory_t::canonicalized_name(), debian_packages_version_string());
    fields.set_field(wpkg_control::control_file::field_architecture_factory_t::canonicalized_name(), "source");
    if(has_build_number)
    {
        fields.set_field(wpkg_control::control_file::field_buildnumber_factory_t::canonicalized_name(), build_number);
    }

    std::string plain_package(controlinfo_fields.get_field(sv.get_value("Package")));
    std::string version(controlinfo_fields.get_field(wpkg_control::control_file::field_version_factory_t::canonicalized_name()));

    // it looks like we are ready, run the process to create a source
    // package:
    //
    //   1. create a temporary directory
    //   2. cd in that directory
    //   3. run cmake with the path set back to the project
    //   4. run make package_source
    //   5. create the binary source package with that source tarball
    //
    // The name of the tarball is expected to be the name of the package
    // as defined in the control.info file followed by a dash and the
    // version with the .tar.gz extension:
    //
    //      <package name>_<version>.tar.gz
    //

    wpkg_filename::uri_filename build_tmpdir(wpkg_filename::uri_filename::tmpdir("build"));
    if(!run_cmake(package, build_tmpdir, wpkg_filename::uri_filename::get_cwd()))
    {
        return;
    }

    std::string cmd;
    prepare_cmd(cmd, build_tmpdir);
    if((wpkg_output::get_output_debug_flags() & wpkg_output::debug_flags::debug_progress) != 0)
    {
#if defined(MO_WINDOWS)
        cmd += "set \"VERBOSE=1\" && ";
#else
        cmd += "VERBOSE=1 ";
#endif
    }
    cmd += wpkg_util::make_safe_console_string(f_make_tool);
    cmd += " package_source";

    wpkg_output::log("system(%1).")
            .quoted_arg(cmd)
        .level(wpkg_output::level_info)
        .module(wpkg_output::module_build_package)
        .package(package)
        .action("execute-script");

    const int r(system(cmd.c_str()));
    if(r != 0)
    {
        wpkg_output::log("building the source tarball failed")
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_package)
            .action("build-source");
        return;
    }

    std::string source_dir(plain_package + "_" + wpkg_util::canonicalize_version_for_filename(version));
    std::string source(source_dir + ".tar.gz");
    wpkg_filename::uri_filename source_file(build_tmpdir.append_child(source));

    // finally create the debian package
    memfile::memory_file debian_ar;
    debian_ar.create(memfile::memory_file::file_format_ar);

    // first we must have the debian-binary file
    memfile::memory_file debian_binary;
    debian_binary.create(memfile::memory_file::file_format_other);
    debian_binary.printf("2.0\n");
    {
        memfile::memory_file::file_info info;
        info.set_filename("debian-binary");
        info.set_mode(0444);
        info.set_user("Administrator");
        info.set_group("Administrators");
        info.set_size(debian_binary.size());
        debian_ar.append_file(info, debian_binary);
    }

    // although the tarball looks like it is ready for inclusion,
    // we want to move the files under /usr/src/<package-name>_<version>
    memfile::memory_file source_tar_gz;
    source_tar_gz.read_file(source_file);
    memfile::memory_file source_tar;
    source_tar_gz.decompress(source_tar);
    source_tar_gz.reset();
    memfile::memory_file data;
    data.create(memfile::memory_file::file_format_tar);
    time_t now(time(NULL));
    memfile::memory_file::file_info info_dir;
    info_dir.set_file_type(memfile::memory_file::file_info::directory);
    info_dir.set_filename("usr");
    info_dir.set_mode(0755);
    info_dir.set_user("Administrator");
    info_dir.set_group("Administrators");
    info_dir.set_mtime(now);
    append_file(data, info_dir, source_tar_gz); // source_tar_gz is empty
    info_dir.set_filename("usr/src");
    append_file(data, info_dir, source_tar_gz);
    memfile::memory_file md5sums;
    md5sums.create(memfile::memory_file::file_format_other);
    source_tar.dir_rewind();
    f_changelog_filename = wpkg_filename::uri_filename(source_dir).append_child(f_changelog_filename.full_path());
    f_copyright_filename = wpkg_filename::uri_filename(source_dir).append_child(f_copyright_filename.full_path());
    f_controlinfo_filename = wpkg_filename::uri_filename(source_dir).append_child(f_controlinfo_filename.full_path());
    for(;;)
    {
        // TODO: tar tools do not always add all the directories
        //       which are expected in package data.tar.gz files
        //       (i.e. usr, usr/src, we create, but for the
        //       rest, not yet, also we could sort the files too)
        f_manager->check_interrupt();

        memfile::memory_file::file_info info;
        memfile::memory_file file_data;
        if(!source_tar.dir_next(info, &file_data))
        {
            break;
        }


        // move (rename) a few files if necessary
        wpkg_filename::uri_filename filename("usr/src");
        if(info.get_uri() == f_changelog_filename)
        {
            if(f_rename_changelog)
            {
                info.set_filename(source_dir + "/wpkg/changelog");
            }
        }
        else if(info.get_uri() == f_copyright_filename)
        {
            if(f_rename_copyright)
            {
                info.set_filename(source_dir + "/wpkg/copyright");
            }
        }
        else if(info.get_uri() == f_controlinfo_filename)
        {
            if(f_rename_controlinfo)
            {
                info.set_filename(source_dir + "/wpkg/control.info");
            }
            // replace that file with the modified control.info file which
            // now includes a version, maintainer, etc.
            controlinfo_fields.write(file_data, wpkg_field::field_file::WRITE_MODE_RAW_FIELDS);
            info.set_size(file_data.size());
        }
        filename = filename.append_child(info.get_filename());

        info.set_filename(filename.full_path());
        info.set_user("Administrator");
        info.set_group("Administrators");
        info.set_mtime(now);
        append_file(data, info, file_data);

        // regular files get an md5sums
        if(info.get_file_type() == memfile::memory_file::file_info::regular_file
        || info.get_file_type() == memfile::memory_file::file_info::continuous)
        {
            md5::raw_md5sum raw;
            file_data.raw_md5sum(raw);
            md5sums.printf("%s %c%s\n",
                    md5::md5sum::sum(raw).c_str(),
                    file_data.is_text() ? ' ' : '*',
                    info.get_filename().c_str());
        }
    }
    data.end_archive();
    data.compress(source_tar_gz, memfile::memory_file::file_format_gz, 9);

    // now create the control_tar file with the control file
    memfile::memory_file control_tar;
    control_tar.create(memfile::memory_file::file_format_tar);

    // add control file (we keep dependencies as is in a source package)
    memfile::memory_file ctrl;
    fields.write(ctrl, wpkg_field::field_file::WRITE_MODE_FIELD_ONLY);
    {
        memfile::memory_file::file_info info;
        info.set_mode(0444);
        info.set_user("Administrator");
        info.set_group("Administrators");
        info.set_filename("control");
        info.set_size(ctrl.size());
        append_file(control_tar, info, ctrl);
    }

    // add md5sums
    {
        memfile::memory_file::file_info info;
        info.set_mode(0444);
        info.set_user("Administrator");
        info.set_group("Administrators");
        info.set_filename("md5sums");
        info.set_size(md5sums.size());
        append_file(control_tar, info, md5sums);
    }

    control_tar.end_archive();

    // now add the control file
    memfile::memory_file control_tar_gz;
    control_tar.compress(control_tar_gz, memfile::memory_file::file_format_gz, 9);
    control_tar.reset();
    {
        memfile::memory_file::file_info info;
        info.set_filename("control.tar.gz");
        info.set_mode(0444);
        info.set_user("Administrator");
        info.set_group("Administrators");
        info.set_size(control_tar_gz.size());
        debian_ar.append_file(info, control_tar_gz);
    }

    // and finally the data tarball
    {
        memfile::memory_file::file_info info;
        //memfile::memory_file data_tar_gz;
        //data_tar_gz.read_file(source_file);
        info.set_filename("data.tar.gz");
        info.set_mode(0444);
        info.set_user("Administrator");
        info.set_group("Administrators");
        info.set_size(source_tar_gz.size());
        debian_ar.append_file(info, source_tar_gz);
    }

    save_package(debian_ar, fields);
}

/** \brief Install the source package and its dependencies.
 *
 * This function is used internally to install the source package and
 * all of its dependencies. This is important to allow the building
 * of the package.
 *
 * The function ensures that tracking happens so that way we can later
 * restore everything using the rollback() function.
 *
 * The function makes use of the f_repository and the 
 */
void wpkgar_build::install_source_package()
{
    wpkgar::wpkgar_install pkg_install(f_manager);
    pkg_install.set_installing();

    // some additional parameters
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_recursive, get_parameter(wpkgar::wpkgar_build::wpkgar_build_recursive, false) != 0);
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_force_file_info, get_parameter(wpkgar::wpkgar_build::wpkgar_build_force_file_info, false) != 0);
    pkg_install.set_parameter(wpkgar::wpkgar_install::wpkgar_install_quiet_file_info, true);

    // add the source package we're working on
    pkg_install.add_package(f_build_directory.full_path());

    // The database must be locked before we call this function
    //wpkgar::wpkgar_lock lock_wpkg(f_manager, "Installing");
    if(pkg_install.validate())
    {
        if(pkg_install.pre_configure())
        {
            for(;;)
            {
                f_manager->check_interrupt();

                int i(pkg_install.unpack());
                if(i < 0)
                {
                    break;
                }
                if(!pkg_install.configure(i))
                {
                    break;
                }
            }
        }
    }
}


/** \brief Build the project by running cmake and make.
 *
 * This function runs the cmake utility to generate a build environment,
 * in generate a large set of Makefile and other such files. The result
 * is then used to run make which actually compiles all the files and
 * generate the resulting binaries.
 *
 * This function does not run any specialized make at this point. The
 * installation of components is done in another function.
 */
void wpkgar_build::build_project()
{
    const wpkg_filename::uri_filename root(f_manager->get_inst_path());
    const wpkg_filename::uri_filename source_path(root.append_child("usr/src"));

    // make sure the package was loaded
    f_manager->load_package(f_build_directory);

    // define the path to the package source
    std::string package_name(f_manager->get_field(f_build_directory, wpkg_control::control_file::field_package_factory_t::canonicalized_name()));
    if(package_name.substr(package_name.length() - 4) != "-src")
    {
        wpkg_output::log("build aborted, the unexpected source package name %1 does not end with \"-src\".")
                .quoted_arg(package_name)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_package)
            .package(package_name)
            .action("build-package");
        return;
    }
    package_name = package_name.substr(0, package_name.length() - 4);

    const std::string package_version(f_manager->get_field(f_build_directory, wpkg_control::control_file::field_version_factory_t::canonicalized_name()));
    f_package_source_path = source_path.append_child(package_name + "_" + package_version);

    // create a build directory
    const wpkg_filename::uri_filename build_tmpdir(wpkg_filename::uri_filename::tmpdir("build"));

    // run cmake
    if(!run_cmake(package_name, build_tmpdir, f_package_source_path))
    {
        return;
    }

    // now build everything with make
    // I do not use make -C <path> because some systems do not support it
    std::string make_all_cmd;
    prepare_cmd(make_all_cmd, build_tmpdir);
    if((wpkg_output::get_output_debug_flags() & wpkg_output::debug_flags::debug_progress) != 0)
    {
#if defined(MO_WINDOWS)
        make_all_cmd += "set \"VERBOSE=1\" && ";
#else
        make_all_cmd += "VERBOSE=1 ";
#endif
    }
    make_all_cmd += wpkg_util::make_safe_console_string(f_make_tool);

    wpkg_output::log("system(%1).")
            .quoted_arg(make_all_cmd)
        .level(wpkg_output::level_info)
        .module(wpkg_output::module_build_package)
        .package(package_name)
        .action("execute-script");

    const int make_all_result(system(make_all_cmd.c_str()));
    if(make_all_result != 0)
    {
        wpkg_output::log("build of binary packages aborted, make command %1 failed with %2.")
                .quoted_arg(make_all_cmd)
                .arg(make_all_result)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_package)
            .package(package_name)
            .action("build-package");
        return;
    }
}


/** \brief Run the unit tests of the project.
 *
 * All projects must have a unit_test target. This function runs it to make
 * sure that the project works. Note that if any test generates an error
 * then the process stops and the binary packages do not get created.
 *
 * By default the unit tests are not run. You have to set the
 * wpkgar_build_run_unit_tests parameter to true for that to happen.
 * From wpkg, use the --run-unit-tests option.
 */
void wpkgar_build::run_project_unit_tests()
{
    if(!get_parameter(wpkgar::wpkgar_build::wpkgar_build_run_unit_tests, false))
    {
        return;
    }

    // get the build directory
    const wpkg_filename::uri_filename build_tmpdir = wpkg_filename::uri_filename::tmpdir("build");

    std::string run_tests_cmd;
    prepare_cmd(run_tests_cmd, build_tmpdir);
    if((wpkg_output::get_output_debug_flags() & wpkg_output::debug_flags::debug_progress) != 0)
    {
#if defined(MO_WINDOWS)
        run_tests_cmd += "set \"VERBOSE=1\" && ";
#else
        run_tests_cmd += "VERBOSE=1 ";
#endif
    }
    run_tests_cmd += wpkg_util::make_safe_console_string(f_make_tool);
    run_tests_cmd += " run_unit_tests";
    const int run_tests_result(system(run_tests_cmd.c_str()));
    if(run_tests_result != 0)
    {
        wpkg_output::log("build aborted, make command %1 to run all unit tests failed with %2.")
                .quoted_arg(run_tests_cmd)
                .arg(run_tests_result)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_package)
            .package(f_manager->get_field(f_build_directory, wpkg_control::control_file::field_package_factory_t::canonicalized_name()))
            .action("build-package");
        return;
    }
}


/** \brief Build the packages of the compiled project.
 *
 * This function runs cmake and make to install the files and then
 * run the build process with the control.info from the source.
 * The result is a set of binary packages that were generated from source.
 */
void wpkgar_build::build_project_packages()
{
    // get the build directory
    const wpkg_filename::uri_filename build_tmpdir = wpkg_filename::uri_filename::tmpdir("build");

    // create an install directory
    const wpkg_filename::uri_filename install_tmpdir = wpkg_filename::uri_filename::tmpdir("install");

    // the source package must place the control.info file under wpkg
    memfile::memory_file ctrl_file;
    f_controlinfo_filename = f_package_source_path.append_child("wpkg/control.info");
    ctrl_file.read_file(f_controlinfo_filename);
    wpkg_control::source_control_file controlinfo_fields;
    f_manager->set_control_variables(controlinfo_fields);
    controlinfo_fields.set_input_file(&ctrl_file);
    const bool cf_result(controlinfo_fields.read());
    controlinfo_fields.set_input_file(NULL);
    if(!cf_result)
    {
        wpkg_output::log("the %1 file does not include a Sub-Packages field; use \"Sub-Packages: runtime*\" by default if needed")
                .quoted_arg(f_controlinfo_filename)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_info)
            .action("build-source");
        return;
    }

    // create a package for each sub-package
    std::map<std::string, bool> created_packages;
    std::string hidden_sub_name;
    const wpkg_field::field_file::list_t sub_packages(controlinfo_fields.get_field_list(wpkg_control::control_file::field_subpackages_factory_t::canonicalized_name()));
    for(wpkg_control::control_file::field_file::list_t::const_iterator it(sub_packages.begin()); it != sub_packages.end(); ++it)
    {
        f_manager->check_interrupt();

        std::string sub_name(*it);

        const bool hide_sub_name(!sub_name.empty() && *sub_name.rbegin() == '*');
        if(hide_sub_name)
        {
            if(!hidden_sub_name.empty())
            {
                throw wpkgar_exception_defined_twice("no more than one sub-package name can be marked as hidden with an *");
            }
            sub_name.erase(sub_name.length() - 1);
            hidden_sub_name = sub_name;
        }
        if(sub_name.empty())
        {
            throw wpkgar_exception_invalid("a package sub-package name cannot be empty or just *");
        }

        if(created_packages.find(sub_name) != created_packages.end())
        {
            throw wpkgar_exception_defined_twice("the same sub-package was defined twice");
        }
        created_packages[sub_name] = true;

        // run cmake
#if defined(MO_WINDOWS)
        std::string cmake_cmd("cd /d ");
#else
        std::string cmake_cmd("cd ");
#endif
        cmake_cmd += build_tmpdir.full_path() + " && cmake";
        wpkg_filename::uri_filename install_dir(install_tmpdir.append_child(sub_name).append_safe_child(f_install_prefix));
        cmake_cmd += " -DCMAKE_INSTALL_PREFIX=" + install_dir.full_path();
        cmake_cmd += " -DCMAKE_INSTALL_COMPONENT=" + sub_name;
        cmake_cmd += " -DCMAKE_INSTALL_DO_STRIP=1";
        cmake_cmd += " -P " + build_tmpdir.append_child("cmake_install.cmake").full_path();
        const int cmake_result(system(cmake_cmd.c_str()));
        if(cmake_result != 0)
        {
            wpkg_output::log("build aborted, cmake command %1 failed with %2.")
                    .quoted_arg(cmake_cmd)
                    .arg(cmake_result)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .package(sub_name)
                .action("build-package");
            return;
        }

        if(hide_sub_name)
        {
            // in the main installation we add the copyright, ChangeLog's,
            // AUTHORs, LICENSE under .../share/doc/<name>/...
            memfile::memory_file file_data;
            memfile::memory_file compressed;
            const std::string package_name(controlinfo_fields.get_field(wpkg_control::control_file::field_package_factory_t::canonicalized_name() + ("/" + sub_name)));

            wpkg_filename::uri_filename in_changelog(f_package_source_path.append_child("wpkg/changelog"));
            file_data.read_file(in_changelog);
            wpkg_filename::uri_filename out_changelog(install_dir.append_child("share/doc").append_child(package_name).append_child("changelog.gz"));
            file_data.compress(compressed, memfile::memory_file::file_format_gz, 9);
            compressed.write_file(out_changelog, true);

            wpkg_filename::uri_filename in_copyright(f_package_source_path.append_child("wpkg/copyright"));
            file_data.read_file(in_copyright);
            wpkg_filename::uri_filename out_copyright(install_dir.append_child("share/doc").append_child(package_name).append_child("copyright"));
            file_data.write_file(out_copyright);

            bool has_authors(false);
            bool has_license(false);
            memfile::memory_file source_dir;
            source_dir.dir_rewind(f_package_source_path, true);
            for(;;)
            {
                f_manager->check_interrupt();

                memfile::memory_file::file_info info;
                if(!source_dir.dir_next(info, NULL))
                {
                    break;
                }

                // move (rename) a few files if necessary
                case_insensitive::case_insensitive_string name(info.get_basename());
                if((name == "AUTHORS" || name == "AUTHORS.txt") && !has_authors)
                {
                    file_data.read_file(info.get_uri());
                    const wpkg_filename::uri_filename out_authors(install_dir.append_child("share/doc").append_child(package_name).append_child("AUTHORS"));
                    file_data.write_file(out_authors);
                    has_authors = true;
                }
                else if((name == "LICENSE" || name == "LICENSE.txt") && !has_license)
                {
                    file_data.read_file(info.get_uri());
                    const wpkg_filename::uri_filename out_license(install_dir.append_child("share/doc").append_child(package_name).append_child("LICENSE"));
                    file_data.write_file(out_license);
                    has_license = true;
                }
            }
        }
    }

    // build the resulting packages using the control.info file
    wpkgar_build info(f_manager, f_controlinfo_filename.full_path());
    info.set_extra_path(install_tmpdir);
    info.set_output_repository_dir(f_output_repository_dir);
    info.set_output_dir(f_output_dir);
    info.build_info();
}


/** \brief Build packages from a source package.
 *
 * This function is used to generate binaries from a source package and then
 * package the resulting binary packages.
 *
 * The source package must have been generated by the --build command by
 * itself:
 *
 * \code
 * cd path/to/your/project/root/directory
 * wpkg --build
 * \endcode
 *
 * Then you can build the package using the --build command again as in:
 *
 * \code
 * wpkg --build <package name>-src_<version>.deb
 * \endcode
 *
 * The result is a set of binary packages which are created by running
 * cmake, make, and make install with each different component. Assuming
 * the project is properly setup, this should generate the perfect set
 * of packages.
 *
 * As an addition, the build process can also run the unit tests defined
 * in this package assuming you called the run_tests() function before
 * the build() function. This simply runs:
 *
 * \code
 * make run_tests
 * \endcode
 *
 * Obviously, all projects should have a run_tests target for the unit tests
 * to work each time.
 *
 * Note that if the package has dependencies, then a repository of source
 * and/or binary packages must be specified. That repository will be used
 * for all required dependencies while installing the source package.
 */
void wpkgar_build::build_packages()
{
    // make sure we track all the changes because at the end we want to
    // restore the system the way it was
    std::shared_ptr<wpkgar_tracker_interface> tracker(f_manager->get_tracker());
    if(!tracker)
    {
        // use wpkg --debug 0100 to keep this file after wpkg exits
        wpkg_filename::uri_filename journal_tmpdir(wpkg_filename::uri_filename::tmpdir("journal"));
        journal_tmpdir = journal_tmpdir.append_child("journal.wpkg-sh");
        tracker = std::shared_ptr<wpkgar_tracker_interface>(new wpkgar_tracker(f_manager, journal_tmpdir));
        // use tracker as is which means it is in "auto-rollback" mode
        // (note that the rollback happens when f_manager is getting destroyed
        // by default... we'll have to make sure that's alright)
        f_manager->set_tracker(tracker);
    }

    install_source_package();
    if(wpkg_output::get_output_error_count() != 0)
    {
        return;
    }

    build_project();
    if(wpkg_output::get_output_error_count() != 0)
    {
        return;
    }

    run_project_unit_tests();
    if(wpkg_output::get_output_error_count() != 0)
    {
        return;
    }

    build_project_packages();
}



/** \brief Build the source packages from a repository.
 *
 * This function goes through all the source packages found in a sources
 * repository directory and transforms them in a list of binary packages
 * in that same repository.
 */
void wpkgar_build::build_repository()
{
    struct source_t
    {
        enum status_t
        {
            source,             // default value until it gets built
            built,              // it was built successfully
            cannot_build,       // cannot be built (missing dependencies)
            error               // an error occurred while building
        };
        typedef controlled_vars::limited_auto_enum_init<status_t, source, error, source> safe_status_t;

        safe_status_t                   f_status;
        wpkg_filename::uri_filename     f_filename;
        wpkg_field::field_file::list_t  f_sub_packages;
    };

    if(f_program_fullname.empty())
    {
        // This should not happen unless you use the build() function
        // improperly (i.e. without initializing f_program_fullname)
        throw wpkgar_exception_io("you cannot build a repository without a program fullname");
    }

    // verify the "sources" sub-directory validity
    wpkg_filename::uri_filename sources_root(f_build_directory.append_child("sources"));
    if(!sources_root.exists())
    {
        throw wpkgar_exception_io("the sources repository sub-directory is missing");
    }
    if(!sources_root.is_dir())
    {
        throw wpkgar_exception_io("the sources file is not a directory");
    }

    const uint32_t err_count(wpkg_output::get_output_error_count());
    std::vector<std::shared_ptr<source_t> > sources;
    typedef std::map<std::string, std::shared_ptr<source_t> > source_map_t;
    source_map_t all_packages;
    memfile::memory_file sources_dir;
    sources_dir.dir_rewind(sources_root);
    for(;;)
    {
        memfile::memory_file::file_info info;
        if(!sources_dir.dir_next(info, NULL))
        {
            break;
        }
        std::shared_ptr<source_t> s(new source_t);
        s->f_filename = info.get_uri();
        if(s->f_filename.extension() == "deb")
        {
            // try loading this package now, if it fails now, then
            // the whole process stops...
            f_manager->load_package(s->f_filename);

            const wpkg_filename::uri_filename package_name(f_manager->get_field(s->f_filename, wpkg_control::control_file::field_package_factory_t::canonicalized_name()));

            // we want the list of packages that this source package generates
            // which is the list of Package/<sub-package> names found in the
            // wpkg/control.info file found in the data file
            memfile::memory_file data;
            std::string data_filename("data.tar");
            f_manager->get_control_file(data, s->f_filename, data_filename, false);

            data.dir_rewind();
            for(;;)
            {
                memfile::memory_file::file_info file_info;
                memfile::memory_file file_data;
                if(!data.dir_next(file_info, &file_data))
                {
                    wpkg_output::log("source package %1 does not include a wpkg/control.info file; a valid wpkg source package must include that file")
                            .quoted_arg(info.get_uri())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_build_info)
                        .package(package_name)
                        .action("build-validation");
                    break;
                }
                // TBD: should we check ignoring case?
                wpkg_filename::uri_filename controlinfo_filename(file_info.get_uri());
                int seg_idx(controlinfo_filename.segment_size());
                if(seg_idx == 5 && controlinfo_filename.segment(4) == "control.info" && controlinfo_filename.segment(3) == "wpkg")
                {
                    wpkg_control::source_control_file fields; //(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::build_control_file_state_t));
                    //f_manager->set_control_variables(fields); -- TBD I think we only read files without variables in this case
                    fields.set_input_file(&file_data);
                    if(fields.read())
                    {
                        if(!fields.field_is_defined(wpkg_control::control_file::field_subpackages_factory_t::canonicalized_name()))
                        {
                            wpkg_output::log("source package %1 does not include a Sub-Packages field; use \"Sub-Packages: runtime*\" by default if needed")
                                    .quoted_arg(info.get_uri())
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_build_info)
                                .package(package_name)
                                .action("build-validation");
                        }
                        wpkg_field::field_file::list_t sub_packages(fields.get_field_list(wpkg_control::control_file::field_subpackages_factory_t::canonicalized_name()));
                        for(wpkg_control::control_file::field_file::list_t::const_iterator it(sub_packages.begin());
                                                                                           it != sub_packages.end();
                                                                                           ++it)
                        {
                            // get the sub-package name
                            std::string sub_name(*it);
                            if(!sub_name.empty() && *sub_name.rbegin() == '*')
                            {
                                sub_name = sub_name.substr(0, sub_name.length() - 1);
                            }
                            if(!sub_name.empty())
                            {
                                std::string field_name(wpkg_control::control_file::field_package_factory_t::canonicalized_name() + ("/" + sub_name));
                                if(!fields.field_is_defined(field_name, true))
                                {
                                    wpkg_output::log("Mandatory field %1 is not defined")
                                            .quoted_arg(field_name)
                                        .level(wpkg_output::level_error)
                                        .module(wpkg_output::module_build_info)
                                        .package(package_name)
                                        .action("build-validation");
                                }
                                else
                                {
                                    std::string name(fields.get_field(field_name));
                                    all_packages[name] = s;
                                }
                            }
                            // else -- if empty we should have caught it when
                            //         validating the field contents
                        }
                    }
                    break;
                }
            }

            // keep the source at hand
            sources.push_back(s);
        }
    }
    if(err_count != wpkg_output::get_output_error_count())
    {
        // there were errors, do not proceed
        return;
    }

    // field names that represent all possible dependencies that are required
    // to build this package
    typedef std::vector<std::string> wpkgar_list_of_strings_t;
    wpkgar_list_of_strings_t field_names;
    field_names.push_back(wpkg_control::control_file::field_depends_factory_t::canonicalized_name());
    field_names.push_back(wpkg_control::control_file::field_builddepends_factory_t::canonicalized_name());
    field_names.push_back(wpkg_control::control_file::field_builddependsarch_factory_t::canonicalized_name());
    field_names.push_back(wpkg_control::control_file::field_builddependsindep_factory_t::canonicalized_name());
    field_names.push_back(wpkg_control::control_file::field_builtusing_factory_t::canonicalized_name());

    const wpkg_filename::filename_list_t::size_type max(sources.size());
    bool repeat;
    do
    {
        repeat = false;
        for(wpkg_filename::filename_list_t::size_type i(0); i < max; ++i)
        {
            if(source_t::source == sources[i]->f_status)
            {
                std::string package_name(f_manager->get_field(sources[i]->f_filename, wpkg_control::control_file::field_package_factory_t::canonicalized_name()));
                bool ready(true);
                for(wpkgar_list_of_strings_t::const_iterator fn(field_names.begin());
                                                             fn != field_names.end() && ready;
                                                             ++fn)
                {
                    if(!f_manager->field_is_defined(sources[i]->f_filename, *fn))
                    {
                        // this field is not defined, skip
                        continue;
                    }
                    wpkg_dependencies::dependencies depends(f_manager->get_dependencies(sources[i]->f_filename, *fn));
                    for(int j(0); j < depends.size() && ready; ++j)
                    {
                        const wpkg_dependencies::dependencies::dependency_t& d(depends.get_dependency(j));

                        source_map_t::const_iterator dependency(all_packages.find(d.f_name));
                        if(dependency == all_packages.end())
                        {
                            // TBD: should we give a chance to the builder
                            //      for packages to be defined in their
                            //      repository (pre-compiled)?
                            wpkg_output::log("package %1 depends on %2 (%3) which is not defined among your source packages.")
                                    .quoted_arg(sources[i]->f_filename)
                                    .quoted_arg(d.f_name)
                                    .arg(d.to_string())
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_validate_installation)
                                .package(package_name)
                                .action("build-validation");

                            sources[i]->f_status = source_t::cannot_build;
                            ready = false;
                        }
                        else if(source_t::built != dependency->second->f_status)
                        {
                            // this is not an error, it's just not ready yet
                            // we'll try again on the next loop
                            ready = false;
                        }
                    }
                }
                if(ready)
                {
                    // this source package is ready to get built
                    std::string cmd(f_program_fullname);
                    cmd += " ";
                    cmd += " --root ";
                    cmd += f_manager->get_root_path().full_path();
                    cmd += " --instdir ";
                    cmd += f_manager->get_inst_path().full_path();
                    cmd += " --admindir ";
                    cmd += f_manager->get_database_path().full_path();
                    cmd += " --build ";
                    cmd += sources[i]->f_filename.full_path();
                    if(!f_install_prefix.empty())
                    {
                        cmd += " --install-prefix ";
                        cmd += wpkg_util::make_safe_console_string(f_install_prefix.original_filename());
                    }
                    cmd += " --output-repository-dir ";
                    bool add_to_repository_list(false);
                    if(f_output_repository_dir.empty())
                    {
                        cmd += f_build_directory.full_path();
                    }
                    else
                    {
                        cmd += f_output_repository_dir.full_path();
                        add_to_repository_list = f_output_repository_dir.exists();
                    }
                    cmd += " --repository ";
                    cmd += f_build_directory.full_path();
                    if(add_to_repository_list)
                    {
                        cmd += " " + f_output_repository_dir.full_path();
                    }
                    const std::string& tmpdir(wpkg_filename::temporary_uri_filename::get_tmpdir());
                    if(!tmpdir.empty())
                    {
                        cmd += " --tmpdir ";
                        cmd += wpkg_util::make_safe_console_string(tmpdir);
                    }
                    cmd += " --create-index index.tar.gz";
                    cmd += " --force-file-info";
                    cmd += " --run-unit-tests";
                    cmd += " --make-tool ";
                    cmd += wpkg_util::make_safe_console_string(f_make_tool);
                    if(!f_cmake_generator.empty())
                    {
                        cmd += " --cmake-generator ";
                        cmd += wpkg_util::make_safe_console_string(f_cmake_generator);
                    }

                    // keep the same debug flags for sub-calls
                    cmd += " --debug ";
                    std::stringstream integer;
                    integer << wpkg_output::get_output()->get_debug_flags();
                    cmd += integer.str();

                    wpkg_output::log("system(%1).")
                            .quoted_arg(cmd)
                        .level(wpkg_output::level_info)
                        .module(wpkg_output::module_run_script)
                        .package(package_name)
                        .action("build-package");

                    const int r(system(cmd.c_str()));
                    if(r != 0)
                    {
                        wpkg_output::log("system(%1) called returned %2")
                                .quoted_arg(cmd)
                                .arg(r)
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_run_script)
                            .package(package_name)
                            .action("build-package");

                        sources[i]->f_status = source_t::error;
                    }
                    else
                    {
                        repeat = true;
                        sources[i]->f_status = source_t::built;
                    }
                }
            }
        }
    }
    while(repeat);
}


/** \brief Build a set of packages using a control.info file.
 *
 * This function goes through the list of Sub-Packages defined in a
 * control.info file and builds each package for each one of them.
 *
 * This is used when a control.info file and an extra path are specified
 * on the command line of wpkg. The build itself is otherwise the same as
 * with a control file.
 */
void wpkgar_build::build_info()
{
    if(!f_filename.empty())
    {
        throw wpkgar_exception_compatibility("an info file cannot be used with the --output-filename command line option");
    }

    memfile::memory_file info;
    wpkg_output::log("loading .info control file %1")
            .quoted_arg(f_build_directory)
        .debug(wpkg_output::debug_flags::debug_basics)
        .module(wpkg_output::module_build_package);
    info.read_file(f_build_directory);
    wpkg_control::info_control_file fields;
    f_manager->set_control_variables(fields);
    wpkg_output::log("reading .info control fields %1")
            .quoted_arg(f_build_directory)
        .debug(wpkg_output::debug_flags::debug_detail_config)
        .module(wpkg_output::module_build_package);
    fields.set_input_file(&info);
    fields.read();
    fields.set_input_file(NULL);
    if(!fields.field_is_defined("Sub-Packages"))
    {
        throw wpkgar_exception_compatibility("an info file must include a Sub-Packages field to be valid");
    }

    if(f_extra_path.empty())
    {
        // no extra path, check for a ROOT_TREE variable
        if(!fields.variable_is_defined("ROOT_TREE"))
        {
            throw wpkgar_exception_undefined("input directory name required on the command line or with ROOT_TREE variable");
        }
        f_extra_path.set_filename(fields.get_variable("ROOT_TREE"));
        if(!f_extra_path.is_absolute())
        {
            // if not absolute, prepend the path to the .info file
            if(!f_build_directory.dirname().empty())
            {
                f_extra_path = wpkg_filename::uri_filename(f_build_directory.dirname()).append_safe_child(f_extra_path);
            }
            // else -- we're at the right place already
        }
    }

    // create a package for each sub-package
    wpkg_control::control_file::field_file::list_t sub_packages(fields.get_field_list(wpkg_control::control_file::field_subpackages_factory_t::canonicalized_name()));
    if(sub_packages.empty())
    {
        throw wpkgar_exception_undefined("the list of Sub-Packages is empty");
    }
    std::map<std::string, bool> created_packages;
    std::string hidden_sub_name;
    wpkg_control::control_file::field_file::list_t excluded;
    excluded.push_back("Sub-Packages");
    for(wpkg_control::control_file::field_file::list_t::const_iterator it(sub_packages.begin());
                                it != sub_packages.end(); ++it)
    {
        // get the sub-package name and make sure it is unique
        std::string sub_name(*it);

        // check whether the sub-name should be hidden in the package filename
        const bool hide_sub_name(!sub_name.empty() && *sub_name.rbegin() == '*');
        if(hide_sub_name)
        {
            if(!hidden_sub_name.empty())
            {
                throw wpkgar_exception_defined_twice("no more than one sub-package name can be marked as hidden with an *");
            }
            sub_name.erase(sub_name.length() - 1);
            hidden_sub_name = sub_name;
        }
        if(sub_name.empty())
        {
            throw wpkgar_exception_invalid("a package sub-package name cannot be empty or just *");
        }
        if(created_packages.find(sub_name) != created_packages.end())
        {
            throw wpkgar_exception_defined_twice("the same sub-package was defined twice");
        }
        created_packages[sub_name] = true;

        // check that the resulting path exists
        wpkg_filename::uri_filename dir_name(f_extra_path.append_child(sub_name));
        if(!dir_name.exists())
        {
            throw wpkgar_exception_io("a sub-package directory is missing");
        }
        if(!dir_name.is_dir())
        {
            throw wpkgar_exception_compatibility("a sub-package name does not point to a directory");
        }

        // ensure there is a control directory in the source directory
        std::string control_path = "WPKG";
        wpkg_filename::uri_filename wpkg_dir = dir_name.append_child(control_path);
        if(!wpkg_dir.exists())
        {
            control_path = "DEBIAN";
            wpkg_dir = dir_name.append_child(control_path);
            if(!wpkg_dir.exists())
            {
                control_path = "WPKG";
                wpkg_dir = dir_name.append_child(control_path);
                wpkg_dir.os_mkdir_p();
            }
        }
        if(!wpkg_dir.is_dir())
        {
            // a file was found, it MUST be a directory though
            throw wpkgar_exception_compatibility("the input file of a sub-package is not a directory");
        }

        // create the sub-package control file
        // the type of file is not important as we do not call the read() function
        wpkg_control::binary_control_file sub_control_file(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::build_control_file_state_t));
        f_manager->set_control_variables(sub_control_file);
        fields.copy(sub_control_file, sub_name, excluded);
        if(!fields.field_is_defined(wpkg_control::control_file::field_package_factory_t::canonicalized_name() + ("/" + sub_name), true) && !hide_sub_name)
        {
            // special case for a non-specific package field needs to include
            // the sub_name in its name unless it's a source package
            std::string arch;
            if(fields.field_is_defined(wpkg_control::control_file::field_architecture_factory_t::canonicalized_name() + ("/" + sub_name)))
            {
                arch = fields.get_field(wpkg_control::control_file::field_architecture_factory_t::canonicalized_name() + ("/" + sub_name));
            }
            else
            {
                // we do not test the existence here since it has to be defined
                // if missing it is an error anyway
                arch = fields.get_field(wpkg_control::control_file::field_architecture_factory_t::canonicalized_name());
            }
            // a source package is "special" in that the sub-package name is
            // never included; so if source, skip since Package is already
            // defined as it should be
            if(arch != "source" && arch != "src")
            {
                std::string package(fields.get_field(wpkg_control::control_file::field_package_factory_t::canonicalized_name()));
                sub_control_file.set_field(wpkg_control::control_file::field_package_factory_t::canonicalized_name(), package + "-" + sub_name);
            }
        }
        if(!f_install_prefix.empty())
        {
            sub_control_file.set_field(wpkg_control::control_file::field_installprefix_factory_t::canonicalized_name(), f_install_prefix.full_path());
        }
        memfile::memory_file ctrl_output;
        sub_control_file.write(ctrl_output, wpkg_field::field_file::WRITE_MODE_FIELD_ONLY);
        ctrl_output.write_file(wpkg_dir.append_child("control"));

        // now we can create the package
        build_deb(dir_name);
    }
}


/** \brief Build one Debian binary package.
 *
 * Build a binary package from the specified directory. This function is the
 * one actually generating binary packages. The build_info() calls this
 * function after preparing each sub-packages as expected.
 *
 * \param[in] dir_name  Name of the directory used to build the package.
 */
void wpkgar_build::build_deb(const wpkg_filename::uri_filename& dir_name)
{
    // in case of error we do not want to "return" a package name
    f_package_name.clear();
    wpkg_output::log("build directory is %1")
            .quoted_arg(f_build_directory)
        .debug(wpkg_output::debug_flags::debug_basics)
        .module(wpkg_output::module_build_package);

    // the directory must have a WPKG or DEBIAN sub-directory
    std::string control_path("WPKG");
    wpkg_filename::uri_filename wpkg_dir(dir_name.append_child(control_path));
    if(!wpkg_dir.exists())
    {
        control_path = "DEBIAN";
        wpkg_dir = dir_name.append_child(control_path);
        if(!wpkg_dir.exists())
        {
            throw wpkgar_exception_io("the WPKG package sub-directory is missing");
        }
    }
    if(!wpkg_dir.is_dir())
    {
        throw wpkgar_exception_compatibility("the package sub-directory file is not a directory");
    }

    // the WPKG sub-directory must at least have a control file
    wpkg_filename::uri_filename control_name(wpkg_dir.append_child("control"));
    if(!control_name.exists())
    {
        throw wpkgar_exception_io("\"control\" file missing from the package sub-directory");
    }
    if(!control_name.is_reg())
    {
        throw wpkgar_exception_io("\"control\" file in the package sub-directory is not a regular file");
    }

    // read the control file
    memfile::memory_file ctrl;
    ctrl.read_file(control_name);
    wpkg_control::binary_control_file fields(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::build_control_file_state_t));
    f_manager->set_control_variables(fields);

    // the WPKG sub-directory may have a substvars file
    wpkg_filename::uri_filename substvars_name(wpkg_dir.append_child("substvars"));
    if(substvars_name.exists())
    {
        if(!substvars_name.is_reg())
        {
            throw wpkgar_exception_io("substvars file in the package sub-directory is not a regular file");
        }
        memfile::memory_file substvars;
        substvars.read_file(substvars_name);
        int offset(0);
        std::string fv;
        while(substvars.read_line(offset, fv))
        {
            // ignore empty lines and comments
            if(fv.empty() || fv[0] == '#')
            {
                continue;
            }
            std::string::size_type p(fv.find('='));
            if(p == std::string::npos)
            {
                throw wpkgar_exception_invalid("the substvars file only accepts variable definitions that include an equal sign");
            }
            if(p == 0)
            {
                throw wpkgar_exception_invalid("the name of a variable in your substvars file cannot be empty");
            }
            std::string name(fv.substr(0, p));
            std::string value(fv.substr(p + 1));
            if(value.length() > 1 && value[0] == '"' && *value.rbegin() == '"')
            {
                value = value.substr(1, value.length() - 2);
            }
            else if(value.length() > 1 && value[0] == '\'' && *value.rbegin() == '\'')
            {
                value = value.substr(1, value.length() - 2);
            }
            fields.set_field_variable(name, value);
        }
    }

    f_manager->set_control_variables(fields);
    fields.set_input_file(&ctrl);
    fields.read();
    fields.set_input_file(NULL);
    if(!fields.field_is_defined("Package"))
    {
        // note: the wpkg_control object already verify the mandatory fields
        // so no need to test more here
        throw wpkgar_exception_compatibility("a control file must include a Package field to be valid");
    }
    const std::string package(fields.get_field("Package"));
    // prevent names that match the name of a directory used by wpkg
    // and all the names that MS-Windows uses as a Namespace
    if(package == "tmp"
    || package == "core"
    || wpkg_util::is_special_windows_filename(package))  // i.e. NUL, COM#, LPT#, etc.
    {
        throw wpkgar_exception_compatibility("a package cannot be named 'tmp' or 'core' or a MS-Windows namespace (con, prn, aux, nul, com?, lpt?)");
    }
    wpkg_output::log("building package %1")
            .quoted_arg(package)
        .module(wpkg_output::module_build_package)
        .package(package)
        .action("build-validation");

    // Moved back in the control file now that we have as state we can make sure to
    // generate the error only when required
    //if(fields.field_is_defined("Source"))
    //{
    //    throw wpkgar_exception_compatibility("a control file cannot include a Source field; either use Package or Origin as may be necessary");
    //}

    if(!fields.field_is_defined(wpkg_control::control_file::field_architecture_factory_t::canonicalized_name()))
    {
        throw wpkgar_exception_compatibility("the Architecture field is mandatory in a control file");
    }

    // canonicalize the architecture
    const char *arch_field(wpkg_control::control_file::field_architecture_factory_t::canonicalized_name());
    const std::string arch_value(fields.get_field(arch_field));
    const wpkg_architecture::architecture arch(arch_value);
    const bool is_source(arch.is_source());

    // check for conffiles in case it exists
    wpkg_filename::uri_filename conffiles_name(wpkg_dir.append_child("conffiles"));
    if(conffiles_name.exists())
    {
        if(!conffiles_name.is_reg())
        {
            throw wpkgar_exception_io("conffiles file in the package sub-directory is not a regular file");
        }
    }
    else
    {
        // if it doesn't exist set to empty
        conffiles_name.set_filename("");
    }

    // force the type, permission, owner/group for listed files
    typedef std::vector<memfile::memory_file::file_info> filesmetadata_vector_t;
    filesmetadata_vector_t filesmetadata;
    wpkg_filename::uri_filename filesmetadata_name(wpkg_dir.append_child("filesmetadata"));
    if(filesmetadata_name.exists())
    {
        if(!filesmetadata_name.is_reg())
        {
            throw wpkgar_exception_io("filesmetadata file in the package sub-directory is not a regular file");
        }
        memfile::memory_file metadata;
        metadata.read_file(filesmetadata_name);
        metadata.dir_rewind();
        for(;;)
        {
            memfile::memory_file::file_info info;
            if(!metadata.dir_next(info, NULL))
            {
                break;
            }
            filesmetadata.push_back(info);
        }
    }

    // force the owner/group names for all the files
    int force_uid(-1);
    std::string force_owner;
    if(fields.field_is_defined("Files-Owner"))
    {
        force_owner = fields.get_field("Files-Owner");
        std::string::size_type p(force_owner.find_first_of('/'));
        if(p == std::string::npos)
        {
            throw wpkgar_exception_invalid("Files-Owner must include a user identifier, a slash (/), and a user name");
        }
        if(p == 0)
        {
            throw wpkgar_exception_invalid("the Files-Owner identifier cannot be empty");
        }
        char *end;
        const char *uid(force_owner.substr(0, p).c_str());
        force_uid = strtol(uid, &end, 0);
        if(end != uid + p)
        {
            throw wpkgar_exception_invalid("the Files-Owner identifier must be a valid number");
        }
        if(force_uid < 0)
        {
            throw wpkgar_exception_invalid("the Files-Owner identifier must be zero or a positive number");
        }
        force_owner = force_owner.substr(p + 1);
        if(force_owner.length() == 0)
        {
            throw wpkgar_exception_invalid("the Files-Owner name cannot be empty");
        }
        fields.delete_field("Files-Owner");
    }
    int force_gid(-1);
    std::string force_group;
    if(fields.field_is_defined("Files-Group"))
    {
        force_group = fields.get_field("Files-Group");
        std::string::size_type p(force_group.find_first_of('/'));
        if(p == std::string::npos)
        {
            throw wpkgar_exception_invalid("Files-Group must include a group identifier, a slash (/), and a group name");
        }
        if(p == 0)
        {
            throw wpkgar_exception_invalid("the Files-Group identifier cannot be empty");
        }
        char *end;
        const char *gid(force_group.substr(0, p).c_str());
        force_gid = strtol(gid, &end, 0);
        if(end != gid + p)
        {
            throw wpkgar_exception_invalid("the Files-Group identifier must be a valid number");
        }
        if(force_gid < 0)
        {
            throw wpkgar_exception_invalid("the Files-Group identifier must be zero or a positive number");
        }
        force_group = force_group.substr(p + 1);
        if(force_group.length() == 0)
        {
            throw wpkgar_exception_invalid("the Files-Group name cannot be empty");
        }
        fields.delete_field("Files-Group");
    }

    // list of files in the archives to ensure that the user does
    // not include two files with the same name (i.e. under a
    // Linux system readme and README are two different files,
    // but that would not be so under MS-Windows.)
    std::map<case_insensitive::case_insensitive_string, memfile::memory_file::file_info> found;

    // create the tarball (data.tar)
    // and since we'll be seeing all the files, get their md5sum
    memfile::memory_file data_tar;
    data_tar.create(memfile::memory_file::file_format_tar);
    memfile::memory_file md5sums;
    md5sums.create(memfile::memory_file::file_format_other);
    memfile::memory_file in;
    size_t total_size(0);
//::fprintf(stderr, "*** start dir_name = [%s]\n", dir_name.original_filename().c_str());
    in.dir_rewind(dir_name, false);
    for(;;)
    {
        memfile::memory_file::file_info info;
        if(!in.dir_next(info, NULL))
        {
            break;
        }
        // we only read from sub-directories
        if(info.get_file_type() != memfile::memory_file::file_info::directory)
        {
            continue;
        }
        // check the directory name as some are ignored
        const wpkg_filename::uri_filename& root(info.get_uri());
        case_insensitive::case_insensitive_string basename(root.basename());
//::fprintf(stderr, "*** root = [%s] basename [%s]\n", root.original_filename().c_str(), basename.c_str());
        if(basename == "." || basename == ".."
        || basename == "WPKG" || basename == "DEBIAN")
        {
            // directories that we know we do not want
            continue;
        }
        // add this directory to the data tarball...

        // remove the dir_name path part since that's artificial
        // in the resulting output
        wpkg_filename::uri_filename directory_name(root.remove_common_segments(dir_name).relative_path());
//::fprintf(stderr, "  directory_name = [%s] dir_name [%s]\n", directory_name.full_path().c_str(), dir_name.full_path().c_str());
        if(is_exception(directory_name))
        {
            // this is forbidden by us or the user
            // but those are silently ignored
            wpkg_output::log("ignore file %1 as it is marked as an exception")
                    .quoted_arg(root)
                .debug(wpkg_output::debug_flags::debug_files)
                .module(wpkg_output::module_build_package);
            continue;
        }
        if(found.find(directory_name.full_path()) != found.end())
        {
            throw wpkgar_exception_defined_twice("same filename (directory) defined twice in data archive");
        }
        info.set_uri(directory_name);
        // remove the drive letter if specified here
        info.set_filename(directory_name.path_only(false));
        if(!force_owner.empty())
        {
            info.set_user(force_owner);
            if(force_uid != -1)
            {
                info.set_uid(force_uid);
            }
        }
        if(!force_group.empty())
        {
            info.set_group(force_group);
            if(force_gid != -1)
            {
                info.set_gid(force_gid);
            }
        }
        memfile::memory_file empty_data;
        append_file(data_tar, info, empty_data);
        found[directory_name.full_path()] = info;

        memfile::memory_file files;
        files.dir_rewind(root, true); // recursive this time!
        for(;;)
        {
            memfile::memory_file input_data;
            if(!files.dir_next(info, &input_data))
            {
                break;
            }
            wpkg_filename::uri_filename filename(info.get_uri());
            if(filename.segment_size() <= 1)
            {
                // this is a logic error as it should not happen
                throw std::logic_error("filename does not include at least one \"/\", it cannot be valid in wpkgar_build::build_deb().");
            }
            const std::string file_basename(filename.segment(filename.segment_size() - 1));
            if(file_basename == ".." || file_basename == ".")
            {
                // ignore this or parent directories
                continue;
            }
            if(!wpkg_filename::uri_filename::is_valid_windows_part(file_basename))
            {
                // These characters are a problem under MS-Windows and it's
                // not a good idea on any computer
                throw wpkgar_exception_defined_twice("filename \"" + file_basename + "\" includes unwanted or 'misplaced' characters.");
            }
            // remove the dir_name path part since that's artificial
            // in the resulting output
            filename = filename.remove_common_segments(dir_name).relative_path();
            if(is_exception(filename))
            {
                // this is forbidden by us or the user
                continue;
            }
            if(found.find(filename.full_path()) != found.end())
            {
                throw wpkgar_exception_defined_twice("same filename (" + filename.original_filename() + ") defined twice in data archive");
            }
            memfile::memory_file::file_info::file_type_t type(info.get_file_type());
            if(is_source)
            {
                // most file types are not allowed in source packages
                switch(type)
                {
                case memfile::memory_file::file_info::regular_file:
                case memfile::memory_file::file_info::continuous:
                case memfile::memory_file::file_info::symbolic_link:
                case memfile::memory_file::file_info::directory:
                    break;

                default:
                    throw wpkgar_exception_compatibility("source packages cannot include special files or hard links");

                }
#if defined(MO_LINUX)
                // also, setuid and setgid are not allowed
                // (not available under MS-Windows)
                if(info.get_mode() & (S_ISUID | S_ISGID))
                {
                    throw wpkgar_exception_compatibility("source packages cannot include files with setuid or setgid");
                }
#endif
            }
            info.set_uri(filename);
            // remove the drive letter if specified here
            info.set_filename(filename.path_only(false));
            if(!force_owner.empty())
            {
                info.set_user(force_owner);
                if(force_uid != -1)
                {
                    info.set_uid(force_uid);
                }
            }
            if(!force_group.empty())
            {
                info.set_group(force_group);
                if(force_gid != -1)
                {
                    info.set_gid(force_gid);
                }
            }
            // check advanced meta data for each file
            for(filesmetadata_vector_t::const_iterator it(filesmetadata.begin());
                        it != filesmetadata.end();
                        ++it)
            {
                const std::string pattern(it->get_filename());
                if(!pattern.empty() && pattern[0] != '+' && filename.glob(pattern.c_str()))
                {
                    bool done(true);
                    // got a match, take that info
                    for(int i(0); i < memfile::memory_file::file_info::field_name_max; ++i)
                    {
                        if(it->is_field_defined(static_cast<memfile::memory_file::file_info::field_name_t>(i)))
                        {
                            switch(static_cast<memfile::memory_file::file_info::field_name_t>(i))
                            {
                            case memfile::memory_file::file_info::field_name_package_name:
                            case memfile::memory_file::file_info::field_name_size:
                            case memfile::memory_file::file_info::field_name_raw_md5sum:
                            case memfile::memory_file::file_info::field_name_original_compression:
                            case memfile::memory_file::file_info::field_name_max:
                                throw wpkgar_exception_invalid("invalid field name defined for a file meta data parameter");

                            case memfile::memory_file::file_info::field_name_filename:
                                // this is defined and used as the pattern, we have to skip it here
                                break;

                            case memfile::memory_file::file_info::field_name_file_type:
                                switch((it->get_file_type() << 8) + info.get_file_type())
                                {
                                case (memfile::memory_file::file_info::regular_file      << 8) + memfile::memory_file::file_info::regular_file:
                                case (memfile::memory_file::file_info::hard_link         << 8) + memfile::memory_file::file_info::hard_link:
                                case (memfile::memory_file::file_info::symbolic_link     << 8) + memfile::memory_file::file_info::symbolic_link:
                                case (memfile::memory_file::file_info::character_special << 8) + memfile::memory_file::file_info::character_special:
                                case (memfile::memory_file::file_info::block_special     << 8) + memfile::memory_file::file_info::block_special:
                                case (memfile::memory_file::file_info::directory         << 8) + memfile::memory_file::file_info::directory:
                                case (memfile::memory_file::file_info::fifo              << 8) + memfile::memory_file::file_info::fifo:
                                case (memfile::memory_file::file_info::continuous        << 8) + memfile::memory_file::file_info::continuous:
                                    // nothing to do in all those cases
                                    break;

                                case (memfile::memory_file::file_info::regular_file      << 8) + memfile::memory_file::file_info::continuous:
                                case (memfile::memory_file::file_info::continuous        << 8) + memfile::memory_file::file_info::regular_file:
                                    // switch between spare and continuous or vice versa
                                    info.set_file_type(it->get_file_type());
                                    break;

                                default:
                                    // type mismatch, try with another pattern
                                    done = false;
                                    i = memfile::memory_file::file_info::field_name_max;
                                    break;
                                    //throw wpkgar_exception_invalid("type mismatch between the filesmetadata definition and the input file for \"" + info.get_filename() + "\"");

                                }
                                break;

                            case memfile::memory_file::file_info::field_name_link:
                                // what can we do here?!
                                if(info.get_file_type() != memfile::memory_file::file_info::symbolic_link)
                                {
                                    throw wpkgar_exception_invalid("the filesmetadata definition expected a link but \"/" + info.get_filename() + "\" is not");
                                }
                                break;

                            case memfile::memory_file::file_info::field_name_user:
                                info.set_user(it->get_user());
                                break;

                            case memfile::memory_file::file_info::field_name_group:
                                info.set_group(it->get_group());
                                break;

                            case memfile::memory_file::file_info::field_name_uid:
                                info.set_uid(it->get_uid());
                                break;

                            case memfile::memory_file::file_info::field_name_gid:
                                info.set_gid(it->get_gid());
                                break;

                            case memfile::memory_file::file_info::field_name_mode:
#if !defined(MO_WINDOWS)
                                if(is_source)
                                {
                                    // setuid and setgid are not allowed
                                    // (not available under MS-Windows)
                                    if(it->get_mode() & (S_ISUID | S_ISGID))
                                    {
                                        throw wpkgar_exception_compatibility("source packages cannot include files with setuid or setgid");
                                    }
                                }
#endif
                                // the mode changes only if both files have the same type
                                // (i.e. all regular file in a tree, all directories in a tree, etc.)
                                info.set_mode(it->get_mode());
                                break;

                            case memfile::memory_file::file_info::field_name_mtime:
                                info.set_mtime(it->get_mtime());
                                break;

                            case memfile::memory_file::file_info::field_name_ctime:
                                info.set_ctime(it->get_ctime());
                                break;

                            case memfile::memory_file::file_info::field_name_atime:
                                info.set_atime(it->get_atime());
                                break;

                            case memfile::memory_file::file_info::field_name_dev_major:
                                if(info.get_file_type() != memfile::memory_file::file_info::character_special
                                && info.get_file_type() != memfile::memory_file::file_info::block_special)
                                {
                                    throw wpkgar_exception_invalid("the filesmetadata definition expected a character or block special file but \"/" + info.get_filename() + "\" is not");
                                }
                                info.set_dev_major(it->get_dev_major());
                                break;

                            case memfile::memory_file::file_info::field_name_dev_minor:
                                if(info.get_file_type() != memfile::memory_file::file_info::character_special
                                && info.get_file_type() != memfile::memory_file::file_info::block_special)
                                {
                                    throw wpkgar_exception_invalid("the filesmetadata definition expected a character or block special file but \"/" + info.get_filename() + "\" is not");
                                }
                                info.set_dev_minor(it->get_dev_minor());
                                break;

                            }
                        }
                    }
                    if(done)
                    {
                        break; // XXX should we consider allowing for continuation?
                    }
                }
            }
            append_file(data_tar, info, input_data);
            found[filename.full_path()] = info;

            // regular files get an md5sums
            if(type == memfile::memory_file::file_info::regular_file
            || type == memfile::memory_file::file_info::continuous)
            {
                // round up the size to the next block
                // TODO: let users define the block size
                total_size += (info.get_size() + 511) & -512;
                md5::raw_md5sum raw;
                input_data.raw_md5sum(raw);
                md5sums.printf("%s %c%s\n",
                        md5::md5sum::sum(raw).c_str(),
                        input_data.is_text() ? ' ' : '*',
                        info.get_filename().c_str());
            }
        }
    }
    for(filesmetadata_vector_t::const_iterator it(filesmetadata.begin());
                        it != filesmetadata.end();
                        ++it)
    {
        std::string pattern(it->get_filename());
        if(pattern[0] == '+')
        {
            pattern.erase(0, 1);

            // make sure that all the fields that can be defined are
            if(!it->is_field_defined(memfile::memory_file::file_info::field_name_file_type)
            || (!it->is_field_defined(memfile::memory_file::file_info::field_name_user) && !it->is_field_defined(memfile::memory_file::file_info::field_name_uid))
            || (!it->is_field_defined(memfile::memory_file::file_info::field_name_group) && !it->is_field_defined(memfile::memory_file::file_info::field_name_gid))
            || !it->is_field_defined(memfile::memory_file::file_info::field_name_mode)
            || !it->is_field_defined(memfile::memory_file::file_info::field_name_mtime))
            {
                throw wpkgar_exception_invalid("the file \"/" + pattern + "\" is being added but you did not define all its fields");
            }
            memfile::memory_file input_data;
            memfile::memory_file::file_info add(*it);
            add.set_filename(pattern);
            switch(it->get_file_type())
            {
            case memfile::memory_file::file_info::character_special:
            case memfile::memory_file::file_info::block_special:
                if(!it->is_field_defined(memfile::memory_file::file_info::field_name_dev_major)
                || !it->is_field_defined(memfile::memory_file::file_info::field_name_dev_minor))
                {
                    throw wpkgar_exception_invalid("the special file \"/" + pattern + "\" is being added but you did not define the major and minor device numbers");
                }
            case memfile::memory_file::file_info::fifo:
            case memfile::memory_file::file_info::directory: // TODO: order is important for directories...
            case memfile::memory_file::file_info::symbolic_link:
                append_file(data_tar, add, input_data);
                break;

            default:
                throw wpkgar_exception_invalid("at this time, only character special, block special, and fifo can be auto-created in your data.tar.gz file, \"" + it->get_filename() + "\" is not one of those types");

            }
        }
    }
    data_tar.end_archive();

    if(found.empty())
    {
        // no file, not even a little directory?!
        if(get_parameter(wpkgar_build_ignore_empty_packages, false))
        {
            // user wants to ignore those!?
            // (can be useful with control.info files or when using the --build-and-install command line)
            return;
        }
        throw wpkgar_exception_invalid_emptydir("there are no files to add to the data tarball, which is not currently supported.");
    }

#if 0
    // this is not compatible with Windows schemes
    // we may formalize this at a later date to match mingw32
    //
    // we could also at least have a warning, although we could include the
    // copyright file at a different location and define that location
    // in our database
    //
    // XXX also we need to support checking the copyright file format
    // this file is NOT the LICENSE or COPYING file; it has a special
    // format in Debian that computers can read (we've got that one
    // down, but not in the standard build, only in source packages.)
    std::string copyright("usr/share/doc/" + package + "/copyright");
    if(found.find(copyright) == found.end())
    {
        fprintf(stderr, "error: /usr/share/doc/%s/copyright file missing.\n", package.c_str());
        throw wpkgar_exception_compatibility("package must include a copyright file");
    }
#endif

    if(total_size == 0)
    {
        // TODO: warning... how do we want to handle those?
        // printf("warning:%s: empty data.tar.gz file?\n");
    }

    // verify that we have all the necessary configuration files
    memfile::memory_file conffiles;
    if(!conffiles_name.empty())
    {
        conffiles.create(memfile::memory_file::file_format_other);
        memfile::memory_file in_conffiles;
        in_conffiles.read_file(conffiles_name);
        int offset(0);
        std::string conf_filename;
        while(in_conffiles.read_line(offset, conf_filename))
        {
            // first we canonicalize those filenames the best we can
            wpkg_filename::uri_filename s(conf_filename.c_str());
            std::string n;
            if(s.is_absolute())
            {
                n = s.full_path();
            }
            else
            {
                n = "/" + s.full_path();
            }
            if(*n.rbegin() == '/')
            {
                throw wpkgar_exception_invalid("configuration filenames cannot end with a slash (/) as it only supports regular files");
            }
            conffiles.write((n + "\n").c_str(), static_cast<int>(conffiles.size()), static_cast<int>(n.size() + 1));
            n = n.substr(1);
            std::map<case_insensitive::case_insensitive_string, memfile::memory_file::file_info>::const_iterator found_it(found.find(n));
            if(found_it == found.end())
            {
                throw wpkgar_exception_defined_twice("configuration file \"" + n + "\" defined in conffiles not present in data.tar.gz");
            }
            switch(found_it->second.get_file_type())
            {
            case memfile::memory_file::file_info::regular_file:
            case memfile::memory_file::file_info::continuous:
                break;

            default:
                throw wpkgar_exception_compatibility("configuration files must be regular files (not even symbolic links) \"" + n + "\" is not compatible");

            }
        }
    }

    // compress the result, now we have a data_tar_gz
    // (note that the compressor may be bz2, 7z, etc.)
    memfile::memory_file data_tar_gz;
    if(f_compressor == memfile::memory_file::file_format_other)
    {
        data_tar.copy(data_tar_gz);
    }
    else
    {
        data_tar.compress(data_tar_gz, f_compressor, f_zlevel);
    }
    data_tar.reset();

    if(fields.field_is_defined("Extra-Size"))
    {
        total_size += fields.get_field_integer("Extra-Size") * 1024;
        fields.delete_field("Extra-Size");
    }

    // in the control file, save the computed "installed size" if not defined
    if(!fields.field_is_defined("Installed-Size"))
    {
        std::stringstream str_total;
        str_total << (total_size + 1023) / 1024;
        fields.set_field("Installed-Size", str_total.str());
    }
    else
    {
        std::string installed_size(fields.get_field("Installed-Size"));
        size_t size(strtol(installed_size.c_str(), 0, 10));
        if(total_size > size)
        {
            // TODO: warning... how do we want to handle those?
            // printf("warning:%s: the total size computed of your data is larger than the Installed-Size you indicated in your control file.\n");
        }
    }

    // Build-Depends should not be defined in a "regular" package,
    // only source packages so delete if not source

    // WARNING: the assignment with = is required with cl in the following!!!
    for(const char * const *non_necessary_fields = (is_source ? non_source_fields : non_binary_fields); *non_necessary_fields != NULL; ++non_necessary_fields)
    {
        if(fields.field_is_defined(*non_necessary_fields))
        {
            fields.delete_field(*non_necessary_fields);
        }
    }

    if(!fields.field_is_defined(wpkg_control::control_file::field_date_factory_t::canonicalized_name()))
    {
        // RFC 2822 date
        fields.set_field(wpkg_control::control_file::field_date_factory_t::canonicalized_name(), wpkg_util::rfc2822_date());
    }

    // save the version of the packager used to create this package
    fields.set_field(wpkg_control::control_file::field_packagerversion_factory_t::canonicalized_name(), debian_packages_version_string());

    // now create the control_tar file with the control and md5sum files
    memfile::memory_file control_tar;
    control_tar.create(memfile::memory_file::file_format_tar);
    found.clear();

    // add control file
    fields.rewrite_dependencies(); // canonicalize the dependencies
    fields.write(ctrl, wpkg_field::field_file::WRITE_MODE_FIELD_ONLY);
    {
        memfile::memory_file::file_info info;
        info.set_mode(0444);
        info.set_user("Administrator");
        info.set_group("Administrators");
        info.set_filename("control");
        info.set_size(ctrl.size());
        append_file(control_tar, info, ctrl);
        found["control"] = info;
    }

    // add md5sums
    {
        memfile::memory_file::file_info info;
        info.set_mode(0444);
        info.set_user("Administrator");
        info.set_group("Administrators");
        info.set_filename("md5sums");
        info.set_size(md5sums.size());
        append_file(control_tar, info, md5sums);
        found["md5sums"] = info;
    }

    // if defined, add conffiles
    if(conffiles.get_format() == memfile::memory_file::file_format_other)
    {
        if(is_source)
        {
            throw wpkgar_exception_compatibility("a conffiles cannot be included in a source package");
        }
        memfile::memory_file::file_info info;
        info.set_mode(0444);
        info.set_user("Administrator");
        info.set_group("Administrators");
        info.set_filename("conffiles");
        info.set_size(conffiles.size());
        append_file(control_tar, info, conffiles);
        found["conffiles"] = info;
    }

    // add whatever else the user provided
    // note that we accept recursivity so you can even have sub-directories which
    // in general wpkg will ignore
    memfile::memory_file extra_control;
    extra_control.dir_rewind(wpkg_dir, false);
    for(;;)
    {
        memfile::memory_file::file_info info;
        memfile::memory_file input_data;
        if(!extra_control.dir_next(info, &input_data))
        {
            break;
        }
        // since it's not recursive and we read the WPKG directly we can
        // get the basename to get the exact filename we're interested in
        wpkg_filename::uri_filename uri(info.get_uri());
        case_insensitive::case_insensitive_string filename(uri.basename());
        if(filename == "."
        || filename == ".."
        || filename == "control"
        || filename == "conffiles"
        || filename == "filesmetadata"
        || filename == "substvars"
        || is_exception(filename))
        {
            // ignore the "." and ".." and "substvars" entries
            // ignore the control and conffiles files which we already added
            // ignore all system and user defined exceptions
            continue;
        }
        if(info.get_file_type() != memfile::memory_file::file_info::regular_file)
        {
            wpkg_output::log("not adding file %1 which is not a regular file to the control.tar archive.")
                    .arg(filename)
                .level(wpkg_output::level_warning)
                .module(wpkg_output::module_build_package)
                .action("build-package");
            continue;
        }
        if(filename == "md5sums"
        || filename == "debian-binary"
        || filename == "wpkg-version"
        || filename == "control"  // this includes control.tar[.gz]
        || filename == "data")    // this includes data.tar[.gz]
        {
            throw wpkgar_exception_compatibility("the control directory cannot include file \"" + info.get_uri().original_filename() + "\"");
        }

        // get the filename with extensions now
        if(uri.segment_size() == 0)
        {
            throw std::logic_error("somehow a filename in the WPKG directory does not have any segment");
        }
        filename = uri.segment(uri.segment_size() - 1);

        // in general this will include scripts (pre/post install/remove)
        // TODO: verify each filename?
        if(filename == "validate.sh")
        {
            filename = "validate";
        }
        else if(filename == "preinst.sh")
        {
            filename = "preinst";
        }
        else if(filename == "postinst.sh")
        {
            filename = "postinst";
        }
        else if(filename == "prerm.sh")
        {
            filename = "prerm";
        }
        else if(filename == "postrm.sh")
        {
            filename = "postrm";
        }
        enum file_type_t
        {
            FILE_TYPE_UNDEFINED,
            FILE_TYPE_SHELL_SCRIPT,
            FILE_TYPE_BATCH_SCRIPT
        };
        file_type_t file_type(FILE_TYPE_UNDEFINED);
        char buf[16];
        int size(input_data.read(buf, 0, sizeof(buf)));
        // most used shells in Unix land (should we limit the test to "#!" ?)
        if((size >= 9 && strncmp("#!/bin/sh", buf, 9) == 0)
        || (size >= 10 && strncmp("#!/bin/csh", buf, 10) == 0)
        || (size >= 11 && strncmp("#!/bin/tcsh", buf, 11) == 0)
        || (size >= 11 && strncmp("#!/bin/dash", buf, 11) == 0)
        || (size >= 11 && strncmp("#!/bin/bash", buf, 11) == 0)
        || (size >= 12 && strncmp("#!/bin/rbash", buf, 12) == 0))
        {
            file_type = FILE_TYPE_SHELL_SCRIPT;
        }
        else if(size >= 4 && strncmp("REM ", buf, 4) == 0)
        {
            file_type = FILE_TYPE_BATCH_SCRIPT;
        }
        if(arch.get_os() != "all" || arch.is_source())
        {
            // Unix specific files
            if(file_type == FILE_TYPE_SHELL_SCRIPT
            || filename == "validate"
            || filename == "preinst" || filename == "postinst"
            || filename == "prerm" || filename == "postrm")
            {
                if(!arch.is_unix() || arch.is_source())
                {
                    // not the right architecture
                    wpkg_output::log("not adding file %1 which is not a valid script for the package architecture.")
                            .quoted_arg(filename)
                        .debug(wpkg_output::debug_flags::debug_detail_files)
                        .module(wpkg_output::module_build_package)
                        .action("build-package");
                    continue;
                }
            }

            // MS-Windows specific files
            if(file_type == FILE_TYPE_BATCH_SCRIPT
            || filename == "validate.bat"
            || filename == "preinst.bat" || filename == "postinst.bat"
            || filename == "prerm.bat" || filename == "postrm.bat")
            {
                if(!arch.is_mswindows() || arch.is_source())
                {
                    // not the right architecture
                    wpkg_output::log("not adding file %1 which is not a valid script for the package architecture.")
                            .quoted_arg(filename)
                        .debug(wpkg_output::debug_flags::debug_detail_files)
                        .module(wpkg_output::module_build_package)
                        .action("build-package");
                    continue;
                }
            }
        }

        if(found.find(filename) != found.end())
        {
            throw wpkgar_exception_defined_twice("two files with the same name cannot be included in the same control archive");
        }
        info.set_filename(filename);
        info.set_mode(0444);
        info.set_user("Administrator");
        info.set_group("Administrators");
        append_file(control_tar, info, input_data);
        found[filename] = info;
    }
    control_tar.end_archive();

    // compress the result, now we have a control_tar_gz
    memfile::memory_file control_tar_gz;
    // To be dpkg compatible the control file must be compressed
    // with gzip, no choice; we may later offer a way to change
    // the compressor for the control tarball file
    //if(f_compressor == memfile::memory_file::file_format_other)
    //{
    //    control_tar.copy(control_tar_gz);
    //}
    //else
    //{
    //    control_tar.compress(control_tar_gz, f_compressor, f_zlevel);
    //}
    control_tar.compress(control_tar_gz, memfile::memory_file::file_format_gz, 9);
    control_tar.reset();

    // finally create the debian package
    memfile::memory_file debian_ar;
    debian_ar.create(memfile::memory_file::file_format_ar);

    // first we must have the debian-binary file
    memfile::memory_file debian_binary;
    debian_binary.create(memfile::memory_file::file_format_other);
    debian_binary.printf("2.0\n");
    {
        memfile::memory_file::file_info info;
        info.set_filename("debian-binary");
        info.set_mode(0444);
        info.set_user("Administrator");
        info.set_group("Administrators");
        info.set_size(debian_binary.size());
        debian_ar.append_file(info, debian_binary);
    }

    // now add the control file
    {
        memfile::memory_file::file_info info;
        switch(control_tar_gz.get_format())
        {
        case memfile::memory_file::file_format_tar:
            // no compression
            info.set_filename("control.tar");
            break;

        case memfile::memory_file::file_format_gz:
            info.set_filename("control.tar.gz");
            break;

        case memfile::memory_file::file_format_bz2:
            info.set_filename("control.tar.bz2");
            break;

        case memfile::memory_file::file_format_lzma:
            info.set_filename("control.tar.lzma");
            break;

        case memfile::memory_file::file_format_xz:
            info.set_filename("control.tar.xz");
            break;

        default:
            throw wpkgar_exception_parameter("the compressed control file data has an unknown compressed format");

        }
        info.set_mode(0444);
        info.set_user("Administrator");
        info.set_group("Administrators");
        info.set_size(control_tar_gz.size());
        debian_ar.append_file(info, control_tar_gz);
    }

    // and finally the data tarball
    {
        memfile::memory_file::file_info info;
        switch(data_tar_gz.get_format())
        {
        case memfile::memory_file::file_format_tar:
            // no compression
            info.set_filename("data.tar");
            break;

        case memfile::memory_file::file_format_gz:
            info.set_filename("data.tar.gz");
            break;

        case memfile::memory_file::file_format_bz2:
            info.set_filename("data.tar.bz2");
            break;

        case memfile::memory_file::file_format_lzma:
            info.set_filename("data.tar.lzma");
            break;

        case memfile::memory_file::file_format_xz:
            info.set_filename("data.tar.xz");
            break;

        default:
            throw wpkgar_exception_parameter("the compressed data has an unknown compressed format");

        }
        info.set_mode(0444);
        info.set_user("Administrator");
        info.set_group("Administrators");
        info.set_size(data_tar_gz.size());
        debian_ar.append_file(info, data_tar_gz);
    }

    save_package(debian_ar, fields);

    if(fields.field_is_defined("Standards-Version"))
    {
        wpkg_control::standards_version v(fields.get_standards_version());
        wpkg_output::log("package %1 was created with standards version %2.%3.%4.%5.")
                .quoted_arg(package)
                .arg(v.get_version(wpkg_control::standards_version::standards_major_version    ))
                .arg(v.get_version(wpkg_control::standards_version::standards_minor_version    ))
                .arg(v.get_version(wpkg_control::standards_version::standards_major_patch_level))
                .arg(v.get_version(wpkg_control::standards_version::standards_minor_patch_level))
            .module(wpkg_output::module_build_info)
            .package(f_package_name.path_only())
            .action("build-info");
    }
}


/** \brief Initialize a default source property.
 *
 * This constructor is available so we can put source properties in a
 * map which requires a way to build properties without parameters.
 *
 * \param[in] name  The name of the property being checked.
 * \param[in] help  The help (detailed description) for this property.
 */
wpkgar_build::source_validation::source_property::source_property()
    : f_name("Unknown")
    , f_help("No help")
    //, f_status(SOURCE_VALIDATION_STATUS_UNKNOWN) -- auto-init
    //, f_value("") -- auto-init
{
}


/** \brief Initialize a source property.
 *
 * By default a source property is marked as unknown and has no value.
 *
 * The name and help string pointers are used to describe the property
 * to the end users.
 *
 * \param[in] name  The name of the property being checked.
 * \param[in] help  The help (detailed description) for this property.
 */
wpkgar_build::source_validation::source_property::source_property(const char *name, const char *help)
    : f_name(name)
    , f_help(help)
    //, f_status(SOURCE_VALIDATION_STATUS_UNKNOWN) -- auto-init
    //, f_value("") -- auto-init
{
}


/** \brief Return the name of this property.
 *
 * This function returns the textual name of this property. It represents
 * what the system checks when marking this property as valid or invalid.
 *
 * \return A constant (read-only) raw pointer to the name.
 */
const char *wpkgar_build::source_validation::source_property::get_name() const
{
    return f_name;
}


/** \brief Return the help for this property.
 *
 * In order to help the users, we have a help about the missing property.
 * This is useful to list properties to the user and give them detailed
 * information about them.
 *
 * \return A constant (read-only) raw pointer to the help.
 */
const char *wpkgar_build::source_validation::source_property::get_help() const
{
    return f_help;
}


/** \brief Change the status of the property.
 *
 * This function changes the status of the property to the specified
 * \p status parameter.
 *
 * This is done while validating a project. It should not be used outside
 * of that process.
 *
 * \param[in] status  The new value of the status.
 */
void wpkgar_build::source_validation::source_property::set_status(status_t status)
{
    f_status = status;
}


/** \brief Retrieve the current status.
 *
 * This function is used to retrieve the current status of the source
 * property. By default the status is set to SOURCE_VALIDATION_STATUS_UNKNOWN.
 * Once the validation process has run, it should be a different value
 * unless that property could not be checked.
 *
 * \return The current status of the source property.
 */
wpkgar_build::source_validation::source_property::status_t wpkgar_build::source_validation::source_property::get_status() const
{
    return f_status;
}


/** \brief Set the value of the property.
 *
 * In some cases, the validation process will set the property value to
 * a value representing the property. For example, we allow the ChangeLog
 * to be written in many different ways such as "changelog" or "Changelog".
 * The exact case of the file is not important in itself, but it can be
 * useful for the caller to know about it so we save that information in
 * the property.
 *
 * After the first call to this function, the value_is_set() function
 * returns true. Before then, it returns false.
 *
 * \param[in] value  The new value for this property.
 */
void wpkgar_build::source_validation::source_property::set_value(const std::string& value)
{
    f_value = value;
    f_value_is_set = true;
}


/** \brief Check whether the value was set.
 *
 * This function tells you whether a value was set in this property.
 *
 * \return true if the value was set at least once.
 */
bool wpkgar_build::source_validation::source_property::value_is_set() const
{
    return f_value_is_set;
}


/** \brief Get the value of the property
 *
 * Assuming that the validation process defines a property's value, it can
 * be retrieved with this function.
 *
 * By default the value is set to the empty string. If the empty string is
 * a valid value, then you may first want to check whether the value was set
 * with the value_is_set() function.
 *
 * \return The current value of this property.
 */
std::string wpkgar_build::source_validation::source_property::get_value() const
{
    return f_value;
}


namespace
{

/** \brief List of source validations.
 *
 * This array includes a list of validations performed when validating the
 * project directory before creating the source package.
 */
const wpkg_control::control_file::list_of_terms_t g_source_validation_property[] =
{
    {
        "changelog",
        "The work done in a project is expected to be reported on "
        "using a changelog file. The changelog has a very specific "
        "format which includes the package name, version, distribution, "
        "urgency, maintainer, date, and the actual change log. "
        "All of that information is used to generate the control file. "
        "The format gets validation, if any errors are detected, then "
        "the building of the source package fails. The changelog file "
        "is found under debian/changelog or wpkg/changelog and the case "
        "is important. Note that we will also accept a ChangeLog in the "
        "root directory because many people put their ChangeLog there, "
        "however, it is likely that the root ChangeLog file will be "
        "invalid (not following the Debian syntax.)"
    },
    {
        "Changes",
        "The list of changes for that version. Changes cannot already "
        "be defined in the control.info file, it always comes from the "
        "changelog file. If defined in the control.info, then an error "
        "is generated and the process stops."
    },
    {
        "Changes-Date",
        "The date in the footer of the changelog represents the date "
        "when changes to the package started. In Debian this represents "
        "the value of the Date field. We think that the Date field should "
        "be the date when the package is being built instead. So we have "
        "of a second date to not lose the changelog footer date. The format "
        "of the date is the format chosen by Debian:\n"
        "   DDD, dd mmm yyyy HH:MM:SS +ZZZ"
    },
    {
        "CMakeLists.txt",
        "A project to be built with wpkg must use cmake to create its "
        "compile environment (Makefile's). This means a file named "
        "CMakeLists.txt must exist in the root directory. It will be "
        "used to create the source tarball, and later to build the "
        "project, run its tests, create its binary packages, etc."
    },
    {
        "control.info",
        "To make use of the wpkg build system, you must create a "
        "control.info file in the root directory of your project. "
        "This file is used to include parameters that are to appear "
        "in the control file of the project binary packages. "
        "The Package and Description fields are mandatory. The other fields "
        "that are mandatory in a binary package are gathered by the packager "
        "and added the control.info file before generating the source "
        "package. For example, the version is found in the changelog file."
    },
    {
        "copyright",
        "A valid Debian package must include a computer compatible "
        "copyright file. This file describes the content of a project "
        "in terms of licenses. It generally includes information about the "
        "project as a whole, and one license per directory and/or per "
        "file. The syntax of the copyright is similar to a control file: "
        "it uses fields and values separated by colons."
    },
    {
        "Distributions",
        "The distributions of a package defines the environments "
        "a source package is compiled for. This value is the top of the "
        "path to this package. The rest of the path is defined in the "
        "Component field."
    },
    {
        "INSTALL",
        "All packages must have an INSTALL or INSTALL.txt file. If it is "
        "not present, then wpkg creates one in the output file. Note that "
        "the default INSTALL file explains how to build and install the "
        "project using wpkg. It may not be what you have in mind if you "
        "expect many users to build your project without wpkg."
    },
    {
        "Maintainer",
        "The maintainer is the user who creates the wpkg package. "
        "His name and email address appears in the footer of each change "
        "log. When the maintainer changes, the old footers do not get "
        "modified, only the new entries make use of the new maintainer. "
        "The name and email address must be a valid email address. This "
        "means, for example, that the name needs to be written between "
        "double quotes if it includes a comma or a period. For example:\n"
        "   \"Wilke, Alexis\" <alexis@m2osw.com>"
    },
    {
        "Package",
        "The name of the package must be specified in the control.info "
        "and the changelog files. That name must match in both places "
        "and if not the validation fails. Note that the control.info "
        "file may also include sub-packages which have a different "
        "name although we check those too (See the Sub-Packages validation.)"
    },
    {
        "README",
        "Most project include a README or README.txt file. This file "
        "includes some basic information about the project. What I would "
        "call a long description (at times programmers put their whole "
        "childhood history in those!) The file should be there because "
        "it is put along the packages in FTP sites. This allows people who "
        "are interested by the project to read about it before downloading "
        "it, which gives them a chance to not waste their time if it is not "
        "a match."
    },
    {
        "Sub-Packages",
        "The control.info file must include a Sub-Packages field with a "
        "list of sub-packages. For example:\n"
        "   Sub-Packages: runtime*, development, documentation\n"
        "This list must reference Package names that all start with the "
        "default package name (as found in the changelog):\n"
        "   Package/runtime: wpkg\n"
        "   Package/development: wpkg-dev\n"
        "   Package/documentation: wpkg-doc\n"
    },
    {
        "Urgency",
        "The wpkg/changelog file may include one parameter named "
        "urgency that is set to one of the valid Debian urgency "
        "values: low, medium, high, emergency, or critical. "
        "Any other valid is considered invalid. Also it must be equal "
        "to the one found in the control.info file if defined there "
        "(you should not defined the Urgency field in your control.info "
        "file, though.)"
    },
    {
        "Version",
        "The build process ensures that the version specification in the "
        "control.info file and the wpkg/changelog file are equal. If not "
        "then the validation fails. When generating the binary packages "
        "we further will check that the tools are given a version that "
        "is equal to the wpkg/changelog latest version."
    },
    { NULL, NULL }
};


} // no name namespace


/** \brief Initialize a source validation object.
 *
 * This function initialize a source validation object, which means creating
 * all the properties currently supported by the validation process used
 * when creating a source build.
 */
wpkgar_build::source_validation::source_validation()
{
    // add all the properties now; they all are marked as UNKNOWN at this
    // point; calling the validate_source() function is required to get
    // the correct information in the properties.
    for(const wpkg_control::control_file::list_of_terms_t *s(g_source_validation_property); s->f_term != NULL; ++s)
    {
        source_property p(s->f_term, s->f_help);
        f_properties[s->f_term] = p;
    }
}


/** \brief Mark that the validation of a given property is complete.
 *
 * This function sets the status of the specified property to something
 * else than the default of SOURCE_VALIDATION_STATUS_UNKNOWN.
 *
 * Note that this function can actually be called any number of times.
 * It should be called at least once to mark the proper as valid or
 * invalid.
 *
 * \param[in] name  Name of the property to be marked as complete.
 * \param[in] status  The new status of the property.
 */
void wpkgar_build::source_validation::done(const char *name, source_property::status_t status)
{
    std::string n(name);
    if(f_properties.find(n) == f_properties.end())
    {
        throw wpkgar_exception_undefined("no build source validation property is called \"" + n + "\" (done)");
    }

    f_properties[n].set_status(status);
}


/** \brief Save a value for a property.
 *
 * When a property has a special value or even just a value that can be
 * representing in a small string (under 80 characters) then it should
 * be set using this function. This is done by the validation process.
 * Do not change this value from the outside of the validation process.
 *
 * \param[in] name  The name of the property to set.
 * \param[in] value  The new value for this property.
 */
void wpkgar_build::source_validation::set_value(const char *name, const std::string& value)
{
    std::string n(name);
    if(f_properties.find(n) == f_properties.end())
    {
        throw wpkgar_exception_undefined("no build source validation property is called \"" + n + "\" (set_value)");
    }

    f_properties[n].set_value(value);
}


/** \brief Get the list of properties.
 *
 * This function returns the list of properties with their actual status
 * and values.
 *
 * Note that we return a constant source_properties_t reference so you may
 * use the map without having to make a copy. Just do not attempt to modify
 * the map from the outside.
 *
 * \return The list of properties as a map keyed by name.
 */
const wpkgar_build::source_validation::source_properties_t& wpkgar_build::source_validation::get_properties() const
{
    return f_properties;
}


/** \brief Retrieve the current status of a property.
 *
 * Check the current status of a specific property.
 *
 * \param[in] name  The name of the property to set.
 */
wpkgar_build::source_validation::source_property::status_t wpkgar_build::source_validation::get_status(const char *name) const
{
    std::string n(name);
    source_properties_t::const_iterator property(f_properties.find(n));
    if(property == f_properties.end())
    {
        throw wpkgar_exception_undefined("no build source validation property is called \"" + n + "\" (get_status)");
    }

    return property->second.get_status();
}


/** \brief Retrieve the current status of a property.
 *
 * Check the current status of a specific property.
 *
 * \param[in] name  The name of the property to set.
 */
std::string wpkgar_build::source_validation::get_value(const char *name) const
{
    std::string n(name);
    source_properties_t::const_iterator property(f_properties.find(n));
    if(property == f_properties.end())
    {
        throw wpkgar_exception_undefined("no build source validation property is called \"" + n + "\" (get_value)");
    }

    return property->second.get_value();
}


/** \brief Returns the list of validations and their help.
 *
 * This function returns a list of validations that the build process
 * conducts in order to generate a source package. This one is clearly
 * documented to ensure that our users can quickly and easily create
 * source packages that we in turn compile on build servers.
 */
const wpkg_control::control_file::list_of_terms_t *wpkgar_build::source_validation::list()
{
    return g_source_validation_property;
}


}  // namespace wpkgar
// vim: ts=4 sw=4 et
