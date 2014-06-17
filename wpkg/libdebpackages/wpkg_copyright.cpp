/*    wpkg_copyright.cpp -- implementation of the copyright file handling
 *    Copyright (C) 2013  Made to Order Software Corporation
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
 * \brief Support for copyright files.
 *
 * When creating a package from source (using the wpkg --build command from
 * within your project root directory) then a copyright file is required.
 * This file gives details about all the copyright information of your
 * project. Primarily this is a list of licenses that cover each part of your
 * project as a whole, per directory, per file.
 *
 * The implementation makes use of the wpkg_field and understands the few
 * fields that the copyright file format expects.
 *
 * This implementation is based on this documentation although it is not
 * yet confirmed that it does indeed follow that documentation properly:
 *
 * http://www.debian.org/doc/packaging-manuals/copyright-format/1.0/
 */
#include    "libdebpackages/wpkg_copyright.h"
#include    "libdebpackages/wpkg_util.h"


/** \brief The namespace used by the copyright file format support.
 *
 * This namespace encompasses the functions used to read copyright files
 * as found in a source project.
 */
namespace wpkg_copyright
{


namespace
{

/** \brief The map of fields.
 *
 * This pointer is used to register all the control fields. It is a pointer
 * because we must make sure that it exists when the first fields gets
 * registered.
 *
 * Note that the map is never getting released.
 */
copyright_file::field_factory_map_t *g_field_factory_map;

} // no name namespace


/** \class copyright_file
 * \brief The copyright file class to handle project copyrights.
 *
 * This class is an implementation of the copyright file format support
 * from Debian. The basic file format is the same as the field format
 * (name colon value) although this format supports empty lines to
 * separate groups of definitions.
 *
 * The very first group represents the copyright information for
 * the project as a whole.
 *
 * The other groups represent specific copyright or license information.
 *
 * The copyright definition assign a license on a per directory
 * or even on a per file basis.
 *
 * The license definitions represents the text of each license.
 */


/** \class copyright_file::copyright_field_factory_t
 * \brief Create fields for the copyright_file format.
 *
 * This factory knows of the different fields supported by the copyright
 * file format. Whenever a copyright file is read, this factory is used
 * to create the fields. Note that other fields can be defined in which
 * case they get the default field item class and do not get verified
 * by this implementation.
 */


/** \class copyright_file::copyright_field_t
 * \brief The base class of the fields of a copyright file.
 *
 * The copyright file supports fields that are all derived from this class.
 *
 * The base class defines a couple of command verification functions.
 */


/** \class copyright_file::copyright_file_state_t
 * \brief The state of a copyright file format.
 *
 * This state is used to read copyright files. At this point this is the
 * only state supported by copyright files.
 */


/** \class copyright_info
 * \brief The basic class used to read a memory file in a copyright file.
 *
 * This class is used to read a copyright file and process all the
 * information. If no error occurs (i.e. the copyright file is valid)
 * then the read function will return.
 */


/** \class files_copyright_file
 * \brief A class used to read a copyright file.
 *
 * Because one copyright file may have multiple parts, we use an additional
 * class so we can handle restarting the read from where we left off. This
 * is the class we use for that purpose.
 */


/** \class header_copyright_file
 * \brief The class used to read the first entry (header) of a copyright file.
 *
 * This class is used to read the header of a copyright file. The header has
 * a different set of requirements than the other parts of a copyright file
 * so a special class was created for it.
 *
 * This class is used to start reading the copyright file. Then it is simply
 * ignored by the parser until the end of the file is reached.
 */


/** \brief Initialize a copyright file.
 *
 * The constructor initializes the copyright file.
 *
 * \param[in] state  The state to be used to alter the behavior of the
 *                   copyright file.
 */
copyright_file::copyright_file(const std::shared_ptr<field_file_state_t> state)
    : field_file(state)
{
}


/** \brief Retrieve the list of fields supported by the copyright file.
 *
 * This function returns the list of fields supported by the Copyright file.
 *
 * \return A map of fields supported by the copyright file format.
 */
const copyright_file::field_factory_map_t *copyright_file::field_factory_map()
{
    return g_field_factory_map;
}


/** \brief The default verify_file() of the control_file.
 *
 * This function is currently empty, but it is defined because the derived
 * classes need to call this function.
 */
void copyright_file::verify_file() const
{
}


/** \brief Initialize a copyright info object.
 *
 * This constructor ensure the readiness of a copyright info object with
 * a copyright state and a header ready to be used. The list of files
 * and licenses remains empty until a copyright file gets read().
 */
copyright_info::copyright_info()
    : f_state(new copyright_file::copyright_file_state_t)
    , f_copyright_header(f_state)
    //, f_copyright_files() -- auto-init
    //, f_copyright_licenses() -- auto-init
{
}


/** \brief Read a complete copyright file.
 *
 * This function reads an entire copyright file by reading each part
 * one by one.
 *
 * We assume that each part is separated from the other by an empty line.
 *
 * \param[in] input  The input file to read the copyright fields from.
 *
 * \return true if the entire file could be read successfully.
 */
bool copyright_info::read(const memfile::memory_file *input)
{
    f_copyright_header.set_input_file(input);
    if(!f_copyright_header.read())
    {
        return false;
    }
    if(f_copyright_header.eof())
    {
        f_copyright_header.set_input_file(NULL);
        return true;
    }
    std::shared_ptr<files_copyright_file> f(new files_copyright_file(f_state));
    f->copy_input(f_copyright_header);
    f_copyright_header.set_input_file(NULL);
    for(;;)
    {
        if(!f->read())
        {
            f->set_input_file(NULL);
            return false;
        }
        if(f->is_license())
        {
            f_copyright_licenses.push_back(f);
        }
        else
        {
            f_copyright_files.push_back(f);
        }
        // TODO: if there are more empty lines then this test is not enough
        if(f->eof())
        {
            return true;
        }
        std::shared_ptr<files_copyright_file> l(new files_copyright_file(f_state));
        l->copy_input(*f);
        f->set_input_file(NULL);
        f = l;
    }
}


/** \brief Return a pointer to the copyright header.
 *
 * This function returns a bare pointer to the copyright header of this
 * copyright file. This pointer can be used as long as the copyright_info
 * object is valid.
 *
 * \return Pointer to the copyright header set of fields.
 */
header_copyright_file *copyright_info::get_header() const
{
    return const_cast<header_copyright_file *>(&f_copyright_header);
}


/** \brief Return the number of file definitions in this copyright info.
 *
 * This function returns the number of file definitions found in this
 * copyright info structure. The number can be used to get each copyright
 * file pointer with the get_file() function.
 *
 * \return Return the number of copyright file objects.
 */
int copyright_info::get_files_count() const
{
    return static_cast<int>(f_copyright_files.size());
}


/** \brief Get the copyright file at offset idx.
 *
 * This function returns a shared pointer to a copyright file. Which file
 * is returned is specified by \p idx. The order in which files are saved
 * in the copyright_file is the order they are found in the input file.
 *
 * The idx parameter must be between 0 and get_files_count() minus one.
 * If get_files_count() returns zero (0) then no files are available
 * and this function cannot be called.
 *
 * \param[in] idx  The index of the file to retrieve.
 *
 * \return The pointer to the copyright file representing this file fields.
 */
std::shared_ptr<copyright_file> copyright_info::get_file(int idx)
{
    return f_copyright_files[idx];
}


/** \brief Return the number of license definitions in this copyright info.
 *
 * This function returns the number of license definitions found in this
 * copyright info structure. The number can be used to get each copyright
 * license pointer with the get_license() function.
 *
 * \return Return the number of copyright license objects.
 */
int copyright_info::get_licenses_count() const
{
    return static_cast<int>(f_copyright_licenses.size());
}


/** \brief Get the copyright license at offset idx.
 *
 * This function returns a shared pointer to a license file. Which license
 * is returned is specified by \p idx. The order in which licenses are saved
 * in the copyright_file is the order they are found in the input file.
 *
 * The idx parameter must be between 0 and get_licenses_count() minus one.
 * If get_licenses_count() returns zero (0) then no licenses are available
 * and this function cannot be called.
 *
 * \param[in] idx  The index of the license to retrieve.
 *
 * \return The pointer to the copyright file representing this license fields.
 */
std::shared_ptr<copyright_file> copyright_info::get_license(int idx)
{
    return f_copyright_licenses[idx];
}


/** \brief Create a field as per its name.
 *
 * This function creates a field depending on its name.
 *
 * If the wpkg_copyright object does not have a specialized field type
 * to handle this case, then it calls the default field_factory()
 * function of the field_file class which creates a default field_t
 * object. This just means the value will not be checked for validity.
 *
 * \param[in] fullname  The name of the field to create.
 * \param[in] value  The raw value of the field to create.
 *
 * \return A pointer to the newly created field.
 */
std::shared_ptr<wpkg_field::field_file::field_t> copyright_file::field_factory(const case_insensitive::case_insensitive_string& fullname, const std::string& value) const
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
void copyright_file::copyright_field_factory_t::register_field(copyright_file::copyright_field_factory_t *field_factory)
{
    if(g_field_factory_map == NULL)
    {
        g_field_factory_map = new field_factory_map_t;
    }
    (*g_field_factory_map)[field_factory->name()] = field_factory;
}


/** \brief Initialize a copyright field.
 *
 * All copyright fields are derived from the copyright_field_t class
 * so as to gain access to a set of validation functions that are
 * easy to access.
 *
 * \param[in] file  The field file linked with this field.
 * \param[in] name  The name of the field being created.
 * \param[in] value  The starting raw value of the field being created.
 */
copyright_file::copyright_field_t::copyright_field_t(const field_file& file, const std::string& name, const std::string& value)
    : field_t(file, name, value)
{
}


/** \brief Verify that the value is a list of emails.
 *
 * This function checks the value which must look like a valid list
 * of email addresses as defined by RFC5322.
 */
void copyright_file::copyright_field_t::verify_emails() const
{
    // TODO: move the libtld code to test emails in wpkg
    // note that this test must really understands a list of emails
}


/** \brief Verify that the value is a URI.
 *
 * This function checks the value which must look like a valid HTTP or
 * HTTPS URI. If not, an error is thrown.
 */
void copyright_file::copyright_field_t::verify_uri() const
{
    // should this be limited to the first line in value?
    std::string value(get_transformed_value());
    if(!wpkg_util::is_valid_uri(value, "http,https"))
    {
        wpkg_output::log("copyright:%1:%2: invalid value for URI field %3 %4 (expected http[s]://domain.tld/path...)")
                .arg(get_filename())
                .arg(get_line())
                .arg(get_name())
                .quoted_arg(value)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_copyright)
            .package(f_field_file->get_package_name())
            .action("copyright");
    }
}


