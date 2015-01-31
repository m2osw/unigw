/*    wpkgar_repository.cpp -- manage a repository of debian packages
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

/** \file
 * \brief Implementation of the functions handling repository files.
 *
 * This file includes the functions used to handle the sources.list files
 * on a target system. It also handles the update and upgrade features
 * of the wpkg tool.
 */
#include    "libdebpackages/wpkgar_repository.h"
#include    "libdebpackages/debian_version.h"
#include    "libdebpackages/wpkg_util.h"
#include    <algorithm>
#include    <sstream>
#include	<time.h>


namespace wpkgar
{


/** \class wpkgar_repository
 * \brief Handle repositories.
 *
 * The repository handling is done with this class. The repository class
 * manages the sources, updates the local cache with sources indexes,
 * and computes potential upgrades.
 */


/** \class wpkgar_repository::index_entry
 * \brief One package fullname and its control file.
 *
 * This class holds the file info of a file found in a tarball representing
 * the index of a remote repository. The file info filename represents the
 * name of the package and its version. Optionally, the filename may include
 * an architecture.
 *
 * The class also includes a memory file which is the contents of the control
 * file for that package. Note that this is really a shared pointer to a
 * memory file. This is used because we cannot copy a memory file to another
 * (and it would anyway be a waste of time in this case.)
 *
 * These members are directly accessible since they both are classes
 * themselves.
 */


/** \class wpkgar_repository::source
 * \brief A sources.lists manager.
 *
 * This class handles the sources.lists file format by reading it and
 * allowing the repository implementation use it to compute the different
 * indexes we talked about.
 */


/** \class wpkgar_repository::update_entry_t
 * \brief Update database management.
 *
 * This class represents one entry in the update database management.
 * Each entry represent one source and thus one index. The entry
 * records information such as whether the last access was successful
 * and whether it is still valid.
 */


/** \class wpkgar_repository::package_item_t
 * \brief The package item handled by the repository object.
 *
 * List the install and remove feature, the repository feature has a
 * package item to be able to manage an internal list of packages.
 * This implementation is compatible with the repository implementation
 * in that it accepts a memory file as input on construction, file that
 * is parsed immediately as the control data. This means the resulting
 * package item can immediately be checked for control file fields.
 */


/** \brief Initialize a package item object.
 *
 * This constructor initializes a repository package item which expects
 * a memory file info with the package file meta data (especially the
 * name, version, and most often architecture of the package), and
 * the control file data.
 *
 * Note that the control data are immediately parsed so if considered
 * invalid by your version of wpkg, an error is raised before the object
 * get finalized.
 */
wpkgar_repository::package_item_t::package_item_t(wpkgar_manager *manager, const memfile::memory_file::file_info& info, const memfile::memory_file& data)
    : f_manager(manager)
    //, f_status(invalid) -- auto-init
    , f_info(info)
    // the following control file comes from a binary package
    , f_control(new wpkg_control::binary_control_file(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::control_file_state_t)))
    , f_cause_for_rejection("package is not yet fully initialized")
{
    f_control->set_input_file(&data);
    f_control->read();
    f_control->set_input_file(NULL);
}


/** \brief Retrieve the current status of this repository package.
 *
 * This function returns a repository package status:
 *
 * \li status_unknown -- at this point the status was not defined, this is the
 *                       default value;
 * \li status_ok -- the status is normal, no worries;
 * \li status_failed -- something did not work as expected, this status manes
 *                      the package cannot be used safely.
 */
wpkgar_repository::package_item_t::package_item_status_t wpkgar_repository::package_item_t::get_status() const
{
    return f_status;
}

const memfile::memory_file::file_info& wpkgar_repository::package_item_t::get_info() const
{
    return f_info;
}

std::string wpkgar_repository::package_item_t::get_name() const
{
    return f_control->get_field("Package");
}

std::string wpkgar_repository::package_item_t::get_architecture() const
{
    return f_control->get_field("Architecture");
}

std::string wpkgar_repository::package_item_t::get_version() const
{
    return f_control->get_field("Version");
}

std::string wpkgar_repository::package_item_t::get_field(const std::string& name) const
{
    return f_control->get_field(name);
}

bool wpkgar_repository::package_item_t::field_is_defined(const std::string& name) const
{
    return f_control->field_is_defined(name);
}

