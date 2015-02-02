/*    wpkg_changelog.cpp -- implementation of the changelog file format
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
 * \brief Implementation of the changelog file parser.
 *
 * This file includes the functions used to read your changelog files. These
 * are used when building a package. The result is generally a set of objects
 * that describe the different versions of your software.
 *
 * This is particularly useful when creating a source package and compiling
 * a source package:
 *
 * \code
 * cd <project directory>
 * wpkg --build
 * wkpg --build <package name>-src.deb
 * \endcode
 */
#include    "libdebpackages/wpkg_changelog.h"
#include    "libdebpackages/wpkg_output.h"
#include    "libdebpackages/wpkg_util.h"
#include    "libdebpackages/debian_version.h"
#include    "libdebpackages/compatibility.h"


/** \brief The changelog namespace.
 *
 * This namespace encompasses the declarations and implementation of
 * the changelog parser and manager.
 */
namespace wpkg_changelog
{


/** \class wpkg_changelog_exception
 * \brief The base exception class used by the changelog class.
 *
 * This class is the base class of the changelog class. It is not itself
 * thrown, however, it can be caught to catch any one of the changelog
 * exceptions.
 *
 * \note
 * At this point the changelog_file class does not raise any exceptions.
 * This is kept here in case you want to use a try/catch since at some
 * point the class may start throwing on different errors.
 *
 * \par
 * Note that although this class doesn't throw, the loading of the file
 * can generate exceptions because of invalid I/O (file does not exist...)
 */


/** \class changelog_file
 * \brief The changelog File class is used to manage changelog files.
 *
 * This class handles the loading of a changelog file and allows
 * access to the data in the changelog:
 *
 * \li Name of the package,
 * \li Different versions of the package,
 * \li The list of distributions the package is from,
 * \li Different parameters, of which only urgency is currently supported,
 * \li A list of changes grouped as defined by empty lines in the list
 *     of change logs,
 * \li A list of bug fixes (bug \em numbers) when available,
 * \li The name of the person responsible for the binary packages,
 * \li The email of the person responsible for the binary packages,
 * \li The date when that changelog was started.
 */


/** \class wpkg_changelog::changelog_file::state
 * \brief The state class handles the current state of the input.
 *
 * The state class is used to handle the input offering functions to
 * read one line of text, know how many spaces there were in the line 
 * and a way to go back to the beginning of the line if necessary.
 *
 * This class is used by the parser to handle the input text file.
 */


/** \class wpkg_changelog::changelog_file::version
 * \brief The version class to memorize the logs of one version of the project.
 *
 * This class is used to memorize all the parameters found in one version of
 * the project. This is one section that starts with the name of the package
 * in the first column to the next section that starts with the name of the
 * package in the first column.
 *
 * So it includes everything for one specific version of the project hence
 * its name:
 *
 * \li Package name
 * \li Version
 * \li List of distribution names
 * \li List of parameters (although we really only support the urgency
 *     parameter)
 * \li Name and email address of the maintainer
 * \li The time when this version was started
 * \li A list of logs defined in groups
 *
 * The logs have their own class since each log may be a group header
 * and it may include a bug \em number.
 */


/** \class wpkg_changelog::changelog_file::version::log
 * \brief Define one line of log.
 *
 * This class is used to memorize one line of log. It includes the log itself
 * with all the newlines removed (i.e. on lone line) and the bug \em number
 * when defined.
 *
 * Also, if the log entry appears after an empty line, or is the very first
 * entry, it is marked as the group header.
 *
 * The log also includes information about the changelog file name and the
 * line on which the log was read.
 */


/** \brief Initialize the state so several parse() function can run.
 *
 * This function initializes the read state so we can use any one
 * of the parse() function to read data from the source.
 *
 * \param[in] input  The input memory file to read from.
 */
changelog_file::state::state(const memfile::memory_file& input)
    : f_input(input)
    //, f_last_line("") -- auto-init
    //, f_space_count(0) -- auto-init
    , f_offset(0)
    //, f_previous_offset(0) -- auto-init
    //, f_line(0) -- auto-init
{
}


/** \brief Read one line of data from the input file.
 *
 * This function reads one line of data from the input file. The data
 * read is saved in the \p str parameter.
 *
 * \return true if a line was read, false once the end of the file is reached.
 */
bool changelog_file::state::next_line()
{
    f_has_empty_line = false;
    f_previous_offset = f_offset;
    for(;;)
    {
        if(!f_input.read_line(f_offset, f_last_line))
        {
            f_last_line.clear();
            break;
        }
        ++f_line;
        f_space_count = 0;
        const char *s(f_last_line.c_str());
        for(; isspace(*s); ++s)
        {
            ++f_space_count;
        }
        if(*s != '\0')
        {
            // remove the leading spaces for "faster" processing later
            f_last_line = f_last_line.substr(f_space_count);
            return true;
        }
        // silently ignore empty lines; but mark the fact because that breaks
        // a log entry (creating a new group)
        f_has_empty_line = true;
    }

    // reached the end
    f_last_line.clear();
    return false;
}


/** \brief Retrieve the last line that was read.
 *
 * This function retrieves the last line that was read. The parser starts
 * by reading a line and then it allows the system to check whether the
 * line matches the next possible entry, if so, return it.
 *
 * \return The last line read from the input file.
 */
const std::string& changelog_file::state::last_line() const
{
    return f_last_line;
}


/** \brief Retrieve the number of spaces on that line.
 *
 * This function returns the number of spaces found on this line.
 * The line is then saved without the leading spaces so it is important
 * to call this function to know how many spaces there were.
 *
 * \return The number leading spaces found on this line.
 */
int changelog_file::state::space_count() const
{
    return f_space_count;
}


/** \brief Restore the last line that was read.
 *
 * This function returns the cursor in the input file to the beginning of
 * the last line that was read. We use this once we are done reading a log
 * to go back to the previous line so we can continue parsing as expected.
 */
void changelog_file::state::restore()
{
    f_offset = f_previous_offset;
}


/** \brief Tell whether an empty line was found.
 *
 * When you call the next_line() function, it returns the next non-empty
 * line. This function lets you know that one or more empty lines were
 * found along the way. Empty lines break the paragraph of a log so it
 * is important to check while we are parsing a log entry.
 *
 * \return true if one or more empty lines were skipped before we found
 *         the next line.
 *
 * \sa next_line()
 */
bool changelog_file::state::has_empty_line() const
{
    return f_has_empty_line;
}


/** \brief Retrieve the filename of the input file.
 *
 * This function is generally used to print errors about this changelog
 * file. It is useful so users have a better idea of what file we're
 * talking about.
 *
 * \return The filename of the input file.
 */
wpkg_filename::uri_filename changelog_file::state::get_filename() const
{
    return f_input.get_filename();
}


/** \brief Retrieve the line number in the source we're reading from.
 *
 * Get the current line number of the last line we've read from the source
 * file.
 *
 * \return The current line number.
 */
int changelog_file::state::get_line() const
{
    return f_line;
}


/** \brief Parse one line of log.
 *
 * This function reads one line of log. Note that one line of log may appear
 * on multiple lines in the changelog file. One line ends when there is an
 * empty line, a line that does not start with at least 2 spaces, a line
 * that starts with an asterisk after the 2 spaces.
 *
 * \param[in] s  The state describing the input parameters.
 * \param[in] group  Whether this log is a group entry.
 *
 * \return true if the end of the file was not yet reached.
 */
bool changelog_file::version::log::parse(state& s, bool& group)
{
    f_filename = s.get_filename();
    f_line = s.get_line();

    f_is_group = group;
    group = false;

    if(s.space_count() != 2)
    {
        // invalid log entry; must start with 2 spaces
        return false;
    }
    std::string log_line(s.last_line());
    if(log_line[0] != '*')
    {
        // a new log entry must starts with an asterisk
        wpkg_output::log("changelog:%1:%2: a changelog log entry must start with an asterisk")
                .arg(f_filename)
                .arg(f_line)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_package)
            .action("changelog");
        return false;
    }

