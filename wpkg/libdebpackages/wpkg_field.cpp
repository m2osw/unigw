/*    wpkg_field.cpp -- implementation of the field file format
 *    Copyright (C) 2012-2013  Made to Order Software Corporation
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
 * \brief Implementation of the fields as found in the Internet Message format.
 *
 * This base class is used to implement the wpkg_control and wpkg_copyright
 * formats that are necessary to read control and copyright files in a Debian
 * package.
 *
 * This base class is capable of reach the fields as expected by the Debian
 * control file specifications. This includes the name of the fields, an
 * optional component separated by a slash (/) from the name, and the value
 * of the fields. The value may be written on one line or multiple lines
 * when the following lines start with one or more spaces. It also supports
 * empty lines.
 *
 * One part in a control file cannot include the same field more than once.
 * However, one control file can include multiple parts and each part can
 * include the same field (i.e. each part can have its own version of a
 * field.) In most cases this is used to overwrite the default that was
 * found in the first part read from the source. But each format has its
 * own specification in that regard.
 *
 * \note
 * This format is the same as the one used by the Internet Message protocol
 * (i.e. emails.)
 */
#include    "libdebpackages/wpkg_field.h"
#include    "libdebpackages/wpkg_util.h"
#include    "libdebpackages/compatibility.h"
#include    "libdebpackages/debian_packages.h"

#ifdef debpackages_EXPORTS
#define EXPR_DLL 1
#endif
#include    "libexpr/expr.h"

#include    <sstream>
#include    <algorithm>


/** \brief The wpkg_field declares and implements field related functions.
 *
 * The wpkg_field namespace is used to declare and implement all the
 * necessary definitions and functions to load and write files with fields
 * and manage fields in memory.
 */
namespace wpkg_field
{


/** \class wpkg_field_exception
 * \brief The base exception of the wpkg_field classes.
 *
 * This exception class is used as the base exception for all the other
 * exceptions of the wpkg_field implementation. This exception is never
 * thrown directly, but it can be caught in order to catch all the
 * wpkg_field exceptions at once.
 */


/** \class wpkg_field_exception_invalid
 * \brief Something is invalid.
 *
 * This exception is raised whenever something invalid is found.
 *
 * This may be a "logic error" (i.e. can be fixed by fixing your code,) but
 * in most cases the exception is raised because the input data is invalid.
 */


/** \class wpkg_field_exception_cyclic
 * \brief A variable reference is cyclic as in: it references itself.
 *
 * When reading a control file that includes variables, the loader reads
 * the data as is. However, once we retrieve that data, the variables
 * and expressions get processes. When that happens, a variable will
 * eventually itself include variable references and expressions. These
 * are themselves processed. This goes as deep as required, although a
 * warning is displayed after 1,000 layers. However, if any one layer
 * references a variable that we are currently computing, then it is
 * viewed as being cyclic. For example, you could write something like
 * this in a control file:
 *
 * \code
 * Description: ${short_description}
 * short_description=This is the short description of ${project_name}.
 * project_name=complex ${name_v3}
 * name_v3=info about ${last_variable} and more
 * last_variable=${short_description}
 * \endcode
 *
 * As we can see, last_variable makes a reference to short_description
 * which itself makes reference to last_variable through project_name
 * and name_v3. This is considered cyclic as it would cycle forever
 * were we to not detect this case (note that was fixed since 0.9.0.
 * It would cycle in older versions.)
 */


/** \class wpkg_field_exception_undefined
 * \brief Something you are trying to access is undefined.
 *
 * This exception is raised whenever something being accessed does not
 * exist or when creating a new field_file object and the pointer to
 * the state is NULL.
 *
 * Note that when trying to retrieve a field that may not exist, you
 * first should check whether it is defined with the field_is_defined()
 * function.
 */


/** \class field_file
 * \brief Base class to read, write, and manage fields.
 *
 * The field_file is the base class that knows how to parse a set of fields
 * defined as in the Internet Message RFC:
 *
 * \code
 * <name>: <value>
 * \endcode
 *
 * The format supports for the value to be written on multiple lines and
 * for the field name to include a sub-package name:
 *
 * \code
 * Description/wpkg: Short Description here
 *  Long Description There
 * \endcode
 *
 * At this time, the control and control.info file formats and the
 * copyright file format make use of the field_file base class.
 */


/** \class field_file::field_t
 * \brief When retrieving a field, you get this class.
 *
 * This class handles one field. It knows about the field name, sub-package
 * specification, the raw value and can read the transformed value.
 *
 * Because it is particularly useful for errors, each field also gets the
 * name of the file from which it was read and the line on which it was
 * found.
 */


/** \class field_file::field_factory_t
 * \brief When reading a field file, we use a factory to create fields.
 *
 * This class is used to create all the fields read from the input file.
 * The create() function is called with the name of the file from which
 * the field was read, the name of the field and the value. Based on
 * that information, the factor must be capable of creating a new field.
 *
 * Note that in most cases only the name of the field is used to determine
 * the field class to use and the value is passed to that object as it
 * gets created. This process is important because the set_value() function
 * strongly checks the validity of the new value whereas the constructor
 * does not, letting go some errors from older control files in some
 * circumstances.
 */


/** \class field_file::field_file_state_t
 * \brief The state or trait of the field_file.
 *
 * This class is the base class used to define a trait while reading the
 * field_file data from an input file. The state defines a set of functions
 * that may return true or false and depending on the result change the
 * behavior of the reader.
 */

namespace
{


/** \brief Evaluator with additional Debian specific functions.
 *
 * This class is a super class of the libexpr::expr_evaluator so as to
 * offer additional functions in expressions that appear in control
 * files.
 */
class check_fields : public libexpr::expr_evaluator
{
public:
    check_fields(field_file& ff)
        : f_field_file(ff)
    {
    }

    virtual void call_function(std::string& name, arglist& list, libexpr::variable& result)
    {
        if(name == "architecture")
        {
            if(list.size() != 0)
            {
                throw libexpr::function_args("an invalid number of arguments was specified, architecture() does not expect any parameter");
            }
            // note: without the std::string() an architecture that starts
            //       with a digit would become a number
            result.set(std::string(debian_packages_architecture()));
        }
        else if(name == "getfield")
        {
            get_field(list, result);
        }
        else if(name == "os")
        {
            if(list.size() != 0)
            {
                throw libexpr::function_args("an invalid number of arguments was specified, os() does not expect any parameter");
            }
            // note: without the std::string() an os that starts
            //       with a digit would become a number
            result.set(debian_packages_os());
        }
        else if(name == "processor")
        {
            if(list.size() != 0)
            {
                throw libexpr::function_args("an invalid number of arguments was specified, processor() does not expect any parameter");
            }
            // note: without the std::string() a processor that starts
            //       with a digit would become a number
            result.set(debian_packages_processor());
        }
        else if(name == "triplet")
        {
            if(list.size() != 0)
            {
                throw libexpr::function_args("an invalid number of arguments was specified, triplet() does not expect any parameter");
            }
            // note: without the std::string() a triplet that starts
            //       with a digit would become a number
            result.set(debian_packages_triplet());
        }
        else if(name == "vendor")
        {
            if(list.size() != 0)
            {
                throw libexpr::function_args("an invalid number of arguments was specified, vendor() does not expect any parameter");
            }
            // note: without the std::string() a vendor that starts
            //       with a digit would become a number
            result.set(std::string(DEBIAN_PACKAGES_VENDOR));
        }
        else if(name == "versioncmp")
        {
            if(list.size() != 2)
            {
                throw libexpr::function_args("an invalid number of arguments was specified, versioncmp() expects exactly 2 arguments");
            }
            std::string v1, v2;
            list[0].get(v1);
            list[1].get(v2);
            result.set(static_cast<long>(wpkg_util::versioncmp(v1, v2)));
        }
        else if(name == "wpkgversion")
        {
            if(list.size() != 0)
            {
                throw libexpr::function_args("an invalid number of arguments was specified, wpkgversion() does not expect any parameter");
            }
            // note: without the std::string() the version gets converted to a number
            result.set(std::string(DEBIAN_PACKAGES_VERSION_STRING));
        }
        else
        {
            // we don't support that function, check the default evaluator functions
            expr_evaluator::call_function(name, list, result);
        }
    }

private:
    check_fields(const check_fields& rhs);
    check_fields& operator = (const check_fields& rhs);

    enum number_t
    {
        NUMBER_NAN,
        NUMBER_DECIMAL,
        NUMBER_OCTAL,
        NUMBER_HEXADECIMAL,
        NUMBER_FLOAT
    };
    number_t is_number(const char *str)
    {
        // ignore spaces at the beginning
        for(; isspace(*str); ++str);

        // skip the sign
        if(*str == '-' || *str == '+')
        {
            ++str;
        }
        bool octal(false);
        if(*str == '0')
        {
            if(str[1] == 'x' || str[1] == 'X')
            {
                // accept C like hexadecimal
                str += 2;
                if(*str == '\0')
                {
                    return NUMBER_NAN;
                }
                for(; *str != '\0' && !isspace(*str); ++str)
                {
                    if((*str < '0' || *str > '9')
                    && (*str < 'a' || *str > 'f')
                    && (*str < 'A' || *str > 'F'))
                    {
                        return NUMBER_NAN;
                    }
                }
                // ignore spaces at the end
                for(; isspace(*str); ++str);
                return *str == '\0' ? NUMBER_HEXADECIMAL : NUMBER_NAN;
            }
            octal = true;
        }
        for(; *str != '\0' && !isspace(*str); ++str)
        {
            const char c(*str);
            if(octal && (c == '8' || c == '9'))
            {
                octal = false;
            }
            if(c < '0' || c > '9')
            {
                if(c == '.')
                {
                    ++str;
                    if(*str == '\0' || isspace(*str))
                    {
                        for(; isspace(*str); ++str);
                        return *str == '\0' ? NUMBER_FLOAT : NUMBER_NAN;
                    }
                    for(; *str != '\0' && !isspace(*str); ++str)
                    {
                        const char d(*str);
                        if(d == 'e' || d == 'E')
                        {
                            break;
                        }
                        if(d < '0' || d > '9')
                        {
                            return NUMBER_NAN;
                        }
                    }
                }
                if(c == 'e' || c == 'E')
                {
                    ++str;
                    // allow a sign after the decimal point
                    if(*str == '+' || *str == '-')
                    {
                        ++str;
                    }
                    if(*str == '\0' || isspace(*str))
                    {
                        return NUMBER_NAN;
                    }
                    for(; *str != '\0' && !isspace(*str); ++str)
                    {
                        const char d(*str);
                        if(d < '0' || d > '9')
                        {
                            return NUMBER_NAN;
                        }
                    }
                }
                for(; isspace(*str); ++str);
                return *str == '\0' ? NUMBER_FLOAT : NUMBER_NAN;
            }
        }
        return octal ? NUMBER_OCTAL : NUMBER_DECIMAL;
    }