void wpkgar_repository::package_item_t::check_installed_package(bool exists)
{
    // user does not have that package installed (default value)
    f_status = static_cast<int>(not_installed); // FIXME cast

    // we loaded the "core" package before creating package items
    const std::string target_architecture = f_manager->get_field("core", "Architecture");
    const std::string arch(get_architecture());
    if(arch != "all" && !wpkg_dependencies::dependencies::match_architectures(arch, target_architecture))
    {
        // that package does not have the right architecture
        f_status = static_cast<int>(invalid); // FIXME cast
        f_cause_for_rejection = "package has an incompatible architecture";
        return;
    }

    if(exists)
    {
        f_manager->load_package(get_name());
        const wpkgar_manager::package_status_t status(f_manager->package_status(get_name()));
        if(status == wpkgar_manager::installed)
        {
            const std::string installed_version(f_manager->get_field(get_name(), "Version"));
            const std::string update_version(get_version());

            const int c(wpkg_util::versioncmp(installed_version, update_version));
            if(c == 0)
            {
                // user has newest version
                f_status = static_cast<int>(installed); // FIXME cast
            }
            else if(c < 0)
            {
                // user should upgrade
                f_status = static_cast<int>(need_upgrade); // FIXME cast

                // if the selection is currently set to "Hold" then the upgrade is blocked instead
                if(f_manager->field_is_defined(get_name(), wpkg_control::control_file::field_xselection_factory_t::canonicalized_name()))
                {
                    wpkg_control::control_file::field_xselection_t::selection_t selection(wpkg_control::control_file::field_xselection_t::validate_selection(f_manager->get_field(get_name(), wpkg_control::control_file::field_xselection_factory_t::canonicalized_name())));
                    if(selection == wpkg_control::control_file::field_xselection_t::selection_hold)
                    {
                        f_status = static_cast<int>(blocked_upgrade); // FIXME cast
                    }
                }
            }
            else
            {
                // user could downgrade; but we ignore the matter here
                // we actually mark the package as invalid
                f_status = static_cast<int>(invalid); // FIXME cast
                f_cause_for_rejection = "package has an older version, we do not allow downgrading in auto-upgrade mode";
            }
        }
    }
}

std::string wpkgar_repository::package_item_t::get_cause_for_rejection() const
{
    return f_cause_for_rejection;
}









std::string wpkgar_repository::source::get_type() const
{
    return f_type;
}

std::string wpkgar_repository::source::get_parameter(const std::string& name, const std::string& def_value) const
{
    parameter_map_t::const_iterator it(f_parameters.find(name));
    if(it == f_parameters.end())
    {
        return def_value;
    }
    return it->second;
}

wpkgar_repository::source::parameter_map_t wpkgar_repository::source::get_parameters() const
{
    return f_parameters;
}

std::string wpkgar_repository::source::get_uri() const
{
    return f_uri;
}

std::string wpkgar_repository::source::get_distribution() const
{
    return f_distribution;
}

int wpkgar_repository::source::get_component_size() const
{
    return static_cast<int>(f_components.size());
}

std::string wpkgar_repository::source::get_component(int index) const
{
    return f_components[index];
}


void wpkgar_repository::source::set_type(const std::string& type)
{
    f_type = type;
}

void wpkgar_repository::source::add_parameter(const std::string& name, const std::string& value)
{
    f_parameters[name] = value;
}

void wpkgar_repository::source::set_uri(const std::string& uri)
{
    f_uri = uri;
}

void wpkgar_repository::source::set_distribution(const std::string& distribution)
{
    f_distribution = distribution;
}

void wpkgar_repository::source::add_component(const std::string& component)
{
    f_components.push_back(component);
}




int32_t wpkgar_repository::update_entry_t::get_index() const
{
    return f_index;
}

wpkgar_repository::update_entry_t::update_entry_status_t wpkgar_repository::update_entry_t::get_status() const
{
    return f_status;
}

std::string wpkgar_repository::update_entry_t::get_uri() const
{
    return f_uri;
}

time_t wpkgar_repository::update_entry_t::get_time(update_entry_time_t t) const
{
    return f_times[t];
}

void wpkgar_repository::update_entry_t::set_index(int index)
{
    if(f_index != 0)
    {
        throw wpkgar_exception_invalid("the index of an update index entry cannot be modified if not zero");
    }
    if(index <= 0)
    {
        throw wpkgar_exception_invalid("the index of an update index entry must be set to a positive number");
    }
    f_index = index;
}

void wpkgar_repository::update_entry_t::set_status(update_entry_status_t status)
{
    f_status = static_cast<int>(status);  // FIXME cast
}

void wpkgar_repository::update_entry_t::set_uri(const std::string& uri)
{
    f_uri = uri;
}