    for(;;)
    {
        // right trim the f_log (it automatically is left trimmed)
        std::string::size_type p(f_log.find_last_not_of(" \t\n\r\v\f"));
        if(p != std::string::npos)
        {
            f_log = f_log.substr(0, p + 1);
        }

        // check whether there is more data that should be added to the log
        // line; if not, leave it there and return, it should be an empty line
        // a new log, or the maintainer information.
        if(!s.next_line())
        {
            break;
        }
        if(s.has_empty_line())
        {
            // we bumped in an empty line, we are starting a new group.
            group = true;
            break;
        }
        // really we should have exactly 4 spaces...
        if(s.space_count() < 2)
        {
            break;
        }
        std::string new_line(s.last_line());
        if(new_line[0] == '*')
        {
            break;
        }
        f_log += " ";
        f_log += new_line;
    }

    f_log = log_line;

    // TODO: find the bugs stuff

    return true;
}


/** \brief Return whether this log entry is at the start of a group.
 *
 * The person who enters the logs may add empty lines between groups of
 * logs. The empty lines get removed from the list of logs, but the first
 * entry after a set of empty lines is marked as a "group" log.
 *
 * \return Whether this log entry is the first of a group of log entries.
 */
bool changelog_file::version::log::is_group() const
{
    return f_is_group;
}


