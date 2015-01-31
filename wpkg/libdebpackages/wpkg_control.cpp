/*    wpkg_control.cpp -- implementation of the control file handling
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
 * \brief Manager of control files.
 *
 * This file includes the implementation of the control file manager.
 *
 * A control file is a specialized field object that understands package
 * information such as the Package field with the name of the package,
 * the Architecture field, the Description field, etc.
 *
 * The objects are capable to manage all the fields that are well defined
 * It also knows how to read control files and write fields to control
 * files (useful just to canonicalize your control files.)
 */
#include    "libdebpackages/wpkg_control.h"
#include    "libdebpackages/wpkg_architecture.h"
#include    "libdebpackages/wpkg_util.h"
#include    "libdebpackages/debian_version.h"
#include    "libdebpackages/debian_packages.h"

#include    "libutf8/libutf8.h"

#ifdef debpackages_EXPORTS
#define EXPR_DLL 1
#endif
#include    "libexpr/expr.h"

#include    <sstream>
#include    <algorithm>
#include    <time.h>


namespace wpkg_control
{


/** \class wpkg_control_exception
 * \brief The base exception of the control field implementation.
 *
 * The control base exception of all the control field exceptions.
 * Although this exception is not itself raised, all the other
 * control exceptions are derived from it so it can be caught
 * in order to catch any of the control exceptions.
 *
 * Note that the control file class is derived from the field_file
 * class so some exceptions will come from that base class instead.
 */


/** \class wpkg_control_exception_invalid
 * \brief An invalid value was detected while working on the control file.
 *
 * This exception is raised whenever an invalid value is detected while
 * working against the control files.
 */


/** \class control_file
 * \brief Handle a control file.
 *
 * This class is a derivation of the wpkg_field base class used to
 * handle control files as understood by Debian packages.
 */


/** \class binary_control_file
 * \brief The binary control file.
 *
 * This class handles a binary control file. This means a control file
 * read from a binary Debian package. Some fields are not checked as
 * harshly in such files because it is less important when handling
 * binary packages.
 */


/** \class status_control_file
 * \brief The status control file.
 *
 * This class handles status control files which do not have the same list
 * of mandatory fields as a regular control file.
 *
 * Also, because the system only expects tools such as wpkg to modify this
 * file via the library, the fields are not checked on read. They are
 * expected to be valid.
 */


/** \class info_control_file
 * \brief The control file for .info files.
 *
 * This class accepts .info file fields. This means sub-package name
 * specifications are allowed and also the transformation of variables.
 */


/** \class source_control_file
 * \brief The source control file.
 *
 * When reading a control file from a source project, this class is used
 * to handle the fields of that file.
 *
 * The source control file knows about simple control files and
 * control.info files. Both can be handled and in both cases all the
 * mandatory fields are not expected to be defined in the file. This
 * is because many of the fields are defined from the changelog
 * information. For example, the changelog defines the Version field
 * and therefore in a source control file the Version is not
 * mandatory. If not present it gets created from the changelog file.
 */


/** \class file_item
 * \brief A helper class used to parse and manage lists of files.
 *
 * The control file format accepts fields such as the Files field which
 * represent lists of files. We support some 5 different formats for
 * those lists. This class helps in parsing these fields and
 * transforming the content of the fields in lists of meta data
 * that can be used by other classes.
 */


/** \class file_list_t
 * \brief A list of file_item.
 *
 * This class is a vector of file_item. It is not just a typedef because
 * it includes a to_string() function used to transform the list of
 * files \em back in a field content (note: \em back is in italic because
 * it can also be used to transform lists created in memory to a new
 * field.)
 *
 * The name of the file_list_t class is the name of the field that was
 * parsed to obtain this information.
 */


/** \class standards_version
 * \brief The standards version parser.
 *
 * This class validates a standards version meaning the version of
 * the documentation used to create the list of control fields
 * and other parts of the package.
 *
 * At this point the version is verified and it has to be 3 or 4
 * numbers separated by periods such as 2.0.3 or 2.0.4.7. The version
 * number must be at least 2.0.0.0 to be considered valid. When the
 * forth number is not defined, it is considered to be zero.
 */


/** \class control_file::control_file_state_t
 * \brief The basic state trait for a control file.
 *
 * This is the basic trait used for control files. It defines the
 * allow transformation and the prevent source flags. Not that by
 * default control files do not allow transformation. Only the control
 * files read while building a new package are allowed to transform
 * variables.
 */


/** \class control_file::build_control_file_state_t
 * \brief Build state trait.
 *
 * This class defines the trait to define a build control. This allows
 * certain fields and forbids others.
 */


/** \class control_file::contents_control_file_state_t
 * \brief Content based state trait.
 *
 * This class defines the trait to define a contents control file. This is
 * generally not checking as much as the other traits. This is because
 * when reading a control file for its contents, it is too late to tell
 * the user that everything is wrong/invalid. Plus, you may want to check
 * the contents of an older control file which would become incompatible
 * if this trait was too strict.
 */


/** \class control_file::control_field_factory_t
 * \brief Factory used to create control file fields.
 *
 * This class supports a list of control fields that can be created while
 * loading a control file. All the fields that this factory supports are
 * those defined in the wpkg_control_field.cpp file.
 *
 * A field defined in the wpkg_control_field.cpp file is automatically
 * added to the list that this class maintains so it can create those
 * fields.
 *
 * If the input control file has a field that is not defined in the
 * wpkg_control_field.cpp file, then the default field constructor is
 * used and this means no checks are applied to the field.
 */


/** \class control_file::control_field_t
 * \brief Overload of the basic field class of the wpkg_field class.
 *
 * This class is used to create all the fields in a control file. The
 * constructor ensures that the field gets registered so that way it
 * can be created with the control_field_factory_t function.
 *
 * The class also has a set of functions that are useful to verify
 * fields that are of a generic type.
 */


/** \class control_file::dependency_field_t
 * \brief The base field type for all dependency fields.
 *
 * There are many dependency fields so we have a base specialization
 * for them. This class defines an additional verification function
 * named verify_value() which ensures that the field is a valid
 * list of dependencies.
 *
 * Since the class is derived from the control_file::control_field_t class,
 * it also has access to all the other verification functions.
 */


/** \struct control_file::list_of_terms_t
 * \brief Define a list of terms.
 *
 * This structure defines a term and its help. Lists of terms are arrays
 * of this structure that end with a null entry (i.e. the f_term and
 * f_help fields are set to NULL.)
 *
 * The terms are expected to be used in certain fields. Lists of terms
 * represent valid values for those fields. For example, the Property
 * field is limited to only very small set of words:
 *
 * \li required
 * \li important
 * \li standard
 * \li optional
 * \li extra
 *
 * This structure is used to valid such fields and the help is there
 * to allow tools such as wpkg to display the help information.
 */


/** \brief Initialize a control file.
 *
 * The constructor initializes the control file. The behavior of the read()
 * and set_field() functions will depend on the state object. For example,
 * one may want fields variables and expressions to be transformed and
 * other times certain fields, like the Architecture, to not be checked
 * for validity.
 *
 * Currently, the supported states by the control_file object are:
 *
 * \li control_file_state_t -- used to read binary files
 * \li build_control_file_state_t -- used to read control files to use by
 *     the --build command
 * \li contents_control_file_state_t -- used to read binary files when the
 *     only intend is to display information
 * \li status_control_file_state_t -- used to read Status files
 *
 * \param[in] state  The state to be used to alter the behavior of the
 *                   control file.
 */
control_file::control_file(const std::shared_ptr<control_file_state_t> state)
    : field_file(state)
    //, f_standards_version() -- auto-init
{
}


/** \brief Whether we're loading the control file for reading only.
 *
 * This function returns true if the file is being loaded for reading only.
 * In that circumstance certain fields should not be checked because it is
 * safe to have an invalid value (especially for functions such as --field
 * which can then show you what's wrong.) Also, importantly some fields
 * such as the Architecture may not support all operating systems and
 * processors and thus it would prevent reading such packages impossible.
 *
 * By default this function returns false meaning that fields will be
 * checked (although it very much depends on the value returned by the
 * allow_transformations() function.)
 *
 * \return true if we're loading just to peruse the contents.
 */
bool control_file::control_file_state_t::reading_contents() const
{
    return false;
}


/** \brief Whether the Source field should generate an error.
 *
 * When this function returns true, prevent the source field from
 * appearing in the control file. The Source field is the same thing
 * as the Package field in a source control file.
 *
 * \return false as we want to allow the Source field most everywhere
 *         except when building packages.
 */
bool control_file::control_file_state_t::prevent_source() const
{
    return false;
}


/** \brief Allow transformations when building packages.
 *
 * This state is used by the --build and --generate commands.
 *
 * This function returns true because the loading of a control file to
 * build a binary package is expected to make use of variables and
 * expressions.
 *
 * \return true to allow variable and expression transformations.
 */
bool control_file::build_control_file_state_t::allow_transformations() const
{
    return true;
}


/** \brief Whether the Source field should generate an error.
 *
 * When this function returns true, prevent the source field from
 * appearing in the control file. The Source field is the same thing
 * as the Package field in a source control file.
 *
 * \return true as control files loaded to build a binary package cannot include a Source field
 */
bool control_file::build_control_file_state_t::prevent_source() const
{
    return true;
}


/** \brief Allow invalid fields to be loaded.
 *
 * This function is defined to allow some fields to be loaded even
 * if their content is reported to be invalid.
 *
 * \return true to allow the loading of fields with invalid values.
 */
bool control_file::contents_control_file_state_t::reading_contents() const
{
    return true;
}


/** \brief Retrieve a specialized field as a list of dependencies.
 *
 * This function retrieves the contents of a field as a specialized field
 * representing a list of dependencies.
 *
 * The list will already have been checked so this function should never
 * raise an exception.
 *
 * \param[in] name  The name of the field to retrieve.
 *
 * \return The list of dependencies.
 */
wpkg_dependencies::dependencies control_file::get_dependencies(const std::string& name) const
{
    wpkg_dependencies::dependencies d(get_field(name));
    return d;
}


/** \brief Write a list of dependencies as it needs to appear in a binary package.
 *
 * The function writes out the dependencies from a dependency list that match a
 * specific architecture. This is important to avoid an invalid list.
 *
 * The function is applied against all the fields known to bear dependencies.
 * (i.e. Breaks, Conflicts, Depends, etc.)
 *
 * \todo
 * The user should be able to dynamically define fields that include a list
 * of dependencies otherwise that list will not be fixed in the output.
 */
void control_file::rewrite_dependencies()
{
    // here we make sure that the dependencies in the output match
    // the architecture of this package and we remove the [...] definitions
    std::string architecture(get_field(field_architecture_factory_t::canonicalized_name()));

    std::vector<const char *> fields;
    fields.reserve(20);
    fields.push_back(field_breaks_factory_t::canonicalized_name());
    fields.push_back(field_buildconflicts_factory_t::canonicalized_name());
    fields.push_back(field_buildconflictsarch_factory_t::canonicalized_name());
    fields.push_back(field_buildconflictsindep_factory_t::canonicalized_name());
    fields.push_back(field_builddepends_factory_t::canonicalized_name());
    fields.push_back(field_builddependsarch_factory_t::canonicalized_name());
    fields.push_back(field_builddependsindep_factory_t::canonicalized_name());
    fields.push_back(field_builtusing_factory_t::canonicalized_name());
    fields.push_back(field_conflicts_factory_t::canonicalized_name());
    fields.push_back(field_depends_factory_t::canonicalized_name());
    fields.push_back(field_enhances_factory_t::canonicalized_name());
    fields.push_back(field_predepends_factory_t::canonicalized_name());
    fields.push_back(field_recommends_factory_t::canonicalized_name());
    fields.push_back(field_replaces_factory_t::canonicalized_name());
    fields.push_back(field_suggests_factory_t::canonicalized_name());

    for(size_t i(0); i < fields.size(); ++i)
    {
        if(field_is_defined(fields[i]))
        {
            wpkg_dependencies::dependencies depends(get_dependencies(fields[i]));
            std::string canonicalized(depends.to_string(architecture));
            if(canonicalized.empty())
            {
                // this can happen
                delete_field(fields[i]);
            }
            else
            {
                set_field(fields[i], canonicalized);
            }
        }
    }
}


/** \brief Get a description from the specified field.
 *
 * This function retrieves the description, the name is expected to be
 * Description but does not really have to. The result is the first
 * line as return value and the other lines as the \p long_description
 * parameter. In the Description, the long_description may be empty,
 * but not the value returned.
 *
 * \param[in] name  The name of the field to read.
 * \param[out] long_description  The string where the long description is saved.
 *
 * \return The short description (first line of the field.)
 */
std::string control_file::get_description(const std::string& name, std::string& long_description) const
{
    std::string d(get_field(name));
    size_t p(d.find_first_of('\n'));
    if(p == std::string::npos)
    {
        long_description = "";
        return d;
    }
    // the long description does not include the first '\n'
    // and it may be empty (you only had spaces there)
    long_description = d.substr(p + 1);
    return d.substr(0, p);
}


/** \brief Get a copy of the standards version.
 *
 * Whenever a control file gets read, the Standards-Version field is
 * made available through this function.
 *
 * The Standards-Version field may not be defined. You should first
 * call the is_defined() function to make sure that what you are
 * getting is a valid standards version.
 *
 * \return The Standards-Version read from the control file.
 */
const standards_version& control_file::get_standards_version() const
{
    return f_standards_version;
}


/** \brief Create a control file for a binary file.
 *
 * This function creates a control that has to check out as a binary control
 * file. This means a file with the 5 mandatory fields: Architecture,
 * Description, Maintainer, Package, and Version.
 *
 * The state can be specified in this case because we allow loading packages
 * for installation (strong checks) and contents perusal (weak checks.)
 * Note that either way the 5 mandatory fields must exist. The weaker checks
 * are for the contents of the actual fields.
 */
binary_control_file::binary_control_file(const std::shared_ptr<control_file_state_t> state)
    : control_file(state)
{
}


/** \brief The default verify_file() of the control_file.
 *
 * This function is currently empty, but it is defined because the derived
 * classes need to call this function.
 */
void control_file::verify_file() const
{
}


/** \brief Verify the binary control file as a global entry.
 *
 * This function checks the control file as a whole after the file was read
 * in full. The main test is to verify that a set of fields are defined.
 *
 * In case of a binary control file we expect the following five fields:
 *
 * \li Architecture
 * \li Description
 * \li Maintainer
 * \li Package
 * \li Version
 */
void binary_control_file::verify_file() const
{
    // sub-package specifications are not allowed in binary packages
    if(has_sub_packages())
    {
        wpkg_output::log("control:%1:-: a binary control file cannot include fields using sub-package specifications")
                .arg(get_filename())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(get_package_name())
            .action("control");
    }

    if(!field_is_defined(field_architecture_factory_t::canonicalized_name())
    || !field_is_defined(field_description_factory_t::canonicalized_name())
    || !field_is_defined(field_maintainer_factory_t::canonicalized_name())
    || !field_is_defined(field_package_factory_t::canonicalized_name())
    || !field_is_defined(field_version_factory_t::canonicalized_name()))
    {
        wpkg_output::log("control:%1:-: one or more of the 5 required fields are missing (Package, Version, Architecture, Maintainer, Description)")
                .arg(get_filename())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(get_package_name())
            .action("control");
    }

    // just in case, run the base class verification function too
    control_file::verify_file();
}


/** \brief Create a status control file.
 *
 * This function creates a control file to read a status control file from
 * disk. The state is automatically set to contents which means a weaker
 * test on the field values. We actually do not really check the field
 * values as these are expected to be checked as needed. Plus it will
 * allow better inter-version support.
 *
 * Note that the file must have an X-Status field to be considered valid.
 */
status_control_file::status_control_file()
    : control_file(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::contents_control_file_state_t))
{
}