void wpkgar_repository::update_entry_t::update_time(time_t t)
{
    if(f_times[first_try] == 0)
    {
        f_times[first_try] = t;
    }

    if(status_ok != f_status)
    {
        f_times[last_failure] = t;
    }
    else
    {
        if(f_times[first_success] == 0)
        {
            f_times[first_success] = t;
        }
        f_times[last_success] = t;
    }
}

void wpkgar_repository::update_entry_t::from_string(const std::string& line)
{
    update_entry_t index_entry;
    std::vector<std::string> v;

    // first break the line in X number of strings (must be 4)
    std::string::size_type p(0), q;
    for(;;)
    {
        q = line.find_first_of(' ', p);
        if(q == std::string::npos)
        {
            break;
        }
        v.push_back(line.substr(p, q - p));
        p = q + 1;
    }
    v.push_back(line.substr(p));
    if(v.size() != 4)
    {
        throw wpkgar_exception_invalid("an index entry line must include 4 entries");
    }

    // read the index
    char *end;
    f_index = strtol(v[0].c_str(), &end, 10);
    if(end == NULL || *end != '\0')
    {
        throw wpkgar_exception_invalid("index is not a valid number");
    }
    if(f_index <= 0)
    {
        throw wpkgar_exception_invalid("index cannot be negative or null");
    }

    // read the status
    if(v[1] == "unknown")
    {
        f_status = static_cast<int>(status_unknown); // FIXME cast
    }
    else if(v[1] == "ok")
    {
        f_status = static_cast<int>(status_ok); // FIXME cast
    }
    else if(v[1] == "failed")
    {
        f_status = static_cast<int>(status_failed); // FIXME cast
    }
    else
    {
        throw wpkgar_exception_invalid("index status \"" + v[1] + "\" not understood");
    }

    // get the uri
    f_uri = v[2];

    // break the times in X number of strings (must be 4)
    std::vector<std::string> times;
    p = 0;
    for(;;)
    {
        q = v[3].find_first_of(',', p);
        if(q == std::string::npos)
        {
            break;
        }
        times.push_back(v[3].substr(p, q - p));
        p = q + 1;
    }
    times.push_back(v[3].substr(p));
    if(times.size() != time_max)
    {
        throw wpkgar_exception_invalid("the times in an index entry line must include 4 entries");
    }

    // get the different Unix times
    for(int i(0); i < time_max; ++i)
    {
        f_times[i] = strtol(times[i].c_str(), &end, 10);
        if(end == NULL || *end != '\0')
        {
            throw wpkgar_exception_invalid("Unix time is not a valid number");
        }
        if(f_times[i] < 0)
        {
            throw wpkgar_exception_invalid("Unix time cannot be negative");
        }
    }
}

std::string wpkgar_repository::update_entry_t::to_string() const
{
    std::stringstream output;

    output << f_index << " ";
    switch(f_status)
    {
    case status_ok:
        output << "ok ";
        break;

    case status_failed:
        output << "failed ";
        break;

    default:
        output << "unknown ";
        break;

    }

    output << f_uri << " ";

    for(int i(0); i < time_max - 1; ++i)
    {
        output << f_times[i] << ",";
    }
    output << f_times[time_max - 1];

    return output.str();
}





wpkgar_repository::wpkgar_repository(wpkgar_manager *manager)
    : f_manager(manager)
    //, f_flags() -- auto-init
    //, f_packages() -- auto-init
    //, f_repository() -- auto-init
    //, f_repository_packages_loaded(false) -- auto-init
    //, f_update_index() -- auto-init
{
}


void wpkgar_repository::set_parameter(parameter_t flag, int value)
{
    f_flags[flag] = value;
}


int wpkgar_repository::get_parameter(parameter_t flag, int default_value) const
{
    wpkgar_flags_t::const_iterator it(f_flags.find(flag));
    if(it == f_flags.end())
    {
        return default_value;
    }
    return it->second;
}


/** \brief Create an index of all the Debian packages.
 *
 * This function reads all the specified repository as specified by the
 * add_repository() function, to an \p index_file.
 *
 * The result is an uncompressed tarball with the headers representing the
 * basic information of the package:
 *
 * \li name -- the path, package name, version, architecture, and .deb;
 * \li size -- the size of the control file
 * \li date -- from the Date field found in the control file
 *
 * \param[in] index_file  The file where the repository information is saved.
 */