    void get_field(arglist& list, libexpr::variable& result)
    {
        if(list.size() != 1)
        {
            throw libexpr::function_args("an invalid number of arguments was specified for getfield(), expected exactly one parameter, the name of the field to check");
        }
        std::string field_name;
        list[0].get(field_name);

        // A "version" field is a special case that is not transformed to an integer
        bool is_version(false);
        std::string::size_type p = field_name.find_first_of("vV");
        if(p != std::string::npos)
        {
            case_insensitive::case_insensitive_string version(field_name.substr(p, 7));
            if(version == "version")
            {
                is_version = true;
            }
        }

        const std::string& fld(f_field_file.get_field(field_name));

        if(!is_version)
        {
            // check whether the field looks like a number
            switch(is_number(fld.c_str()))
            {
            case NUMBER_DECIMAL:
                result.set(strtol(fld.c_str(), 0, 10));
                return;

            case NUMBER_OCTAL:
                result.set(strtol(fld.c_str(), 0, 8));
                return;

            case NUMBER_HEXADECIMAL:
                result.set(strtol(fld.c_str(), 0, 16));
                return;

            case NUMBER_FLOAT:
                result.set(strtod(fld.c_str(), 0));
                return;

            case NUMBER_NAN:
                // treat as a string
                break;

            default:
                // should never occur
                throw std::runtime_error("unhandled NUMBER_... type");

            }
        }

        // otherwise, that's a string
        result.set(fld);
    }

    field_file&     f_field_file;
};


} // no name namespace


/** \brief Clean up function for the field_file state.
 *
 * This destructor ensures a proper virtual table for
 * the class.
 */
field_file::field_file_state_t::~field_file_state_t()
{
}


/** \brief Retrieve the state of the field_file.
 *
 * When you create a field_file you assign it a state which represents
 * how different parts of the software have to behave. For example, the
 * Architecture is not checked when loading a package for perusal (i.e.
 * for the --info or --field command line options in wpkg.)
 *
 * This is a copy of the state pointer assigned to this field_file.
 *
 * When assigned by a higher level field_file implementation, it is
 * likely that you will be able to dynamic_cast<>() the state to a
 * higher state. Although you could use static_cast<>() we strongly
 * advise against it because the static_cast<>() will always give
 * you a non-NULL pointer even if the state is not what it seems to
 * be.
 *
 * \return A shared pointer to the field_file state.
 */
std::shared_ptr<field_file::field_file_state_t> field_file::get_state() const
{
    return f_state;
}


/** \brief Copy the input information source to this field file.
 *
 * This function copies the input information from source to this field file
 * so one can continue to read the input file from another field_file object.
 * This is useful when reading control files with multiple package entries
 * (used by Debian, we use the .info file format instead for those,) or
 * to read the wpkg/copyright file.
 *
 * After this copy you should remove the input pointer from the source by
 * doing a set_input(NULL) to avoid any problems.
 *
 * \param[in] source  The source field file to copy.
 */
void field_file::copy_input(const field_file& source)
{
    f_input = source.f_input;
    f_offset = source.f_offset;
    f_line = source.f_line;
    f_filename = source.f_filename;
    f_package_name = source.f_package_name;
}


/** \brief Whether this state allows transformations or not.
 *
 * The different state may or may not allow transformations of variables
 * and expressions.
 *
 * In most cases, transformations are not allowed because they should
 * already have been applied. More or less, the control files used with
 * the --build command support transformations. Pretty much all other
 * control files do not. .pc files also support transformations (used
 * with commands such as the --libs and --includes.)
 *
 * For this reason, the default function returns false.
 *
 * \return true if the variables and expressions can be transformed.
 */
bool field_file::field_file_state_t::allow_transformations() const
{
    return false;
}


/** \brief Whether this state can be assigned a sub-package name.
 *
 * The control.info file format allows for sub-package definitions. These
 * allow for one file definition and several packages to be defined in
 * one go. The sub-package definition, however, is not allowed in all formats
 * and thus you may redefine this function and return false to prevent its
 * use.
 *
 * \return true if fields and variables can be assigned a sub-package name.
 */
bool field_file::field_file_state_t::accept_sub_packages() const
{
    return true;
}


/** \brief Field file reads and writes fields in a file.
 *
 * The field file main use is to load control files. The functions understands
 * the field name, the separator (colon or equal sign,) and the value.
 *
 * The value can be pretty much anything you want. It can be written on multiple
 * lines by starting a new line with a white space (tab or space, in a control
 * file, the space is usually the default.)
 *
 * Once the file was parsed, one can use the get/set functions to retrieve
 * the information. The data is segregated in a list of fields (colon separator)
 * and a list of variables (equal sign separator.)
 *
 * To give feedback to the user, you probably want to use the set_output()
 * function. This is quite useful if there is an error in the control file.
 *
 * The build command also makes use of the substitution capability via the
 * set_field_variable(). The variables are used when the content of a field
 * references them via the ${...} syntax. They may also be accessed through
 * the expression $(...) syntax. These transformations are automatically
 * applied when you call the get_field() function.
 *
 * \param[in] state  Different states that the system makes use of to
 *                   work on the input file. This pointer cannot be NULL.
 */
field_file::field_file(const std::shared_ptr<field_file_state_t> state)
    //: f_fields() -- auto-init
    //, f_variables() -- auto-init
    //, f_substitutions() -- auto-init
    : f_state(state)
    //, f_transform_stack() -- auto-init
    //, f_input(NULL) -- auto-init
    //, f_offset(0) -- auto-init
    //, f_line(0) -- auto-init
    //, f_errcnt(0) -- auto-init
    //, f_package_name("") -- auto-init
    //, f_field_name("") -- auto-init
    //, f_field_value("") -- auto-init
    //, f_is_variable(false) -- auto-init
    //, f_is_reading(false) -- auto-init
{
    if(!state)
    {
        throw wpkg_field_exception_undefined("the state parameter to the field_file cannot be NULL");
    }
}


/** \brief Set a substitution variable.
 *
 * The field_file supports variable and expression substitution. This
 * function can be used to define a variable for later substitution.
 * References to variables are made with the ${...} syntax where ...
 * is the name of the variable to replace the ${...} characters.
 *
 * The replacement is done on the fly when a field is read with the
 * get_field() function. It is possible to avoid the substitution
 * though.
 *
 * When creating a package, the substvars is read to fill the
 * table of substitution variables of the field_file object used
 * to read the package control files.
 *
 * Note that the name of the variable is not case sensitive. So
 * the variables "ROOT" and "Root" are the same.
 *
 * Note that the variable substitutions are not to be mistaken as
 * being the same as the variables found in the field_file. See
 * the variable_is_defined(), number_of_variables(), get_variable(),
 * get_variable_name(), delete_variable(), set_variable() functions
 * to manage variables found in the field_file.
 *
 * \param[in] name  The name of the value.
 * \param[in] value  The value of the variable.
 */
void field_file::set_field_variable(const std::string& name, const std::string& value)
{
    // this is more like a variable than a field because we do not want
    // to have the system test for invalid field data (variables are much
    // less likely to be specialized.)
    std::shared_ptr<field_t> field(create_variable(name, value));
    f_substitutions[name] = field;
}


/** \brief Set the name of the package.
 *
 * When generating errors, we include the name of the package if possible.
 * This function can be used by any control file that represent a package
 * as soon as the exact name of the package is known.
 *
 * This function is called by the wpkg_control::field_package_t constructor
 * and the verify_value() function (since that function is called by the
 * set_value(), it is the best place to have it.)
 *
 * \note
 * If you know the name of the package you are about to read, it is a good
 * idea to call this function to initialize the name so any errors that
 * occur before we reach the Package field will also be reported
 * appropriately.
 *
 * \param[in] package_name  The name of the package that this field_file represent.
 */
void field_file::set_package_name(const std::string& package_name)
{
    f_package_name = package_name;
}


/** \brief Get the name of the package in link with this field_file.
 *
 * This function returns the name of the Package field, assuming it is defined
 * and we are reading a control file. This is often used to link errors to be
 * displayed with the package they come from.
 *
 * The name may also be set explicitly with the use of the set_package_name().
 *
 * This parameter is optional so the value returned may be the empty string.
 *
 * \return The name of the package, most often the contents of the Package field.
 */
std::string field_file::get_package_name() const
{
    return f_package_name;
}


/** \brief Check whether one field or more has a sub-package.
 *
 * While reading a control file, we detect whether a file has a sub-package
 * specification. If so, then the file is considered to be a .info file
 * (opposed to a general control file.)
 *
 * \return true if at least one field has a sub-package specification.
 */
bool field_file::has_sub_packages() const
{
    return f_has_sub_package;
}


/** \brief Reset the input file to read a field_file.
 *
 * This function resets the input file state so we can read it as a list
 * of fields. You must call this function once before you can call the
 * read() function.
 *
 * \param[in] input  The input file to use with the following calls to the read() function.
 */
void field_file::set_input_file(const memfile::memory_file *input)
{
    f_input = input;
    if(f_input)
    {
        f_filename = f_input->get_filename().original_filename();
    }
    else
    {
        f_filename.clear();
    }
    f_offset = 0;
    f_line = 0;

    // the following are set to defaults although it is not necessary
    // it shows what the defaults are
    f_field_name = "";
    f_field_value = "";
    f_is_variable = false;
}


/** \brief Retrieve the name of the file being read.
 *
 * Retrieve the name of the file assigned to this field_file. Note that when
 * the set_input_file() is called with NULL the name is also cleared. We do
 * so because once the file is disconnected further errors do not really come
 * from the file itself.
 *
 * \return The name of the file used to load this control file if currently
 *         defined, otherwise the empty string.
 */
std::string field_file::get_filename() const
{
    return f_filename;
}


/** \brief Read a field file from a memory file.
 *
 * This function reads the contents of a field file. It reads the file
 * line by line and saves the result in this field_file object. Once
 * done the fields are accessible using the get_field() and get_field_list()
 * functions. The existence of a field can be checked with the
 * field_is_defined() function.
 *
 * The fields found in the file are not checked in any way by this
 * function. The class can be derived in order to have the fields
 * checked since the read function is virtual and can thus be re-defined
 * in the new class.
 *
 * The file format is the same as the Debian control file. This is
 * very similar to an email header or a pkgconfig file (.pc). The file
 * content must match or errors will occur.
 *
 * A field is defined as a name followed by a colon and then a value on
 * one or more lines. When defined on multiple lines, the next line must
 * start with a white character (a space or tab character.) The field
 * names are case insensitive, although the function keeps the case found
 * in the input file so it can write it back out with the same case.
 *
 * Field names are limited to letters, digits, and the plus (+), minus (-),
 * period (.), and underscore (_) characters. We also support the special
 * syntax of the .info format which accepts sub-package names after a
 * slash character. Only one sub-package name can appear in a field name.
 *
 * \code
 * Description/debug: The debug version of this package is special.
 * \endcode
 *
 * The name of a field must start with a letter or the underscore character.
 * Any other character is not considered valid as the first character.
 * (Note that Debian does not forbid digits as first characters, but that
 * is not compliant with emails and .pc files. Plus at this point there
 * are no fields that start with a digit and none should be created if
 * that matter.)
 *
 * \note
 * The function always accepts variables. If the format you are reading
 * should not support such, then check how many variables were found and
 * if not zero generate an error.
 *
 * \return true if the read() function succeeded, false if errors were found.
 */
bool field_file::read()
{
    if(!f_input)
    {
        throw wpkg_field_exception_undefined("the field_file::read() function cannot be called without first defining the input with set_input_file()");
    }

    {
        class is_reading
        {
        public:
            is_reading(controlled_vars::fbool_t& f)
                : f_flag(f)
            {
                f_flag = true;
            }

            ~is_reading()
            {
                f_flag = false;
            }

        private:
            controlled_vars::fbool_t&   f_flag;
        };
        is_reading reading_now(f_is_reading);

        // reset the error counter
        f_errcnt = 0;
        while(read_field())
        {
            // save the data only if no error occured so far
            if(f_errcnt == 0)
            {
                if(f_is_variable)
                {
                    if(f_variables.find(f_field_name) != f_variables.end())
                    {
                        wpkg_output::log("field:%1:%2: a variable cannot be defined more than once; %3 found twice")
                                .arg(f_filename)
                                .arg(f_line)
                                .quoted_arg(f_field_name)
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_field)
                            .package(f_package_name)
                            .action("field");
                        ++f_errcnt;
                    }
                    else
                    {
                        std::shared_ptr<field_t> field(create_variable(f_field_name, f_field_value, f_filename, f_line));
                        f_variables[f_field_name] = field;
                        //set_variable(field); -- do not call set_variable()
                        // or we'll get a verify_value() call
                    }
                }
                else
                {
                    if(f_fields.find(f_field_name) != f_fields.end())
                    {
                        wpkg_output::log("field:%1:%2: a field cannot be defined more than once; %3 found twice")
                                .arg(f_filename)
                                .arg(f_line)
                                .quoted_arg(f_field_name)
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_field)
                            .package(f_package_name)
                            .action("field");
                        ++f_errcnt;
                    }
                    else
                    {
                        std::shared_ptr<field_t> field(create_field(f_field_name, f_field_value, f_filename, f_line));
                        f_fields[f_field_name] = field;
                        //set_field(field); -- do not call set_field() or
                        // we'll get a verify_value() call
                    }
                }
            }
        }
    }
    // f_is_reading is false

    // verify all the fields as defined by the state
    for(field_map_t::const_iterator it(f_fields.begin()); it != f_fields.end(); ++it)
    {
        try
        {
            it->second->verify_value();
        }
        catch(const std::exception& e)
        {
            // print the error message as such and go on
            wpkg_output::log("%1")
                    .quoted_arg(e.what())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_field)
                .package(f_package_name)
                .action("field-verify");
            ++f_errcnt;
        }
    }

    // also check the variables although it is not likely that anyone will
    // add verification of variables
    for(field_map_t::const_iterator it(f_variables.begin()); it != f_variables.end(); ++it)
    {
        try
        {
            it->second->verify_value();
        }
        catch(const std::exception& e)
        {
            // print the error message as such and go on
            wpkg_output::log("%1")
                    .quoted_arg(e.what())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_field)
                .package(f_package_name)
                .action("field-verify");
            ++f_errcnt;
        }
    }

    // let the derived version check fields if necessary
    verify_file();

    return f_errcnt == 0;
}