/** \brief Return the log entry as is.
 *
 * This function returns the log entry as it was read, although it does
 * not include the asterisk character and it will be trimmed. Also, logs
 * written on multiple lines are re-written on a single long line.
 *
 * Writing the log back out will automatically wrap the log information.
 *
 * The bug information is also kept as is in the log string.
 *
 * \return This log entry.
 */
std::string changelog_file::version::log::get_log() const
{
    return f_log;
}


/** \brief Return the bug information of that entry.
 *
 * Different entries may have bug information specified. The Debian changelog
 * format only allows for one syntax:
 *
 * \code
 *   * blah ... blah Closes: #123
 * \endcode
 *
 * where the bug number is 123.
 *
 * We also support an introducer that ends with a colon as in:
 *
 * \code
 *   * USYS-123: blah ... blah
 * \endcode
 *
 * In our case the bug information is expected to not include any spaces
 * and end with the colon. If the entry is only letters then we generate
 * an error because we assume that it is not a bug \e number.
 *
 * Note that we do not automate anything such as closing the corresponding
 * bug in some tracking system.
 *
 * \return The bug information, not unlikely the empty string.
 */
std::string changelog_file::version::log::get_bug() const
{
    return f_bug;
}


/** \brief Retrieve the filename of the input file.
 *
 * This function is generally used to print errors about this changelog
 * file. It is useful so users have a better idea of what file we're
 * talking about.
 *
 * \return The filename of the input file.
 */
wpkg_filename::uri_filename changelog_file::version::log::get_filename() const
{
    return f_filename;
}


/** \brief Retrieve the line number in the source we're reading from.
 *
 * Get the current line number of the last line we've read from the source
 * file.
 *
 * \return The current line number.
 */
int changelog_file::version::log::get_line() const
{
    return f_line;
}


/** \brief Parse a line from a version entry.
 *
 * This function reads the header (one line), a list of logs, and then
 * the footer of a version entry.
 *
 * If a version exists, even if it is wrong, the function returns true.
 * The function returns false if the end of the file is reached.
 *
 * \param[in] s  The state to read from.
 *
 * \return true if the end of the file was not yet reached when this
 *         version was read in full.
 */