/** \brief Verify the status control file as a global entry.
 *
 * This function checks the control file as a whole after the file was read
 * in full. The main test is to verify that a set of fields are defined.
 *
 * In case of a status control file we expect the X-Status field to exist.
 */
void status_control_file::verify_file() const
{
    // sub-package specifications are not allowed in status files
    if(has_sub_packages())
    {
        wpkg_output::log("control:%1:-: a status control file cannot include fields using sub-package specifications")
                .arg(get_filename())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(get_package_name())
            .action("control");
    }

    // at this point, a status file is expected to have an X-Status field
    // nothing more
    if(!field_is_defined(field_xstatus_factory_t::canonicalized_name()))
    {
        wpkg_output::log("control:%1:-: a status file must have a %2 field")
                .arg(get_filename())
                .arg(field_xstatus_factory_t::canonicalized_name())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(get_package_name())
            .action("control");
    }

    // just in case, run the base class verification function too
    control_file::verify_file();
}


/** \brief Read a .info control file.
 *
 * This function is used to create a .info control file. These files are
 * automatically set to the build state meaning that they are always
 * strongly checked. This makes sense because when loading a .info control
 * file we are about to create a package and we want to check most everything
 * in this circumstance.
 */
info_control_file::info_control_file()
    : control_file(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::build_control_file_state_t))
{
}