/** \brief Verify that the file as a whole is valid.
 *
 * This function is used to check the file as a whole, opposed to a per
 * field (or variable) basis.
 *
 * For example, a binary control file has to have a Package field. An
 * email has to have a To: field, etc.
 *
 * Note that at this point this function does not do anything as the base
 * accepts pretty much anything.
 */
void field_file::verify_file() const
{
}


/** \brief Read one field/variable.
 *
 * This function reads one field or variable and then return.
 *
 * If no more fields or variables are defined, then the function returns false
 * meaning that the name and value parameters are not defined.
 *
 * \return true if the read_field() found an empty line or the end if the file.
 */
bool field_file::read_field()
{
    std::string input_line;
    std::string value;

    int32_t old_line(f_line);
    while(f_input->read_line(*f_offset.ptr(), input_line))
    {
        ++f_line;
        const char *s(input_line.c_str());
        bool has_spaces(false);
        for(; isspace(*s); ++s)
        {
            has_spaces = true;
        }
        if(*s == '#')
        {
            // ignore comments silently
            continue;
        }
        if(*s == '\0')
        {
            // stop on empty lines; this trick can be used to have multiple
            // definitions in a single control file (i.e. each section must
            // have a Package field that defines a different name)
            // in case of an email or HTTP server, this ends the header
            return false;
        }
        if(has_spaces)
        {
            // continuation of a field cannot happen before we find the first field!
            wpkg_output::log("field:%1:%2: you cannot continue a long field before defining an actual field")
                    .arg(f_filename)
                    .arg(f_line)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_field)
                .package(f_package_name)
                .action("field");
            ++f_errcnt;
            continue;
        }

        // the field name ends with a colon
        // a variable name ends with an equal
        const char *start(s);
        while(*s != '\0' && *s != ':' && *s != '=')
        {
            ++s;
        }
        if(*s == '\0')
        {
            wpkg_output::log("field:%1:%2: this line has no field or variable (: and = missing)")
                    .arg(f_filename)
                    .arg(f_line)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_field)
                .package(f_package_name)
                .action("field");
            ++f_errcnt;
            f_field_name = "invalid";
        }
        else
        {
            const size_t q(s - start);
            if(q == 0)
            {
                wpkg_output::log("field:%1:%2: a line cannot start with : or =")
                        .arg(f_filename)
                        .arg(f_line)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_field)
                    .package(f_package_name)
                    .action("field");
                ++f_errcnt;
                f_field_name = "invalid";
            }
            else
            {
                // get the name and trim it (remove spaces, although there shouldn't be any)
                f_field_name = input_line.substr(0, q);
                if(isspace(f_field_name[q - 1]))
                {
                    wpkg_output::log("field:%1:%2: a field or variable name must immediately be followed by ':' or '='")
                            .arg(f_filename)
                            .arg(f_line)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_field)
                        .package(f_package_name)
                        .action("field");
                    ++f_errcnt;
                    f_field_name = "invalid";
                }
                else
                {
                    // the read is valid so far, copy the value
                    value = input_line.substr(q + 1);
                }
            }
        }
        f_is_variable = *s == '=';

        // verify the name validity
        size_t sub_package(0);
        for(const char *n(f_field_name.c_str()); *n != '\0' && sub_package == 0; ++n)
        {
            switch(*n)
            {
            case '.':
            case '+':
            case '-':
                if(n == f_field_name.c_str())
                {
                    wpkg_output::log("field:%1:%2: a field name cannot start with period (.), plus (+), or dash (-), %3 is not valid")
                            .arg(f_filename)
                            .arg(f_line)
                            .quoted_arg(f_field_name)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_field)
                        .package(f_package_name)
                        .action("field");
                    ++f_errcnt;
                    sub_package = static_cast<size_t>(-1);
                    continue;
                }
            case '_':
                break;

            case '/': // supported info files
                if(sub_package != 0)
                {
                    wpkg_output::log("field:%1:%2: only one Sub-Package name can be defined after a field name, %3 is not valid")
                            .arg(f_filename)
                            .arg(f_line)
                            .quoted_arg(f_field_name)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_field)
                        .package(f_package_name)
                        .action("field");
                    ++f_errcnt;
                    sub_package = static_cast<size_t>(-1);
                    continue;
                }
                f_has_sub_package = true;
                sub_package = n - f_field_name.c_str() + 1;
                break;

            default:
                if(*n >= '0' && *n <= '9')
                {
                    if(n == f_field_name.c_str())
                    {
                        wpkg_output::log("field:%1:%2: a field name cannot start with a digit (0-9), %3 is not valid")
                                .arg(f_filename)
                                .arg(f_line)
                                .quoted_arg(f_field_name)
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_field)
                            .package(f_package_name)
                            .action("field");
                        ++f_errcnt;
                        sub_package = static_cast<size_t>(-1);
                        continue;
                    }
                    break;
                }
                if((*n >= 'a' && *n <= 'z')
                || (*n >= 'A' && *n <= 'Z'))
                {
                    break;
                }
                // A true Debian field name supports characters 33-57
                // and 59-126 (i.e. US-ASCII except controls, colon (:),
                // and spaces, and it cannot start with a #)
                wpkg_output::log("field:%1:%2: a field name only supports these characters: [-+._/0-9A-Za-z], %3 is not valid")
                        .arg(f_filename)
                        .arg(f_line)
                        .quoted_arg(f_field_name)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_field)
                    .package(f_package_name)
                    .action("field");
                ++f_errcnt;
                sub_package = static_cast<size_t>(-1);
                continue;

            }
        }
        if(sub_package != static_cast<size_t>(-1) && sub_package != 0)
        {
            if(sub_package == 1)
            {
                wpkg_output::log("field:%1:%2: a field name (%3) cannot be empty even if a Sub-Package name is specified.")
                        .arg(f_filename)
                        .arg(f_line)
                        .arg(f_field_name)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_field)
                    .package(f_package_name)
                    .action("field");
                ++f_errcnt;
                f_field_name = "invalid";
            }
            else
            {
                // make sure the sub-package name itself isn't empty
                std::string sub_package_name(f_field_name, sub_package);
                if(sub_package_name.empty())
                {
                    wpkg_output::log("field:%1:%2: a field Sub-Package name (%3) cannot be empty")
                            .arg(f_filename)
                            .arg(f_line)
                            .arg(f_field_name)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_field)
                        .package(f_package_name)
                        .action("field");
                    ++f_errcnt;
                }
                else if(!wpkg_util::is_package_name(sub_package_name))
                {
                    wpkg_output::log("field:%1:%2: field Sub-Package name %3 is not a valid package name")
                            .arg(f_filename)
                            .arg(f_line)
                            .quoted_arg(sub_package_name)
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_field)
                        .package(f_package_name)
                        .action("field");
                    ++f_errcnt;
                }
            }
        }

        // we're done here, we use the while() for the continue
        break;
    }
    if(old_line == f_line)
    {
        // we hit the end of the file
        return false;
    }

    int next_offset(f_offset);
    while(f_input->read_line(next_offset, input_line))
    {
        const char *s(input_line.c_str());
        int has_spaces(0);
        for(; isspace(*s); ++s)
        {
            ++has_spaces;
        }
        if(*s == '#')
        {
            // ignore comments silently; here we're done, this means
            // there is no continuation of the previous line
            // also we can skip that line immediately
            f_offset = next_offset;
            break;
        }
        if(!has_spaces)
        {
            // no spaces means that we do not accept this line, we're
            // done here for now
            break;
        }

        ++f_line;
        f_offset = next_offset;

        // empty line? (represented by a period by itself)
        if(s[0] == '.')
        {
            if(s[1] != '\0')
            {
                if(isspace(s[1]))
                {
                    if(s[2] != '\0')
                    {
                        //
                        // Lines that start with 1 or more spaces/tabs, a
                        // period, a space, and then something are viewed
                        // as a future "improvement" and must be forbidden
                        // at this point
                        //   . <something>
                        //
                        // Note:
                        // This is forbidden in Description only, should we leave
                        // people alone for other fields? (from what I can tell
                        // it should not be a problem at this point)
                        //
                        wpkg_output::log("field:%1:%2: a continuation field cannot start with a period unless the whole field is just a period")
                                .arg(f_filename)
                                .arg(f_line)
                            .level(wpkg_output::level_error)
                            .module(wpkg_output::module_field)
                            .package(f_package_name)
                            .action("field");
                        ++f_errcnt;
                        // ignore this line, but continue to read the file
                        continue;
                    }
                    // something starting with a period and no space is just fine
                }
                else
                {
                    // skip the period and the space
                    s += 2;
                    has_spaces = 1;
                }
            }
            else
            {
                ++s;
                has_spaces = 1;
            }
        }
        value += "\n";

        // append the line to the last entry (trimmed on the left)
        if(f_is_variable && has_spaces > 1)
        {
            // spaces are significant in a field when there is more than one
            // (transforms the Description to a <pre>...</pre> entry)
            value += input_line.substr(1);
        }
        else
        {
            value += s;
        }
    }

    // get the trimmed value (it is very likely that we'll find some
    // spaces around most values, especially at the start)
    size_t p, e;
    for(p = 0; p < value.length() && (value[p] == ' ' || value[p] == '\t'); ++p);
    for(e = value.length(); e > p && (value[e - 1] == ' ' || value[e - 1] == '\t'); --e);
    f_field_value = value.substr(p, e - p);

    return true;
}