void wpkgar_repository::create_index(memfile::memory_file& index_file)
{
    // save all the data in a map so we can have it in alphabetical order
    // before creating the output tarball
    typedef std::map<std::string, index_entry> map_t;
    map_t map;

    std::string index_date(wpkg_util::rfc2822_date());
    index_file.create(memfile::memory_file::file_format_tar);
    index_file.set_package_path(".");
    const wpkg_filename::filename_list_t& repositories(f_manager->get_repositories());
    for(wpkg_filename::filename_list_t::const_iterator it(repositories.begin());
                                                       it != repositories.end();
                                                       ++it)
    {
        memfile::memory_file r;
        r.dir_rewind(*it, get_parameter(wpkgar_repository_recursive, false) != 0);
        for(;;)
        {
            memfile::memory_file::file_info info;
            memfile::memory_file data;
            if(!r.dir_next(info, &data))
            {
                break;
            }
            const wpkg_filename::uri_filename& filename(info.get_uri());
            if(info.get_file_type() != memfile::memory_file::file_info::regular_file)
            {
                // we're only interested by regular files,
                // anything else we skip silently
                if(info.get_file_type() != memfile::memory_file::file_info::directory
                || !get_parameter(wpkgar_repository_recursive, false))
                {
                    wpkg_output::log("skip file %1 since it is not a regular file.")
                            .quoted_arg(filename)
                        .debug(wpkg_output::debug_flags::debug_detail_config)
                        .module(wpkg_output::module_repository)
                        .package(filename);
                }
                continue;
            }
            if(filename.extension() != "deb")
            {
                wpkg_output::log("skip file %1 as its extension is not .deb.")
                        .quoted_arg(filename)
                    .debug(wpkg_output::debug_flags::debug_detail_config)
                    .module(wpkg_output::module_repository)
                    .package(filename);
                continue;
            }
            std::string::size_type p(filename.basename().find_first_of('_'));
            if(p == std::string::npos)
            {
                wpkg_output::log("package %1 has an invalid filename.")
                        .quoted_arg(filename)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_repository)
                    .package(filename)
                    .action("install-validation");
                continue;
            }
            data.dir_rewind();
            for(;;)
            {
                memfile::memory_file::file_info ar_info;
                memfile::memory_file control_tar;
                if(!data.dir_next(ar_info, &control_tar))
                {
                    break;
                }
                const wpkg_filename::uri_filename& ar_filename(ar_info.get_filename());
                if(ar_info.get_file_type() == memfile::memory_file::file_info::regular_file
                && ar_filename.basename() == "control")
                {
                    if(control_tar.is_compressed())
                    {
                        memfile::memory_file d;
                        control_tar.copy(d);
                        d.decompress(control_tar);
                    }
                    control_tar.dir_rewind();
                    for(;;)
                    {
                        memfile::memory_file::file_info ctrl_info;
                        std::shared_ptr<memfile::memory_file> control(new memfile::memory_file);
                        if(!control_tar.dir_next(ctrl_info, control.get()))
                        {
                            break;
                        }
                        const wpkg_filename::uri_filename& ctrl_filename(ctrl_info.get_filename());
                        if(ctrl_info.get_file_type() == memfile::memory_file::file_info::regular_file
                        && ctrl_filename.basename() == "control")
                        {
                            // here we want to use a mix of the data found in
                            // info, ar_info, and control.
                            wpkg_control::binary_control_file ctrl(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::control_file_state_t));
                            ctrl.set_input_file(&*control);
                            ctrl.read();
                            ctrl.set_input_file(NULL);
                            memfile::memory_file::file_info idx_info;
                            wpkg_filename::uri_filename path(filename.remove_common_segments(*it).dirname(false));
                            std::string ctrl_name(path.append_child(filename.basename() + ".ctrl").path_only());
                            wpkg_output::log("add package %1 to this repository index file.")
                                    .quoted_arg(ctrl_name)
                                .module(wpkg_output::module_repository)
                                .action("repository-index");
                            idx_info.set_filename(ctrl_name);
                            idx_info.set_file_type(memfile::memory_file::file_info::regular_file);
                            idx_info.set_user("root");
                            idx_info.set_group("root");
                            idx_info.set_uid(0);
                            idx_info.set_gid(0);
                            idx_info.set_mode(0644);
                            idx_info.set_mtime(info.get_mtime());
                            if(ctrl.field_is_defined("Date"))
                            {
                                std::string date(ctrl.get_field("Date"));
                                struct tm time_info;
                                if(strptime(date.c_str(), "%a, %d %b %Y %H:%M:%S %z", &time_info) != NULL)
                                {
                                    // unfortunately the tar format does not support time64_t
                                    idx_info.set_mtime(mktime(&time_info));
                                }
                            }
                            ctrl.set_field("Index-Date", index_date);
                            ctrl.set_field("Package-md5sum", data.md5sum());
                            ctrl.set_field("Package-Size", data.size());
                            ctrl.write(*control, wpkg_field::field_file::WRITE_MODE_FIELD_ONLY);
                            idx_info.set_size(control->size());
                            index_entry e;
                            e.f_info = idx_info;
                            e.f_control = control;
                            map[ctrl_name] = e;
                            break;
                        }
                    }
                    break; // this is all.
                }
            }
        }
    }
    wpkg_output::log("finalizing output file.")
        .module(wpkg_output::module_repository)
        .action("repository-index");
    for(map_t::const_iterator it(map.begin()); it != map.end(); ++it)
    {
        index_file.append_file(it->second.f_info, *it->second.f_control);
    }
}