// Format
COPYRIGHT_FILE_FIELD_FACTORY(format, "Format",
    "Copyright header files must start with a Format field which is a "
    "URI to a page describing the format. At this point the Debian "
    "manual provides such a page:\n"
    "  http://www.debian.org/doc/packaging-manuals/copyright-format/1.0/\n"
    "We may at some pointer define our own copyright definition. "
    "This field only appears in the header.\n"
    "IMPORTANT: This field is required."
)
COPYRIGHT_FILE_FIELD_CONSTRUCTOR(format, copyright)

void copyright_file::field_format_t::verify_value() const
{
    verify_uri();
}


// Upstream-Name
COPYRIGHT_FILE_FIELD_FACTORY(upstreamname, "Upstream-Name",
    "This field defines the name of the original project. It is optional "
    "in case the name upstream is the same as the wpkg name."
    "This field only appears in the header."
)
COPYRIGHT_FILE_FIELD_CONSTRUCTOR(upstreamname, copyright)

void copyright_file::field_upstreamname_t::verify_value() const
{
}


// Upstream-Contact
COPYRIGHT_FILE_FIELD_FACTORY(upstreamcontact, "Upstream-Contact",
    "Name and email addresses of people to contact for the project. "
    "These are rarely the same people as the ones creating the "
    "package. This field only appears in the header. One person's "
    "name and email address must appear per line. The first line "
    "must be kept empty. Note that the references do not need to "
    "include valid email addresses, although it is better if so."
)
COPYRIGHT_FILE_FIELD_CONSTRUCTOR(upstreamcontact, copyright)

