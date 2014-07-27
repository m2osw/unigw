/*    wpkg_dependencies.cpp -- implementation of the control file handling
 *    Copyright (C) 2012-2014  Made to Order Software Corporation
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
 * \brief The implementation of the dependency fields in a control file.
 *
 * The dependency fields in a control file are numerous (Depends,
 * Build-Depends, Conflicts, etc.) and they all support the same format:
 * a list of dependencies which are composed of a package name, an optional
 * version preceded by an optional operator, and an optional architecture
 * constraint.
 *
 * The implementation is used to parse any one of those fields, transform
 * the textual data in an array of objects that can easily be used by
 * our software, and a way to write what we read back to a control file.
 */
#include    "libdebpackages/wpkg_dependencies.h"
#include    "libdebpackages/wpkg_architecture.h"
#include    "libdebpackages/wpkg_util.h"
#include    "libdebpackages/debian_version.h"


/** \brief The wpkg_dependencies namespace declares and implements dependency related functions.
 *
 * This namespace includes all the functions used to handle dependency fields.
 * It is used by the wpkg_control classes to handle fields such as the
 * Depends and Conflicts fields.
 */
namespace wpkg_dependencies
{


/** \class dependencies
 * \brief Class to parse and manage dependencies.
 *
 * This class is capable of reading a dependency field and spit it back
 * out in its entirety or just for a specific architecture.
 */


/** \struct dependencies::dependency_t
 * \brief The structure memorizing one dependency entry.
 *
 * This structure is the one used to memorize one dependency. It includes
 * all the necessary information as can be found in the input file:
 *
 * \li Package Name
 * \li Package Version
 * \li Version Comparison Operator
 * \li Whether this is AND or an OR in the list of dependencies
 * \li Whether the architecture is used as a positive or negative match.
 * \li Architecture
 */


/** \class wpkg_dependencies_exception
 * \brief The base class of all the dependencies class exceptions.
 *
 * This class is used as the base exception of all the dependency
 * exceptions. It is not itself raised, but it can be used to
 * catch any one dependency exception.
 */


/** \class wpkg_dependencies_exception_invalid
 * \brief The exception raised when something invalid is found.
 *
 * This exception is raised whenever an invalid entry is found in a
 * dependency list. For example, if you try to use the != operator
 * which is not valid in dependency lists, this exception is
 * raised.
 */


/** \brief Transform a dependency operator to a string.
 *
 * This function transforms the operator of a dependency into a string.
 * This is used to save canonicalized dependencies back to a file.
 *
 * This function returns an empty string instead of the ">=" operator
 * since this is the default. This allows us to canonicalize entries
 * such as:
 *
 * \code
 * wpkg (>= 0.8.0)
 * \endcode
 *
 * Into a string that has no operator:
 *
 * \code
 * wpkg (0.8.0)
 * \endcode
 *
 * \exception wpkg_dependencies_exception_invalid
 * This exception should never be raised. It happens only if the dependency
 * object was assigned an invalid operator which should never happen.
 *
 * \return The string representing the operator.
 */
std::string dependencies::dependency_t::operator_to_string() const
{
    switch(f_operator)
    {
    case operator_lt:
        return "<<";

    case operator_le:
        return "<=";

    case operator_eq:
        return "=";

    case operator_ge:
        // not required, this is the default
        return "";

    case operator_gt:
        return ">>";

    case operator_ne:
        throw wpkg_dependencies_exception_invalid("unexpected operator \"ne\" for a dependency in operator_to_string()");

    case operator_any:
        throw wpkg_dependencies_exception_invalid("unexpected operator \"any\" for a dependency in operator_to_string()");

    default:
        throw wpkg_dependencies_exception_invalid("unexpected operator for a dependency in operator_to_string()");

    }
}


/** \brief Transform a dependency object into a string.
 *
 * This function transform this dependency into a canonicalized string.
 * When the \p remove_arch parameter is set to true, it further removes
 * the the architecture specifications. Note, however, that this function
 * still returns a string even if the architecture does not match the
 * architecture for which wpkg was compiled.
 *
 * Note that the result is canonicalized.
 *
 * \param[in] remove_arch  Whether to keep the architectures (false) or to remove them (true).
 *
 * \return The dependency in the form of a string.
 */
std::string dependencies::dependency_t::to_string(bool remove_arch) const
{
    // TODO: cache the result -- however to do that safely we want to
    //       also have full control of all the variable members
    std::string result(f_name);
    if(!f_version.empty())
    {
        result += " (";
        std::string op(operator_to_string());
        if(op.length() > 0)
        {
            result += op + " ";
        }
        result += f_version;
        result += ')';
    }
    if(!remove_arch && !f_architectures.empty())
    {
        result += " [";
        for(std::vector<std::string>::size_type j(0); j < f_architectures.size(); ++j)
        {
            if(j != 0)
            {
                result += ' ';
            }
            if(f_not_arch)
            {
                result += '!';
            }
            result += f_architectures[j];
        }
        result += ']';
    }
    return result;
}


/** \brief Parse a list of dependencies and assign them to a dependencies object.
 *
 * This constructor parses a list of dependencies with a package name, an optional
 * version with an operator, and an optional list of architectures.
 *
 * If a dependency is invalid (bad version, unknown architecture, etc.) then the
 * function throws an error since that string cannot be represented properly in
 * this dependencies object.
 */
dependencies::dependencies(const std::string& dependency_field)
    //: f_dependencies -- auto-init
{
    // TBD: allow for multiple commas in a row? (wpkg, , , libz)
    const char *s(dependency_field.c_str()); 
    for(;;)
    {
        while(isspace(*s))
        {
            ++s;
        }
        const char *start(s);
        // skip the name
        while(*s != '\0' && *s != '|' && *s != ',' && *s != '(' && *s != '[' && !isspace(*s))
        {
            ++s;
        }
        if(s - start == 0)
        {
            // the field could be completely empty (only spaces)
            // this means we accept a comma or pipe at the end even
            // if not followed by another entry
            if(*s == '\0')
            {
                break;
            }
            throw wpkg_dependencies_exception_invalid("invalid dependency name (empty)");
        }
        dependency_t d;
        d.f_name.assign(start, s - start);
        if(!wpkg_util::is_package_name(d.f_name))
        {
            throw wpkg_dependencies_exception_invalid("a dependency package name is not valid");
        }
        // skip spaces
        while(isspace(*s))
        {
            ++s;
        }
        // has version specifications?
        if(*s == '(')
        {
            ++s; // skip '('

            // get the version
            // default operator when there is a version
            // TODO: controlled_vars bad enum handling
            d.f_operator = static_cast<int>(operator_ge);
            if((*s == '!' && s[1] == '=')
            || (*s == '<' && s[1] == '>'))
            {
                throw wpkg_dependencies_exception_invalid("'not equal' (!= or <>) as a dependency relationship operator is not acceptable");
            }
            if(*s == '<')
            {
                ++s;
                if(*s == '<')
                {
                    ++s;
                    // TODO: controlled_vars bad enum handling
                    d.f_operator = static_cast<int>(operator_lt);
                }
                else if(*s == '=')
                {
                    ++s;
                    // TODO: controlled_vars bad enum handling
                    d.f_operator = static_cast<int>(operator_le);
                }
                else
                {
                    // NOTE: dpkg accepts '<' by itself as an equivalent of '<='
                    throw wpkg_dependencies_exception_invalid("invalid dependency relationship operator ('<' by itself is not accepted by wpkg, use '<=' instead)");
                }
            }
            else if(*s == '>')
            {
                ++s;
                if(*s == '>')
                {
                    ++s;
                    // TODO: controlled_vars bad enum handling
                    d.f_operator = static_cast<int>(operator_gt);
                }
                else if(*s == '=')
                {
                    ++s;
                    // TODO: controlled_vars bad enum handling
                    // This is the default when not specified
                    d.f_operator = static_cast<int>(operator_ge);
                }
                else
                {
                    // NOTE: dpkg accepts '>' by itself as an equivalent of '>='
                    throw wpkg_dependencies_exception_invalid("invalid dependency relationship operator ('>' by itself is not accepted by wpkg, use '>=' instead)");
                }
            }
            else if(*s == '=')
            {
                ++s;
                // TODO: controlled_vars bad enum handling
                d.f_operator = static_cast<int>(operator_eq);
            }
            // operator is not mandatory (ge by default)
            // skip spaces after operator
            while(isspace(*s))
            {
                ++s;
            }
            start = s;
            while(
                (*s >= '0' && *s <= '9')
             || (*s >= 'a' && *s <= 'z')
             || (*s >= 'A' && *s <= 'Z')
             || *s == '.' || *s == '~' || *s == '+' || *s == '-' || *s == ':'
            )
            {
                ++s;
            }
            d.f_version.assign(start, s - start);
            char err[256];
            if(!validate_debian_version(d.f_version.c_str(), err, sizeof(err)))
            {
                throw wpkg_dependencies_exception_invalid("invalid dependency version");
            }
            while(isspace(*s))
            {
                ++s;
            }
            if(*s != ')')
            {
                if(*s == ',' || *s == '|' || *s == '\0')
                {
                    throw wpkg_dependencies_exception_invalid("invalid dependency version: missing ')'");
                }
                throw wpkg_dependencies_exception_invalid("invalid dependency version string");
            }
            ++s; // skip ')'
        }
        else
        {
            // TODO: controlled_vars bad handling of enums
            d.f_operator = static_cast<int>(operator_any);
            d.f_version = ""; // not specified by default
        }

        while(isspace(*s))
        {
            ++s;
        }
        // next check for architecture specifications
        if(*s == '[')
        {
            ++s; // skip '['

            // the architectures are separated by spaces and eventually
            // start with '!' (not)
            while(isspace(*s))
            {
                ++s;
            }
            // the first one defines the flag
            d.f_not_arch = *s == '!';
            for(;;)
            {
                if(d.f_not_arch)
                {
                    ++s; // skip '!'
                }
                const char *arch(s);
                while(*s != '\0' && *s != ']' && !isspace(*s))
                {
                    ++s;
                }
                if(s == arch)
                {
                    throw wpkg_dependencies_exception_invalid("invalid architecture specification in control file");
                }
                std::string arch_name(arch, s - arch);
                wpkg_architecture::architecture arch_pattern(arch_name);
                //if(!valid_architecture_pattern(arch_name))
                //{
                //    throw wpkg_dependencies_exception_invalid("invalid architecture pattern specification in control file dependencies");
                //}
                d.f_architectures.push_back(arch_name);
                while(isspace(*s))
                {
                    ++s;
                }
                if(*s == ']')
                {
                    ++s; // skip ']'
                    break;
                }
                if((*s == '!') ^ d.f_not_arch)
                {
                    throw wpkg_dependencies_exception_invalid("when specifying architectures using the not (!) operator, either all or none of the entries use the not operator, a mix is not acceptable.");
                }
            }
        }

        // move the pointer to the next dependency name
        while(isspace(*s))
        {
            ++s;
        }
        if(*s == ',')
        {
            ++s; // skip ','
        }
        else if(*s == '|')
        {
            d.f_or = true; // this one or the next
            ++s; // skip '|'
        }
        else if(*s != '\0')
        {
            throw wpkg_dependencies_exception_invalid("invalid dependency list, comma (,) or end of list expected");
        }

        // make a copy in our table
        f_dependencies.push_back(d);
    }
}


/** \brief Check how many dependencies are defined in this object.
 *
 * This function returns the number of dependencies defined in this object.
 * It is useful to get dependencies with the get_dependency() function.
 *
 * Note that zero is a valid result (i.e. an empty list.)
 *
 * \return The number of dependencies.
 */
int dependencies::size() const
{
    return static_cast<int>(f_dependencies.size());
}


/** \brief Retrieve one dependency.
 *
 * This function allows you to retrieve one dependency from the list of
 * dependencies.
 *
 * The index must be between zero (0) and size() minus one. If size() is
 * zero, then this function cannot be called.
 *
 * \param[in] idx  The index of the dependency to retrieve.
 *
 * \return A reference to one of the dependencies in this object.
 */
const dependencies::dependency_t& dependencies::get_dependency(int idx)
{
    return f_dependencies[idx];
}


/** \brief Delete a dependency.
 *
 * This function can be used to delete a dependency from the list
 * of dependencies.
 *
 * In most cases this is used to delete dependencies that do not apply. In
 * most cases, these are the dependencies that have an incompatible
 * architecture.
 *
 * \warning
 * Obviously, if you held a reference to a dependency as returned by
 * the get_dependency() function, it is rendered invalid by this
 * call.
 *
 * \param[in] idx  The index of the dependency to delete.
 */
void dependencies::delete_dependency(int idx)
{
    if(idx < 0)
    {
        throw std::underflow_error("index out of bounds to delete a dependency (too small)");
    }
    if(idx >= static_cast<int>(f_dependencies.size()))
    {
        throw std::overflow_error("index out of bounds to delete a dependency (too large)");
    }
    f_dependencies.erase(f_dependencies.begin() + idx);
}


/** \brief Transform the list of dependencies to a string.
 *
 * This function transform the list of dependencies back to a string. The
 * result is a canonicalized version of the list of dependencies. Plus you
 * may request the function to remove the architecture specifications,
 * which wpkg does when creating binary packages.
 *
 * Note that the architecture parameter is generally the same as the
 * architecture of the running wpkg tool (i.e. what you get when you use
 * wpkg --architecture).
 *
 * Dependencies without architecture specifications are always a match.
 *
 * \param[in] architecture  The architecture that needs to (not) match in order to keep the dependency in the output.
 * \param[in] remove_arch  Whether the architecture specifications should be removed in the final output.
 */
std::string dependencies::to_string(const std::string& architecture, bool remove_arch) const
{
    std::string result;
    for(dependency_vector_t::size_type idx(0); idx < f_dependencies.size(); ++idx)
    {
        // architecture specific dependency?
        if(!f_dependencies[idx].f_architectures.empty())
        {
            bool skip(false);
            if(f_dependencies[idx].f_not_arch)
            {
                for(std::vector<std::string>::size_type j(0); j < f_dependencies[idx].f_architectures.size(); ++j)
                {
                    if(match_architectures(architecture, f_dependencies[idx].f_architectures[j]))
                    {
                        skip = true;
                        break;
                    }
                }
            }
            else
            {
                skip = true;
                for(std::vector<std::string>::size_type j(0); j < f_dependencies[idx].f_architectures.size(); ++j)
                {
                    if(match_architectures(architecture, f_dependencies[idx].f_architectures[j]))
                    {
                        skip = false;
                        break;
                    }
                }
            }
            if(skip)
            {
                // architecture did not match, ignore that dependency
                continue;
            }
        }
        if(result.length() != 0)
        {
            if(f_dependencies[idx - 1].f_or)
            {
                result += " | ";
            }
            else
            {
                result += ", ";
            }
        }
        result += f_dependencies[idx].to_string(remove_arch);
    }

    return result;
}


bool dependencies::is_architecture_valid(const std::string& architecture)
{
    wpkg_architecture::architecture arch;
    if(!arch.set(architecture))
    {
        return false;
    }
    return !arch.is_pattern() && !arch.empty();
    //return valid_architecture(architecture);
}


bool dependencies::match_architectures(const std::string& architecture, const std::string& pattern, const bool ignore_vendor_field)
{
    // if pattern is any-any-any, any-any, or any then it match anything
    // if pattern is exactly the same as architecture then it is a match too
    if(pattern == "any-any-any" || pattern == "any-any" || pattern == "any" || pattern == architecture)
    {
        return true;
    }

    // more complicated test with more complicated patterns
    wpkg_architecture::architecture arch(architecture, ignore_vendor_field);
    wpkg_architecture::architecture pat(pattern, ignore_vendor_field);
    return arch == pat;
}


}   // namespace wpkg_dependencies
// vim: ts=4 sw=4 et