/** \brief Read the specified file as a repository index file.
 *
 * This function expects the input to be a repository index as created
 * by the \p create_index() function. The load function does not verify
 * each control file, however, it tests that each filename corresponds
 * to a Debian package filename (1 or 2 underscores, a valid package name
 * and version, and when present, a valid architecture.)
 *
 * The input \p file can be compressed. The function will automatically
 * decompress the data before reading the entries.
 *
 * \param[in] file  The file to load in the array of entries.
 * \param[out] entries  A vector that is filled with all the data found in this index file.
 */
void wpkgar_repository::load_index(const memfile::memory_file& file, entry_vector_t& entries)
{
    memfile::memory_file index_file;
    file.copy(index_file);
    if(index_file.is_compressed())
    {
        file.decompress(index_file);
    }

    index_file.dir_rewind();
    for(;;)
    {
        memfile::memory_file::file_info idx_info;
        std::shared_ptr<memfile::memory_file> control(new memfile::memory_file);
        if(!index_file.dir_next(idx_info, control.get()))
        {
            break;
        }
        const std::string filename(idx_info.get_filename());

        // no path allowed; the path is part of the sources.list file URI
        if(filename.find_first_of('/') != std::string::npos)
        {
            throw wpkgar_exception_invalid("an index filename cannot include a \"/\" character");
        }

        // all files must have .ctrl as their extension
        std::string::size_type dot(filename.find_last_of('.'));
        if(dot == std::string::npos)
        {
            throw wpkgar_exception_invalid("an index filename must have a valid extension");
        }
        if(filename.substr(dot) != ".ctrl")
        {
            throw wpkgar_exception_invalid("all the files in an index must have the \".ctrl\" extension, \"" + filename.substr(dot) + "\" is not valid");
        }
        const std::string basename = filename.substr(0, dot);

        // we must at least have a package name and a version
        std::string::size_type p(basename.find_first_of('_'));
        if(p == std::string::npos)
        {
            throw wpkgar_exception_invalid("an index filename must include at least one \"_\" character");
        }
        // verify the package name
        std::string package_name(basename.substr(0, p));
        if(!wpkg_util::is_package_name(package_name))
        {
            throw wpkgar_exception_invalid("\"" + package_name + "\" is not a valid package name and thus this index filename cannot be valid");
        }
        std::string::size_type q(basename.find_last_of('_'));
        std::string version;
        if(p != q)
        {
            std::string arch(basename.substr(q + 1));
            if(!wpkg_dependencies::dependencies::is_architecture_valid(arch))
            {
                throw wpkgar_exception_invalid("\"" + arch + "\" is not a valid architecture and thus this index filename cannot be valid");
            }
            version = basename.substr(p + 1, q - p - 1);
        }
        else
        {
            version = basename.substr(p + 1);
        }
        char version_err[256];
        if(!validate_debian_version(version.c_str(), version_err, sizeof(version_err)))
        {
            throw wpkgar_exception_invalid("\"" + version + "\" is an invalid version and thus this index filename cannot be valid");
        }

        index_entry e;
        e.f_info = idx_info;
        e.f_control = control;
        entries.push_back(e);
    }
}


namespace {
bool not_isspace(char c)
{
    return !isspace(c);
}
}

/** \brief Load a sources.list file to a vector of sources.
 *
 * This function reads a sources.list file with a set of URIs, distribution,
 * and components in memory.
 *
 * \note
 * The \p sources vector is not cleared before the newly read sources are
 * added to the array, thus you can read multiple files one after another.
 *
 * \param[in] file  The file to read from.
 * \param[in,out] sources  The resulting list of sources.
 */