/** \brief Generate the output of a field value.
 *
 * This function rebuilds the value of a field the way it needs to
 * be in order to be re-read unmodified later.
 *
 * \param[in] value  The field value to transform.
 *
 * \return The transformed value.
 */
std::string field_file::output_multiline_field(const std::string& value)
{
    std::string result(value);

    std::string::size_type p(0);
    for(;;)
    {
        p = result.find('\n', p);
        if(p == std::string::npos)
        {
            break;
        }
        // we save lines with just a period as empty lines, in this
        // case we want to restore the period if necessary
        if(p + 1 < result.length() && result[p + 1] == '\n')
        {
            result.insert(p + 1, 1, '.');
        }
        result.insert(p + 1, 1, ' ');
        p += 2;
    }

    return result;
}


/** \brief Write this field_file to the specified memory file.
 *
 * This function writes all the fields of this field file to the specified
 * memory file. By default the attached variables do not get saved.
 *
 * The \p ordered_fields parameter may be used to list a set of fields that
 * should be saved ahead of the others. The order in which they appear in
 * the vector is respected in the output file.
 *
 * Field values are processed so all variables are replaced by their value
 * before they get saved.
 *
 * In order to hide the list of ordered_fields in specific implementation
 * of the field_file, the write() function was made virtual.
 *
 * \param[out] file  The output file. It is reset before the write() starts.
 * \param[in] save_variables  Whether to save the variables after the fields.
 * \param[in] ordered_fields  A list of fields to write ahead of the others.
 */
void field_file::write(memfile::memory_file& file, write_mode_t mode, const list_t& ordered_fields) const
{
    // we do not need the name map to use case insensitive strings because
    // we save the name as we find it in the field (therefore we get the
    // exact same case)
    typedef std::map<std::string, int> name_map_t;

    file.reset();
    file.create(memfile::memory_file::file_format_other);

    name_map_t output_fields;
    for(list_t::const_iterator it(ordered_fields.begin()); it != ordered_fields.end(); ++it)
    {
        if(field_is_defined(*it))
        {
            std::shared_ptr<field_t> info(get_field_info(*it));
            std::string value(mode != WRITE_MODE_RAW_FIELDS ? info->get_transformed_value() : info->get_value());
            std::string field(*it + ": " + output_multiline_field(value) + "\n");
            file.write(field.c_str(), file.size(), static_cast<int>(field.length()));
            output_fields[*it] = 0;
        }
    }

    for(field_map_t::const_iterator it(f_fields.begin()); it != f_fields.end(); ++it)
    {
        if(output_fields.find(it->first) == output_fields.end())
        {
            std::shared_ptr<field_t> info(get_field_info(it->first));
            std::string value(mode != WRITE_MODE_RAW_FIELDS ? info->get_transformed_value() : info->get_value());
            std::string field(it->first + ": " + output_multiline_field(value) + "\n");
            file.write(field.c_str(), file.size(), static_cast<int>(field.length()));
        }
    }

    if(mode != WRITE_MODE_FIELD_ONLY)
    {
        for(field_map_t::const_iterator it(f_variables.begin()); it != f_variables.end(); ++it)
        {
            // TBD: should we also pass variables through the transformation code?
            std::string field(it->first + "=" + output_multiline_field(it->second->get_value()) + "\n");
            file.write(field.c_str(), file.size(), static_cast<int>(field.length()));
        }
    }
}


/** \brief check whether the end of the file was reached.
 *
 * This function is generally used when a file may include multiple entries
 * such as a copyright file that has Files and Licenses fields repeated.
 *
 * \return true if the end of the file was reached.
 */
bool field_file::eof() const
{
    if(f_input)
    {
        return f_offset >= f_input->size();
    }
    // if no input, assume eof()
    return true;
}


/** \brief Copy a set of fields from one field file to another.
 *
 * This function reads fields from this field file and copies the
 * fields to the field file defined as \p destination.
 *
 * The function may copy only the fields that match a specific
 * sub-package name. In this case, the fields that are not assigned
 * a package name are always copied. Others are copied only when
 * their sub-package name matches the \p sub_package parameter.
 * Setting the \p sub_package parameter to an empty string prevents
 * the copy of any field with a sub-package specification.
 *
 * The \p excluded list of fields can be used to further prevent
 * the copy of named fields. Note that a find() function is used
 * against this list. It is recommended that the list be small to
 * avoid slowness.
 *
 * \note
 * I changed the implementation so non-specialized fields do not get
 * copied in the event there is a specialized field to copy. Not calling
 * the set_field() is better than calling saving time otherwise.
 *
 * \param[in,out] destination  The destination field file receiving the fields.
 * \param[in] sub_package  If specified and some fields are specific to a sub-package, only copy if the package name matches.
 * \param[in] excluded  List of names of the fields that must not be copied in this operation.
 */
void field_file::copy(field_file& destination, const std::string& sub_package, const list_t& excluded) const
{
    if(this == &destination)
    {
        // although we could just ignore the call, this should not happen
        throw wpkg_field_exception_invalid("the field_file::copy() function was called with &destination == this");
    }

    std::map<case_insensitive::case_insensitive_string, bool> defined_fields;

    for(field_map_t::const_iterator it(f_fields.begin()); it != f_fields.end(); ++it)
    {
        bool use(true);
        if(!sub_package.empty())
        {
            if(it->second->has_sub_package_name())
            {
                // use only if sub-package name matches
                // (or if the field exists but does not have a sub-package
                // definition that match, which happens in a second loop)
                use = sub_package == it->second->get_sub_package_name();
            }
            else
            {
                use = false;
            }
        }
        if(use)
        {
            std::string name(it->second->get_field_name());
            case_insensitive::case_insensitive_string n(name);
            if(std::find(excluded.begin(), excluded.end(), n) == excluded.end())
            {
                // overwrite when the destination already exists
                // create a new field because if &destination != this then
                // the f_field_file pointer would be wrong (i.e. we cannot
                // share the same field between different field_file objects)
                std::string value(it->second->get_transformed_value());
                std::string filename(it->second->get_filename());
                std::shared_ptr<field_t> field(create_field(n, value, filename, it->second->get_line()));
                destination.set_field(field);
                defined_fields[n] = true;
            }
        }
    }

    if(!sub_package.empty())
    {
        // just in case, check for fields that did not match sub_package
        // and have a definition without the
        for(field_map_t::const_iterator it(f_fields.begin()); it != f_fields.end(); ++it)
        {
            if(!it->second->has_sub_package_name())
            {
                std::string name(it->second->get_field_name());
                case_insensitive::case_insensitive_string n(name);
                if(std::find(excluded.begin(), excluded.end(), n) == excluded.end()
                && defined_fields.find(n) == defined_fields.end())
                {
                    // as before we overwrite even if destination already
                    // exists
                    std::string value(it->second->get_transformed_value());
                    std::string filename(it->second->get_filename());
                    std::shared_ptr<field_t> field(create_field(n, value, filename, it->second->get_line()));
                    destination.set_field(field);
                }
            }
        }
    }
}


/** \brief Replace variables understood by the low level wpkg_field object.
 *
 * This function transforms variables using the low level wpkg_field object
 * known variables.
 *
 * At this point, the variables known by the field_file implementation
 * are:
 *
 * \li ${Newline} -- replace with "\n"
 * \li ${Space} -- replace with " "
 * \li ${Tab} -- replace with "\t"
 * \li ${wpkg:Upstream-Version} -- the version of this instance of wpkg
 * \li ${wpkg:Version} -- the version of this instance of wpkg
 * \li ${F:\<field>} -- the contents of the named \<field> which will itself be transformed
 * \li ${V:\<variable>} -- the contents of the named \<variable> which will itself be transformed
 *
 * If the named variable is not known, then a warning is generated. In case
 * of the ${F:\<field name>}, if the named field is not known, then a warning
 * is also generated. The result of the function when accessing an undefined
 * value is the empty string ("").
 *
 * Derived classes can redefine this function to support additional
 * variables. In that case, the redefined function needs to check
 * for its variables (i.e. higher priority) and then return by calling
 * this function:
 *
 * \code
 * std::string my_field_file::replace_variable(const field_t *field, const case_insensitive::case_insensitive_string& name) const
 * {
 *     if(name == "Arch")
 *     {
 *         return "in ruins";
 *     }
 *     return field_file::replace_variable(field, name);
 * }
 * \endcode
 *
 * \note
 * Variable names are case insensitive, so ${Newline} is equivalent to
 * ${NEWLINE}. Similarly references to fields or field_file variables
 * (entries defined with the colon (:) or the equal (=) characters by
 * the read() function) are also case insensitive. For example, the
 * following few groups are equivalent:
 *
 * \code
 * ${NewLine}
 * ${Newline}
 * ${NEWLINE}
 *
 * ${F:Package}
 * ${f:package}
 * ${F:PACKAGE}
 *
 * ${V:ROOT_TREE}
 * ${v:root_tree}
 * \endcode
 *
 * \param[in] field  The field being transformed with details such as the
 *                   source filename and line number.
 * \param[in] name  The name of the variable to load.
 *
 * \return The contents of the variable or the empty string ("").
 */
