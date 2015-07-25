/*    wpkg_control_fields.cpp -- implementation of the control file handling
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
 * \brief Implementation of the different fields supported in control files.
 *
 * The control files support so many fields that their definitions were
 * extracted from the main wpkg_control.cpp file to this file.
 *
 * The file also includes the help information of each field as shown in
 * your console when using:
 *
 * \code
 * wpkg --help field <field name>
 * \endcode
 */
#include    "libdebpackages/wpkg_control.h"
#include    "libdebpackages/wpkg_architecture.h"
#include    "libdebpackages/wpkg_util.h"
#include    "libdebpackages/debian_version.h"

#include    "libutf8/libutf8.h"

namespace wpkg_control
{




namespace
{

/** \typedef field_factory_map_t
 * \brief Map of fields.
 *
 * This defines a map used to register all the fields.
 * Fields get registered at startup when C++ global objects are created.
 */

/** \brief The map of fields.
 *
 * This pointer is used to register all the control fields. It is a pointer
 * because we must make sure that it exists when the first fields gets
 * registered.
 *
 * Note that the map is never getting released.
 */
control_file::field_factory_map_t *g_field_factory_map;

} // no name namespace





/** \brief Retrieve a pointer to the list of fields.
 *
 * This function returns a pointer to the list of fields defined in the
 * control file. This is generally used for documentation although in
 * a GUI environment it can be used to define drop downs to select a
 * field.
 *
 * The pointer is constant and the contents of the map cannot be modified.
 * To add your own field, define a field as a global with an equivalent to
 * the CONTROL_FILE_FIELD_FACTORY_CLASS() macro.
 *
 * \return The field factory map pointer.
 */
const control_file::field_factory_map_t *control_file::field_factory_map()
{
    return g_field_factory_map;
}


namespace
{

template<class T> const control_file::list_of_terms_t *find_typed_term(const control_file::list_of_terms_t *list, const T& term)
{
    for(; list->f_term != NULL; ++list)
    {
        std::string terms(list->f_term);
        for(;;)
        {
            std::string::size_type p(terms.find_first_of(','));
            if(p == std::string::npos)
            {
                break;
            }
            if(term == T(terms.substr(0, p)))
            {
                return list;
            }
            // note: we assume that no one writes multiple commas
            terms = terms.substr(p + 1);
        }
        if(term == T(terms))
        {
            return list;
        }
    }
    return NULL;
}

} // no name namespace


/** \brief Verify that a term is defined in a list.
 *
 * This function searches for the \p term parameter in the \p list of
 * terms for validity. The search can be done case sensitively or
 * insensitively as may be required.
 *
 * \param[in] list  The list of valid terms.
 * \param[in] term  The term to search in the list.
 * \param[in] case_insensitive  Whether to test the term as is or case
 *                              insensitively.
 *
 * \return The term found in the list of terms, or NULL if not found.
 */
const control_file::list_of_terms_t *control_file::find_term(const control_file::list_of_terms_t *list, const std::string& term, const bool case_insensitive)
{
    if(case_insensitive)
    {
        const case_insensitive::case_insensitive_string t(term);
        return find_typed_term(list, t);
    }
    return find_typed_term(list, term);
}


/** \brief Set the version.
 *
 * This function saves the specified standards version in this object.
 * If the version is invalid then the function throws an error and
 * the current version number is reset to 0.0.0.0.
 *
 * If the function succeeds, then the values are available for retrieval
 * using the get_version() function. To know whether a standards_version
 * object was defined and is valid, use the is_defined() function.
 *
 * \param[in] version  The version in text format.
 */
void standards_version::set_version(const std::string& version)
{
    f_defined = false;

    // reset the version in case it fails we get just what's correct
    // and not some old version numbers (although we have a flag too
    // it is still safer this way)
    for(int i(static_cast<int>(standards_major_version)); i < static_cast<int>(standards_version_max); ++i)
    {
        f_version[i] = 0;
    }

    if(!parse_version(version))
    {
        throw wpkg_control_exception_invalid("invalid standards version");
    }

    f_defined = true;
}


/** \brief Parse a standards version string.
 *
 * This function transforms a version string in 4 components (4 numbers).
 * Note that the last component is optional and set to zero if undefined.
 *
 * \param[in] version  The version string to be parsed.
 *
 * \return true if the standards version is valid.
 */
bool standards_version::parse_version(const std::string& version)
{
    const char *s(version.c_str());

    for(; isspace(*s); ++s);
    for(int i(static_cast<int>(standards_major_version)); i < static_cast<int>(standards_version_max); ++i)
    {
        if(*s < '0' || *s > '9')
        {
            return false;
        }
        uint32_t v(*s - '0');
        for(++s; *s >= '0' && *s <= '9'; ++s)
        {
            v = v * 10 + *s - '0';
            if(v > 1000000000)
            {
                // a version over 1 billion?!
                return false;
            }
        }
        f_version[i] = v;
        if(i == 3)
        {
            break;
        }
        if(*s != '.')
        {
            if(i == 2)
            {
                break;
            }
            return false;
        }
        ++s;
    }
    for(; isspace(*s); ++s);

    return *s == '\0';
}


/** \brief Check whether the standards version was defined.
 *
 * This function returns true if the version was defined and is valid.
 *
 * \return true if the version components can be queried with get_version().
 */
bool standards_version::is_defined() const
{
    return f_defined;
}


/** \brief Retrieve a component of the standards version.
 *
 * The standards version is composed of 4 components. This function can be used
 * to retrieve any one of those components. The available components are:
 *
 * \li standards_major_version
 * \li standards_minor_version
 * \li standards_major_patch_level
 * \li standards_minor_patch_level
 *
 * \return The value of one of those components.
 */
uint32_t standards_version::get_version(standards_version_number_t n) const
{
    if(n < standards_major_version || n >= standards_version_max)
    {
        throw std::overflow_error("standards_version_... invalid (out of range for version array)");
    }
    return f_version[n];
}










/** \brief Create a field as per its name.
 *
 * This function creates a field depending on its name.
 *
 * If the wpkg_control object does not have a specialized field type
 * to handle this case, then it calls the default field_factory()
 * function of the field_file class which creates a default field_t
 * object. This just means the value will not be checked for validity.
 *
 * \param[in] name  The name of the field to create.
 * \param[in] value  The raw value of the field to create.
 *
 * \return A pointer to the newly created field.
 */
std::shared_ptr<wpkg_field::field_file::field_t> control_file::field_factory(const case_insensitive::case_insensitive_string& fullname, const std::string& value) const
{
    std::string name(fullname);
    std::string::size_type p(name.find_first_of('/'));
    if(p != std::string::npos)
    {
        name = name.substr(0, p);
    }
    field_factory_map_t::const_iterator factory(g_field_factory_map->find(name));
    if(factory != g_field_factory_map->end())
    {
        return factory->second->create(*this, fullname, value);
    }

    return field_file::field_factory(fullname, value);
}









/** \brief Register a field to the control field factory.
 *
 * This register function is used by the fields to allow the control
 * file to handle fields in a fairly automated manner.
 *
 * \param[in] field_factory  The field factory object to register.
 */
void wpkg_control::control_file::control_field_factory_t::register_field(control_file::control_field_factory_t *field_factory)
{
    if(g_field_factory_map == NULL)
    {
        g_field_factory_map = new field_factory_map_t;
    }
    (*g_field_factory_map)[field_factory->name()] = field_factory;
}


/** \brief Initialize a control field.
 *
 * All control fields are derived from the control_field_t class
 * so as to gain access to a set of validation functions that are
 * easy to access.
 *
 * \param[in] file  The field file linked with this field.
 * \param[in] name  The name of the field being created.
 * \param[in] value  The starting raw value of the field being created.
 */
control_file::control_field_t::control_field_t(const field_file& file, const std::string& name, const std::string& value)
    : field_t(file, name, value)
{
}


/** \brief Verify the date of a field.
 *
 * The Date and Changes-Date field must have a valid date as per Debian
 * expected date format.
 */
void control_file::control_field_t::verify_date() const
{
    std::string date(get_transformed_value());
    struct tm time_info;
    if(strptime(date.c_str(), "%a, %d %b %Y %H:%M:%S %z", &time_info) == NULL)
    {
        wpkg_output::log("control:%1:%2: date %3 in field %4 is invalid")
                .arg(get_filename())
                .arg(get_line())
                .quoted_arg(date)
                .arg(get_name())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }
}


/** \brief Verify that the value is a list of dependencies.
 *
 * This function parses the value to make sure that it is a valid
 * list of dependencies.
 */
void control_file::control_field_t::verify_dependencies() const
{
    std::string value(get_transformed_value());

    // verify the list of dependencies
    try
    {
        wpkg_dependencies::dependencies d(value);
    }
    catch(const wpkg_control_exception_invalid& e)
    {
        wpkg_output::log("control:%1:%2: invalid dependencies in %3 -- %4")
                .arg(get_filename())
                .arg(get_line())
                .quoted_arg(value)
                .arg(e.what())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }
}


/** \brief Verify that the value is a list of emails.
 *
 * This function checks the value which must look like a valid list
 * of email addresses as defined by RFC5322.
 */
void control_file::control_field_t::verify_emails() const
{
    // TODO: move the libtld code to test emails in wpkg
    // note that this test must really understands a list of emails
}


/** \brief Verify that the value is a list of files.
 *
 * This function gets the value and parses it as if it were a list of
 * files. The format is defined by the field name or the first line
 * of the field.
 */
void control_file::control_field_t::verify_file() const
{
    file_list_t files(get_name());
    std::string value(get_transformed_value());
    files.set(value);
}


/** \brief Check whether the field name includes a sub-package name.
 *
 * Some fields do not support a sub-package specification. For example
 * there should be only one place where your project users are to enter
 * bugs that they find. This means the Bugs field should never have a
 * sub-package specification because it would not make sense.
 *
 * This function is used by the different fields which must be unique
 * across all the sub-packages that you are creating.
 */
void control_file::control_field_t::verify_no_sub_package_name() const
{
    if(has_sub_package_name())
    {
        wpkg_output::log("control:%1:%2: field %3 cannot include a sub-package name.")
                .arg(get_filename())
                .arg(get_line())
                .arg(get_name())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }
}


/** \brief Verify that the value is a URI.
 *
 * This function checks the value which must look like a valid HTTP or
 * HTTPS URI. If not, an error is thrown.
 */
void control_file::control_field_t::verify_uri() const
{
    // should this be limited to the first line in value?
    std::string value(get_transformed_value());
    if(!wpkg_util::is_valid_uri(value, "http,https"))
    {
        wpkg_output::log("control:%1:%2: invalid value for URI field %3 %4 (expected http[s]://domain.tld/path...)")
                .arg(get_filename())
                .arg(get_line())
                .arg(get_name())
                .quoted_arg(value)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }
}


/** \brief Verify that the value represents a valid version.
 *
 * This function checks that the transformed value does represent a valid
 * Debian version.
 */
void control_file::control_field_t::verify_version() const
{
    std::string value(get_transformed_value());
    char err[256];
    if(!validate_debian_version(value.c_str(), err, sizeof(err)))
    {
        wpkg_output::log("control:%1:%2: %3 %4 is not a valid Debian version")
                .arg(get_filename())
                .arg(get_line())
                .arg(get_name())
                .quoted_arg(value)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }
}


/** \brief Initialize the Dependency field.
 *
 * This constructor ensures that the field is given a name.
 *
 * \param[in] file  The field_file this field is from.
 * \param[in] name  The name of the field.
 * \param[in] value  The raw value of the field.
 */
control_file::dependency_field_t::dependency_field_t(const field_file& file, const std::string& name, const std::string& value)
    : control_field_t(file, name, value)
{
}


/** \brief Verify the field value.
 *
 * This function verifies the value of the field. In most cases it is
 * called after the set_value() function saved the new value.
 */
void control_file::dependency_field_t::verify_value() const
{
    verify_dependencies();
}



// Architecture
CONTROL_FILE_FIELD_FACTORY(architecture, "Architecture",
    "The Architecture field is used to define the architecture on "
    "which this package is expected to be installed. Packages that "
    "can be installed on any architecture use \"all\". The "
    "architecture is defined as <operating system>-<processor>."
    "If you want to, the architecture can also include a vendor "
    "string as in: <operating system>-<vendor>-<processor>. This can "
    "be used to make sure a target does not include packages from "
    "an unwanted source."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(architecture, control)

void control_file::field_architecture_t::verify_value() const
{
    // verify that we are NOT reading this data for contents perusal
    // because if it is just to see the contents the verification
    // must be avoided
    std::shared_ptr<control_file_state_t> state(std::dynamic_pointer_cast<control_file_state_t>(f_field_file->get_state()));
    if(state && !state->reading_contents())
    {
        wpkg_architecture::architecture arch(get_transformed_value());
    }
}


// Breaks
CONTROL_FILE_FIELD_FACTORY(breaks, "Breaks",
    "The Breaks field includes a list of dependencies. This includes "
    "other package names, optional versions, and architectures. "
    "The packages listed in the Breaks field cannot be installed "
    "at the same time as this package. It may, however, be unpacked. "
    "Unpackaged means that the files from the package are available, "
    "whereas installed means the files are available and the package "
    "is configured. In other words, you can often install two servers "
    "offering the same capability, but only one can run at once."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(breaks, dependency)


// Bugs
CONTROL_FILE_FIELD_FACTORY(bugs, "Bugs",
    "The Bugs field is a URI to a website where users of the package "
    "can enter information about bugs that they encounter with the "
    "package."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(bugs, control)

void control_file::field_bugs_t::verify_value() const
{
    verify_no_sub_package_name();
    verify_uri();
}


// Build-Conflicts
CONTROL_FILE_FIELD_FACTORY(buildconflicts, "Build-Conflicts",
    "The Build-Conflicts field defines a list of packages, including their "
    "version and optionally architectures, which cannot be installed for "
    "this package to get built."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(buildconflicts, dependency)


// Build-Conflicts-Arch
CONTROL_FILE_FIELD_FACTORY(buildconflictsarch, "Build-Conflicts-Arch",
    "The Build-Conflicts-Arch field defines a list of packages, including their "
    "version and optionally architectures, which cannot be installed for "
    "this package architecture specific packages to get built."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(buildconflictsarch, dependency)


// Build-Conflicts-Indep
CONTROL_FILE_FIELD_FACTORY(buildconflictsindep, "Build-Conflicts-Indep",
    "The Build-Conflicts-Indep field defines a list of packages, including their "
    "version and optionally architectures, which cannot be installed for "
    "this package architecture independent packages to get built."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(buildconflictsindep, dependency)


// Build-Depends
CONTROL_FILE_FIELD_FACTORY(builddepends, "Build-Depends",
    "The Build-Depends field defines a list of packages, including their "
    "version and optionally architectures, which must be installed for "
    "this package to get built."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(builddepends, dependency)


// Build-Depends-Arch
CONTROL_FILE_FIELD_FACTORY(builddependsarch, "Build-Depends-Arch",
    "The Build-Depends-Arch field defines a list of packages, including their "
    "version and optionally architectures, which must be installed for "
    "this package architecture specific packages to get built."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(builddependsarch, dependency)


// Build-Depends-Indep
CONTROL_FILE_FIELD_FACTORY(builddependsindep, "Build-Depends-Indep",
    "The Build-Depends-Indep field defines a list of packages, including their "
    "version and optionally architectures, which must be installed for "
    "this package architecture independent packages to get built."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(builddependsindep, dependency)


// Build-Number
CONTROL_FILE_FIELD_FACTORY(buildnumber, "Build-Number",
    "The Build-Number field is a decimal number that represents the number "
    "of official builds done of this project. You may use the different "
    "build number functions of the library and wpkg to manage this number "
    "in a fairly automated manner. Actually, if you create an empty file "
    "named wpkg/build_number, the build number will automatically be "
    "increased when you run a --build command to generate the source "
    "package of your project."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(buildnumber, control)

void control_file::field_buildnumber_t::verify_value() const
{
    const std::string& value(get_transformed_value());

    for(const char *s(value.c_str()); *s != '\0'; ++s)
    {
        if(*s < '0' || *s > '9')
        {
            wpkg_output::log("control:%1:%2: %3 is not a valid Build-Number, only one positive decimal number is valid in this field")
                    .arg(get_filename())
                    .arg(get_line())
                    .quoted_arg(value)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_control)
                .package(f_field_file->get_package_name())
                .action("control");
        }
    }
}



// Built-Using
CONTROL_FILE_FIELD_FACTORY(builtusing, "Built-Using",
    "The Built-Using field defines a list of sources packages, including "
    "their version and optionally architectures, which are used to build "
    "this project. Without those other source files, the project would not "
    "build properly. The naming convention used is different because the "
    "dependencies in this case are source packages instead of binary ones."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(builtusing, dependency)


// Changed-By
CONTROL_FILE_FIELD_FACTORY(changedby, "Changed-By",
    "The Changed-By field is the list of package maintainers. There should "
    "always be at least one name if the field is defined."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(changedby, control)

void control_file::field_changedby_t::verify_value() const
{
    verify_emails();
}


// Changes
CONTROL_FILE_FIELD_FACTORY(changes, "Changes",
    "The Changes field is a copy of the log entries found in your "
    "wpkg/changelog file. It includes changes from only for this very "
    "version."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(changes, control)

void control_file::field_changes_t::verify_value() const
{
}


// Changes-Date
CONTROL_FILE_FIELD_FACTORY(changesdate, "Changes-Date",
    "The Changes-Date field represents the date when the maintainer started "
    "work on the project. This is the date found in the footer of each "
    "change log version entry."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(changesdate, control)

void control_file::field_changesdate_t::verify_value() const
{
    verify_date();
}


// Checksums-Sha1
CONTROL_FILE_FIELD_FACTORY(checksumssha1, "Checksums-Sha1",
    "The Checksums-Sha1 field is a list of filenames with their SHA-1 "
    "checksums. The SHA-1 checksums format is expected to be: "
    "\"checksum size filename\" the filename may include a path."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(checksumssha1, control)

void control_file::field_checksumssha1_t::verify_value() const
{
    verify_file();
}


// Checksums-Sha256
CONTROL_FILE_FIELD_FACTORY(checksumssha256, "Checksums-Sha256",
    "The Checksums-Sha256 field is a list of filenames with their SHA-256 "
    "checksums. The SHA-256 checksums format is expected to be: "
    "\"checksum size filename\" the filename may include a path."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(checksumssha256, control)

void control_file::field_checksumssha256_t::verify_value() const
{
    verify_file();
}


// Component
CONTROL_FILE_FIELD_FACTORY(component, "Component",
    "The Component field defines the area/section/sub-section or "
    "'categorization' of the project. The sub-section part is optional. "
    "The area/section part, along with the Distribution field, are used by "
    "the --build process to save binary packages in the right repository "
    "location. In case of a source package, the path is forced to \"source\" "
    "instead of the value of the Distribution field (which could be multiple "
    "paths in case of a source package.)"
)
CONTROL_FILE_FIELD_CONSTRUCTOR(component, control)

void control_file::field_component_t::verify_value() const
{
    verify_no_sub_package_name();

    const std::string& value(get_transformed_value());

    // only one line allowed
    for(const char *d(value.c_str()); *d != '\0'; ++d)
    {
        if(*d < '!')
        {
            wpkg_output::log("control:%1:%2: the Component field cannot be defined on multiple lines, include spaces, or other control characters")
                    .arg(get_filename())
                    .arg(get_line())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_control)
                .package(f_field_file->get_package_name())
                .action("control");
            break;
        }
    }

    // cannot be an absolute path
    wpkg_filename::uri_filename component(value);
    if(component.is_absolute())
    {
        wpkg_output::log("control:%1:%2: the Component path %3 cannot be absolute")
                .arg(get_filename())
                .arg(get_line())
                .arg(value)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }

    if(component.segment_size() < 2)
    {
        wpkg_output::log("control:%1:%2: the Component path %3 must include at least two segments; only the sub-section is optional")
                .arg(get_filename())
                .arg(get_line())
                .arg(value)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }
}


// Conf-Files
CONTROL_FILE_FIELD_FACTORY(conffiles, "Conffiles",
    "The Conffiles field is a list of filenames with their MD5 "
    "checksums. The Conffiles format is expected to be: "
    "\"checksum size filename\" the filename may include a path. "
    "All these file names represent configuration files that may "
    "get edited by an administrator. This list can be used instead "
    "of the conffiles file."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(conffiles, control)

void control_file::field_conffiles_t::verify_value() const
{
    verify_file();
}


// Conflicts
CONTROL_FILE_FIELD_FACTORY(conflicts, "Conflicts",
    "The Conflicts field is the list of packages that cannot be installed "
    "along this package. The list can include version specifications as well "
    "as architectures. When a package in conflict is installed or even just "
    "unpacked this package cannot be installed."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(conflicts, dependency)


// Date
CONTROL_FILE_FIELD_FACTORY(date, "Date",
    "The Date field represents the date when the package was built. "
    "In most cases you want that date to be automatically generated by "
    "by the package at the time it creates your packages."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(date, control)

void control_file::field_date_t::verify_value() const
{
    verify_date();
}


// Depends
CONTROL_FILE_FIELD_FACTORY(depends, "Depends",
    "The Depends field is the list of packages that must be installed "
    "before this package can itself be installed. The list of dependencies "
    "can include version and architecture specifications. When a Depends "
    "package of this package is not already installed and is not specified "
    "on the command line then this package cannot be installed."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(depends, dependency)


// Description
CONTROL_FILE_FIELD_FACTORY(description, "Description",
    "The Description field explains what the package is about. It is "
    "composed of a small description (up to 70 characters on the first line) "
    "and an optional long description (after the first new-line character.)"
)
CONTROL_FILE_FIELD_CONSTRUCTOR(description, control)

void control_file::field_description_t::verify_value() const
{
    std::string first_line(f_field_file->get_field_first_line(get_name()));

    // Compute the length in UTF-8 since it is "character" not "bytes"
    // that we are interested in for this one case
    size_t len(libutf8::mbslen(first_line));
    if(len > 70)
    {
        wpkg_output::log("control:%1:%2: the first line of a package %3 is limited to 70 characters, it is %4 at this time")
                .arg(get_filename())
                .arg(get_line())
                .arg(get_name())
                .arg(len)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }

    std::string value(get_transformed_value());
    for(const char *d(value.c_str()); *d != '\0'; ++d)
    {
        if(*d == '\t' || *d == '\v')
        {
            wpkg_output::log("control:%1:%2: the %3 field does not support \\t and \\v characters")
                    .arg(get_filename())
                    .arg(get_line())
                    .arg(get_name())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_control)
                .package(f_field_file->get_package_name())
                .action("control");
        }
    }
}


// Distribution
CONTROL_FILE_FIELD_FACTORY(distribution, "Distribution",
    "The Distribution field is a relative path to where binary package files "
    "are saved within your repository. It is used for all binary packages "
    "and defaults to stable. The Distribution field is also expected to be "
    "defined in source packages in which case it actually support multiple "
    "distribution names defining all the paths for all the distributions for "
    "which the package should be compiled. Note that in regard to your "
    "repository, source packages are forcibly placed under the directory "
    "named \"sources\", although Component field is used for sources too."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(distribution, control)

void control_file::field_distribution_t::verify_value() const
{
    const std::string& value(get_transformed_value());

    // note we support new lines in this field, although not Debian
    const char *d(NULL);
    for(const char *start(value.c_str()); *start != '\0'; start = d)
    {
        for(d = start; *d != '\0' && !isspace(*d) && *d != '\n'; ++d);
        // verify that it is a valid filename (really it is a path)
        wpkg_filename::uri_filename distribution(std::string(start, d - start));
        if(distribution.empty())
        {
            wpkg_output::log("control:%1:%2: the Distribution field cannot be empty; do not define it if you want to use the default")
                    .arg(get_filename())
                    .arg(get_line())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_control)
                .package(f_field_file->get_package_name())
                .action("control");
            break;
        }
        // skip spaces to the next distribution
        for(; isspace(*d) || *d == '\n'; ++d);
    }
}


// DM-Upload-Allowed
CONTROL_FILE_FIELD_FACTORY(dmuploadallowed, "DM-Upload-Allowed",
    "The DM-Upload-Allowed field is not used by wpkg. It is there because "
    "it is defined in Debian, but it does not look like a sensible field."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(dmuploadallowed, control)

void control_file::field_dmuploadallowed_t::verify_value() const
{
    const case_insensitive::case_insensitive_string boolean(get_transformed_value());
    if(boolean != "yes")
    {
        wpkg_output::log("control:%1:%2: invalid value for a the DM-Upload-Allowed field (expected yes)")
                .arg(get_filename())
                .arg(get_line())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }
}


// Enhances
CONTROL_FILE_FIELD_FACTORY(enhances, "Enhances",
    "The Enhances field defines a list of binary packages, including "
    "their version and optionally architectures, which may optionally be "
    "installed in order to enhance the functionality of this package."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(enhances, dependency)


// Essential
CONTROL_FILE_FIELD_FACTORY(essential, "Essential",
    "The Essential field can be set to Yes or No. If No, the default, the "
    "package can be installed and removed as is. If Yes, then the package "
    "is considered essential for your operating system target and it cannot "
    "be removed as easily."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(essential, control)

void control_file::field_essential_t::verify_value() const
{
    const case_insensitive::case_insensitive_string essential(get_transformed_value());
    if(essential != "yes"
    && essential != "no")
    {
        wpkg_output::log("control:%1:%2: invalid value for boolean field %3 (expected yes or no)")
                .arg(get_filename())
                .arg(get_line())
                .arg(essential)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }
}


// Files
CONTROL_FILE_FIELD_FACTORY(files, "Files",
    "The Files field is a list of filenames. The format is simply a list "
    "of filenames: \"filename\"; the filename may include a path."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(files, control)

void control_file::field_files_t::verify_value() const
{
    verify_file();
}


// Homepage
CONTROL_FILE_FIELD_FACTORY(homepage, "Homepage",
    "The Homepage field is a URI to the home page of the project."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(homepage, control)

void control_file::field_homepage_t::verify_value() const
{
    verify_uri();
}


// Install-Prefix
CONTROL_FILE_FIELD_FACTORY(installprefix, "Install-Prefix",
    "The Install-Prefix field is defined when creating a package from its "
    "source package. This is the value of the --install-prefix option. "
    "Note that the build process overwrites this value when creating "
    "a package from a source package."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(installprefix, control)

void control_file::field_installprefix_t::verify_value() const
{
    verify_no_sub_package_name();
    // verify that it is a valid path
    wpkg_filename::uri_filename filename;
    filename.set_filename(get_transformed_value());
}


// Maintainer
CONTROL_FILE_FIELD_FACTORY(maintainer, "Maintainer",
    "The Maintainer field is a list of names and email addresses as defined "
    "in RFC5322 (Internet Message Header, or email header.) Although the "
    "name of the field is not plural, multiple emails can be indicated. "
    "Note that this is the name of the project maintainer. For the package "
    "maintainer, use the Changed-By field instead."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(maintainer, control)

void control_file::field_maintainer_t::verify_value() const
{
    verify_no_sub_package_name();
    verify_emails();
}


// Minimum-Upgradable-Version
CONTROL_FILE_FIELD_FACTORY(minimumupgradableversion, "Minimum-Upgradable-Version",
    "The Minimum-Upgradable-Version field is a Debian version defining the "
    "smallest version of the package that this version can upgrade. "
    "There are times when a package upgrade path becomes very complicated "
    "and continually supporting all the version from the very first one "
    "can become particularly tedious. This field is used to break the "
    "upgrade pass at a given version. For example, when you jump to version "
    "2.0 of your project, you may only want to support upgrades from the "
    "latest version of the 1.x branch. Say you are at 1.54 and the last 3 "
    "versions did not add any new upgrade processes, then 2.0 may use version "
    "1.51 as the breaking point and use:\n"
    "   Minimum-Upgradable-Version: 1.51\n"
    "The result is that the administrator of a target system that has version "
    "1.50 or older will be forced to first upgrade to 1.51, 1.52, 1.53, or "
    "1.54 before he can jump to 2.0. The administrator could also choose to "
    "remove or purge the 1.x version before upgrading to 2.0."
)
// we have a specialized constructor...
//CONTROL_FILE_FIELD_CONSTRUCTOR(version, control)

/** \brief Initialize the Minimum-Upgradable-Version field.
 *
 * This constructor ensures that the version makes use of colons only
 * as we support versions with semi-colons as well.
 *
 * \param[in] file  The field_file this field is from.
 * \param[in] name  The name of the field.
 * \param[in] value  The raw value of the field.
 */
control_file::field_minimumupgradableversion_t::field_minimumupgradableversion_t(const field_file& file, const std::string& name, const std::string& value)
    : control_field_t(file, name, wpkg_util::canonicalize_version(value))
{
}


void control_file::field_minimumupgradableversion_t::set_value(const std::string& value)
{
    // make sure that the value has ':' only, no ';'
    control_field_t::set_value(wpkg_util::canonicalize_version(value));
}


void control_file::field_minimumupgradableversion_t::verify_value() const
{
    verify_no_sub_package_name();
    verify_version();
}


// Origin
CONTROL_FILE_FIELD_FACTORY(origin, "Origin",
    "The Origin field defines the name of the original project. This field "
    "is not limited like the Package field and the sole purpose is "
    "documentation."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(origin, control)

void control_file::field_origin_t::verify_value() const
{
    verify_no_sub_package_name();
}


// Package
CONTROL_FILE_FIELD_FACTORY(package, "Package",
    "The Package field is the name of this package. It is mandatory. "
    "This field is also used for source packages because the field named "
    "Source is misused in many cases. To avoid confusion, we make use of "
    "Package everywhere instead."
)
// we have a specialized constructor...
//CONTROL_FILE_FIELD_CONSTRUCTOR(package, control)

/** \brief Initialize the Package field.
 *
 * This constructor ensures that the field is given a name.
 *
 * Also, a Package field is not allowed to include a variable reference
 * or an expression. If such is found, an error is emitted.
 *
 * \note
 * The restriction on the Package field is purely for security (or maybe
 * even sanity) reasons and not technical reasons. Debian, on the other
 * hand, has technical reasons for limiting their substitutions that we
 * do not have.
 *
 * \param[in] file  The field_file this field is from.
 * \param[in] name  The name of the field.
 * \param[in] value  The raw value of the field.
 */
control_file::field_package_t::field_package_t(const field_file& file, const std::string& name, const std::string& value)
    : control_field_t(file, name, value)
{
    // since the '$' is forbidden in a package name, testing for it is enough
    std::string::size_type p(name.find_first_of('$'));
    if(p != std::string::npos)
    {
        throw wpkg_control_exception_invalid("the Package field cannot include a variable reference or an expression (" + value + ")");
    }

    const_cast<field_file&>(file).set_package_name(value);
}


/** \brief Verify the field value.
 *
 * This function sets the value of the field after verifying that it is
 * valid for this field.
 */
void control_file::field_package_t::verify_value() const
{
    std::string value(get_transformed_value());
    if(!wpkg_util::is_package_name(value))
    {
        throw wpkg_control_exception_invalid("invalid name \"" + value + "\" for the Package field");
    }

    f_field_file->set_package_name(value);
}


// Packager-Version
CONTROL_FILE_FIELD_FACTORY(packagerversion, "Packager-Version",
    "The Packager-Version field is the version of the packager used to "
    "created this package. It is expected to be a valid Debian version. "
    "You do not define this field. The packager saves it as it builds "
    "packages."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(packagerversion, control)

void control_file::field_packagerversion_t::verify_value() const
{
    verify_version();
}


// Pre-Depends
CONTROL_FILE_FIELD_FACTORY(predepends, "Pre-Depends",
    "The Pre-Depends field defines a list of sources packages, including "
    "their version and optionally architectures, which must be installed "
    "before the installation of this package can be started. In most cases "
    "this is used when a newly installed package needs to access a "
    "configured package in one of its installation scripts."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(predepends, dependency)


// Priority
CONTROL_FILE_FIELD_FACTORY(priority, "Priority",
    "The Priority field is a string defining how soon the package should be "
    "upgraded on a target system. Only one of the following accepted "
    "priorities can be indicated for this field: required, important, "
    "standard, optional, extra."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(priority, control)

namespace
{

/** \brief List of priority terms.
 *
 * This array lists all the possible priority terms. Other values are not
 * considered valid as far as the Priority field is concerned.
 */
const control_file::list_of_terms_t priority_terms[] =
{
    {
        "required",
        "This package is required. This means the --remove command line "
        "option does not work against this package. Although you will still "
        "be able to upgrade the package when new versions are published."
    },
    {
        "important",
        "This package is considered important. It probably should be kept "
        "installed at all time."
    },
    {
        "standard",
        "This package is a standard package. Install and remove as you see "
        "fit for your target system."
    },
    {
        "optional",
        "This package is optional, meaning that it generally does not get "
        "installed by default. If you want it, install it explicitly."
    },
    {
        "extra",
        "This package includes extras for another package. For example "
        "a package that comes with sample data or a very large documentation "
        "may place such packages in the extra bin."
    },
    { NULL, NULL }
};

}

/** \brief Return the complete list of possible priority terms.
 *
 * This function returns the list of valid priority terms. This can be
 * useful to present to a user to choose from.
 *
 * \return The NULL terminated list of priority terms.
 */
const control_file::list_of_terms_t *control_file::field_priority_t::list()
{
    return priority_terms;
}


/** \brief Check whether a string represents a valid priority.
 *
 * This function verifies that a string does indeed represent a valid
 * priority. If so then it can be used on the Priority field.
 *
 * \param[in] priority  A string that is expected to represent a priority.
 *
 * \return true if the field does represent a priority.
 */
bool control_file::field_priority_t::is_valid(const std::string& priority)
{
    return find_term(priority_terms, priority) != NULL;
}

void control_file::field_priority_t::verify_value() const
{
    const std::string priority(get_transformed_value());
    if(!is_valid(priority))
    {
        wpkg_output::log("control:%1:%2: %3 is not a valid priority")
                .arg(get_filename())
                .arg(get_line())
                .quoted_arg(priority)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }
}


// Provides
CONTROL_FILE_FIELD_FACTORY(provides, "Provides",
    "The Provides field is a list of comma separated aliases that must "
    "represent valid package names. These names can be used as dependency "
    "names and they will reference packages that have such names just as "
    "if their real name had been specified. This is used in circumstances "
    "such as a help tool that makes use of a browser; in that case which "
    "browser is not important so that package can use Depends: browser; "
    "and each browser can indicate that it provides that functionality "
    "by indicating Provides: browser."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(provides, control)

void control_file::field_provides_t::verify_value() const
{
    list_t l(f_field_file->get_field_list(get_name()));
    for(list_t::const_iterator provides(l.begin()); provides != l.end(); ++provides)
    {
        if(!wpkg_util::is_package_name(*provides))
        {
            wpkg_output::log("control:%1:%2: %3 is an invalid package name in Provides field, only letters (a-z), digits (0-9), dashes (-), pluses (+), and periods (.) are accepted")
                    .arg(get_filename())
                    .arg(get_line())
                    .quoted_arg(*provides)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_control)
                .package(f_field_file->get_package_name())
                .action("control");
        }
    }
}


// Recommends
CONTROL_FILE_FIELD_FACTORY(recommends, "Recommends",
    "The Recommends field defines a list of sources packages, including "
    "their version and optionally architectures, which are are recommended "
    "while using this package. Without those recommended packages, the "
    "functionality is greatly diminished. If no closely related, then use "
    "the Suggest field instead."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(recommends, dependency)


// Replaces
CONTROL_FILE_FIELD_FACTORY(replaces, "Replaces",
    "The Replaces field defines a list of sources packages, including "
    "their version and optionally architectures, which are being replaced "
    "when this package gets installed. In other words, installing this "
    "package requires the old one's files to be removed. However, you can "
    "still just upgrade (opposed to removing the old package being replaced "
    "and installing the new package.)"
)
CONTROL_FILE_FIELD_CONSTRUCTOR(replaces, dependency)


// Section
CONTROL_FILE_FIELD_FACTORY(section, "Section",
    "The Section field groups packages together. Section names are limited "
    "by the Debian manual. There are also three official main groups: "
    "main, contrib, and non-free. Main group names can be used before the "
    "group name and separated by the group name by a slash. For example: "
    "non-free/video. \"main\" is the default main group and as such it does "
    "not need to be specified, so main/base and base are considered the same."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(section, control)

namespace
{

/** \brief List of valid section names.
 *
 * This array lists all the possible section terms. Other values are not
 * considered valid as far as the Section field is concerned.
 *
 * The list of valid Debian sections is used to verify that the
 * Section field is set to a Debian approved section name.
 *
 * Note that we do not really approve those as they do not
 * really make sense to use (ocaml? hamradio? ...) but we
 * want to be as close to the standard as possible.
 *
 * If you want to define a different set of sections for your
 * own packages, we suggest that you look into creating your
 * own field instead.
 */
const control_file::list_of_terms_t section_terms[] =
{
    {
        "admin",
        "This section includes all administrative tools, scripts, etc."
    },
    {
        "base",
        "The base section include packages that represent the base system. "
        "In terms of a Unix system these are packages that let you "
        "run a terminal, and a (fairly small) set of shell "
        "commands that are necessary to run a configure script."
    },
    {
        "cli-mono",
        "Mono related packages."
    },
    {
        "comm",
        "Communication related packages such as ppp, modem, etc."
    },
    {
        "contrib",
        "Contribution packages."
    },
    {
        "database",
        "Database managers such as MySQL and PostgreSQL."
    },
    {
        "debian-installer",
        "The Debian (or wpkg) installation tools."
    },
    {
        "devel",
        "Packages in this section are for developers. Others really do not "
        "to install such packages."
    },
    {
        "debug",
        "All packages may have a debug version that can be installed to "
        "help the project author find where problems arise."
    },
    {
        "doc",
        "Packages documentation."
    },
    {
        "editors",
        "Packages used to edit files of all kinds."
    },
    {
        "education",
        "Educational packages."
    },
    {
        "electronics",
        "Packages that help you work with electronics."
    },
    {
        "embedded",
        "Packages helpful to create embedded software."
    },
    {
        "fonts",
        "Packages used to install fonts on your system."
    },
    {
        "games",
        "Packages representing games."
    },
    {
        "gnome",
        "Packages that run under Gnome."
    },
    {
        "graphics",
        "Packages that help you work on images, photos, 3D, videos."
    },
    {
        "gnu-r",
        "GNU R language related packages."
    },
    {
        "gnustep",
        "GNU Step related packages."
    },
    {
        "hamradio",
        "Ham radio (modem) related packages."
    },
    {
        "haskell",
        "Haskell related packages."
    },
    {
        "httpd",
        "Web server packages such as Apache and lighthttp."
    },
    {
        "interpreters",
        "Packages of languages that interprets scripts."
    },
    {
        "introspection",
        "No idea..."
    },
    {
        "java",
        "Packages written in Java."
    },
    {
        "kde",
        "Packages written for the KDE, and the KDE itself."
    },
    {
        "kernel",
        "Kernel related packages."
    },
    {
        "libs",
        "Packages representing libraries such as libdebpackages."
    },
    {
        "libdevel",
        "Packages of development libraries (as in debug versions, etc.)"
    },
    {
        "lisp",
        "Packages related to the lisp language."
    },
    {
        "localization",
        "Packages related to translations and locales of all countries."
    },
    {
        "mail",
        "Packages that manage emails in a way or another."
    },
    {
        "math",
        "Packages related to math such as BLAS."
    },
    {
        "metapackages",
        "Meta packages (or virtual packages) are used to group a set of "
        "packages together so users can easily install very large sets "
        "of packages and get a working environment. For example, the X11 "
        "environment is very complex and comes with a very large (100's) "
        "number of packages to run properly under a Linux system. Under "
        "Ubuntu there is a metapackage that allows you to install everything "
        "with one command line and in the end it works. Metapackages are "
        "not directly related to one project."
    },
    {
        "misc",
        "Miscellaneous packages."
    },
    {
        "net",
        "Network related packages."
    },
    {
        "news",
        "News related packages (Gopher, RSS and other news feeds and systems.)"
    },
    {
        "non-free",
        "Packages that come from a private party that do not release the "
        "source code with a truly free license."
    },
    {
        "ocaml",
        "Packages related to ocaml"
    },
    {
        "oldlibs",
        "Packages of libraries that are still packaged but should not be "
        "used in new projects."
    },
    {
        "otherosfs",
        "Other open source file systems..."
    },
    {
        "perl",
        "Perl related packages."
    },
    {
        "php",
        "PHP related packages"
    },
    {
        "python",
        "Python related packages."
    },
    {
        "ruby",
        "Ruby related packages."
    },
    {
        "science",
        "Packages that offer science related tools."
    },
    {
        "shells",
        "Packages offering shells like sh, tcsh, bash, etc."
    },
    {
        "sound",
        "Packages that allow you to work and play audio tracks."
    },
    {
        "tex",
        "TeX related packages"
    },
    {
        "text",
        "Text related packages (curses)"
    },
    {
        "utils",
        "Utilities"
    },
    {
        "vcs",
        "Source control related packages, like CVS, svn, etc."
    },
    {
        "video",
        "Video related packages such as camera feeds and video editors."
    },
    {
        "web",
        "Web related packages such as browsers."
    },
    {
        "x11",
        "X11 related packages, generally the X11 core packages and samples."
    },
    {
        "xfce",
        "XFCE Desktop Environment related packages."
    },
    {
        "zope",
        "Zope related packages (web applications written in python)."
    },
    { NULL, NULL }
};

}

/** \brief Return the complete list of possible section terms.
 *
 * This function returns the list of valid section terms. This can be
 * useful to present to a user to choose from.
 *
 * \return The NULL terminated list of section terms.
 */
const control_file::list_of_terms_t *control_file::field_section_t::list()
{
    return section_terms;
}

/** \brief Check whether a string represents a valid section.
 *
 * This function parses a line of content representing a value from the
 * Section field and validates the value against the list of acceptable
 * section names.
 *
 * When the function returns true, it also returns a valid name in
 * the \p section parameter and a valid prefix in the \p area parameter.
 * The prefix is set to "main" (the default) if no area was defined in the
 * section parameter.
 *
 * \param[in] value  The value to be parsed as a section.
 * \param[out] section  The section name (after the slash.)
 * \param[out] area  The area name (before the slash.)
 *
 * \return true if the section name is considered valid, false otherwise.
 */
bool control_file::field_section_t::is_valid(const std::string& value, std::string& section, std::string& area)
{
    // extract the section and area parameters
    const char *s(value.c_str());
    for(; !isspace(*s) && *s != '/' && *s != '\0'; ++s)
    {
        // areas and sections must be in lowercase
        if(*s >= 'A' && *s <= 'Z')
        {
            return false;
        }
        section += *s;
    }

    // extract the actual section after the slash
    // if there is one
    if(*s == '/')
    {
        // if there is a slash then the first value is the area,
        // not the section, so copy that!
        area = section;
        section = s + 1;
    }
    else
    {
        // no area, set to default value
        area = "main";
    }

    // verify the section now
    return find_term(section_terms, section, false) != NULL;
}


/** \brief Verify the field value.
 *
 * This function verifies the value of the field. In most cases it is
 * called after the set_value() function saved the new value.
 *
 * \note
 * Section names are defined in Debian in that paragraph:
 * http://www.debian.org/doc/debian-policy/ch-archive.html#s-subsections
 *
 * \note
 * The Debian definition of the section field does not define whether the
 * names are case insensitive. At this point we accept only lower case
 * names as it seems to be the way Debian functions.
 *
 * \todo
 * The section name may include an area specification that we do not support
 * yet. Areas in Debian are defined as "main", "contrib" and "non-free". The
 * "main" area is the default and it does not need to be used.
 *
 * \code
 * Section: section
 * or
 * Section: area/section
 * \endcode
 */
void control_file::field_section_t::verify_value() const
{
    verify_no_sub_package_name();

    std::string section;
    std::string area;
    std::string value(get_transformed_value());
    if(!is_valid(value, section, area))
    {
        wpkg_output::log("control:%1:%2: invalid %3 name %4, it is not recognized as a Debian section")
                .arg(get_filename())
                .arg(get_line())
                .arg(get_name())
                .quoted_arg(value)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }
}


// Source
CONTROL_FILE_FIELD_FACTORY(source, "Source",
    "The Source field is accepted in existing binary packages but forbidden "
    "otherwise. Some people use this field improperly hence our idea of not "
    "using it at all. In most cases it can be replaced by the Package field "
    "or by the Origin field."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(source, control)

void control_file::field_source_t::verify_value() const
{
    // verify that we are NOT reading this data for contents perusal
    // because if it is just to see the contents the verification
    // must be avoided
    std::shared_ptr<control_file_state_t> state(std::dynamic_pointer_cast<control_file_state_t>(f_field_file->get_state()));
    if(state && state->prevent_source())
    {
        wpkg_output::log("control:%1:%2: a control file cannot include a Source field; either use Package or Origin as may be necessary")
                .arg(get_filename())
                .arg(get_line())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }
}


// Standards-Version
CONTROL_FILE_FIELD_FACTORY(standardsversion, "Standards-Version",
    "The Standards-Version field represents the standards used to create "
    "a package. This information can be used to check that everything "
    "matches one to one to what the standards say it should be. For example "
    "we could have a rule that says that if the Location field is defined "
    "it must be a Longitude and a Latitude and if the values do not "
    "correspond to such values, then generate an error. Contrary to the "
    "Debian behavior, we keep this field in binary packages as well."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(standardsversion, control)

/** \brief Verify the field value.
 *
 * This function sets the value of the field after verifying that it is
 * valid for this field.
 */
void control_file::field_standardsversion_t::verify_value() const
{
    const case_insensitive::case_insensitive_string version(get_transformed_value());
    try
    {
        standards_version sv;
        sv.set_version(version);

        if(sv.get_version(standards_version::standards_major_version) < 2)
        {
            wpkg_output::log("control:%1:%2: %3 is invalid, the Standards-Version field expects a major version of at least 2")
                    .arg(get_filename())
                    .arg(get_line())
                    .quoted_arg(version)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_control)
                .package(f_field_file->get_package_name())
                .action("control");
        }
        else
        {
            dynamic_cast<control_file *>( f_field_file.operator primary_type_t() )->f_standards_version = sv;
        }
    }
    catch(const wpkg_control_exception_invalid& e)
    {
        wpkg_output::log("control:%1:%2: %3: %4")
                .arg(get_filename())
                .arg(get_line())
                .quoted_arg(version)
                .arg(e.what())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }
}


// Sub-Packages
CONTROL_FILE_FIELD_FACTORY(subpackages, "Sub-Packages",
    "The Sub-Packages field is a list of sub-packages that can be created "
    "from one .info file. This field never appears in a binary package. "
    "It is used by the build process of the library to ease the creation "
    "of control files by having just one with sub-package specifications."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(subpackages, control)

void control_file::field_subpackages_t::verify_value() const
{
    verify_no_sub_package_name();

    bool hide(false);
    std::map<std::string, bool> found;
    list_t l(f_field_file->get_field_list(get_name()));
    for(list_t::const_iterator package(l.begin()); package != l.end(); ++package)
    {
        std::string sub_name(*package);
        if(*sub_name.rbegin() == '*')
        {
            if(hide)
            {
                wpkg_output::log("control:%1:%2: %3 is the second name ending with an asterisk, there can be at most one such name in a Sub-Packages field")
                        .arg(get_filename())
                        .arg(get_line())
                        .quoted_arg(*package)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_control)
                    .package(f_field_file->get_package_name())
                    .action("control");
            }
            sub_name.assign(sub_name.begin(), sub_name.end() - 1);
            hide = true;
        }
        if(sub_name.empty() || !wpkg_util::is_package_name(sub_name))
        {
            wpkg_output::log("control:%1:%2: %3 is an invalid sub-package name in Sub-Packages field, only letters (a-z), digits (0-9), dashes (-), pluses (+), and periods (.) are accepted in the name, and one asterisk at the end of one of the names")
                    .arg(get_filename())
                    .arg(get_line())
                    .quoted_arg(*package)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_control)
                .package(f_field_file->get_package_name())
                .action("control");
        }
        if(found.find(sub_name) != found.end())
        {
            wpkg_output::log("control:%1:%2: %3 is defined twice in Sub-Packages field")
                    .arg(get_filename())
                    .arg(get_line())
                    .quoted_arg(*package)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_control)
                .package(f_field_file->get_package_name())
                .action("control");
        }
        else
        {
            found[sub_name] = true;
        }
    }
}


// Suggests
CONTROL_FILE_FIELD_FACTORY(suggests, "Suggests",
    "The Suggests field defines a list of sources packages, including "
    "their version and optionally architectures, which are used to offer "
    "the users installing this package suggestions on other packages that "
    "may be useful along this package. For example, when installing the "
    "main package of your project you may suggest installing the "
    "documentation package."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(suggests, dependency)


// Uploaders
CONTROL_FILE_FIELD_FACTORY(uploaders, "Uploaders",
    "The Uploaders field is a list of names and email addresses defined "
    "as per RFC5322 (Internet Message, or email) of the users who helped "
    "in getting this package ready for download."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(uploaders, control)

void control_file::field_uploaders_t::verify_value() const
{
    verify_emails();
}


// Urgency
CONTROL_FILE_FIELD_FACTORY(urgency, "Urgency",
    "The Urgency field defines how urgent it is to upgrade your current "
    "version of the package to the new version. Only the following terms "
    "are valid: low, medium, high, emergency, and critical. Case is not "
    "important. \"emergency\" and \"critical\" should be used with care."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(urgency, control)

namespace
{

/** \brief List of Urgency terms.
 *
 * This array lists all the possible urgency terms. Other values are not
 * considered valid as far as the Urgency field is concerned.
 */
const control_file::list_of_terms_t urgency_terms[] =
{
    {
        "low",
        "Upgrading is not necessary unless you like to live on the edge. "
        "Note also that a package is not supposed to depend on another that "
        "has an urgency set to low."
    },
    {
        "medium",
        "upgrade at your leisure"
    },
    {
        "high",
        "you should upgrade quickly as the current version has flows"
    },
    {
        "emergency",
        "the project had really bad code and it needs to be upgraded as soon as possible"
    },
    {
        "critical",
        "the project had security issues and needs to be upgraded now"
    },
    { NULL, NULL }
};

}


/** \brief Return the complete list of possible urgency terms.
 *
 * This function returns the list of valid urgency terms. This can be
 * useful to present to a user to choose from.
 *
 * \return The NULL terminated list of urgency terms.
 */
const control_file::list_of_terms_t *control_file::field_urgency_t::list()
{
    return urgency_terms;
}


/** \brief Check whether a string represents a valid urgency.
 *
 * This function verifies that a string does indeed represent a valid
 * urgency. If so then it can be used on the Urgency field.
 *
 * \note
 * The urgency is often followed by a human readable sentence. This
 * function only checks the first word.
 *
 * \param[in] value  The value to check out
 * \param[in] urgency  The actual urgency
 * \param[in] urgency  A string that is expected to represent an urgency.
 *
 * \return true if the field does represent an urgency.
 */
bool control_file::field_urgency_t::is_valid(const std::string& value, std::string& urgency, std::string& comment)
{
    // extract the urgency term
    urgency = "";
    const char *u(value.c_str());
    for(; !isspace(*u) && *u != ';' && *u != '\0'; ++u)
    {
        if(*u >= 'A' && *u <= 'Z')
        {
            urgency += *u | 0x20;
        }
        else
        {
            urgency += *u;
        }
    }

    // extract the human readable comment
    for(; isspace(*u) || *u == ';'; ++u);
    comment = u;

    // verify the urgency term
    return find_term(urgency_terms, urgency, false) != NULL;
}


void control_file::field_urgency_t::verify_value() const
{
    // note that the urgency field must start with a valid urgency word but
    // it can continue with a human readable sentence, a reference, etc.
    const case_insensitive::case_insensitive_string value(get_transformed_value());
    std::string urgency, comment;
    if(!is_valid(value, urgency, comment))
    {
        wpkg_output::log("control:%1:%2: %3 is not a valid urgency")
                .arg(get_filename())
                .arg(get_line())
                .quoted_arg(value)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(f_field_file->get_package_name())
            .action("control");
    }
}


// Vcs-Browser
CONTROL_FILE_FIELD_FACTORY(vcsbrowser, "Vcs-Browser",
    "The Vcs-Browser field is a URI to a VCS (source control) that can be "
    "accessed via a browser (standard HTTP protocol)."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(vcsbrowser, control)

void control_file::field_vcsbrowser_t::verify_value() const
{
    verify_uri();
}


// Version
CONTROL_FILE_FIELD_FACTORY(version, "Version",
    "The Version field represents the current Debian version of this "
    "package. Debian knows of several types of versions, in many cases "
    "the Debian version is the same as the source project version. However "
    "there are times when the scheme of the project version is not "
    "compatible with the Debian version scheme in which case this Version "
    "field is the Debian version and the project version is called the "
    "up-stream version. The format of a version is: "
    "\"epoch:major.minor.release-revision\" where only the major version "
    "is required."
)
// we have a specialized constructor...
//CONTROL_FILE_FIELD_CONSTRUCTOR(version, control)

/** \brief Initialize the Version field.
 *
 * This constructor ensures that the version makes use of colons only
 * as we support versions with semi-colons as well.
 *
 * \param[in] file  The field_file this field is from.
 * \param[in] name  The name of the field.
 * \param[in] value  The raw value of the field.
 */
control_file::field_version_t::field_version_t(const field_file& file, const std::string& name, const std::string& value)
    : control_field_t(file, name, wpkg_util::canonicalize_version(value))
{
}


void control_file::field_version_t::set_value(const std::string& value)
{
    // make sure that the value has ':' only, no ';'
    control_field_t::set_value(wpkg_util::canonicalize_version(value));
}


void control_file::field_version_t::verify_value() const
{
    verify_no_sub_package_name();
    verify_version();
}


// X-PrimarySection
CONTROL_FILE_FIELD_FACTORY(xprimarysection, "X-PrimarySection",
    "The X-PrimarySection defines a term that groups software together "
    "for display in a tree like presentation in applications that present "
    "that information to end users. This is generally used in concert with "
    "the X-SecondarySection."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(xprimarysection, control)

void control_file::field_xprimarysection_t::verify_value() const
{
    verify_no_sub_package_name();
}


// X-SecondarySection
CONTROL_FILE_FIELD_FACTORY(xsecondarysection, "X-SecondarySection",
    "The X-SecondarySection defines a term that sub-groups software together "
    "for better display in a tree like form presenting that information to "
    "end users in graphical applications. This is used in concert with the "
    "X-PrimarySection. The X-PrimarySection must be used first in the final "
    "tree like presentation."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(xsecondarysection, control)

void control_file::field_xsecondarysection_t::verify_value() const
{
    verify_no_sub_package_name();
}


// X-Selection
CONTROL_FILE_FIELD_FACTORY(xselection, "X-Selection",
    "The X-Selection field represents the current selection of this "
    "package. The selection may be set to: normal, auto, hold, or "
    "reject. \"manual\" can also be used as a synonym to \"normal\"."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(xselection, control)

namespace
{

/** \brief List of selection terms.
 *
 * This array lists all the possible selection terms. Other values are not
 * considered valid as far as the Selection field is concerned.
 */
const control_file::list_of_terms_t selection_terms[] =
{
    {
        "normal,manual",
        "A \"normal\" package is a package that was installed explicitly, "
        "which means that it was specified on the command line of wpkg."
    },
    {
        "hold",
        "A package on hold cannot be changed (no --install and no --remove) "
        "until changed back to another selection."
    },
    {
        "auto",
        "A package that was installed automatically, or explicitly is marked "
        "with the auto selection."
    },
    {
        "reject",
        "To prevent the installation of a package, one can select it "
        "with the reject selection type."
    },
    { NULL, NULL }
};

}


/** \brief Return the complete list of possible selection terms.
 *
 * This function returns the list of valid selection terms. This can be
 * useful to present to a user to choose from.
 *
 * \return The NULL terminated list of selection terms.
 */
const control_file::list_of_terms_t *control_file::field_xselection_t::list()
{
    return selection_terms;
}


/** \brief Check whether \p selection is a valid value for the Selection field.
 *
 * This function checks whether the \p selection parameter is a valid value
 * for the Selection field. If so, then it returns a pointer to the
 *
 * \param[in] selection  The selection to check.
 */
bool control_file::field_xselection_t::is_valid(const std::string& selection)
{
    return find_term(selection_terms, selection) != NULL;
}

control_file::field_xselection_t::selection_t control_file::field_xselection_t::validate_selection(const std::string& selection)
{
    case_insensitive::case_insensitive_string s(selection);
    if(s == "normal" || s == "manual")
    {
        return selection_normal;
    }
    if(s == "hold")
    {
        return selection_hold;
    }
    if(s == "auto")
    {
        return selection_auto;
    }
    if(s == "reject")
    {
        return selection_reject;
    }
    return selection_unknown;
}


void control_file::field_xselection_t::verify_value() const
{
}


// X-Status
CONTROL_FILE_FIELD_FACTORY(xstatus, "X-Status",
    "The X-Status field represents the current status of a package in a "
    "target environment. This field is dynamically managed by the packager "
    "when you install or remove a package from a target system."
)
CONTROL_FILE_FIELD_CONSTRUCTOR(xstatus, control)

void control_file::field_xstatus_t::verify_value() const
{
}


}   // namespace wpkg_control

// vim: ts=4 sw=4 et