/** \brief Verify the .info control file as a global entry.
 *
 * This function checks the control file as a whole after the file was read
 * in full. The main test is to verify that a set of fields are defined.
 *
 * In case of a .info control file we expect the usual 5 mandatory fields:
 * Architecture, Description, Maintainer, Package, and Version.
 */
void info_control_file::verify_file() const
{
    // The fact that we are here does not mean that the function
    // has_sub_packages() returns true; i.e. a .info file must have
    // a Sub-Packages field, but it does not require the use of
    // sub-package specifications in each .info file

    // an info file must have a Sub-Package field
    if(!field_is_defined(field_subpackages_factory_t::canonicalized_name()))
    {
        wpkg_output::log("control:%1:-: an info file must have a %2 field")
                .arg(get_filename())
                .arg(field_subpackages_factory_t::canonicalized_name())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_control)
            .package(get_package_name())
            .action("control");

        // those two fields cannot be used with a Sub-Package names
        if(!field_is_defined(field_maintainer_factory_t::canonicalized_name())
        || !field_is_defined(field_version_factory_t::canonicalized_name()))
        {
            wpkg_output::log("control:%1:-: a non-specialized required field is missing in your .info file (Maintainer or Version)")
                    .arg(get_filename())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_control)
                .package(get_package_name())
                .action("control");
        }
    }
    else
    {
        std::shared_ptr<field_t> sub_package_field(get_field_info(field_subpackages_factory_t::canonicalized_name()));
        list_t l(get_field_list(field_subpackages_factory_t::canonicalized_name()));
        for(list_t::const_iterator it(l.begin()); it != l.end(); ++it)
        {
            std::string sub_package_name(*it);
            if(!sub_package_name.empty() && *sub_package_name.rbegin() == '*')
            {
                sub_package_name.erase(sub_package_name.length() - 1);
            }
            if(sub_package_name.empty())
            {
                wpkg_output::log("control:%1:%2: a sub-package name cannot be empty or just \"*\"")
                        .arg(sub_package_field->get_filename())
                        .arg(sub_package_field->get_line())
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_control)
                    .package(get_package_name())
                    .action("control");
            }
            else if(!field_is_defined(field_package_factory_t::canonicalized_name() + ("/" + sub_package_name))
            || !field_is_defined(field_architecture_factory_t::canonicalized_name() + ("/" + sub_package_name))
            || !field_is_defined(field_description_factory_t::canonicalized_name() + ("/" + sub_package_name)))
            {
                wpkg_output::log("control:%1:%2: a required field is missing in your .info file (Package, Architecture, or Description)")
                        .arg(sub_package_field->get_filename())
                        .arg(sub_package_field->get_line())
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_control)
                    .package(get_package_name())
                    .action("control");
            }
        }

        // however those two fields cannot be used with a Sub-Package names
        if(!field_is_defined(field_maintainer_factory_t::canonicalized_name())
        || !field_is_defined(field_version_factory_t::canonicalized_name()))
        {
            wpkg_output::log("control:%1:%2: a non-specialized required field is missing in your .info file (Maintainer or Version)")
                    .arg(sub_package_field->get_filename())
                    .arg(sub_package_field->get_line())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_control)
                .package(get_package_name())
                .action("control");
        }
    }

    // just in case, run the base class verification function too
    control_file::verify_file();
}