std::string field_file::replace_variable(const field_t *field, const case_insensitive::case_insensitive_string& name) const
{
    if(name == "Newline")
    {
        // explicit newline
        return "\n";
    }

    if(name == "Space")
    {
        // explicit space
        return " ";
    }

    if(name == "Tab")
    {
        // explicit tab
        return "\t";
    }

    if(name == "wpkg:Upstream-Version" || name == "wpkg:Version")
    {
        // TBD: should we support dpkg:Upstream-Version and dpkg:Version?
        return DEBIAN_PACKAGES_VERSION_STRING;
    }

    if(name.size() > 2 && (name[0] == 'F' || name[0] == 'f') && name[1] == ':')
    {
        // F:<fieldname>
        std::string field_name(name.substr(2));
        if(field_is_defined(field_name))
        {
            // explicit field
            return get_field(field_name);
        }

        wpkg_output::log("field:%1:%2: field named %3 is not defined.")
                .arg(field->get_filename())
                .arg(field->get_line())
                .quoted_arg(name)
            .level(wpkg_output::level_warning)
            .module(wpkg_output::module_field)
            .package(f_package_name)
            .action("field");

        return "";
    }

    if(name.size() > 2 && (name[0] == 'V' || name[0] == 'v') && name[1] == ':')
    {
        // V:<variablename>
        std::string variable_name(name.substr(2));
        if(variable_is_defined(variable_name))
        {
            // explicit variable
            return get_variable(variable_name);
        }
    }
    else if(f_auto_transform_variables)
    {
        // <variablename>
        if(variable_is_defined(name))
        {
            // explicit variable
            return get_variable(name);
        }
    }

    wpkg_output::log("field:%1:%2: variable named %3 is not defined")
            .arg(field->get_filename())
            .arg(field->get_line())
            .quoted_arg(name)
        .level(wpkg_output::level_warning)
        .module(wpkg_output::module_field)
        .package(f_package_name)
        .action("field");

    return "";
}


/** \brief Transform a field value with variables and expressions.
 *
 * This function loops over the specified value from the specified field
 * until the resulting value does not contain any ${...} and $(...).
 * All the variables and all the expressions are processed. At this point
 * there are no other controls over the behavior of this function.
 *
 * This function is called whenever you use the get_field() function to
 * read a field. Note that variables (i.e. ROOT_TREE=/this/root/dir)
 * found in control files are not transformed in this way.
 *
 * Additional information about the debian capability can be found using
 * the following Linux terminal command line:
 *
 * \code
 * man deb-substvars
 * \endcode
 *
 * \todo
 * The support for a lone $ is fairly poor at this time. It is not unlikely
 * that it won't work right in some cases. A lone $ is expected to be
 * written as "${}". However, because of the repetitive parsing of the
 * result for further transformations we may end up transforming a lone
 * $ in an unexpected way.
 *
 * \exception wpkg_field_exception_invalid
 * wpkg_field_exception_invalid is raised if the transformation
 * includes an empty expression such as: "$()", or when a variable
 * or an expression reference is missing the closing bracket or
 * parenthesis as in "${ThisVariable" or "$(3 * (7 + 5)".
 *
 * \param[in] field  The field being transformed, used for errors.
 * \param[in,out] value  The value being transformed.
 */
void field_file::transform_dynamic_variables(const field_t *field, std::string& value) const
{
    // Transformation allowed?
    // We allow transformations in the source and in the build processes.
    if(!f_state->allow_transformations())
    {
        return;
    }

    // make sure that the function doesn't get called for the same field when
    // it is parsing that field because if that happens the value cannot be
    // computed (we break the recursive call)
    if(std::find(f_transform_stack.begin(), f_transform_stack.end(), case_insensitive::case_insensitive_string(field->get_name())) != f_transform_stack.end())
    {
        // we could instead generate an error and then return an empty string?
        std::string errmsg;
        wpkg_output::log(errmsg, "field:%1:%2: the field %3 is cyclic (depends on itself,) so we cannot transform its value safely.")
                .arg(field->get_filename())
                .arg(field->get_line())
                .quoted_arg(field->get_name());
        throw wpkg_field_exception_cyclic(errmsg);
    }

    class fifo_raii
    {
    public:
        fifo_raii(field_stack_t& s, const std::string& v)
            : f_stack(s)
        {
            f_stack.push_back(v);
        }

        ~fifo_raii()
        {
            f_stack.pop_back();
        }

    private:
        field_stack_t       f_stack;
    };
    fifo_raii fifo(f_transform_stack, field->get_name());

    // replace dynamic variables: ${varname}
    // replace dynamic C-like expressions: $(expr)
    // this only applies when building packages
    bool repeat;
    do
    {
        std::string result;
        repeat = false;
        std::string::size_type start(0);
        for(;;)
        {
            std::string::size_type p(value.find_first_of('$', start));
            if(p == std::string::npos || p + 1 == value.size())
            {
                // no more $ in value or it ends with $
                result += value.substr(start);
                break;
            }
            if(value[p + 1] != '{' && value[p + 1] != '(')
            {
                // lone $ characters are added as is
                result += "$";
                start = p + 1;
                continue;
            }
            result += value.substr(start, p - start);
            bool is_expression(value[p + 1] == '(');
            p += 2;
            // in case of expression, the value may include ( and ) so we have
            // to count them; for variables, the { and } characters are not
            // valid within the variable definition
            std::string::size_type q(p);
            if(is_expression)
            {
                int count(1);
                --q; // in case of an empty entry: "$()"
                while(count > 0 && q + 1 < value.size())
                {
                    ++q;
                    if(value[q] == '(')
                    {
                        ++count;
                    }
                    else if(value[q] == ')')
                    {
                        --count;
                    }
                }
                if(count != 0)
                {
                    // search failed
                    wpkg_output::log("field:%1:%2: an expression must always end with ')', there is a mismatch at this point.")
                            .arg(field->get_filename())
                            .arg(field->get_line())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_field)
                        .package(f_package_name)
                        .action("field");
                    ++f_errcnt;
                    throw wpkg_field_exception_invalid("invalid expression, ')' is missing");
                }
            }
            else
            {
                for(; q < value.size(); ++q)
                {
                    char c = value[q];
                    if((c < 'A' || c > 'Z')
                    && (c < 'a' || c > 'z')
                    && (c < '0' || c > '9')
                    && c != '_' && c != ':')
                    {
                        break;
                    }
                }
                if(value[q] != '}')
                {
                    wpkg_output::log("field:%1:%2: a variable reference must always end with '}', there is a mismatch at this point")
                            .arg(field->get_filename())
                            .arg(field->get_line())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_field)
                        .package(f_package_name)
                        .action("field");
                    ++f_errcnt;
                    throw wpkg_field_exception_invalid("invalid variable name, '}' is missing");
                }
            }
            // mark for repeat since we want this to be recursive
            repeat = true;
            // get the string between the ${ and } or $( and )
            case_insensitive::case_insensitive_string data(value.substr(p, q - p));
            if(is_expression)
            {
                if(data.empty())
                {
                    wpkg_output::log("field:%1:%2: an expression in a field file cannot be empty")
                            .arg(field->get_filename())
                            .arg(field->get_line())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_field)
                        .package(f_package_name)
                        .action("field");
                    ++f_errcnt;
                    throw wpkg_field_exception_invalid("an expression cannot be empty");
                }
                check_fields evaluator(const_cast<field_file&>(*this));
                libexpr::variable r;
                evaluator.eval(data, r);
                std::string v;
                r.to_string(v);
                result += v;
            }
            else
            {
                if(data.empty())
                {
                    // ${} is equivalent to $ by itself (it is in Debian)
                    // note however that we have to support $ by itself as
                    // a $ character otherwise the repeat loop would fail
                    result += "$";
                }
                else
                {
                    field_map_t::const_iterator subst(f_substitutions.find(data));
                    if(subst == f_substitutions.end())
                    {
                        result += replace_variable(field, data);
                    }
                    else
                    {
                        result += subst->second->get_value();
                    }
                }
            }
            start = q + 1;
        }
        value = result;
    }
    while(repeat);
}


/** \brief Create a field.
 *
 * This function is a factory. Each derived create_field() function has a
 * chance to create the field and return it. If a derived function does
 * not know how to create the field, it can call its base class
 * create_field() function. In effect this is a way to create a field of
 * any type acceptable in the field_file you are managing.
 *
 * Note that the name is passed as a case insensitive string. This makes
 * it easy to test the string since in most cases the case is not
 * significant. However, if you need to test case sensitively, you can
 * simply copy the name in an std::string first.
 *
 * Not only will the function create the field, it will also verify that
 * the value is valid for that field. If not valid, then the function
 * throws an error.
 *
 * \note
 * Since this is a factory it has to allocate the field which is a little
 * different from what we had before, but does not make such a difference.
 * We return a shared pointer so you can copy the shared pointer around
 * and once done you can simply reset() the pointer.
 *
 * \param[in] name  The name of the field.
 * \param[in] value  The value of the field, the function does an
 *                   immediate set_value() on the new field.
 * \param[in] filename  The name of the file the field was read from or
 *                      the empty string if not from a file.
 * \param[in] line  The line on which the field was read in filename.
 * \param[in] package_name  The name of the package corresponding to this
 *                          field_file.
 *
 * \return A shared pointer to a field object.
 */
std::shared_ptr<field_file::field_t> field_file::create_field(
        const case_insensitive::case_insensitive_string& name,
        const std::string& value,
        const std::string& filename,
        const int line
    ) const
{
    std::shared_ptr<field_t> result(field_factory(name, value));

    result->set_filename(filename);
    result->set_line(line);

    return result;
}


/** \brief Allocate the field.
 *
 * This function is the one that actually allocates the field. The
 * true factory. From the outside you can only use the create_field()
 * function, which in turn calls this function to get a pointer to
 * the actual field. Then the create_field() function can finish up
 * the setting up of the field by saving the value, filename, and
 * line information in the field.
 *
 * \param[in] name  The name of the field to allocate.
 * \param[in] value  The raw value of the field.
 *
 * \return A shared pointer to a field object.
 */
std::shared_ptr<field_file::field_t> field_file::field_factory(const case_insensitive::case_insensitive_string& name, const std::string& value) const
{
    return std::shared_ptr<field_t>(new field_t(*this, name, value));
}


/** \brief Create a variable.
 *
 * This function creates a variable by calling the variable_factory()
 * virtual function. The result is a shared pointer to a variable.
 *
 * \note
 * Note that at this point there is no plan to have more than one
 * variable_factory() function but it makes sense to have the flexibility
 * available from the start.
 *
 * \param[in] name  The name of the field.
 * \param[in] value  The value of the field, the function does an
 *                   immediate set_value() on the new field.
 * \param[in] filename  The name of the file the field was read from or
 *                      the empty string if not from a file.
 * \param[in] line  The line on which the field was read in filename.
 * \param[in] package_name  The name of the package corresponding to this
 *                          field_file.
 *
 * \return A shared pointer to a field object.
 */
