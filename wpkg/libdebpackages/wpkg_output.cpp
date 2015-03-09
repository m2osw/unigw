/*    wpkg_output.cpp -- implementation of the wpkg_output class methods
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
 * \brief Implementation of the message output.
 *
 * This library supports a log mechanism which allows applications that
 * make use of the library to capture that output and print it either in
 * a console, in a window, or in a file.
 *
 * The implementation also deals with the current output level (info, debug,
 * warning, etc.) and with different formats.
 */
#include    "libdebpackages/wpkg_output.h"
#include    "libdebpackages/compatibility.h"
#include    <time.h>
#include    <sstream>

#if !defined(MO_WINDOWS)
#   include    <unistd.h>
#endif

namespace wpkg_output
{


/** \class wpkg_output_exception
 * \brief The base exception of the wpkg_output exceptions.
 *
 * This exception class is the base class of all the wpkg_output
 * classes. The class is never used to raise an exception directly,
 * but it can be used to catch all the wpkg_output exceptions in
 * one go.
 */


/** \class wpkg_output_exception_parameter
 * \brief A function was called with an invalid parameter.
 *
 * This exception is raised if a function detects that an input
 * parameter is not valid.
 */


/** \class wpkg_output_exception_format
 * \brief An error was found in the format of a log message.
 *
 * The log format of a message may include references to arguments that
 * are defined as %1, %2, %3, etc. If the % is not followed by another
 * % (to output one %) or by at least one digit, or if the number is
 * larger than the number of arguments available then this exception
 * is raised.
 *
 * \note
 * This error should be an std::logic_error since it is a programmer's
 * error.
 */


/** \class wpkg_output::output
 * \brief Class used to declare the output object.
 *
 * Each instance of a program should have one output object. This one
 * object should be passed to the different objects that handle an
 * output object.
 *
 * Note that at first the output object was passed to all the functions
 * that wanted to log some data. Instead, we now have a global object
 * that gets called so the log functions can be called from anywhere
 * and it all works (assuming you did create an output object and
 * registered it with the set_output() function.)
 *
 * This means that the addref() and release() are not likely useful at this
 * point. It may get removed at a later time since it may cause confusion
 * in the long term.
 */


/** \class debug_flags
 * \brief Class to manage debug flags definitions.
 *
 * The list of debug flags are managed by this class. The flags are
 * passed to log messages via the log::debug() function.
 *
 * \todo
 * The debug_flags class is here because it is expected that at some
 * point we'll allow wpkg users to enter those flags by name instead
 * of a bit number as it works right now.
 */


/** \class log
 * \brief Class used to log messages.
 *
 * This class is used to create one log message and send it to the current
 * output object.
 *
 * The destructor of the log class is actually the most interesting part.
 * That's the one function that makes use of all the data accumulated by
 * calling all the functions on the log object you are creating on the
 * stack (i.e. temporary object) and converts them in a complete message
 * that is sent to the current output object output::log() function.
 *
 * The log class is only used temporarily as follow:
 *
 * \code
 * wpkg_output::log("<log message with %1 and %2 arguments>")
 *      .arg(arg1_value)
 *      .quoted_arg(arg2_value)
 *      .debug(...)
 *      .module("<name>")
 *      . ...
 *      ;
 * \endcode
 *
 * This creates a log object and then destroys it immediately (since we
 * do not put the object in a named variable!) The effect is that it sends
 * the message to the output object. The different functions one can call
 * are used to define parameter that are not to be set to the default value.
 */


/** \class message_t
 * \brief An output message.
 *
 * Whenever the output::log() function is used it expects a message. This
 * message can be setup manually by creating a message and then calling
 * the different function to setup the parameters of the message. Or it
 * can be set with the use of the log class, which is the preferred way
 * in the library.
 *
 * The result is a message with a wealth of information, as available at
 * the time the message is created. The message can include a package name
 * in the event it is specific to a package. It is expected to be assigned
 * a module name, which represents the name of the section of the library
 * that generated the message. The message is automatically given a time
 * stamp although it is possible to assigned a different time.
 *
 * The message also has a function to retrieve that information and to
 * transform all the information in a message that can be logged in
 * a computer readable format (i.e. a format that is very easy to parse
 * so one can check the logs with tools.)
 */


/** \brief Converts a module value to a string.
 *
 * This function transforms the module \p s to a string that represents
 * that module.
 *
 * \bug
 * At this point the string is returned in English.
 *
 * \param[in] s  The module to convert.
 *
 * \return A constant string representing the module.
 */
const char *module_to_string(const module_t module)
{
    switch(module)
    {
    case module_attached:              return "attached";
    case module_detached:              return "detached";
    case module_build_info:            return "build-info";
    case module_build_package:         return "build-package";
    case module_validate_installation: return "validate-installation";
    case module_unpack_package:        return "unpack-package";
    case module_configure_package:     return "configure-package";
    case module_validate_removal:      return "validate-removal";
    case module_remove_package:        return "remove-package";
    case module_deconfigure_package:   return "deconfigure-package";
    case module_run_script:            return "run-script";
    case module_repository:            return "repository";
    case module_control:               return "control";
    case module_changelog:             return "changelog";
    case module_copyright:             return "copyright";
    case module_field:                 return "field";
    case module_tool:                  return "tool";
    case module_track:                 return "track";

    default:
        throw std::overflow_error("the module_to_string() function was called with an invalid module");

    }
    /*NOTREACHED*/
}


/** \brief Transform the \p level to a string.
 *
 * This function transforms the \p level parameter to a static string.
 *
 * \todo
 * At this time the string is always in English.
 *
 * \param[in] level  The level to convert to a string.
 *
 * \return A constant string representing the level.
 */
const char *level_to_string(const level_t level)
{
    switch(level)
    {
    case level_debug:   return "debug";
    case level_info:    return "info";
    case level_warning: return "warning";
    case level_error:   return "error";
    case level_fatal:   return "fatal";

    default:
        throw std::overflow_error("the level_to_string() function was called with an invalid level");

    }
    /*NOTREACHED*/
}


/** \brief Compare level l1 and l2 against each other.
 *
 * Compare the two levels l1 and l2 and return whether l1 is smaller,
 * equal, or larger than l2.
 *
 * \param[in] l1  The first level to test.
 * \param[in] l2  The second level to test.
 *
 * \return -1 when l1 < l2, 0 when l1 == l2, and 1 when l1 > l2.
 */
int compare_levels(level_t l1, level_t l2)
{
    if(static_cast<int>(l1) < static_cast<int>(l2))
    {
        return -1;
    }
    if(static_cast<int>(l1) > static_cast<int>(l2))
    {
        return 1;
    }
    return 0;
}


/** \brief Generate a string of the current time.
 *
 * This function generates a formatted string with the current local
 * date and time with the following format:
 *
 * \code
 * YYYY/mm/dd HH:MM:SS
 * \endcode
 *
 * Where YYYY represents the year with the century, mm represents the
 * month in number, dd represents the day of the month, HH represents
 * the hour, MM represents the minutes, and the SS represents the
 * seconds.
 *
 * We may want to offer the user to change the format at some point.
 *
 * \return A string with the formatted date and time.
 */
std::string generate_timestamp()
{
    char buf[1024];
    time_t t;
    time(&t);
    struct tm *m(localtime(&t));
    // WARNING: remember that format needs to work on MS-Windows
    strftime_utf8(buf, sizeof(buf) - 1, "%Y/%m/%d %H:%M:%S", m);
    buf[sizeof(buf) / sizeof(buf[0]) - 1] = '\0';
    //
    return buf;
}


/** \brief Transform a string in a computer parsable string.
 *
 * This function transforms a string so that all double quotes
 * and backslashes are escaped with a backslash. This renders
 * the string easy to parse by a computer.
 *
 * \li backslash characters become '\\\\'
 * \li double quotes characters become '\\"'
 *
 * Everything else is copied as is. Although we do not expect newline
 * or carriage return characters in the string, We may later add
 * support to escape those and other controls.
 *
 * \return The parsable string.
 */
std::string make_raw_message_parsable(const std::string& raw_message)
{
    std::string parsable_message;
    for(const char *str(raw_message.c_str()); *str != '\0'; ++str)
    {
        if(*str == '\\' || *str == '"')
        {
            parsable_message += "\\";
        }
        parsable_message += *str;
    }
    return parsable_message;
}


/** \brief Initialize a message.
 *
 * This function initializes a message to output as a log message.
 * In most cases this is part of the log object that you create
 * temporarily to send a log message to your output object.
 *
 * By default the message is initialized with the following values:
 *
 * \li level is set to \em level_info
 * \li module is set to \em module_tool
 * \li program name is set to the empty string, your log object makes
 *     use of the output::get_program_name() function to set this
 *     parameter
 * \li package name is set to the empty string
 * \li time stamp is set to \em time() (now)
 * \li action is set to the empty string
 * \li debug flags are set to \em debug_none
 * \li raw message is set to the empty string
 */
message_t::message_t()
    //: f_level(level_info) -- auto-init
    //, f_module(module_tool) -- auto-init
    //, f_program_name("") -- auto-init
    //, f_package_name("") -- auto-init
    : f_time_stamp(generate_timestamp())
    //, f_action("") -- auto-init
    //, f_debug_flags(debug_none) -- auto-init
    //, f_raw_message("") -- auto-init
{
}


/** \brief Set the level at which this message is.
 *
 * This function is used to set the level of the message. Usually this is
 * done with the level() function of the log class.
 *
 * The level must be one of:
 *
 * \li level_debug
 * \li level_info -- this is the default if you do not set the level
 * \li level_warning
 * \li level_error
 * \li level_fatal
 *
 * Note that printing a fatal error does not have any special effect. You
 * must make sure to throw, exit, or do the necessary to exit the software
 * if required.
 *
 * You may transform the level to a string with the level_to_string() function.
 *
 * \param[in] level  The new message level.
 */
void message_t::set_level(level_t level)
{
    if(level < level_debug)
    {
        throw std::underflow_error("level is out of range in message_t::set_level()");
    }
    if(level > level_fatal)
    {
        throw std::overflow_error("level is out of range in message_t::set_level()");
    }
    safe_level_t l(static_cast<int>(level));  // FIXME cast
    f_level = l;
}


/** \brief Set the module of the message.
 *
 * This function is used to change the module of the message. The module
 * actually represents what part of the software generated the message.
 * By default it is set to module_tool which represents your tool (a tool
 * outside of the libdebpackages library.)
 *
 * For example, the --install command has a large set of validations that
 * makes use of the module_validate_installation module.
 *
 * \li module_attached
 * \li module_detached
 * \li module_build_info
 * \li module_build_package
 * \li module_validate_installation
 * \li module_unpack_package
 * \li module_configure_package
 * \li module_validate_removal
 * \li module_remove_package
 * \li module_deconfigure_package
 * \li module_run_script
 * \li module_repository
 * \li module_control
 * \li module_changelog
 * \li module_copyright
 * \li module_field
 * \li module_tool -- this is the default when creating a new message
 * \li module_track
 *
 * \param[in] module  The new module of the message.
 */
void message_t::set_module(module_t module)
{
    if(module < module_attached)
    {
        throw std::underflow_error("module is out of range in message_t::set_module()");
    }
    if(module > module_track)
    {
        throw std::overflow_error("module is out of range in message_t::set_module()");
    }
    safe_module_t m(static_cast<int>(module));    // FIXME cast
    f_module = m;
}


/** \brief Define the name of the running program.
 *
 * This function is called to set the name of the program running so as
 * to print it in the message.
 *
 * When using a log object, the program name is set using the attached
 * output object get_program_name().
 *
 * \param[in] program_name  The name of the program generating this message.
 */
void message_t::set_program_name(const std::string& program_name)
{
    f_program_name = program_name;
}


/** \brief Set the package name.
 *
 * When working on packages (i.e. while installing a package) then errors
 * should be linked to the package that generates an error. This parameter
 * is used for that purpose.
 *
 * \param[in] package_name  The package name as a const char.
 */
void message_t::set_package_name(const char *package_name)
{
    f_package_name = package_name;
}


/** \brief Set the package name.
 *
 * When working on packages (i.e. while installing a package) then errors
 * should be linked to the package that generates an error. This parameter
 * is used for that purpose.
 *
 * \param[in] package_name  The package name as an std::string.
 */
void message_t::set_package_name(const std::string& package_name)
{
    f_package_name = package_name;
}


/** \brief Set the package name.
 *
 * When working on packages (i.e. while installing a package) then errors
 * should be linked to the package that generates an error. This parameter
 * is used for that purpose.
 *
 * \param[in] package_name  The filename of which we take the original filename.
 */
void message_t::set_package_name(const wpkg_filename::uri_filename& package_name)
{
    f_package_name = package_name.original_filename();
}


/** \brief Set the time stamp of the message.
 *
 * By default the message is assigned now as its time stamp. This
 * function can be used to change that time stamp.
 *
 * Note that you should actually probably never call this function
 * unless you want a quite specific time stamp format.
 *
 * \param[in] time_stamp  The new time stamp for this message.
 */
void message_t::set_time_stamp(const std::string& time_stamp)
{
    f_time_stamp = time_stamp;
}


/** \brief Set the action of the message.
 *
 * Set the action of the message. The action defines what the process
 * the error occurs is. For example, the installation process often
 * uses the action: "install-validation" as its action.
 *
 * \param[in] action  The action the process was taking at the time the error occured.
 */
void message_t::set_action(const std::string& action)
{
    f_action = action;
}


/** \brief Set the debug flags of this message.
 *
 * In most cases a message has one debug flag (i.e. one bit set) although
 * it is legal to define more than one flag.
 *
 * In wpkg, the user can set debug flags with the -D command line option.
 * If a message flags match any one flag that the user defined, then the
 * message is shown in stderr. You can do a similar action in your own
 * tool.
 *
 * \param[in] dbg_flags  A set of debug flags that this message represents.
 */
void message_t::set_debug_flags(const debug_flags::debug_t dbg_flags)
{
    if((dbg_flags & ~debug_flags::debug_all) != 0)
    {
        throw wpkg_output_exception_parameter("the debug flags parameter must be limited to the supported flags, some unknown flags were set");
    }
    f_debug_flags = dbg_flags;
}


/** \brief Set the raw message of this message_t.
 *
 * This function defines the raw message of this message_t object.
 * The raw message is just and only the error message. When using
 * a log object, it is possible to include arguments that get
 * replaced in the original message. The raw message already has
 * these parameters replaced.
 *
 * By default the raw message is empty. It must be set before you
 * can send the message to the output or you will get an error.
 *
 * \param[in] raw_message  The raw message.
 */
void message_t::set_raw_message(const std::string& raw_message)
{
    f_raw_message = raw_message;
}



/** \brief Retrieve the full message.
 *
 * The full message is the message including all the artifacts
 * that the message_t object knows about:
 *
 * \li program name
 * \li level
 * \li time stamp
 * \li package
 * \li raw message
 * \li module
 *
 * The flag \p raw_message can be used to get the message as is.
 * If false, it instead ensures that the message is 100% computer
 * parsable.
 *
 * If you are looking into getting the message itself, use the
 * get_raw_message() function instead.
 *
 * \param[in] raw_message  Whether to make the message computer parsable.
 *
 * \return The full message.
 *
 * \sa get_raw_message()
 */
std::string message_t::get_full_message(bool raw_message) const
{
    std::string msg(f_program_name);
    msg += ":";
    msg += level_to_string(f_level);
    msg += ": ";
    msg += f_time_stamp;
    msg += ": ";

    if(raw_message)
    {
        if(!f_package_name.empty())
        {
            msg += "[package:" + f_package_name + "] ";
        }

        msg += get_raw_message() + " (";
        msg += module_to_string(f_module);
        msg += ")";
    }
    else
    {
        msg += f_action;
        msg += " ";

        if(f_package_name.empty())
        {
            msg += "\"\"";
        }
        else
        {
            msg += f_package_name;
        }
        msg += " ";

        msg += "\"" + make_raw_message_parsable(f_raw_message) + "\" (";
        msg += module_to_string(f_module);
        msg += ")";
    }

    return msg;
}


/** \brief Get the level of this message.
 *
 * This function returns the level as defined by the set_level() function.
 *
 * By default the level of a message is set to level_info.
 *
 * \return The message level.
 */
level_t message_t::get_level() const
{
    return f_level;
}


/** \brief Get the module of this message.
 *
 * This function returns the module as defined by the set_module() function.
 *
 * By default the module of a message is set to module_tool.
 *
 * \return The message module.
 */
module_t message_t::get_module() const
{
    return f_module;
}


/** \brief Get the name of the program attached to this message.
 *
 * This function returns the name of the program that generates this message.
 *
 * In most cases this is set by the log class destructor using the output
 * function of the same name.
 *
 * \return The program name.
 *
 * \sa output::get_program_name()
 */
std::string message_t::get_program_name() const
{
    return f_program_name;
}


/** \brief Get the package name.
 *
 * This function returns the name of the package that was being worked on when
 * the message was being created. This parameter may be the empty string.
 *
 * \return The name of the package that generated this message.
 */
std::string message_t::get_package_name() const
{
    return f_package_name;
}


/** \brief Get the time and date when this message was created.
 *
 * This function returns a string with the date and time when the message
 * was generated. It may also return what the user set as the time stamp
 * with the set_time_stamp() function.
 *
 * \return The time stamp of the message as a string.
 */
std::string message_t::get_time_stamp() const
{
    return f_time_stamp;
}


/** \brief Get the action.
 *
 * This function returns the action string representing the part of the
 * software that generated the message.
 *
 * \return A string representing the part of the software generating this
 *         message.
 */
std::string message_t::get_action() const
{
    return f_action;
}


/** \brief Retrieve the debug flags.
 *
 * This function retrieves the debug flags that this message was assigned.
 * Note that debug flags are ignored if the message level is not level_debug.
 *
 * \return The debug flags currently set, may be zero (debug_none).
 */
debug_flags::debug_t message_t::get_debug_flags() const
{
    return f_debug_flags;
}


/** \brief Get the raw message as is.
 *
 * This function can be used to get the raw message from the message object.
 * The raw message is returned as is in this case. If you want to have a
 * message you can print in a console or a log file, use the
 * get_full_message() function instead.
 *
 * \return The raw message of this message_t.
 */
std::string message_t::get_raw_message() const
{
    return f_raw_message;
}



namespace
{

/** \brief The output pointer used by the log class.
 *
 * This pointer is set by calling the log::set_output() function with a
 * pointer to your output object. It is used by log objects when they
 * get destroyed in order to post the message that they carry.
 */
controlled_vars::ptr_auto_init<output> g_log_output;

} // no name namespace



/** \brief Define the output attached to the log class.
 *
 * All log objects, when being created, are expected to send their
 * output message to an output object. This object is defined by
 * this function.
 *
 * This function first releases the current log output pointer,
 * then it saves the new pointer in a variable that is accessible
 * by all the log objects.
 *
 * Once done in your software, make sure to call this function
 * again, this time with NULL, to release the pointer before you
 * exit.
 *
 * \param[in] out  The output pointer.
 */
void set_output(output *out)
{
    if(out == g_log_output)
    {
        return;
    }
    if(g_log_output)
    {
        g_log_output->release();
        g_log_output.reset();
    }
    if(out)
    {
        out->addref();
        g_log_output = out;
    }
}


/** \brief Retrieve the pointer of the current output object.
 *
 * This function retrieves the current output pointer. Note that this pointer
 * may very well be NULL if it was not yet defined.
 *
 * \return The pointer of the current output object or NULL.
 */
output *get_output()
{
    return g_log_output;
}


/** \brief Return the debug flags.
 *
 * This function checks whether a log output object was defined, if so,
 * get the debug flags defined in that object, otherwise assume that none
 * of those flags are defined.
 *
 * \return The flags of the log output or zero (0).
 */
debug_flags::debug_t get_output_debug_flags()
{
    if(g_log_output)
    {
        return g_log_output->get_debug_flags();
    }

    // if no g_log_output, assume that there are no debug flags set
    return 0;
}


/** \brief Return the number of errors found so far.
 *
 * This function returns the number of errors that were found so
 * far. It retrieves the error count of the currently assigned
 * log output object.
 *
 * If the log object is not defined, then no errors could be counted
 * so the function returns zero.
 *
 * \return The number of errors found so far.
 */
uint32_t get_output_error_count()
{
    if(g_log_output)
    {
        return g_log_output->error_count();
    }

    // if no g_log_output we could not count the number of errors
    // (although we could do that in the log::~log() function instead?
    return 0;
}


/** \brief Initialize the log with a message format.
 *
 * Set the format of the message to the specified \p format parameter.
 * The format allows the user of the log message to translate messages
 * and still have the arguments appear in the right place.
 *
 * The format should include %\<number> references to the different
 * parameters that will be specified after the creation of the log
 * object. For example, a message that includes three arguments
 * could use:
 *
 * \code
 * log("This message is about %1 when error #%2 occurs and %3 was considered valid")
 *      .arg("greatness")       // replace %1
 *      .arg(57)                // replace %2
 *      .quoted_arg("package"); // replace %3 written inside quotes
 * \endcode
 *
 * (note that the log would probably also receive a level(), action(),
 * package_name() and any other parameter that is expected.)
 *
 * The parser also supports having a number right after a reference using a
 * semi-colon. So, for example, to write %1 followed by the number 00 you
 * write: "%1;00". Note that means the semi-colon itself is removed. So to
 * enter a semi-colon right after a reference, type two of them:
 * "this %3;; but not %2"
 *
 * The input string is expected to be UTF-8 encoded.
 *
 * \param[in] format  The format used to generate the raw message.
 */
log::log(const char *format)
    : f_format(format)
    //, f_args() -- auto-init
    //, f_message() -- auto-init
    //, f_output_message(NULL) -- auto-init
{
}


/** \brief Initialize the log with a message format.
 *
 * See the log::log(const char *format); documentation for details.
 *
 * The wchar_t string is converted to UTF-8 before use.
 *
 * \param[in] format  The format used to generate the raw message.
 */
log::log(const wchar_t *format)
    : f_format(libutf8::wcstombs(format))
    //, f_args() -- auto-init
    //, f_message() -- auto-init
    //, f_output_message(NULL) -- auto-init
{
}


/** \brief Initialize the log with a message format.
 *
 * See the log::log(const char *format); documentation for details.
 *
 * \param[in] format  The format used to generate the raw message.
 */
log::log(const std::string& format)
    : f_format(format)
    //, f_args() -- auto-init
    //, f_message() -- auto-init
    //, f_output_message(NULL) -- auto-init
{
}


/** \brief Initialize the log with a message format.
 *
 * See the log::log(const char *format); documentation for details.
 *
 * The wstring is converted to UTF-8 before use.
 *
 * \param[in] format  The format used to generate the raw message.
 */
log::log(const std::wstring& format)
    : f_format(libutf8::wcstombs(format))
    //, f_args() -- auto-init
    //, f_message() -- auto-init
    //, f_output_message(NULL) -- auto-init
{
}


/** \brief Initialize the log with a message format.
 *
 * Please see the log::log(const char *format) function for more details
 * about the format.
 *
 * This version accepts a message string reference where we save the 
 * resulting message. In this case the message does not get sent to
 * the g_log_output. It is often used to create complex messages to
 * throw or to write in a file.
 *
 * \param[in] output_message  The string where the final message is saved.
 * \param[in] format  The format used to generate the raw message.
 */
log::log(std::string& output_message, const char *format)
    : f_format(format)
    //, f_args() -- auto-init
    //, f_message() -- auto-init
    , f_output_message(&output_message)
{
}


/** \brief Initialize the log with a message format.
 *
 * Please see the log::log(const char *format) function for more details
 * about the format.
 *
 * This version accepts a message string reference where we save the 
 * resulting message. In this case the message does not get sent to
 * the g_log_output. It is often used to create complex messages to
 * throw or to write in a file.
 *
 * \param[in] output_message  The string where the final message is saved.
 * \param[in] format  The format used to generate the raw message.
 */
log::log(std::string& output_message, const wchar_t *format)
    : f_format(libutf8::wcstombs(format))
    //, f_args() -- auto-init
    //, f_message() -- auto-init
    , f_output_message(&output_message)
{
}


/** \brief Initialize the log with a message format.
 *
 * Please see the log::log(const char *format) function for more details
 * about the format.
 *
 * This version accepts a message string reference where we save the 
 * resulting message. In this case the message does not get sent to
 * the g_log_output. It is often used to create complex messages to
 * throw or to write in a file.
 *
 * \param[in] output_message  The string where the final message is saved.
 * \param[in] format  The format used to generate the raw message.
 */
log::log(std::string& output_message, const std::string& format)
    : f_format(format)
    //, f_args() -- auto-init
    //, f_message() -- auto-init
    , f_output_message(&output_message)
{
}


/** \brief Initialize the log with a message format.
 *
 * Please see the log::log(const char *format) function for more details
 * about the format.
 *
 * This version accepts a message string reference where we save the 
 * resulting message. In this case the message does not get sent to
 * the g_log_output. It is often used to create complex messages to
 * throw or to write in a file.
 *
 * \param[in] output_message  The string where the final message is saved.
 * \param[in] format  The format used to generate the raw message.
 */
log::log(std::string& output_message, const std::wstring& format)
    : f_format(libutf8::wcstombs(format))
    //, f_args() -- auto-init
    //, f_message() -- auto-init
    , f_output_message(&output_message)
{
}


/** \brief Clean up the log object.
 *
 * This function is where the magic occurs. When destructing the log
 * object, the message is finalized and actually sent to the output
 * object.
 *
 * Note that if the log::set_output() was not yet called then nothing
 * happens since no output would be available for this function to
 * send the log message.
 */
log::~log()
{
    if(f_output_message)
    {
        *f_output_message = replace_arguments();
    }
    else if(g_log_output)
    {
        // mark the action as "debug" if undefined and the level is debug
        if(f_message.get_action().empty()
        && f_message.get_level() == level_debug)
        {
            f_message.set_action("debug");
        }

        // setup the program name from the output object
        f_message.set_program_name(g_log_output->get_program_name());

        // generate the final raw message
        f_message.set_raw_message(replace_arguments());

        // send the log message
        g_log_output->log(f_message);
    }
}


/** \brief Define the debug flags of this log message.
 *
 * This function saves the specified debug flags in the log message.
 * This represents the debug that this message is attached to.
 *
 * When this function is used, the message level is automatically
 * set to level_debug.
 *
 * In most cases a message is at most defined with one debug flag
 * although it is possible to combine flags with the OR operator (|)
 * it is unlikely that a message would pertain to more than one
 * debug group.
 *
 * \param[in] dbg_flags  A set of one or more debug flags.
 *
 * \return A reference to this log object.
 */
log& log::debug(debug_flags::debug_t dbg_flags)
{
    f_message.set_debug_flags(dbg_flags);
    f_message.set_level(level_debug);
    return *this;
}


/** \brief Define the level of the log message.
 *
 * This function defines the level of the message. Only valid levels
 * are accepted:
 *
 * \li level_debug (automatically set when you set the debug flags
 * with the debug() function);
 * \li level_info (this is the default);
 * \li level_warning
 * \li level_error
 * \li level_fatal
 *
 * Note that the level may affect whether a message is displayed and
 * is counted as an error or not.
 *
 * \param[in] l  The level of the log message.
 *
 * \return This log reference.
 */
log& log::level(level_t l)
{
    f_message.set_level(l);
    return *this;
}


/** \brief Define the module of the log message.
 *
 * This function defines the module of the log message. Only valid
 * modules are accepted.
 *
 * \param[in] m  The module of the message.
 *
 * \return This log reference.
 */
log& log::module(module_t m)
{
    f_message.set_module(m);
    return *this;
}


/** \brief Define the package name of the log message.
 *
 * This function defines the package name of the log message.
 *
 * \param[in] package_name  The name of the package in link with this message.
 *
 * \return This log reference.
 */
log& log::package(const char *package_name)
{
    f_message.set_package_name(package_name);
    return *this;
}


/** \brief Define the package name of the log message.
 *
 * This function defines the package name of the log message.
 *
 * \param[in] package_name  The name of the package in link with this message.
 *
 * \return This log reference.
 */
log& log::package(const wchar_t *package_name)
{
    f_message.set_package_name(libutf8::wcstombs(package_name));
    return *this;
}


/** \brief Define the package name of the log message.
 *
 * This function defines the package name of the log message.
 *
 * \param[in] package_name  The name of the package in link with this message.
 *
 * \return This log reference.
 */
log& log::package(const std::string& package_name)
{
    f_message.set_package_name(package_name);
    return *this;
}


/** \brief Define the package name of the log message.
 *
 * This function defines the package name of the log message.
 *
 * \param[in] package_name  The name of the package in link with this message.
 *
 * \return This log reference.
 */
log& log::package(const std::wstring& package_name)
{
    f_message.set_package_name(libutf8::wcstombs(package_name));
    return *this;
}


/** \brief Define the package name of the log message.
 *
 * This function defines the package name of the log message
 * from the original filename of a URI filename.
 *
 * \param[in] package_name  A URI filename with the name of the package in link with this message.
 *
 * \return This log reference.
 */
log& log::package(const wpkg_filename::uri_filename& package_name)
{
    f_message.set_package_name(package_name.original_filename());
    return *this;
}


/** \brief Define the action being perform when creating this log message.
 *
 * This function defines the action being performed as this message is
 * being created.
 *
 * \param[in] action_name  The name of the action in link with this message.
 *
 * \return The log reference.
 */
log& log::action(const char *action_name)
{
    f_message.set_action(action_name);
    return *this;
}


/** \brief Define the action being perform when creating this log message.
 *
 * This function defines the action being performed as this message is
 * being created.
 *
 * \param[in] action_name  The name of the action in link with this message.
 *
 * \return The log reference.
 */
log& log::action(const wchar_t *action_name)
{
    f_message.set_action(libutf8::wcstombs(action_name));
    return *this;
}


/** \brief Define the action being perform when creating this log message.
 *
 * This function defines the action being performed as this message is
 * being created.
 *
 * \param[in] action_name  The name of the action in link with this message.
 *
 * \return The log reference.
 */
log& log::action(const std::string& action_name)
{
    f_message.set_action(action_name);
    return *this;
}


/** \brief Define the action being perform when creating this log message.
 *
 * This function defines the action being performed as this message is
 * being created.
 *
 * \param[in] action_name  The name of the action in link with this message.
 *
 * \return The log reference.
 */
log& log::action(const std::wstring& action_name)
{
    f_message.set_action(libutf8::wcstombs(action_name));
    return *this;
}


/** \brief Replace an argument with the string.
 *
 * This function replaces an argument in the format string with the
 * specified parameter.
 *
 * \param[in] s  The string argument to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const char *s)
{
    f_args.push_back(s);
    return *this;
}


/** \brief Replace an argument with the string.
 *
 * This function replaces an argument in the format string with the
 * specified parameter.
 *
 * \param[in] s  The string argument to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const wchar_t *s)
{
    f_args.push_back(libutf8::wcstombs(s));
    return *this;
}


/** \brief Replace an argument with the string.
 *
 * This function replaces an argument in the format string with the
 * specified parameter.
 *
 * \param[in] s  The string argument to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const std::string& s)
{
    f_args.push_back(s);
    return *this;
}


/** \brief Replace an argument with the string.
 *
 * This function replaces an argument in the format string with the
 * specified parameter.
 *
 * \param[in] s  The string argument to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const std::wstring& s)
{
    f_args.push_back(libutf8::wcstombs(s));
    return *this;
}


/** \brief Replace an argument with the original filename.
 *
 * This function replace an argument in the format string with the
 * original filename of the URI filename parameter.
 *
 * \param[in] filename  A URI filename of which the original filename will be used.
 *
 * \return This log reference.
 */
log& log::arg(const wpkg_filename::uri_filename& filename)
{
    f_args.push_back(filename.original_filename());
    return *this;
}


/** \brief Replace an argument with the quoted string.
 *
 * This function replaces an argument in the format string with the
 * specified parameter.
 *
 * \param[in] s  The string argument to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::quoted_arg(const char *s)
{
    f_args.push_back("\"" + make_raw_message_parsable(s) + "\"");
    return *this;
}


/** \brief Replace an argument with the quoted string.
 *
 * This function replaces an argument in the format string with the
 * specified parameter.
 *
 * \param[in] s  The string argument to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::quoted_arg(const wchar_t *s)
{
    f_args.push_back("\"" + make_raw_message_parsable(libutf8::wcstombs(s)) + "\"");
    return *this;
}


/** \brief Replace an argument with the quoted string.
 *
 * This function replaces an argument in the format string with the
 * specified parameter.
 *
 * \param[in] s  The string argument to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::quoted_arg(const std::string& s)
{
    f_args.push_back("\"" + make_raw_message_parsable(s) + "\"");
    return *this;
}


/** \brief Replace an argument with the quoted string.
 *
 * This function replaces an argument in the format string with the
 * specified parameter.
 *
 * \param[in] s  The string argument to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::quoted_arg(const std::wstring& s)
{
    f_args.push_back("\"" + make_raw_message_parsable(libutf8::wcstombs(s)) + "\"");
    return *this;
}


/** \brief Replace an argument with the quoted original filename.
 *
 * This function replace an argument in the format string with the
 * original filename of the URI filename parameter.
 *
 * \param[in] filename  A URI filename of which the original filename will be used.
 *
 * \return This log reference.
 */
log& log::quoted_arg(const wpkg_filename::uri_filename& filename)
{
    f_args.push_back("\"" + make_raw_message_parsable(filename.original_filename()) + "\"");
    return *this;
}


/** \brief Replace an argument with the character.
 *
 * This function replaces an argument in the format string with the
 * specified parameter.
 *
 * In this case the value of \p v is viewed as a character, whereas
 * the unsigned char and signed char functions view the the value
 * as a number.
 *
 * \param[in] c  The character argument to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const char c)
{
    char buf[2];
    buf[0] = c;
    buf[1] = '\0';
    f_args.push_back(buf);
    return *this;
}


/** \brief Replace an argument with the unsigned character.
 *
 * This function replaces an argument in the format string with the
 * numeric value of the specified parameter.
 *
 * \param[in] v  The value to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const unsigned char v)
{
    std::ostringstream ss;
    ss << v;
    f_args.push_back(ss.str());
    return *this;
}


/** \brief Replace an argument with the signed character.
 *
 * This function replaces an argument in the format string with the
 * numeric value of the specified parameter.
 *
 * \param[in] v  The value to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const signed char v)
{
    std::ostringstream ss;
    ss << v;
    f_args.push_back(ss.str());
    return *this;
}


/** \brief Replace an argument with the unsigned short.
 *
 * This function replaces an argument in the format string with the
 * numeric value of the specified parameter.
 *
 * \param[in] v  The value to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const signed short v)
{
    std::ostringstream ss;
    ss << v;
    f_args.push_back(ss.str());
    return *this;
}


/** \brief Replace an argument with the unsigned short.
 *
 * This function replaces an argument in the format string with the
 * numeric value of the specified parameter.
 *
 * \param[in] v  The value to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const unsigned short v)
{
    std::ostringstream ss;
    ss << v;
    f_args.push_back(ss.str());
    return *this;
}


/** \brief Replace an argument with the signed int.
 *
 * This function replaces an argument in the format string with the
 * numeric value of the specified parameter.
 *
 * \param[in] v  The value to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const signed int v)
{
    std::ostringstream ss;
    ss << v;
    f_args.push_back(ss.str());
    return *this;
}


/** \brief Replace an argument with the unsigned int.
 *
 * This function replaces an argument in the format string with the
 * numeric value of the specified parameter.
 *
 * \param[in] v  The value to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const unsigned int v)
{
    std::ostringstream ss;
    ss << v;
    f_args.push_back(ss.str());
    return *this;
}


/** \brief Replace an argument with the signed long.
 *
 * This function replaces an argument in the format string with the
 * numeric value of the specified parameter.
 *
 * \param[in] v  The value to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const signed long v)
{
    std::ostringstream ss;
    ss << v;
    f_args.push_back(ss.str());
    return *this;
}


/** \brief Replace an argument with the unsigned long.
 *
 * This function replaces an argument in the format string with the
 * numeric value of the specified parameter.
 *
 * \param[in] v  The value to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const unsigned long v)
{
    std::ostringstream ss;
    ss << v;
    f_args.push_back(ss.str());
    return *this;
}


#if defined(MO_WINDOWS) && defined(_WIN64)
/** \brief Replace an argument with the size_t value.
 *
 * This function replaces an argument in the format string with the
 * numeric value of the specified parameter.
 *
 * \param[in] v  The value to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const size_t v)
{
    std::ostringstream ss;
    ss << v;
    f_args.push_back(ss.str());
    return *this;
}
#endif


/** \brief Replace an argument with the character.
 *
 * This function replaces an argument in the format string with the
 * numeric value of the specified parameter.
 *
 * \param[in] v  The value to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const float v)
{
    std::ostringstream ss;
    ss << v;
    f_args.push_back(ss.str());
    return *this;
}


/** \brief Replace an argument with the character.
 *
 * This function replaces an argument in the format string with the
 * numeric value of the specified parameter.
 *
 * \param[in] v  The value to replace the corresponding argument in the format string.
 *
 * \return The log reference.
 */
log& log::arg(const double v)
{
    std::ostringstream ss;
    ss << v;
    f_args.push_back(ss.str());
    return *this;
}


/** \brief Replace the next argument with the specified string.
 *
 * The log object keeps track of the arguments that it replaces. It
 * starts with argument 1 and then replaces argument 2, 3, 4, etc.
 * Arguments are referenced in format strings as %1, %2, %3, etc.
 *
 * Note that leading zeroes are legal and do not mean that the value
 * is written in octal or hexadecimal. It is still taken as a decimal
 * number.
 *
 * \exception wpkg_output_exception_format
 * This function raises the wpkg_output_exception_format exception when
 * it finds an invalid % sequence. At this time we only support %\<number>
 * optionally followed by a semi-colon. We also accept %% to represent
 * one % in the output. Anything else is an error. Also if the \<number>
 * is zero or too larger (larger than the number of arguments) then
 * this exception is also raised.
 *
 * \bug
 * If an argument includes %1, %2, etc. then these may end up
 * being replaced too.
 *
 * \param[in] msg  The argument string to replace an argument.
 */
std::string log::replace_arguments()
{
    std::string result;
    for(const char *s(f_format.c_str()); *s != '\0'; ++s)
    {
        if(*s == '%')
        {
            if(s[1] == '%')
            {
                // a lone '%'
                s += 2;
                result += '%';
            }
            else if(s[1] < '0' || s[1] > '9')
            {
                // invalid % in the format
                throw wpkg_output_exception_format("log() object created with an invalid format string: \""
                                        + f_format + "\" (a % character is not followed by a % or a digit.)");
            }
            else
            {
                size_t a(0);
                for(++s; *s >= '0' && *s <= '9'; ++s)
                {
                    a = a * 10 + *s - '0';
                }
                // we have a way to write a number right after a parameter
                // by adding a semi-colon (i.e. "%3;123")
                if(*s == ';')
                {
                    ++s;
                }
                if(a == 0 || a - 1 >= f_args.size())
                {
                    // invalid % reference (out of scope)
                    throw wpkg_output_exception_format("log() object created with an invalid format string reference: \""
                                        + f_format + "\" references a parameter that does not exist.");
                }
                result += f_args[static_cast<int>(a - 1)];
                --s; // compensate for the for() ++s
            }
        }
        else
        {
            result += *s;
        }
    }
    return result;
}













output::output()
    : f_refcount(1)
{
}


void output::addref()
{
    ++f_refcount;
}


void output::release()
{
    --f_refcount;
    if(0 == f_refcount)
    {
        delete this;
    }
}


void output::set_program_name(const std::string& program_name)
{
    f_program_name = program_name;
}

const std::string& output::get_program_name() const
{
    return f_program_name;
}


/** \brief Send a log message.
 *
 * This function sends the specified \p message to the log_message()
 * and output_message() functions which in general were overloaded
 * in your output class.
 *
 * The log_message() function should exclusively be used to save the
 * messages in log files.
 *
 * The output_message() function is called only if the message is
 * not a debug message or if the corresponding debug flag is set
 * in the message and in the output object.
 *
 * This function increases the f_error_count each time a message has
 * a level of error or more (level_error and level_fatal).
 *
 * \param[in] message  The message to send to the log_impl() function.
 */
void output::log(const message_t& message) const
{
    if(compare_levels(message.get_level(), level_error) >= 0)
    {
        ++f_error_count;
    }

    // always send to the log (i.e. no test against the debug flags)
    log_message(message);

    // if the message is a debug message, then make sure that
    // at least one of the debug flags was turned on by the user
    if(message.get_level() != level_debug
    || (message.get_debug_flags() & f_debug_flags) != 0)
    {
        output_message(message);
    }
}


/** \brief Define the set of debug flags that the user wants to see.
 *
 * This function saves a set of debug flags that the user wants to
 * see on the screen.
 *
 * It is expected that more than one debug flag be set in the
 * \p debug_flags parameter.
 *
 * \param[in] debug_flags  A set of debug flags.
 */
void output::set_debug_flags(debug_flags::debug_t debug_flags)
{
    f_debug_flags = debug_flags;
}


/** \brief Retrieve the set of debug flags that are current set.
 *
 * This function returns the set of flags that the user set as the
 * flags of debug information that he is interested in.
 *
 * It is very likely that more than one flag is set in this list.
 *
 * \return A set of debug flags.
 */
debug_flags::debug_t output::get_debug_flags() const
{
    return f_debug_flags;
}


/** \brief Retrieve the current number of errors.
 *
 * This function returns the number of errors that have already been emitted
 * through this output object.
 *
 * In most cases this is used at the end of your program to know whether the
 * process you ran succeeded (count is zero) or failed in some ways.
 *
 * \return The number of errors encountered.
 */
uint32_t output::error_count() const
{
    return f_error_count;
}


/** \brief Reset the error counter.
 *
 * In some cases it may be useful to reset the error counter before
 * continuing. This function does that. A call to error_count() right
 * after this function returns zero.
 *
 * This is particularly useful in a GUI application where you want to
 * reuse the same output over and over again.
 */
void output::reset_error_count()
{
    f_error_count = 0;
}


/** \brief Default the log_message() implementation
 *
 * The default implementation of the log_message() function
 * does nothing with the message.
 *
 * In your application you want to write those messages to
 * your log files. Actually, you should call the get_full_message()
 * with the \p raw_message parameter set to false so as to get a
 * machine readable string for your log files.
 *
 * \warning
 * Note that the log_message() function is always called, even if the
 * corresponding debug flag is not set in the output object. You should
 * really only use it for logs. Plus the output_message() will also be
 * called, and thus you do not want to display the \p msg or you'd
 * get the messages twice in your output.
 *
 * \param[in] msg  The message to be logged.
 */
void output::log_message( const message_t& msg ) const
{
    // do nothing by default
    (void)msg;
}


/** \brief Default output_message() implementation.
 *
 * The default implementation of the output_message() function
 * does nothing with the message.
 *
 * In your application you want to derive the wpkg_output
 * class and re-implement this function to output messages
 * to your users.
 *
 * \warning
 * The output_message() is called along the log_message() function,
 * which means that BOTH functions are called (although the
 * output_message() function may not get called for debug messages
 * that the user did not require to see.)
 *
 * \param[in] msg  The message to be output.
 */
void output::output_message( const message_t& msg ) const
{
    // do nothing by default
    (void)msg;
}



} // namespace wpkg_output
// vim: ts=4 sw=4 et