bool changelog_file::version::parse(state& s)
{
    f_filename = s.get_filename();
    f_line = s.get_line();

    // the current line must be the header
    if(s.space_count() != 0)
    {
        wpkg_output::log("changelog:%1:%2: a changelog version entry must start with a valid header")
                .arg(f_filename)
                .arg(f_line)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_package)
            .action("changelog");
        return s.next_line();
    }

    bool good_header(true);
    const std::string& header(s.last_line());
    const char *h(header.c_str());
    const char *start(h);

    // *** Package Name ***
    for(; !isspace(*h) && *h != '('; ++h)
    {
        if(*h == '\0')
        {
            wpkg_output::log("changelog:%1:%2: invalid header, expected the project name, version, distributions, and urgency information")
                    .arg(f_filename)
                    .arg(f_line)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .action("changelog");
            good_header = false;
            break;
        }
    }
    if(good_header)
    {
        std::string package_name(start, h - start);
        if(!wpkg_util::is_package_name(package_name))
        {
            wpkg_output::log("changelog:%1:%2: the package name %3 is not valid")
                    .arg(f_filename)
                    .arg(f_line)
                    .quoted_arg(package_name)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .action("changelog");
        }
        else
        {
            f_package = package_name;
        }

        if(!isspace(*h))
        {
            // this is just a warning, but the user is expected to put a space
            // after the package name and before the version
            wpkg_output::log("changelog:%1:%2: the package name %3 is not followed by a space before the version information")
                    .arg(f_filename)
                    .arg(f_line)
                    .quoted_arg(package_name)
                .level(wpkg_output::level_warning)
                .module(wpkg_output::module_build_package)
                .package(f_package)
                .action("changelog");
            good_header = false;
        }
    }

    // *** Version ***
    if(good_header)
    {
        for(; isspace(*h); ++h);

        if(*h != '(')
        {
            wpkg_output::log("changelog:%1:%2: invalid header, expected the version between parenthesis after the package name")
                    .arg(f_filename)
                    .arg(f_line)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .package(f_package)
                .action("changelog");
            good_header = false;
        }
    }
    if(good_header)
    {
        // read the version
        ++h;
        start = h;
        for(; !isspace(*h) && *h != ')' && *h != '\0'; ++h);

        std::string version_str(start, h - start);
        char err[256];
        if(!validate_debian_version(version_str.c_str(), err, sizeof(err)))
        {
            wpkg_output::log("control:%1:%2: %3 is not a valid Debian version")
                    .arg(f_filename)
                    .arg(f_line)
                    .quoted_arg(version_str)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_changelog)
                .package(f_package)
                .action("changelog");
            good_header = false;
        }
        else
        {
            f_version = version_str;
        }
    }
    if(good_header)
    {
        if(isspace(*h))
        {
            for(; isspace(*h); ++h);
            wpkg_output::log("control:%1:%2: version %3 is not immediately followed by a closing parenthesis")
                    .arg(f_filename)
                    .arg(f_line)
                    .quoted_arg(f_version)
                .level(wpkg_output::level_warning)
                .module(wpkg_output::module_changelog)
                .package(f_package)
                .action("changelog");
        }
        if(*h != ')')
        {
            wpkg_output::log("control:%1:%2: version %3 is not followed by a closing parenthesis: ')'")
                    .arg(f_filename)
                    .arg(f_line)
                    .quoted_arg(f_version)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_changelog)
                .package(f_package)
                .action("changelog");
            good_header = false;
        }
    }

    // *** Distributions ***
    if(good_header)
    {
        // the first ++h is to skip the ')' from the version
        for(++h; isspace(*h); ++h);
        do
        {
            start = h;
            for(; !isspace(*h) && *h != '\0' && *h != ';' && *h != ','; ++h);
            std::string d(start, h - start);
            wpkg_filename::uri_filename distribution(d);
            if(distribution.is_absolute())
            {
                wpkg_output::log("control:%1:%2: a distribution must be a relative path, %3 is not acceptable")
                        .arg(f_filename)
                        .arg(f_line)
                        .quoted_arg(distribution)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_changelog)
                    .package(f_package)
                    .action("changelog");
                good_header = false;
            }
            else if(distribution.segment_size() < 1)
            {
                // This happens if no distribution is specified
                wpkg_output::log("control:%1:%2: a distribution cannot be the empty string")
                        .arg(f_filename)
                        .arg(f_line)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_changelog)
                    .package(f_package)
                    .action("changelog");
                good_header = false;
            }
            else
            {
                f_distributions.push_back(distribution.original_filename());
            }
            for(; isspace(*h); ++h);
        }
        while(*h != '\0' && *h != ';' && *h != ',');
    }
    // We want to support more than one in source packages, that way a package
    // can be part of stable and unstable (because we view our distributions
    // as separate entities rather than complements.)
    //if(good_header)
    //{
    //    if(f_distributions.size() > 1)
    //    {
    //        wpkg_output::log("control:%1:%2: although the syntax allows for more than one distribution, we do not support more than one at this time")
    //                .arg(f_filename)
    //                .arg(f_line)
    //            .level(wpkg_output::level_error)
    //            .module(wpkg_output::module_changelog)
    //            .package(f_package)
    //            .action("changelog");
    //        good_header = false;
    //    }
    //}

    // *** Parameters ***
    if(good_header)
    {
        for(; isspace(*h); ++h);

        if(*h != ';')
        {
            wpkg_output::log("changelog:%1:%2: invalid header, expected the list of distributions to end with ';'")
                    .arg(f_filename)
                    .arg(f_line)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .package(f_package)
                .action("changelog");
            good_header = false;
        }
        else
        {
            for(++h; isspace(*h); ++h);
        }
    }
    while(good_header && *h != '\0')
    {
        start = h;
        const char *e(h);
        const char *equal(NULL);
        for(; *h != ',' && *h != '\0'; ++h)
        {
            if(!isspace(*h))
            {
                e = h + 1;
            }
            if(*h == '=')
            {
                equal = h;
            }
        }
        if(equal == NULL)
        {
            wpkg_output::log("changelog:%1:%2: invalid header, parameter %3 is expected to include an equal sign (=) after the parameter name")
                    .arg(f_filename)
                    .arg(f_line)
                    .quoted_arg(std::string(start, h - start))
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .package(f_package)
                .action("changelog");
            good_header = false;
        }
        else if(start == equal)
        {
            wpkg_output::log("changelog:%1:%2: invalid header, parameter %3 is missing a name before the equal character")
                    .arg(f_filename)
                    .arg(f_line)
                    .quoted_arg(std::string(start, h - start))
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .package(f_package)
                .action("changelog");
            good_header = false;
        }
        else
        {
            std::string name(start, equal - start);
            if(f_parameters.find(name) != f_parameters.end())
            {
                wpkg_output::log("changelog:%1:%2: invalid header, parameter %3 is defined twice")
                        .arg(f_filename)
                        .arg(f_line)
                        .quoted_arg(name)
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_build_package)
                    .package(f_package)
                    .action("changelog");
                good_header = false;
            }
            else
            {
                ++equal;
                std::string value(equal, e - equal);
                f_parameters[name] = value;
            }
        }

        // skip commas and spaces and repeat for next parameter
        for(; *h == ',' || isspace(*h); ++h);
    }

    // We got the header, now we check for the list of logs
    // the log::parse() function is expected to return false
    // once we reach the end of that list, then we must find
    // a footer that starts with a space and two dashes

    if(!s.next_line())
    {
        wpkg_output::log("changelog:%1:%2: every changelog version entry must have at least one log and end with a valid footer")
                .arg(f_filename)
                .arg(s.get_line())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_package)
            .package(f_package)
            .action("changelog");
        return false;
    }

    bool group(true);
    for(;;)
    {
        log l;
        if(!l.parse(s, group))
        {
            break;
        }
        f_logs.push_back(l);
    }

    // we are at the end of the log stream for this version entry,
    // there has to be a valid footer now

    bool good_footer(true);

    if(s.space_count() != 1)
    {
        wpkg_output::log("changelog:%1:%2: a changelog version entry must end with a valid footer, which must start with exactly one space")
                .arg(f_filename)
                .arg(s.get_line())
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_build_package)
            .package(f_package)
            .action("changelog");
        good_footer = false;
    }

    const std::string& footer(s.last_line());
    const char *f(footer.c_str());

    if(good_footer)
    {
        if(f[0] != '-' || f[1] != '-' || f[2] != ' ')
        {
            wpkg_output::log("changelog:%1:%2: a changelog version entry must end with a valid footer, which must start with two dashes")
                    .arg(f_filename)
                    .arg(s.get_line())
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .package(f_package)
                .action("changelog");
            good_footer = false;
        }
    }

    if(good_footer)
    {
        // search for two spaces to break the maintainer name/email and date
        f += 3;
        start = f;
        for(; f[0] != '\0' && (f[0] != ' ' || f[1] != ' '); ++f);
        std::string maintainer(start, f - start);

        // TODO: use libtld to verify email

        f_maintainer = maintainer;

        std::string date(f + 2);
        struct tm time_info;
        if(strptime(date.c_str(), "%a, %d %b %Y %H:%M:%S %z", &time_info) == NULL)
        {
            wpkg_output::log("changelog:%1:%2: the footer in this changelog version entry has an invalid date: %3")
                    .arg(f_filename)
                    .arg(s.get_line())
                    .quoted_arg(date)
                .level(wpkg_output::level_error)
                .module(wpkg_output::module_build_package)
                .package(f_package)
                .action("changelog");
            good_footer = false;
        }
        else
        {
            f_date = date;
        }
    }

    // do not read another line in case this one is the next header
    return true;
}