void wpkgar_repository::read_sources(const memfile::memory_file& file, source_vector_t& sources)
{
    memfile::memory_file sources_file;
    file.copy(sources_file);
    if(sources_file.is_compressed())
    {
        file.decompress(sources_file);
    }

    int offset(0);
    for(;;)
    {
        source src;

        // read the next line, if empty or comment, silently skip
        std::string line;
        do
        {
            if(!sources_file.read_line(offset, line))
            {
                // we're all done here
                return;
            }
            // remove comments
            std::string::size_type p(line.find_first_of('#'));
            if(p != std::string::npos)
            {
                line = line.substr(0, p);
            }
            // trim
            // find_if_not() is C++11
            line.erase(0, std::find_if(line.begin(), line.end(), not_isspace) - line.begin());
            p = std::find_if(line.rbegin(), line.rend(), not_isspace) - line.rbegin();
            if(p > 0)
            {
                line.erase(line.size() - p, p);
            }
        }
        while(line.empty() || line[0] == '#');

        // parse the line
        // format is:
        //      1. deb type (i.e. "deb" or "deb-src")
        //      2. [ options... ] (optional, set of variable/value pairs defined in square brackets)
        //           Debian defines:   arch=arch1,arch2,... and trusted=yes|no
        //      3. URI (protocols we understand: file, http, copy, smb)
        //      4. distribution
        //      5. components (optional, multiple entries possible)
        const char *l(line.c_str());

        // 1. Type
        const char *start(l);
        for(; !isspace(*l); ++l)
        {
            if(*l == '\0')
            {
                throw wpkgar_exception_invalid("a line in a sources.list file cannot only include a type (" + line + ")");
            }
        }
        std::string type(start, l - start);
        if(type != "deb" && type != "deb-src"
        && type != "wpkg" && type != "wpkg-src")
        {
            throw wpkgar_exception_invalid("unknown sources.list type \"" + type + "\"");
        }
        src.set_type(type);
        for(; isspace(*l); ++l);

        // 2. Options
        if(l[0] == '[')
        {
            // skip spaces
            for(;;)
            {
                for(++l; isspace(*l); ++l);
                if(*l == ']')
                {
                    ++l;
                    break;
                }
                start = l;
                const char *value(NULL);
                for(; !isspace(*l) && *l != ']'; ++l)
                {
                    if(*l == '\0')
                    {
                        throw wpkgar_exception_invalid("invalid option definitions in sources.list \"" + line + "\"");
                    }
                    if(*l == '=' && value == NULL)
                    {
                        value = l + 1;
                    }
                }
                if(value == NULL)
                {
                    // accept empty values (TBD?)
                    std::string n(start, l - start);
                    src.add_parameter(n, "");
                }
                else
                {
                    std::string n(start, value - 1 - start);
                    std::string v(value, l - value);
                    src.add_parameter(n, v);
                }
            }
            for(; isspace(*l); ++l);
        }

        // 3. URI
        if(*l == '\0')
        {
            throw wpkgar_exception_invalid("URI missing in sources.list \"" + line + "\"");
        }
        start = l;
        for(; !isspace(*l) && *l != '\0'; ++l);
        std::string uri(start, l - start);
        src.set_uri(uri);
        for(; isspace(*l); ++l);

        // 4. Distribution
        if(*l == '\0')
        {
            throw wpkgar_exception_invalid("distribution missing in sources.list \"" + line + "\"");
        }
        start = l;
        for(; !isspace(*l) && *l != '\0'; ++l);
        std::string distribution(start, l - start);
        src.set_distribution(distribution);
        for(; isspace(*l); ++l);
        if(*distribution.rbegin() == '/' && *l != '\0')
        {
            throw wpkgar_exception_invalid("distribution ends with / and yet the line includes components in sources.list \"" + line + "\"");
        }

        // 5. Components
        while(l[0] != '\0')
        {
            start = l;
            for(; !isspace(*l) && *l != '\0'; ++l);
            std::string component(start, l - start);
            src.add_component(component);
            for(; isspace(*l); ++l);
        }

        sources.push_back(src);
    }
}


/** \brief Write a source_vector_t to a sources.list file.
 *
 * This function writes a vector of sources to a file that can then be
 * saved in a sources.list file.
 *
 * \param[in] file  The file to read from.
 * \param[out] sources  The resulting list of sources.
 */