/** \brief Create a source control file.
 *
 * Source control files are heavily checked for their contents. However,
 * contrary to a binary control file, the mandatory fields are not required
 * to exist in the control file because we can gather that information from
 * other files. For example, the ChangeLog defines the package name, version,
 * urgency, maintainer, version date, and change logs (obviously!)
 *
 * In most cases, the Architecture field is not defined. This is because we
 * expect that it will be possible to recompile the project on any
 * architecture. In this case, the control file is assigned a default
 * value:
 *
 * \code
 * Architecture: $(architecture())
 * \endcode
 *
 * If the project only compiles on one Architecture, then it should be
 * specified. If one of the component is the same on all architectures
 * (i.e. documentation) then the Architecture can be set to \b all:
 *
 * \code
 * Sub-Packages: runtime*, documentation
 * ...
 * Architecture/documentation: all
 * \endcode
 *
 * Note that under MS-Windows we generally do not place the files under
 * a /usr sub-directory so even the documentation would have to be
 * regenerated.
 */
source_control_file::source_control_file()
    : control_file(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::build_control_file_state_t))
{
}


/** \brief Verify the source control file as a global entry.
 *
 * This function checks the control file as a whole after the file was read
 * in full. The main test is to verify that a set of fields are defined.
 *
 * In case of a source control file we expect the Package field to exist.
 * Although the Package field could be taken from the ChangeLog, it is used
 * as a sanity check.
 *
 * We also expect the Description field to be defined. Most everything else
 * is optional in a source control file.
 */