std::shared_ptr<field_file::field_t> field_file::create_variable(
        const case_insensitive::case_insensitive_string& name,
        const std::string& value,
        const std::string& filename,
        const int line
    ) const
{
    std::shared_ptr<field_t> result(variable_factory(name, value));

    result->set_filename(filename);
    result->set_line(line);

    // if we're not reading we want to make sure that the set_value()
    // verifies the validity of the new value (otherwise the value is
    // set by the constructor without being checked.)
    if(!f_is_reading)
    {
        result->set_value(value);
    }

    return result;
}


/** \brief Allocate the variable.
 *
 * This function is the one that actually allocates the variable. The
 * true factory. From the outside you can only use the create_variable()
 * function, which in turn calls this function to get a pointer to
 * the actual variable. Then the create_variable() function can finish
 * up the setting up of the variable by saving the value, filename, and
 * line information in the variable.
 *
 * \param[in] name  The name of the field to allocate.
 * \param[in] value  The raw value of the field.
 *
 * \return A shared pointer to a field object.
 */
std::shared_ptr<field_file::field_t> field_file::variable_factory(const case_insensitive::case_insensitive_string& name, const std::string& value) const
{
    return std::shared_ptr<field_t>(new field_t(*this, name, value));
}


/** \brief Check whether the field is defined in this file.
 *
 * This function searches for the specified field and return true if defined.
 * The function understands the concept of sub-packages so you may check
 * with a field name that includes a sub-package name. If that full name is
 * defined, the function returns true immediately. If the full name does
 * not exist, then that sub-package name is remove and we check again for
 * the existence of a field without that sub-package name.
 *
 * \code
 * // the following call checks for a field named:
 * //   "Foo/blah"
 * // and if not found, it tries again to find the field named:
 * //   "Foo"
 * // The value of "Foo" is considered to be the default value when the
 * // "Foo/blah" field is not explicitly defined.
 * field_is_defined("Foo/blah");
 * \endcode
 *
 * \note
 * This function does not check whether the \p name parameter is valid
 * and/or whether that name supports sub-package names.
 *
 * \note
 * If you know that you are checking for a bare field name (i.e. no
 * sub-package specified) then it is a good idea to set the \p as_is
 * parameter to true to avoid checking for the '/' character in
 * \p name.
 *
 * \param[in] name  The name of the field to find.
 * \param[in] as_is  Whether the function should check for a sub-package
 *                   name (false) or not check for a default field (true).
 *
 * \return Return true if the field is defined as is or without a sub-package name
 *         if as_is is set to false, return false otherwise.
 */
bool field_file::field_is_defined(const std::string& name, bool as_is) const
{
    if(f_fields.find(name) != f_fields.end())
    {
        // found this field with the name used as is
        return true;
    }
    if(!as_is)
    {
        std::string::size_type p(name.find_last_of('/'));
        if(p != std::string::npos)
        {
            // try again without the '/<sub-package name>'
            return f_fields.find(name.substr(0, p)) != f_fields.end();
        }
        // name does not include a sub-package specification
    }
    return false;
}


/** \brief Add or replace a field in a field_file.
 *
 * This function saves the specified field in this field_file.
 *
 * \param[in] field  The field to be added to the field_file.
 */
void field_file::set_field(const std::shared_ptr<field_t> field)
{
    // TBD: should we not overwrite the source filename & line
    //      if the new field information does not include them?
    f_fields[field->get_name()] = field;

    // verify the value now that it was saved in the field;
    // sooner and the get_transformed_value() fails.
    field->verify_value();
}


/** \brief Helper function to set a field with a name and value.
 *
 * This function is a helper function that is used in many places
 * to setup a field with just a name and a value.
 *
 * If the field is already defined, the value is overwritten.
 *
 * \param[in] name  The name of the field.
 * \param[in] value  The new value for this field.
 */
void field_file::set_field(const std::string& name, const std::string& value)
{
    std::shared_ptr<field_t> field(create_field(name, value));
    set_field(field);
}


/** \brief Helper function to set a field with a long value.
 *
 * This function is a helper function used to set a field with
 * a long value. The long value is converted to a string by
 * the function before calling the set_field() with a field_t
 * as a parameter.
 *
 * If the field already exists, the value is overwritten.
 *
 * \param[in] name  The name of the field.
 * \param[in] value  The new value for this field.
 */
void field_file::set_field(const std::string& name, long value)
{
    std::stringstream ss;
    ss << value;
    std::shared_ptr<field_t> field(create_field(name, ss.str()));
    set_field(field);
}


/** \brief Get the value of a field.
 *
 * This function searches for the specified field, transforms the
 * field value with dynamic variables and expressions, and returns
 * the result.
 *
 * The \p name parameter is case insensitive. So a field named
 * "Package" can be read with "package" or "PACKAGE" all the same.
 *
 * To read the variable value without getting the value transformed
 * use the get_field_info() function. Also, the get_field_list()
 * function returns the field without transformation, but in that
 * case you have to search for the field you are interested in.
 *
 * The function understands sub-package specification. If the
 * \p name parameter includes a slash, then the field is also
 * searched without the sub-package specification since that
 * other field is the default value in that case. If you do
 * not want to have this side effect, look at the
 * field_is_defined() function as you can use it with the
 * \p as_is parameter set to true to know whether the field
 * name with the sub-package specification is defined.
 *
 * \exception wpkg_field_exception_undefined
 * wpkg_field_exception_undefined is raised if the field is not
 * defined. To avoid this exception, call the field_is_defined()
 * function first, if false, do not call this function.
 *
 * \exception wpkg_field_exception_invalid
 * This function may also raise wpkg_field_exception_invalid when
 * the value includes an invalid variable or expression reference.
 *
 * \param[in] name  The name of the field to retrieve.
 *
 * \return The value of the field with all variables and expressions
 * transformed to their content and compute result respectively.
 */
std::string field_file::get_field(const std::string& name) const
{
    // find the field if available
    field_map_t::const_iterator it(f_fields.find(name));
    if(it == f_fields.end())
    {
        size_t p(name.find_last_of('/'));
        if(p != std::string::npos)
        {
            it = f_fields.find(name.substr(0, p));
        }
        if(it == f_fields.end())
        {
            // with or without the sub-package name we cannot find it
            throw wpkg_field_exception_undefined("field \"" + name + "\" is undefined");
        }
    }

    // avoid calling the transform function if the value does not
    // include at least one '$' (it creates at least one extra copy)
    std::string result(it->second->get_value());
    if(result.find('$') != std::string::npos)
    {
        // TBD: should we cache the result? can the result ever change
        //      over time?
        transform_dynamic_variables(it->second.get(), result);
        return result;
    }

    return result;
}


/** \brief Retrieve the first line of the field value.
 *
 * This function reads the first line of the field value. The function
 * calls the get_field() which may throw an exception.
 *
 * \param[in] name  The name of the field of which the first line is to be read.
 *
 * \return The first line of the field.
 */
std::string field_file::get_field_first_line(const std::string& name) const
{
    const std::string value(get_field(name));
    const char *start(value.c_str());
    for(const char *s(start); *s != '\0'; ++s)
    {
        if(*s == '\r' || *s == '\n')
        {
            return value.substr(0, s - start);
        }
    }
    return value;
}


/** \brief Retrieve the field value except the first line.
 *
 * This function reads the field value after removing the first line.
 * The function calls the get_field() which may throw an exception.
 * If only a first line is defined, then the function returns an
 * empty string.
 *
 * \param[in] name  The name of the field of which the first line is to be read.
 *
 * \return The long value (i.e. the value without the first line) of the field.
 */
std::string field_file::get_field_long_value(const std::string& name) const
{
    const std::string value(get_field(name));
    const char *start(value.c_str());
    for(const char *s(start); *s != '\0'; ++s)
    {
        if(*s == '\r' || *s == '\n')
        {
            for(; *s == '\r' || *s == '\n'; ++s);
            return s;
        }
    }
    return "";
}


/** \brief Retrieve a field value as a list of items.
 *
 * This is a helper function that retrieves the value of a field as an
 * array of strings.
 *
 * Lists in fields are expected to be defined as "words" separated by
 * commas. This function has no intelligence. It does not understand
 * any concepts such as strings, parenthesis, etc. However, the value
 * of the field is retrieved using the get_field() function and thus
 * the field content may have variables and expressions which will
 * be transformed before the list is parsed.
 *
 * Note that the items are trimmed of white spaces on both sides.
 * Also, multiple commas in a row are ignored (it does not generate
 * empty entries.)
 *
 * \param[in] name  The name of the field of which the first line is to be read.
 *
 * \return List of items found in this field value.
 */
field_file::list_t field_file::get_field_list(const std::string& name) const
{
    list_t result;
    const std::string str(get_field(name));
    const char *s(str.c_str());
    for(;;)
    {
        // in effect this is a left trim
        while(isspace(*s) || *s == ',')
        {
            ++s;
        }
        if(*s == '\0')
        {
            break;
        }
        const char *start(s);
        const char *e(s);
        for(; *s != ',' && *s != '\0'; ++s)
        {
            if(!isspace(*s))
            {
                // in effect this is a right trim
                e = s;
            }
        }
        std::string r(start, e + 1 - start);
        result.push_back(r);
    }
    return result;
}


/** \brief Get the value of a field as a boolean value.
 *
 * This function checks the value of the named field and if it
 * has one of the following values, ignoring case, then the
 * function returns true, otherwise it returns false:
 *
 * \li Yes
 * \li True
 * \li On
 * \li 1
 *
 * (The value is checked case insensitively.)
 *
 * Note that in general Debian defines Boolean fields with only
 * Yes or No. Any other value is not allowed.
 *
 * \param[in] name  The name of the field of which the first line is to be read.
 *
 * \return true if the value of this field represents true.
 */
bool field_file::get_field_boolean(const std::string& name) const
{
    const case_insensitive::case_insensitive_string b(get_field(name));
    return b == "yes" || b == "true" || b == "on" || b == "1";
}


/** \brief Get the value of a field as an integer value.
 *
 * The get_integer_field() retrieves the value of the field with
 * the get_field() function and thus transforms the variables and
 * expressions first, then it transforms the value to an integer
 * and returns that number.
 *
 * \exception wpkg_field_exception_invalid
 * If the value does not represent a valid integer, then
 * wpkg_field_exception_invalid is raised. Note that if the
 * field does not exist and has an invalid variable or expression
 * reference then this exception is also raised.
 *
 * \param[in] name  The name of the field of which the first line is to be read.
 *
 * \return The value as an integer.
 */
long field_file::get_field_integer(const std::string& name) const
{
    unsigned long result(0);

    std::string value(get_field(name));
    const char *s(value.c_str());
    long sign(*s == '-' ? -1 : 1);
    if(*s == '-' || *s == '+')
    {
        ++s;
    }

    for(; *s != '\0'; ++s)
    {
        if(*s < '0' || *s > '9')
        {
            throw wpkg_field_exception_invalid("value of " + name + " field does not represent a valid integer (" + value + ")");
        }
        unsigned long n = result * 10 + *s - '0';
        if(n < result)
        {
            throw wpkg_field_exception_invalid("value of " + name + " field is too large and may not be a valid integer (" + value + ")");
        }
        result = n;
    }

    // TBD: should we generate an error if result > MAX_LONG
    return static_cast<long>(result) * sign;
}