void wpkgar_repository::write_sources(memfile::memory_file& file, const source_vector_t& sources)
{
    file.printf("# Auto-generated sources.list file\n");
    for(wpkgar::wpkgar_repository::source_vector_t::const_iterator it(sources.begin()); it != sources.end(); ++it)
    {
        file.printf("%s", it->get_type().c_str());
        wpkgar::wpkgar_repository::source::parameter_map_t params(it->get_parameters());
        if(!params.empty())
        {
            file.printf(" [ ");
            for(wpkgar::wpkgar_repository::source::parameter_map_t::const_iterator p(params.begin()); p != params.end(); ++p)
            {
                file.printf("%s=%s ", p->first.c_str(), p->second.c_str());
            }
            file.printf("]");
        }
        file.printf(" %s %s", it->get_uri().c_str(), it->get_distribution().c_str());

        int cnt(it->get_component_size());
        for(int j(0); j < cnt; ++j)
        {
            file.printf(" %s", it->get_component(j).c_str());
        }

        file.printf("\n");
    }
}

/** \brief Update the package indexes.
 *
 * This function reads the sources.list file defined in the core directory
 * of the specified target and then checks each source for a new repository
 * index file. These files are saved in the indexes sub-directory found
 * inside the core directory.
 *
 * The process maintains a file with information about each one of the
 * index. This is important since all the indexes have the exact same
 * filename in each repository.
 *
 * \param[in] manager  The manager defining the target to update.
 */
void wpkgar_repository::update()
{
    load_index_list();
    wpkg_filename::uri_filename name(f_manager->get_database_path());
    name = name.append_child("core/sources.list");
    wpkgar::wpkgar_repository::source_vector_t sources;
    memfile::memory_file sources_file;
    sources_file.read_file(name);
    read_sources(sources_file, sources);
    size_t max(sources.size());
    for(size_t i(0); i < max; ++i)
    {
        // at this time we only understand wpkg types
        if(sources[i].get_type() == "wpkg")
        {
            wpkg_filename::uri_filename uri(sources[i].get_uri());
            std::string distribution(sources[i].get_distribution());
            uri = uri.append_child(distribution);
            int count(sources[i].get_component_size());
            if(count == 0)
            {
                // if count is zero then distribution is the direct path
                update_index(uri);
            }
            else
            {
                for(int j(0); j < count; ++j)
                {
                    std::string component(sources[i].get_component(j));
                    wpkg_filename::uri_filename full_uri(uri);
                    full_uri = full_uri.append_child(component);
                    update_index(full_uri);
                }
            }
        }
    }
    save_index_list();
}

void wpkgar_repository::update_index(const wpkg_filename::uri_filename& uri)
{
    const wpkg_filename::uri_filename index_filename(uri.append_child("index.tar.gz"));
    memfile::memory_file index_file;
    time_t now(time(NULL));
    update_entry_t::update_entry_status_t status(update_entry_t::status_unknown);
    try
    {
        index_file.read_file(index_filename);
        status = update_entry_t::status_ok;

        wpkg_output::log("successfully updated index file from repository: %1.")
                .quoted_arg(index_filename)
            .module(wpkg_output::module_repository)
            .action("repository-update");
    }
    catch(const std::runtime_error&)
    {
        status = update_entry_t::status_failed;

        wpkg_output::log("failed updating index file from repository: %1.")
                .quoted_arg(index_filename)
            .level(wpkg_output::level_warning)
            .module(wpkg_output::module_repository)
            .action("repository-update");
    }

    long max_index(0);
    size_t i(0), max(f_update_index.size());
    int index(0);
    for(; i < max; ++i)
    {
        if(f_update_index[i].get_uri() == uri.full_path())
        {
            // found the entry
            f_update_index[i].set_status(status);
            f_update_index[i].update_time(now);
            index = f_update_index[i].get_index();
            break;
        }
        if(f_update_index[i].get_index() > max_index)
        {
            max_index = f_update_index[i].get_index();
        }
    }
    if(i == max)
    {
        // no extra entry and we did not find this one, add it
        update_entry_t entry;
        index = max_index + 1;
        entry.set_index(index);
        entry.set_status(status);
        entry.set_uri(uri.full_path());
        entry.update_time(now);
        f_update_index.push_back(entry);
    }
    if(status == update_entry_t::status_ok)
    {
        // on success save the file we just loaded
        wpkg_filename::uri_filename name(f_manager->get_database_path());
        std::stringstream s;
        s << index;
        name = name.append_child("core/indexes/update-" + s.str() + ".index.gz");
        index_file.write_file(name, true);
    }
}