void source_control_file::verify_file() const
{
    // a source file may be an info file (actually it is expected to be so)
    // in which case we want to check all the sub-fields
    if(field_is_defined(field_subpackages_factory_t::canonicalized_name()))
    {
        // .info case
        std::shared_ptr<field_t> sub_package_field(get_field_info(field_subpackages_factory_t::canonicalized_name()));
        list_t l(get_field_list(field_subpackages_factory_t::canonicalized_name()));
        for(list_t::const_iterator it(l.begin()); it != l.end(); ++it)
        {
            std::string sub_package_name(*it);
            if(!sub_package_name.empty() && *sub_package_name.rbegin() == '*')
            {
                sub_package_name.erase(sub_package_name.length() - 1);
            }
            if(!sub_package_name.empty()
            && (!field_is_defined(field_package_factory_t::canonicalized_name() + ("/" + sub_package_name))
             || !field_is_defined(field_description_factory_t::canonicalized_name() + ("/" + sub_package_name))))
            {
                wpkg_output::log("control:%1:%2: a required field is missing in your source control file (Package or Description)")
                        .arg(sub_package_field->get_filename())
                        .arg(sub_package_field->get_line())
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_control)
                    .package(get_package_name())
                    .action("control");
            }
        }
    }
    else
    {
        // standard control file (assume a runtime component only)
        if(!field_is_defined(field_package_factory_t::canonicalized_name())
        || !field_is_defined(field_description_factory_t::canonicalized_name()))
        {
            wpkg_output::log("control:%1:-: a required field is missing in your source control file (Package or Description)")
                    .arg(get_filename())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_control)
                .package(get_package_name())
                .action("control");
        }
    }

    // just in case, run the base class verification function too
    control_file::verify_file();
}




}   // namespace wpkg_control
// vim: ts=4 sw=4 et