void copyright_file::field_upstreamcontact_t::verify_value() const
{
}


// Source
COPYRIGHT_FILE_FIELD_FACTORY(source, "Source",
    "Source of the project, as in where one can find the original files "
    "or if not available online, a way to obtain the source. In most cases "
    "this field is a simple URI. This field only appears in the header."
)
COPYRIGHT_FILE_FIELD_CONSTRUCTOR(source, copyright)

void copyright_file::field_source_t::verify_value() const
{
}


// Disclaimer
COPYRIGHT_FILE_FIELD_FACTORY(disclaimer, "Disclaimer",
    "Disclaimer from the project. In most cases this is only used when a "
    "project has a non-free license and thus has restrictions that you "
    "are expected to follow. This field only appears in the header."
)
COPYRIGHT_FILE_FIELD_CONSTRUCTOR(disclaimer, copyright)

void copyright_file::field_disclaimer_t::verify_value() const
{
}


// Comment
COPYRIGHT_FILE_FIELD_FACTORY(comment, "Comment",
    "A comment about this entry. Comments may appear in the header, "
    "files, and license entries."
)
COPYRIGHT_FILE_FIELD_CONSTRUCTOR(comment, copyright)

void copyright_file::field_comment_t::verify_value() const
{
}


// License
COPYRIGHT_FILE_FIELD_FACTORY(license, "License",
    "The license used by this project or a set of files in this project. "
    "In case of a license entry, this is the actual license as referenced "
    "in Files entries. The license may use one line for a few default "
    "licenses: GPL, LGPL, BSD, Apache 2.0, Artistic, GFDL. All other "
    "licenses need at least one entry with a long description."
)
COPYRIGHT_FILE_FIELD_CONSTRUCTOR(license, copyright)