const wpkgar_repository::update_entry_vector_t *wpkgar_repository::load_index_list()
{
    f_update_index.clear();
    wpkg_filename::uri_filename name(f_manager->get_database_path());
    name = name.append_child("core/update.index");
    if(name.exists())
    {
        memfile::memory_file update_file;
        update_file.read_file(name);
        int offset(0);
        std::string line;
        for(;;)
        {
            if(!update_file.read_line(offset, line))
            {
                return &f_update_index;
            }
            update_entry_t update_entry;
            update_entry.from_string(line);
            f_update_index.push_back(update_entry);
        }
    }
    return NULL;
}

void wpkgar_repository::save_index_list() const
{
    wpkg_filename::uri_filename name(f_manager->get_database_path());
    name = name.append_child("core/update.index");
    memfile::memory_file update_file;
    update_file.create(memfile::memory_file::file_format_other);
    for(update_entry_vector_t::const_iterator it(f_update_index.begin()); it != f_update_index.end(); ++it)
    {
        update_file.printf("%s\n", it->to_string().c_str());
    }
    update_file.write_file(name);
}


/** \brief Compute a list of packages that should be upgraded.
 *
 * This function runs through the list of packages defined in the
 * index files read by the --update command and creates a list of
 * packages with a main status as follow:
 *
 * \li not_installed -- are not installed on the target
 * \li need_upgrade -- are installed and need to be upgraded
 * \li blocked_upgrade -- are installed and need to be upgraded, but are on hold
 * \li installed -- are installed and do not need to be upgraded
 * \li invalid -- are not valid for the target (wrong architecture, older version)
 *
 * There is a secondary status defined in the update_index_t object
 * of the package item. If the update index is in a failed status
 * then it is to be expected that loading packages from that repository
 * will fail.
 *
 * The output is a vector with all the packages listed and their
 * current status. The packages Control file is available so one
 * can check fields if necessary. The package name (As defined in
 * the Package field), version, and architecture are always available.
 *
 * The function computes this list once. On the second and further calls
 * the exact same list is returned even if you changed things in the
 * system. If you need to get a new list, create a new wpkgar_repository
 * object.
 *
 * \return A constant reference to a wpkgar_package_list_t vector.
 */
const wpkgar_repository::wpkgar_package_list_t& wpkgar_repository::upgrade_list()
{
    // any reason for us to do any of this?
    if(f_packages.empty() && load_index_list() != NULL && !f_update_index.empty())
    {
        // some initialization
        f_manager->load_package("core");
        f_manager->list_installed_packages(f_installed_packages);

        size_t max(f_update_index.size());
        for(size_t i(0); i < max; ++i)
        {
            // make sure we had at least one successful read
            // otherwise there is no index for that entry
            if(f_update_index[i].get_time(update_entry_t::last_success) != 0)
            {
                wpkg_filename::uri_filename name(f_manager->get_database_path());
                std::stringstream s;
                s << f_update_index[i].get_index();
                name = name.append_child("core/indexes/update-" + s.str() + ".index.gz");
                memfile::memory_file index_file;
                index_file.read_file(name);
                if(index_file.is_compressed())
                {
                    memfile::memory_file d;
                    index_file.copy(d);
                    d.decompress(index_file);
                }

                // we have an index, go ahead and upgrade
                upgrade_index(i, index_file);
            }
        }
    }

    return f_packages;
}

void wpkgar_repository::upgrade_index(size_t i, memfile::memory_file& index_file)
{
    index_file.dir_rewind();
    for(;;)
    {
        memfile::memory_file::file_info info;
        memfile::memory_file data;
        if(!index_file.dir_next(info, &data))
        {
            break;
        }

        // define the source .deb filename as the info URI
        wpkg_filename::uri_filename uri(f_update_index[i].get_uri());
        std::string filename(info.get_filename());
        std::string::size_type p(filename.find_last_of('.'));
        if(p != std::string::npos && filename.substr(p) == ".ctrl")
        {
            filename = filename.substr(0, p) + ".deb";
        }
        uri = uri.append_child(filename);
        info.set_uri(uri);

        package_item_t item(f_manager, info, data);
        item.check_installed_package(is_installed_package(item.get_name()));
        f_packages.push_back(item);
    }
}

bool wpkgar_repository::is_installed_package(const std::string& name) const
{
    size_t max(f_installed_packages.size());
    for(size_t i(0); i < max; ++i)
    {
        if(f_installed_packages[i] == name)
        {
            return true;
        }
    }

    return false;
}

} // namespace wpkgar
// vim: ts=4 sw=4 et