/** \brief Return the number of fields.
 *
 * This function returns the number of fields that are found in the
 * field_file. Note that this is the total number of fields including
 * those read by the read() function and those added with the
 * set_field(). This number is useful to get all the field names
 * using the get_field_name() function.
 *
 * Note that if set_field() is called, the number of fields may change.
 *
 * \return The number of fields current defined in the field_file object.
 */
int field_file::number_of_fields() const
{
    return static_cast<int>(f_fields.size());
}


/** \brief Retrieve all the information about a field.
 *
 * This function searches for the named field and returns its field_t
 * instance. This allows you to emit errors and warnings with the
 * source filename and the line on which the field appeared.
 *
 * This function never returns a NULL pointer. Instead it throws if
 * the field is not defined.
 *
 * \exception 
 * Like the get_field() if the field does not exit, you get this
 * exception.
 *
 * \param[in] name  The name of the field to retrieve.
 */
std::shared_ptr<field_file::field_t> field_file::get_field_info(const std::string& name) const
{
    // find the field if available
    field_map_t::const_iterator it(f_fields.find(name));
    if(it == f_fields.end())
    {
        size_t p(name.find_last_of('/'));
        if(p != std::string::npos)
        {
            it = f_fields.find(name.substr(0, p));
        }
        if(it == f_fields.end())
        {
            // with or without the sub-package name we cannot find it
            throw wpkg_field_exception_undefined("get_field_info(): field \"" + name + "\" is undefined");
        }
    }

    return it->second;
}


/** \brief Retrieve the name of a given field.
 *
 * This function determines the name of a field from its index.
 *
 * The function cannot return an empty string since all fields must
 * have a non-empty name.
 *
 * \exception std::underflow_error
 * The underflow_error exception is raised if the \p idx parameter is
 * negative.
 *
 * \exception std::overflow_error
 * The overflow_error exception is raised if the \p idx parameter is
 * too large (larger or equal to the value returned by the number_of_fields()
 * function.)
 *
 * \param[in] idx  The index of the field for which you are interested.
 */
const std::string& field_file::get_field_name(int idx) const
{
    if(idx < 0)
    {
        throw std::underflow_error("index out of bounds (too small) to get a field name");
    }

    // f_fields is a map so we cannot just jump at the right place!
    field_map_t::const_iterator it(f_fields.begin());
    for(; idx > 0; --idx, ++it)
    {
        if(it == f_fields.end())
        {
            throw std::overflow_error("index out of bounds trying to get a field name");
        }
    }

    return it->first;
}


/** \brief Delete the named field.
 *
 * This function is used to delete a field from a field_file object.
 * Once deleted, the field is lost.
 *
 * \param[in] name  The name of the field to delete.
 *
 * \return Returned the value true if the field gets deleted.
 */
bool field_file::delete_field(const std::string& name)
{
    field_map_t::iterator it(f_fields.find(name));
    const bool result(it != f_fields.end());
    if(result)
    {
        f_fields.erase(it);
    }

    return result;
}


/** \brief Check whether a variable exists.
 *
 * This function can be used to determine whether a variable was
 * defined before attempting to retrieve it.
 *
 * Note that a variable can also be defined to include a sub-package
 * specification. In that case, the variable with the sub-package
 * specification is checked as well as the name without the sub-package
 * specification. If either exists, the function returns true.
 *
 * \param[in] name  The name of the variable to check.
 *
 * \return true if the variable is defined.
 */
bool field_file::variable_is_defined(const std::string& name) const
{
    // full name exists?
    if(f_variables.find(name) != f_variables.end())
    {
        return true;
    }

    // includes a sub-package specification?
    size_t p(name.find_last_of('/'));
    if(p != std::string::npos)
    {
        // name by itself exists?
        return f_variables.find(name.substr(0, p)) != f_variables.end();
    }

    return false;
}


/** \brief Check how many variables are defined in this field_file object.
 *
 * This function returns the number of variables found in the field_file.
 * Variables can be read by the read() function and added later by the
 * set_variable() functions.
 *
 * \return The number of variables defined in the field_file object.
 */
int field_file::number_of_variables() const
{
    return static_cast<int>(f_variables.size());
}


/** \brief Retrieve all the information about a field.
 *
 * This function searches for the named field and returns its field_t
 * instance. This allows you to emit errors and warnings with the
 * source filename and the line on which the field appeared.
 *
 * This function never returns a NULL pointer. Instead it throws if
 * the field is not defined.
 *
 * \exception 
 * Like the get_field() if the field does not exit, you get this
 * exception.
 *
 * \param[in] name  The name of the variable to retrieve.
 *
 * \return The pointer to the field object representing this variable.
 */
std::shared_ptr<field_file::field_t> field_file::get_variable_info(const std::string& name) const
{
    // find the variable if available
    field_map_t::const_iterator it(f_variables.find(name));
    if(it == f_variables.end())
    {
        size_t p(name.find_last_of('/'));
        if(p != std::string::npos)
        {
            it = f_variables.find(name.substr(0, p));
        }
        if(it == f_variables.end())
        {
            // with or without the sub-package name we cannot find it
            throw wpkg_field_exception_undefined("get_variable_info(): variable \"" + name + "\" is undefined");
        }
    }

    return it->second;
}


/** \brief Delete the named variable.
 *
 * The delete_variable() function searches for the named variable and if
 * it exists, deletes it.
 *
 * \param[in] name  The name of the variable to delete.
 *
 * \return true if the variable was deleted, false otherwise
 */
bool field_file::delete_variable(const std::string& name)
{
    field_map_t::iterator it(f_variables.find(name));
    const bool result(it != f_variables.end());
    if(result)
    {
        f_variables.erase(it);
    }

    return result;
}


/** \brief Set a variable.
 *
 * This function sets the variable defined in the \p field parameter.
 * The name of the variable is taken as the name of the field with
 * the get_name() function.
 *
 * If a variable with the same name already exists, its value gets
 * replaced.
 *
 * \param[in] field  The variable name and value to save as a variable.
 */
void field_file::set_variable(const std::shared_ptr<field_t> field)
{
    f_variables[field->get_name()] = field;
}


/** \brief Set a variable by name and value.
 *
 * This helper function is used to set a variable with just a name and
 * value. It will call the set_variable() with a field_t as parameter.
 * This means the filename and line remain undefined. Also the
 * function can be overwritten as it is virtual and that allows one
 * to verify each variable set in a field_file.
 *
 * \param[in] name  The name of the variable to set.
 * \param[in] value  The value of the new variable.
 */
void field_file::set_variable(const std::string& name, const std::string& value)
{
    std::shared_ptr<field_t> field(create_variable(name, value));
    set_variable(field);
}


/** \brief Retrieve a variable by name.
 *
 * This function returns the value of a variable.
 *
 * Note that a variable value is NOT transformed. So if it contains variables
 * or expressions, these will still look like what is found in the original
 * value for that variable.
 *
 * The name may include a sub-package specification. If so, the full name is
 * checked first, then the name without the sub-package specification. If
 * that exists, then that value is returned.
 *
 * \exception wpkg_field_exception_undefined
 * The wpkg_field_exception_undefined is raised if the variable is
 * undefined.
 *
 * \param[in] name  The name of the variable to retrieve.
 * \param[in] substitutions  If true, also check substitutions.
 *
 * \return The raw value of the variable.
 */
std::string field_file::get_variable(const std::string& name, const bool substitutions) const
{
    // substitutions, if used, have priority
    if(substitutions)
    {
        field_map_t::const_iterator subst(f_substitutions.find(name));
        if(subst != f_substitutions.end())
        {
            return subst->second->get_value();
        }
    }
    field_map_t::const_iterator it(f_variables.find(name));
    if(it == f_variables.end())
    {
        std::string::size_type p(name.find_last_of('/'));
        if(p != std::string::npos)
        {
            it = f_variables.find(name.substr(0, p));
        }
        if(it == f_variables.end())
        {
            // with or without the sub-package name or within the
            // substitutions we just cannot find it
            throw wpkg_field_exception_undefined("variable \"" + name + "\" is undefined");
        }
    }
    return it->second->get_value();
}


/** \brief Get the variable name of the variable at the specified index.
 *
 * This function retrieves the name of the variable at the specified index.
 * The index must be between 0 and the value returned by
 * number_of_variables() minus one.
 *
 * The function cannot return an empty string since all variables must
 * have a non-empty name.
 *
 * \exception std::underflow_error
 * std::underflow_error is raised if the index is negative.
 *
 * \exception std::overflow_error
 * std::overflow_error is raised if the index is too large, which means
 * larger or equal to the number returned by number_of_variables().
 *
 * \param[in] idx  The index of the name to be retrieved.
 *
 * \return The name of the variable which index was specified.
 */
const std::string& field_file::get_variable_name(int idx) const
{
    if(idx < 0)
    {
        throw std::underflow_error("index out of bounds");
    }

    // f_variables is a map so we cannot just jump at the right place!
    field_map_t::const_iterator it(f_variables.begin());
    for(; idx > 0; --idx, ++it)
    {
        if(it == f_variables.end())
        {
            throw std::overflow_error("index out of bounds");
        }
    }
    return it->first;
}


/** \brief Validate fields as defined by the expression.
 *
 * This function runs the expression and verify that it returns exactly
 * one (1). If so, then the function returns true.
 *
 * The expression must be valid and is expected to return a Boolean value.
 *
 * For details about the supported expressions, see the libexpr library.
 * It supports most of the C/C++ expressions and stings.
 *
 * An empty expression is considered valid and always returns the
 * value true.
 *
 * \param[in] expression  The expression used to validate the field file.
 *
 * \return true if the result of the expression is exactly 1.
 */
bool field_file::validate_fields(const std::string& expression)
{
    if(expression.empty())
    {
        return true;
    }

    check_fields evaluator(*this);
    libexpr::variable result;
    evaluator.eval(expression, result);

    // it has to be an integer
    long valid;
    result.get(valid);

    // TODO: error if valid != 0 && valid != 1 ?

    // valid if 1 (boolean true)
    return valid == 1;
}


/** \brief Request that variables defined in the field file be accessible directly.
 *
 * By default, variables defined in a field file are accessible using the
 * V modifier as in:
 *
 * \code
 * ${V:ROOT_TREE}
 * \endcode
 *
 * It is possible to change the behavior to not have to use the V modifier
 * by calling this function. After this call, the following is an equivalent:
 *
 * \code
 * ${ROOT_TREE}
 * \endcode
 *
 * Note that the ${V:...} is still working as before so both method to access
 * variables are still fully valid.
 *
 * This function should be called right after you create a field_file object
 * so the access to variables does not change en route.
 */
void field_file::auto_transform_variables()
{
    f_auto_transform_variables = true;
}


/** \brief Initialize an empty field.
 *
 * This constructor allows us to create an \em empty field used to
 * allow the use of the field in vectors and maps.
 *
 * Note that the f_field_file is NULL in this case. It should, however,
 * never get used until initialized with a valid pointer.
 */