void copyright_file::field_license_t::verify_value() const
{
}


// Copyright
COPYRIGHT_FILE_FIELD_FACTORY(copyright, "Copyright",
    "The copyright for the entire project is defined in the header. If you "
    "use other people files, then you may enter other Copyright notices "
    "for those files."
)
COPYRIGHT_FILE_FIELD_CONSTRUCTOR(copyright, copyright)

void copyright_file::field_copyright_t::verify_value() const
{
}


// Files
COPYRIGHT_FILE_FIELD_FACTORY(files, "Files",
    "A list of file patterns that reference files in the source project "
    "and defines the copyright information of each one of those files or "
    "group of files. Any number of patterns can be defined on one line. "
    "Debian only authorize * and ? as pattern characters. We also allow "
    "the [a-z] syntax to allow (or not allow with the ^) a range of "
    "characters."
)
COPYRIGHT_FILE_FIELD_CONSTRUCTOR(files, copyright)

void copyright_file::field_files_t::verify_value() const
{
}


/** \brief Copyright files do not accept sub-package specifications.
 *
 * This function prevents sub-packages specifications on copyright file
 * fields.
 *
 * \return This function returns false to prevent any sub-package specifications.
 */
bool copyright_file::copyright_file_state_t::accept_sub_packages() const
{
    return false;
}


header_copyright_file::header_copyright_file(const std::shared_ptr<field_file_state_t> state)
    : copyright_file(state)
{
}


void header_copyright_file::verify_file() const
{
    // The fact that we are here does not mean that the function
    // has_sub_packages() returns true; i.e. a .info file must have
    // a Sub-Packages field, but it does not require the use of
    // sub-package specifications in each .info file

    // an info file must have a Sub-Package field
    if(!field_is_defined(field_format_factory_t::canonicalized_name()))
    {
        wpkg_output::log("copyright:%1:-: the header of a copyright file must have a %2 field")
                .arg(get_filename())
                .arg(field_format_factory_t::canonicalized_name())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_copyright)
            .package(get_package_name())
            .action("copyright");
    }

    // just in case, run the base class verification function too
    copyright_file::verify_file();
}


files_copyright_file::files_copyright_file(const std::shared_ptr<field_file_state_t> state)
    : copyright_file(state)
{
}


/** \brief Determine whether a list of fields is a Files or License.
 *
 * When reading a copyright file, Files field can make references to a license
 * that is defined by itself. This way it is possible to create multiple
 * Files fields that all reference the same license without having to
 * duplicate that license.
 *
 * \return true if the field is a License only field.
 */
bool files_copyright_file::is_license() const
{
    return !field_is_defined(field_files_factory_t::canonicalized_name());
}


void files_copyright_file::verify_file() const
{
    // if we have a Files field then we are a Files copyright entry
    // otherwise we are a License entry; test accordingly

    // an info file must have a Sub-Package field
    if(!field_is_defined(field_files_factory_t::canonicalized_name())
    && !field_is_defined(field_license_factory_t::canonicalized_name()))
    {
        wpkg_output::log("copyright:%1:-: a %2 or %3 field is required in a copyright file entry after the header")
                .arg(get_filename())
                .arg(field_files_factory_t::canonicalized_name())
                .arg(field_license_factory_t::canonicalized_name())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_copyright)
            .package(get_package_name())
            .action("copyright");
    }

    // just in case, run the base class verification function too
    copyright_file::verify_file();
}



}   // namespace wpkg_control
// vim: ts=4 sw=4 et