/** \brief Return the name of the package.
 *
 * In a changelog, each new version entry includes the name of the package.
 * All of them must be equal and the name must also match the Package name
 * found in the control.info file of a project.
 *
 * \return The name of the package as found in this version entry.
 */
std::string changelog_file::version::get_package() const
{
    return f_package;
}


/** \brief The version of the package.
 *
 * In a changelog, the version is defined between parenthesis right after
 * the package name. This must be a valid Debian version. All versions in
 * a changelog MUST be sorted from the most recent (largest) to the oldest
 * (smallest.)
 *
 * \return The version of the package as found in this version entry.
 */
std::string changelog_file::version::get_version() const
{
    return f_version;
}


/** \brief A space separated list of distributions.
 *
 * This field includes a list of what we call distributions, which is
 * the same as the Distribution field. In our source packages, we accept
 * multiple distributions (all those defined in the changelog file.)
 * In a binary package, we only accept one distribution name in that field
 * since that will have been compiled for that one distribution only with
 * those files specific to that one distribution.
 *
 * For example, a package that is part of stable and unstable is defined
 * with distributions defined as:
 *
 * \code
 * stable unstable
 * \endcode
 *
 * Note that in your environment you may give any type of name to your
 * distributions. For example, you may have an official version (that
 * you sell to your customers) named "end-user", another used by testers
 * named "test", and yet another used by developers named "development".
 * (Obviously testers also have to test the end-user version, but the
 * "test" distribution may include tools that are not otherwise available
 * to end users.)
 *
 * We copy that information as is to the Distribution field of your
 * control.info file. Note that if already defined in your control file,
 * then it must be equal.
 *
 * We offer more details about the Distribution field on this page:
 *
 * http://windowspackager.org/documentation/control-files/distribution
 *
 * \todo
 * Add tests to make sure that each segment of the distribution path is valid,
 * as is and as an accepted name as per our definitions in our documentation.
 * This should be checked only if the user used the --validate-to-standards
 * option (names are TBD.)
 *
 * \return The distributions as found in this version entry.
 */