field_file::field_t::field_t()
    //: f_field_file() -- auto-init
    //, f_name() -- auto-init
    //, f_value() -- auto-init
    //, f_filename() -- auto-init
    //, f_line() -- auto-init
{
}


/** \brief Initialize a field as the copy of another.
 *
 * This constructor copies the \p rhs field in this new field.
 * We copy fields in many occasions and having an explicit copy
 * constructor ease some operations.
 *
 * \param[in] rhs  The right hand side field to copy.
 */
field_file::field_t::field_t(const field_t& rhs)
    : f_field_file(rhs.f_field_file)
    , f_name(rhs.f_name)
    , f_value(rhs.f_value)
    , f_filename(rhs.f_filename)
    , f_line(rhs.f_line)
{
}


/** \brief Initialize a new field being read from a file.
 *
 * This constructor is specially designed to create fields as they are read
 * from a file. The filename and line parameters are expected to be set to
 * the name of the file where the field is being read from and the line it
 * was found on. Fields that span over multiple lines define the first line
 * on which the field definition starts.
 *
 * \note
 * It is important to note that the name of a field cannot be changed. This
 * is quite important because list of fields are saved in maps which would
 * have a broken index were we able to change their name under their nose.
 *
 * \param[in] name  The name of the field as it appears in the source file.
 * \param[in] value  The value of the field or variable.
 * \param[in] filename  The name of the file this field is being read from or an empty string.
 * \param[in] line  The line of the where the field was read from the file.
 */
field_file::field_t::field_t(const field_file& file, const std::string& name, const std::string& value)
    : f_field_file(const_cast<field_file *>(&file))
    , f_name(name)
    , f_value(value)
    //, f_filename() -- auto-init
    //, f_line() -- auto-init
{
    if(name.empty())
    {
        throw wpkg_field_exception_invalid("the name of a field cannot be the empty string.");
    }
}


/** \brief Define so virtual tables are created as expected.
 *
 * This destructor is defined to ensure valid virtual tables. At this time no
 * clean up is required for a field.
 */
field_file::field_t::~field_t()
{
}


/** \brief Retrieve the name of the field.
 *
 * This function retrieves the name of the field. The case of the name is
 * the same as the case found in the original file or in the library.
 *
 * \note
 * It is important to note that the name of a field cannot be changed. This
 * is quite important because list of fields are saved in maps which would
 * have a broken index were we able to change their name under their nose.
 *
 * \return The name of the field.
 */
std::string field_file::field_t::get_name() const
{
    return f_name;
}


/** \brief Check whether the name of the field includes a sub-package.
 *
 * A field name may include a sub-package specification. This function
 * determines whether there is, indeed, a sub-package specification.
 *
 * \return true if the name of this field includes a sub-package specification.
 */
bool field_file::field_t::has_sub_package_name() const
{
    return f_name.find_first_of('/') != std::string::npos;
}


/** \brief Get the sub-package name defined in this field.
 *
 * This function retrieves the name of the sub-package defined in this
 * field. If no sub-package name is defined in this field, then the
 * function returns an empty string. When a name is specified, it cannot
 * be an empty string.
 *
 * \return The name of the sub-package specified or the empty string.
 */
std::string field_file::field_t::get_sub_package_name() const
{
    std::string::size_type p(f_name.find_first_of('/'));
    if(p == std::string::npos)
    {
        // there is no sub-package specified in this field name
        return "";
    }

    // return the sub-package name
    return f_name.substr(p + 1);
}


/** \brief Get the field name without the sub-package specification.
 *
 * This function returns the field name only by removing the sub-package
 * specification if it is present.
 */
std::string field_file::field_t::get_field_name() const
{
    std::string::size_type p(f_name.find_first_of('/'));
    if(p == std::string::npos)
    {
        // there is no sub-package specified in this field name
        return f_name;
    }

    // return the field name
    return f_name.substr(0, p);
}


/** \brief Set the value of this field.
 *
 * This function lets you change the value of the field with the string
 * defined in the \p value parameter. The value can be the empty string
 * and it can include expressions and variable references. Note, however,
 * that a field used for a field_file variable does not itself support
 * expressions and variables (although if referenced from a field, then
 * its expressions and variables will be interpreted.)
 *
 * Note that the set_value() function is a virtual function. If you define
 * a field of a specialized type, you may additionally check the value
 * for validity within the derived version of this function. Invalid field
 * values must raise an exception.
 *
 * Any previous value is overwritten by this call.
 *
 * \exception std::exception
 * An exception derived from std::exception is raised if the value is not
 * valid for this field. For example, setting the "Package" field of a
 * control file to "!My Package!" is not valid and raises an exception.
 * Note that the value of the field is restored to its original value if
 * an exception is raised.
 *
 * \param[in] value  The new value for this field.
 */
void field_file::field_t::set_value(const std::string& value)
{
    std::string original(f_value);
    f_value = value;

    try
    {
        verify_value();
    }
    catch(const std::exception&)
    {
        // fulfill the contract, restore a valid value
        f_value = original;
        throw;
    }
}


/** \brief Verify the value of this field.
 *
 * This function is used to validate the fields found in a field_file. By
 * default fields are not immediately validated when read from a file. They
 * get validated at the end of the read(). This is important because the
 * fields may reference each others through variables ${F:\<fieldname>}
 * and expressions $(get_field("\<fieldname>")) and thus all fields need to
 * be loaded to make sure that it works as expected.
 *
 * Note that the transformed field values do not get cached because if they
 * reference a field that changes over time the cached value could then be
 * rendered invalid.
 *
 * The default verification function verifies that the value as a whole is
 * valid for a control file.
 *
 * \todo
 * Test that the value does not include any entry that starts with a
 * period, a space, and some other characters as this is forbidden
 * on continuation lines (these are reserved by Debian).
 */
void field_file::field_t::verify_value() const
{
}


/** \brief Retrieve the current value of this field.
 *
 * This function returns the raw value of the field. This value may
 * include untransformed variables and expressions. You should instead
 * get fields via the field_file interface.
 *
 * \return A copy of the current field value.
 */
std::string field_file::field_t::get_value() const
{
    return f_value;
}


/** \brief Retrieve the current value of this field.
 *
 * This function returns the raw value of the field. This value may
 * include untransformed variables and expressions. You should instead
 * get fields via the field_file interface.
 *
 * \return A copy of the current field value.
 */
std::string field_file::field_t::get_transformed_value() const
{
    std::string result(f_value);
    if(result.find('$') != std::string::npos)
    {
        f_field_file->transform_dynamic_variables(this, result);
    }
    return result;
}


/** \brief Set the filename where the field is being read from.
 *
 * This function is used to attach a filename to the field. This is
 * particularly useful to generate an error message referencing the
 * file and line number where the error came from.
 *
 * The filename should be defined when available.
 *
 * Note that the set_field() function of the field_file class does
 * overwrite this parameter with the new field information.
 *
 * \param[in] filename  The name of the file the field was read from.
 */
void field_file::field_t::set_filename(const std::string& filename)
{
    f_filename = filename;
}


/** \brief Retrieve the filename where this field was found.
 *
 * This function returns the name of the file this field was read from with
 * the field_file::read() function.
 *
 * The return value may be the empty string if the field was instead created
 * by the system in some way.
 *
 * \return The name of the file from which the field was read.
 */
std::string field_file::field_t::get_filename() const
{
    return f_filename;
}


/** \brief Save the line number where the field was found.
 *
 * This function records the line number where the field was read from. This
 * line is used later to print out errors about the field.
 *
 * \param[in] line  The line where the field was read from.
 */
void field_file::field_t::set_line(int line)
{
    f_line = line;
}


/** \brief Retrieve the line where the field was found.
 *
 * This function retrieves the line where the field was read from in a source
 * field_file. This is particularly useful to print out errors about the
 * field. We use this in our new version of the code as it may generate errors
 * at a much later time than when loading the field.
 *
 * If the value is zero (0) then the line number is considered unspecified.
 *
 * \return The line number at which the field was read from the source file.
 */
int field_file::field_t::get_line() const
{
    return f_line;
}


/** \brief Compare two fields against each others.
 *
 * Fields can be sorted using this operator. The sorting applies to the
 * name of the field in a case insensitive manner.
 *
 * The value of fields is ignored in this test.
 *
 * \return true if this is smaller than rhs, false otherwise.
 */
bool field_file::field_t::operator < (const field_t& rhs) const
{
    return strcasecmp(f_name.c_str(), rhs.f_name.c_str()) < 0;
}


/** \brief Check whether two fields is considered equal.
 *
 * This operator compares the name of this field and the \p rhs field
 * in a case insensitive manner. If the function returns true, then
 * both fields are considered \em equal.
 *
 * Note that fields that are considered equal overwrite each others in
 * the map of fields that the field_file keeps track of.
 *
 * \param[in] rhs  The right hand side field.
 *
 * \return true if this and rhs both have the same field name.
 */
bool field_file::field_t::operator == (const field_t& rhs) const
{
    return strcasecmp(f_name.c_str(), rhs.f_name.c_str()) == 0;
}


/** \brief Copy rhs in this field.
 *
 * The assignment operator copies the \p rhs field in this field.
 * At this point all the fields are copied no matter what.
 *
 * \param[in] rhs  The right hand side field.
 *
 * \return A reference to this field.
 */
field_file::field_t& field_file::field_t::operator = (const field_t& rhs)
{
    // copy only if not self
    if(this != &rhs)
    {
        f_field_file = rhs.f_field_file;
        f_name       = rhs.f_name;
        f_value      = rhs.f_value;
        f_filename   = rhs.f_filename;
        f_line       = rhs.f_line;
    }

    return *this;
}


/** \brief The field default constructor.
 *
 * Field factories are created without any parameters since they are
 * expected to be created when the library is initialized (automatically.)
 * This allows us to automatically register those field factories within a
 * table (see the wpkg_control::control_file::register_field() function for
 * details and an example.)
 */
field_file::field_factory_t::field_factory_t()
{
}


/** \brief The field factory destructor.
 *
 * This destructor is here to ensure a perfect virtual tables.
 */
field_file::field_factory_t::~field_factory_t()
{
}


/** \brief A list of names that are equivalent to this field's name.
 *
 * This function returns an empty list by default. Different fields may
 * be given multiple names in which case it returns equivalents. This allows
 * the user to use any one of those names to create a field of that type.
 * In the output, though, only the canonicalized name is used.
 *
 * So, for example, we want wpkg to understands the two names:
 *
 * \code
 * X-Status
 * wpkg-Status
 * \endcode
 *
 * Because in older versions it would use X-Status and in newer versions we
 * decided to switch to wpkg-Status. If possible, the current version of the
 * packager, the version of the package, and the standards version may be
 * used to clearly define the list of equivalents.
 *
 * \return A list of equivalent names for this field.
 */
field_file::field_factory_t::name_list_t field_file::field_factory_t::equivalents() const
{
    name_list_t empty;
    return empty;
}




}   // namespace wpkg_field

// vim: ts=4 sw=4 et