const changelog_file::version::distributions_t& changelog_file::version::get_distributions() const
{
    return f_distributions;
}


/** \brief Check whether a named parameter was defined.
 *
 * This function searches for the named parameter, if defined, it returns
 * true, otherwise false.
 *
 * \return Whether a parameter was found in this version entry.
 */
bool changelog_file::version::parameter_is_defined(const std::string& name) const
{
    return f_parameters.find(name) != f_parameters.end();
}


/** \brief Retrieve the specified parameter.
 *
 * After the list of distributions, a changelog file defines a set of comma
 * separated parameters defined as name=\<value>. Although we only really
 * support "urgency" at this point, we implemented the changelog loader
 * to support any number of parameters.
 *
 * \param[in] name  The name of the parameter to retrieve.
 *
 * \return The value of the specified parameter.
 */
std::string changelog_file::version::get_parameter(const std::string& name) const
{
    if(parameter_is_defined(name))
    {
        parameter_list_t::const_iterator i(f_parameters.find(name));
        return i->second;
    }

    wpkg_output::log("changelog:%1:%2: parameter %3 is not defined")
            .arg(f_filename)
            .arg(f_line)
            .quoted_arg(name)
        .level(wpkg_output::level_error)
        .module(wpkg_output::module_build_package)
        .action("changelog");

    return "";
}


/** \brief Return the map of parameters.
 *
 * This function returns the map of all the parameters found in the
 * changelog file. You may then go through the map to list all the
 * available parameters.
 *
 * Note that we return a const parameter_list_t reference so you can
 * save that value in a reference to avoid a copy (if you're not in
 * a multi-threading environment.)
 *
 * \return The map of parameters, it may be empty.
 */
const changelog_file::version::parameter_list_t& changelog_file::version::get_parameters() const
{
    return f_parameters;
}


/** \brief Get the name and email address of the maintainer.
 *
 * This function returns the name of email address of the maintainer as
 * found in this version entry. The entry is verified at the time it is
 * loaded.
 *
 * This value can safely be saved as is in the Maintainer field.
 *
 * \return The name and email address of the maintainer.
 */
std::string changelog_file::version::get_maintainer() const
{
    return f_maintainer;
}


/** \brief Get the date the maintainer entered in this entry.
 *
 * The footer includes a date that can be retrieved with this function.
 * This date is considered to be the date and time when the log changes
 * started.
 *
 * Note that in Debian this date is copied in the Date field and it
 * represents the date when the project is packaged. Instead, we generate
 * the Date field dynamically so it shows exactly when the corresponding
 * packages were created. We save this date as the Change-Date field
 * instead.
 *
 * \return The date changes started appearing in the changelog.
 */
std::string changelog_file::version::get_date() const
{
    return f_date;
}


/** \brief Return the number of groups in the list of logs.
 *
 * This function counts the number of groups. In most cases it is of interest
 * if the number of groups is exactly 1 (i.e. no groups really.)
 *
 * \return The number of log entries marked as being a group.
 */
int changelog_file::version::count_groups() const
{
    int group(0);
    for(log_list_t::const_iterator it(f_logs.begin()); it != f_logs.end(); ++it)
    {
        if(it->is_group())
        {
            ++group;
        }
    }
    return group;
}


/** \brief Return the list of logs.
 *
 * This function returns a constant references to the existing log
 * entries of this change log version.
 */
const changelog_file::version::log_list_t& changelog_file::version::get_logs() const
{
    return f_logs;
}


/** \brief Retrieve the filename of the input file.
 *
 * This function is generally used to print errors about this changelog
 * file. It is useful so users have a better idea of what file we're
 * talking about.
 *
 * \return The filename of the input file.
 */
wpkg_filename::uri_filename changelog_file::version::get_filename() const
{
    return f_filename;
}


/** \brief Retrieve the line number in the source we're reading from.
 *
 * Get the current line number of the last line we've read from the source
 * file.
 *
 * \return The current line number.
 */
int changelog_file::version::get_line() const
{
    return f_line;
}


/** \brief Initialize a changelog_file object.
 *
 * This constructor is here to allow construction of a changelog_file object.
 */
changelog_file::changelog_file()
    //: f_versions() -- auto-init
{
}


/** \brief Read the specified memory file as a changelog.
 *
 * This function reads the changelog file found in the data file.
 *
 * \param[in] data  The changelog memory file to read from.
 *
 * \return true if no error occurs while reading the file.
 */
bool changelog_file::read(const memfile::memory_file& data)
{
    enum
    {
        STATE_HEADER,
        STATE_LOG,
        STATE_LOG_MORE,
        STATE_FOOTER
    };

    state s(data);

    uint32_t err(wpkg_output::get_output_error_count());

    // read the first line
    while(s.next_line())
    {
        version v;
        if(!v.parse(s))
        {
            break;
        }
        f_versions.push_back(v);
    }

    if(err == wpkg_output::get_output_error_count())
    {
        // verify that all versions are in the correct order
        // (i.e. larger to smaller)
        version_list_t::size_type max(f_versions.size());
        for(version_list_t::size_type i(1); i < max; ++i)
        {
            if(wpkg_util::versioncmp(f_versions[i - 1].get_version(), f_versions[i].get_version()) <= 0)
            {
                wpkg_output::log("changelog:%1:%2: version %4 (on line %2) is smaller or equal to version %5 (on line %3), this changelog is not valid")
                        .arg(s.get_filename())
                        .arg(f_versions[i - 1].get_line())
                        .arg(f_versions[i].get_line())
                        .quoted_arg(f_versions[i - 1].get_version())
                        .quoted_arg(f_versions[i].get_version())
                    .level(wpkg_output::level_error)
                    .module(wpkg_output::module_build_package)
                    .package(f_versions[i].get_package())
                    .action("changelog");
            }
        }
    }

    return err == wpkg_output::get_output_error_count();
}


/** \brief Retrieve the number of versions defined in this changelog file.
 *
 * The read() function reads all the change logs found in the changelog
 * file. Each new header represents a new version of the package which
 * are saved in an array. This function returns the number of versions
 * that were read.
 *
 * \return The number of versions defined in this changelog.
 */
size_t changelog_file::get_version_count() const
{
    return f_versions.size();
}


/** \brief Get the version of this entry.
 *
 * Each entry has a version definition. The number of version entries
 * can be retrieved with the get_version_count() function. Then each
 * version entry can be retrieved using this get_version() function.
 *
 * \param[in] idx  Index representing which version to retrieve.
 *
 * \return A reference to the version in question.
 */
const changelog_file::version& changelog_file::get_version(int idx) const
{
    return f_versions[idx];
}



}   // namespace wpkg_field

// vim: ts=4 sw=4 et
