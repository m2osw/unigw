/*    deb2html.cpp -- create an HTML file from a .deb file
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

/** \file
 * \brief Debian packages to HTML.
 *
 * This file is the implementation of the deb2html tool which transforms the
 * control file of a Debian package into HTML that can be viewed in a browser.
 *
 * The feature is actually used by the graphical tool pkg_explorer to present
 * packages that can be installed and packages that are installed in a target.
 */
#include    "libdebpackages/wpkgar.h"
#include    "libdebpackages/wpkg_control.h"
#include    "libdebpackages/advgetopt.h"
#include    "libdebpackages/debian_packages.h"
#include    "license.h"
#include    <stdio.h>
#include    <stdlib.h>
#include    <errno.h>
#include    <time.h>


/** \brief Derived version of the wpkg_output::output class for the deb2html tool.
 *
 * This class is used so one can get the output of errors if any occurs
 * while generating the HTML from Debian packages.
 *
 * The level can be specified (verbose, normal, quiet) however this version
 * does not support logging. It just prints messages in your console.
 */
class tool_output : public wpkg_output::output
{
public:
    tool_output();
    void set_level(wpkg_output::level_t level);

protected:
    virtual void log_message( const wpkg_output::message_t& msg ) const;

private:
    wpkg_output::level_t        f_log_level;
};

tool_output::tool_output()
    : f_log_level(wpkg_output::level_warning)
{
}


void tool_output::log_message( const wpkg_output::message_t& msg ) const
{
    // Note: the log_message() function receives ALL messages, including all
    //       the debug messages
    const std::string message = msg.get_full_message();
    if(compare_levels(msg.get_level(), f_log_level) >= 0)
    {
        printf("%s\n", message.c_str());
    }
}

void tool_output::set_level(wpkg_output::level_t level)
{
    f_log_level = level;
}


tool_output g_output;
wpkgar::wpkgar_manager g_manager;

/** \brief Package information.
 *
 * This simple structure holds the package name and a set of filenames.
 * The exact same package may have multiple versions in a repository so
 * we save all the filenames in the same structure to group all the
 * packages together.
 */
struct package_t
{
    std::string                                 f_package_name;
    std::vector<wpkg_filename::uri_filename>    f_filenames;
};
typedef std::map<std::string, package_t>    package_list_t;
package_list_t    g_packages;


void load_package(const wpkg_filename::uri_filename& package_filename)
{
    wpkg_output::log("loading %1")
            .quoted_arg(package_filename)
        .package(package_filename)
        .action("loading");
    g_manager.load_package(package_filename);
    const std::string package(g_manager.get_field(package_filename, "Package"));
    package_list_t::iterator it(g_packages.find(package));
    if(it == g_packages.end())
    {
        package_t p;
        p.f_package_name = package;
        p.f_filenames.push_back(package_filename);
        g_packages[package] = p;
    }
    else
    {
        it->second.f_filenames.push_back(package_filename);
    }
}

void load_packages(const std::string dir_name)
{
    struct stat s;
    if(stat(dir_name.c_str(), &s) != 0)
    {
        wpkg_output::log("file name %1 is invalid")
                .quoted_arg(dir_name)
            .level(wpkg_output::level_error)
            .action("loading");
        return;
    }
    if((s.st_mode & S_IFMT) == S_IFDIR)
    {
        memfile::memory_file in;
        in.dir_rewind(dir_name);
        for(;;)
        {
            memfile::memory_file::file_info info;
            if(!in.dir_next(info, NULL))
            {
                break;
            }
            switch(info.get_file_type())
            {
            case memfile::memory_file::file_info::regular_file:
            case memfile::memory_file::file_info::hard_link:
            case memfile::memory_file::file_info::symbolic_link:
            case memfile::memory_file::file_info::continuous:
                {
                    const wpkg_filename::uri_filename filename(info.get_filename());
                    if(filename.glob("*_*_*.deb"))
                    {
                        load_package(filename);
                    }
                    // else skip file it's not a .deb
                    // (although source packages are defined as "*_*.deb")
                }
                break;

            default:
                // ignore anything else
                break;

            }
        }
    }
    else
    {
        load_package(dir_name);
    }
}



std::string html_template(
// The following is a default template that works but may not be good enough
// for your needs. You can specify a new template on the command line with
// the --template command line option
"<html>"
"<head>"
"<title>Package @TITLE@</title>"
"<style>"
"body {"
"background-color: #ffffcc;"
"font-family: sans-serif;"
"}"
"table.package-info {"
"border-top: 1px solid #dddddd;"
"border-spacing: 0;"
"border-collapse: collapse;"
"margin: 10px 5px;"
"}"
"table.package-info td.field-name {"
"text-align: right;"
"vertical-align: top;"
"font-weight: bold;"
"padding-left: 5px;"
"padding-right: 15px;"
"border-right: 1px solid #dddddd;"
"border-bottom: 1px solid #dddddd;"
"white-space: nowrap;"
"}"
"table.package-info td.field-value {"
"padding-left: 15px;"
"padding-right: 5px;"
"border-bottom: 1px solid #dddddd;"
"vertical-align: top;"
"}"
"</style>"
"</head>"
"<body>"
"<h1>Package @TITLE@</h1>"
// repeat what's between @START@ and @END@ for each version, architecture, etc.
"@START@<div style=\"border: 1px solid #888888; padding: 5px 20px; margin: 10px 5px; background-color: white;\">"
"<div style=\"font-weight: bold; font-size: 150%; text-align: center;\">@PACKAGE@ v@VERSION@</div>"
"<div style=\"font-size: 120%; text-align: center;\">@DESCRIPTION@</div>"
"<table class=\"package-info\">"
"<tr><td class=\"field-name\">Package:</td><td class=\"field-value\">@PROVIDES@</td></tr>"
"<tr><td class=\"field-name\">Version:</td><td class=\"field-value\">@VERSION@</td></tr>"
"<tr><td class=\"field-name\">Architecture:</td><td class=\"field-value\">@ARCHITECTURE@</td></tr>"
"<tr><td class=\"field-name\">Distribution:</td><td class=\"field-value\">@DISTRIBUTION@</td></tr>"
"<tr><td class=\"field-name\">Maintainer:</td><td class=\"field-value\">@MAINTAINER@</td></tr>"
"<tr><td class=\"field-name\">Priority:</td><td class=\"field-value\">@PRIORITY@</td></tr>"
"<tr><td class=\"field-name\">Urgency:</td><td class=\"field-value\">@URGENCY@</td></tr>"
"<tr><td class=\"field-name\">Section:</td><td class=\"field-value\">@SECTION@</td></tr>"
"<tr><td class=\"field-name\">Description:</td><td class=\"field-value\">@LONG_DESCRIPTION@</td></tr>"
"<tr><td class=\"field-name\">Links:</td><td class=\"field-value\">@LINKS@</td></tr>"                                       // Bugs, Homepage, Vcs-Browser
"<tr><td class=\"field-name\">Dependencies:</td><td class=\"field-value\">@DEPENDENCIES@</td></tr>"                         // Depends, Pre-Depends, Suggests, Recommends, ..., Build-Depends[-...]
"<tr><td class=\"field-name\">Conflicts:</td><td class=\"field-value\">@CONFLICTS@</td></tr>"                               // Breaks, Conflicts, Build-Conflicts[-...]
"<tr><td class=\"field-name\">Other Packages of Interest:</td><td class=\"field-value\">@OTHER_DEPENDENCIES@</td></tr>"     // recommends, enhances, suggests
"<tr><td class=\"field-name\">Installed-Size:</td><td class=\"field-value\">@INSTALLED_SIZE@ (@INSTALLED_SIZE_BYTES@)</td></tr>"
"<tr><td class=\"field-name\">Packager-Version:</td><td class=\"field-value\">@PACKAGER_VERSION@</td></tr>"
"</table>"
"<div class=\"files\">"
"<p>Files:</p><div>@FILES@</div>"
"</div>"
"</div>@END@"
"<div style=\"border-top: 1px solid black; margin-top: 10px; padding-top: 5px; text-align: center; font-size: 80%; color: #666666;\">Package File Generated by deb2html on @NOW@<br/>"
"See the <a href=\"http://windowspackager.org/\" style=\"color: #6666ff\">Windows Packager</a> website for additional details.</div>"
"</body>"
"</html>"
);


void replace(std::string& out, const std::string& pattern, const std::string& replacement)
{
    for(;;)
    {
        std::string::size_type pos(out.find(pattern));
        if(pos == std::string::npos)
        {
            break;
        }
        out.replace(pos, pattern.length(), replacement);
    }
}


// escape HTML characters
std::string str_to_html(const std::string& str)
{
    std::string result;
    for(const char *s(str.c_str()); *s != '\0'; ++s)
    {
        switch(*s)
        {
        case '<':
            result += "&lt;";
            break;

        case '>':
            result += "&gt;";
            break;

        case '&':
            result += "&amp;";
            break;

        case '"':
            result += "&quot;";
            break;

        case '\'':
            result += "&#39;";
            break;

        default:
            result += *s;
            break;

        }
    }
    return result;
}

void dependency_to_link(std::string& result, const wpkg_filename::uri_filename& package_name, const std::string& field_name)
{
    if(g_manager.field_is_defined(package_name, field_name))
    {
        if(!result.empty())
        {
            result += "<br/>";
        }
        result += field_name + ": ";

        wpkg_dependencies::dependencies deps(g_manager.get_dependencies(package_name, field_name));

        std::string comma;
        int max(deps.size());
        for(int i(0); i < max; ++i)
        {
            const wpkg_dependencies::dependencies::dependency_t& d(deps.get_dependency(i));
            result += comma + "<a href=\"package_" + d.f_name + ".html\">" + d.f_name + "</a>";
            if(!d.f_version.empty())
            {
                result += " (";
                std::string op(d.operator_to_string());
                if(op.length() > 0)
                {
                    result += op + " ";
                }
                result += d.f_version;
                result += ')';
            }
            if(!d.f_architectures.empty())
            {
                result += " [";
                for(std::vector<std::string>::size_type j(0); j < d.f_architectures.size(); ++j)
                {
                    if(j != 0)
                    {
                        result += ' ';
                    }
                    if(d.f_not_arch)
                    {
                        result += '!';
                    }
                    result += d.f_architectures[j];
                }
                result += ']';
            }
            comma = ", ";
        }
    }
}

bool is_letter(char c)
{
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z');
}

void package_to_html(const wpkg_filename::uri_filename& output_directory, const package_t& p, std::string& index)
{
    std::string out(html_template);

    // TODO: sort the filenames with the newest version first

    // first take care of global entries
    const std::string package(g_manager.get_field(p.f_filenames[0], "Package"));
    replace(out, "@TITLE@", package);
    time_t tt;
    time(&tt);
    std::string r(ctime(&tt)); // format as per RFC 822?
    replace(out, "@NOW@", r);

    std::string package_count;
    if(p.f_filenames.size() > 1)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", static_cast<int>(p.f_filenames.size()));
        buf[(sizeof(buf) / sizeof(buf[0])) - 1] = '\0';
        package_count = buf;
        package_count = " (" + package_count + ")";
    }
    index += "<li><a href=\"package_" + package + ".html\">" + package + "</a>" + package_count + "</li>";

    std::string long_description;
    std::string description(str_to_html(g_manager.get_description(p.f_filenames[0], "Description", long_description)));
    replace(out, "@DESCRIPTION@", description);

    std::string::size_type start(out.find("@START@"));
    std::string::size_type end(out.find("@END@"));
    if(start == std::string::npos || end == std::string::npos)
    {
        wpkg_output::log("template %1 does not include the @START@ and/or @END@ markers")
                .quoted_arg(p.f_filenames[0])
            .level(wpkg_output::level_error)
            .action("output");
        return;
    }

    const std::string header(out.substr(0, start));
    start += 7;
    const std::string repeat(out.substr(start, end - start));
    end += 5;
    const std::string footer(out.substr(end));

    std::string body;
    for(std::vector<wpkg_filename::uri_filename>::const_iterator it(p.f_filenames.begin());
            it != p.f_filenames.end(); ++it)
    {
        std::string o(repeat);

        // Package (mandatory field)
        std::string package_name(g_manager.get_field(*it, "Package"));
        replace(o, "@PACKAGE@", package_name);

        // Package (mandatory field), Provides (optional), Essential, Priority
        std::string package_names(g_manager.get_field(*it, "Package"));
        if(g_manager.field_is_defined(*it, "Provides"))
        {
            package_names += ", " + g_manager.get_field(*it, "Provides");
        }
        bool required(false);
        if(g_manager.field_is_defined(*it, "Priority"))
        {
            case_insensitive::case_insensitive_string priority(g_manager.get_field(*it, "Priority"));
            required = priority == "required";
        }
        if(required)
        {
            package_names = "<strong style=\"color: red;\">" + package_names + " (Required)</strong>";
        }
        else if(g_manager.field_is_defined(*it, "Essential") && g_manager.get_field_boolean(*it, "Essential"))
        {
            package_names = "<strong>" + package_names + " (Essential)</strong>";
        }
        replace(o, "@PROVIDES@", package_names);

        // Version (mandatory field)
        replace(o, "@VERSION@", str_to_html(g_manager.get_field(*it, "Version")));

        // Architecture (mandatory field)
        replace(o, "@ARCHITECTURE@", str_to_html(g_manager.get_field(*it, "Architecture")));

        // Distribution
        if(g_manager.field_is_defined(*it, "Distribution"))
        {
            replace(o, "@DISTRIBUTION@", str_to_html(g_manager.get_field(*it, "Distribution")));
        }
        else
        {
            replace(o, "@DISTRIBUTION@", "not specified");
        }

        // Maintainer (mandatory field)
        // TODO transform with a mailto:...
        replace(o, "@MAINTAINER@", str_to_html(g_manager.get_field(*it, "Maintainer")));

        // Priority
        if(g_manager.field_is_defined(*it, "Priority"))
        {
            replace(o, "@PRIORITY@", str_to_html(g_manager.get_field(*it, "Priority")));
        }
        else
        {
            replace(o, "@PRIORITY@", "default (Standard)");
        }

        // Urgency
        if(g_manager.field_is_defined(*it, "Urgency"))
        {
            // XXX -- only show the first line in this placement?
            replace(o, "@URGENCY@", str_to_html(g_manager.get_field(*it, "Urgency")));
        }
        else
        {
            replace(o, "@URGENCY@", "default (Low)");
        }

        // Section
        if(g_manager.field_is_defined(*it, "Section"))
        {
            replace(o, "@SECTION@", str_to_html(g_manager.get_field(*it, "Section")));
        }
        else
        {
            replace(o, "@SECTION@", "Other");
        }

        // Description (mandatory field)
        // XXX fix the formatting
        if(long_description.empty())
        {
            long_description = "(no long description)";
        }
        replace(o, "@LONG_DESCRIPTION@", long_description);

        // Links (Homepage, Bugs, Vcs-Browser)
        std::string links;
        if(g_manager.field_is_defined(*it, "Homepage"))
        {
            // MAKE SURE TO KEEP THIS ONE FIRST!
            if(g_manager.field_is_defined(*it, "Origin"))
            {
                links = "<a href=\"" + g_manager.get_field(*it, "Homepage") + "\">" + str_to_html(g_manager.get_field(*it, "Origin")) + "</a>";
            }
            else
            {
                links = "<a href=\"" + g_manager.get_field(*it, "Homepage") + "\">Homepage</a>";
            }
        }
        if(g_manager.field_is_defined(*it, "Bugs"))
        {
            if(!links.empty())
            {
                links += ", ";
            }
            links += "<a href=\"" + g_manager.get_field(*it, "Bugs") + "\">Bugs</a>";
        }
        if(g_manager.field_is_defined(*it, "Vcs-Browser"))
        {
            if(!links.empty())
            {
                links += ", ";
            }
            links += ", <a href=\"" + g_manager.get_field(*it, "Vcs-Browser") + "\">Source Version Control System</a>";
        }
        if(links.empty())
        {
            links = "no links available";
        }
        replace(o, "@LINKS@", links);

        // Dependencies
        std::string dependencies;
        dependency_to_link(dependencies, *it, "Depends");
        dependency_to_link(dependencies, *it, "Pre-Depends");
        dependency_to_link(dependencies, *it, "Build-Depends");
        dependency_to_link(dependencies, *it, "Build-Depends-Arch");
        dependency_to_link(dependencies, *it, "Build-Depends-Indep");
        dependency_to_link(dependencies, *it, "Built-Using");
        dependency_to_link(dependencies, *it, "");
        if(dependencies.empty())
        {
            dependencies = "no dependencies";
        }
        replace(o, "@DEPENDENCIES@", dependencies);

        // Conflicts
        std::string conflicts;
        dependency_to_link(conflicts, *it, "Conflicts");
        dependency_to_link(conflicts, *it, "Breaks");
        dependency_to_link(conflicts, *it, "Build-Conflicts");
        dependency_to_link(conflicts, *it, "Build-Conflicts-Arch");
        dependency_to_link(conflicts, *it, "Build-Conflicts-Indep");
        if(conflicts.empty())
        {
            conflicts = "no conflicts defined";
        }
        replace(o, "@CONFLICTS@", conflicts);

        // Other Dependencies
        std::string other_dependencies;
        dependency_to_link(other_dependencies, *it, "Replaces");
        dependency_to_link(other_dependencies, *it, "Recommends");
        dependency_to_link(other_dependencies, *it, "Suggests");
        dependency_to_link(other_dependencies, *it, "Enhances");
        if(other_dependencies.empty())
        {
            other_dependencies = "no other dependencies defined";
        }
        replace(o, "@OTHER_DEPENDENCIES@", other_dependencies);

        // Installed-Size
        if(g_manager.field_is_defined(*it, "Installed-Size"))
        {
            replace(o, "@INSTALLED_SIZE@", g_manager.get_field(*it, "Installed-Size") + "Kb");
            uint32_t installed_size(g_manager.get_field_integer(*it, "Installed-Size") * 1024);
            char buf[16];
            snprintf(buf, sizeof(buf), "%d", installed_size);
            buf[(sizeof(buf) / sizeof(buf[0])) - 1] = '\0';
            replace(o, "@INSTALLED_SIZE_BYTES@", buf);
        }
        else
        {
            replace(o, "@INSTALLED_SIZE@", "undefined");
            replace(o, "@INSTALLED_SIZE_BYTES@", "undefined");
        }

        // Packager-Version
        if(g_manager.field_is_defined(*it, "Packager-Version"))
        {
            replace(o, "@PACKAGER_VERSION@", g_manager.get_field(*it, "Packager-Version"));
        }
        else
        {
            replace(o, "@PACKAGER_VERSION@", "undefined");
        }

        // Files
        std::string files_list("<pre class=\"files\">");
        memfile::memory_file files;
        std::string data_filename("data.tar");
        g_manager.get_control_file(files, *it, data_filename, false);
        bool use_drive_letter(false);
        if(g_manager.field_is_defined(*it, "X-Drive-Letter"))
        {
            use_drive_letter = g_manager.get_field_boolean(*it, "X-Drive-Letter");
        }
        files.dir_rewind();
        for(;;) {
            memfile::memory_file::file_info info;
            memfile::memory_file data;
            if(!files.dir_next(info, &data)) {
                break;
            }
            std::string filename(info.get_filename());
            if(filename.length() >= 2 && filename[0] == '.' && filename[1] == '/') {
                filename.erase(0, 1);
            }
            if(use_drive_letter && filename.length() >= 3 && filename[0] == '/' && is_letter(filename[1]) && filename[2] == '/') {
                filename[0] = filename[1] & 0x5F; // capital letter for drives
                filename[1] = ':';
            }

            files_list += info.get_mode_flags() + " ";
            std::string user(info.get_user());
            std::string group(info.get_group());
            if(user.empty() || group.empty())
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "%4d/%-4d", info.get_uid(), info.get_gid());
                buf[(sizeof(buf) / sizeof(buf[0])) - 1] = '\0';
                files_list += buf;
            }
            else
            {
                char buf[32];
                snprintf(buf, sizeof(buf), "%8.8s/%-8.8s", user.c_str(), group.c_str());
                buf[(sizeof(buf) / sizeof(buf[0])) - 1] = '\0';
                files_list += buf;
            }
            if(info.get_file_type() == memfile::memory_file::file_info::character_special
            || info.get_file_type() == memfile::memory_file::file_info::block_special)
            {
                char buf[32];
                snprintf(buf, sizeof(buf), " %3d,%3d", info.get_dev_major(), info.get_dev_minor());
                buf[(sizeof(buf) / sizeof(buf[0])) - 1] = '\0';
                files_list += buf;
            }
            else
            {
                char buf[32];
                snprintf(buf, sizeof(buf), " %7d", info.get_size());
                buf[(sizeof(buf) / sizeof(buf[0])) - 1] = '\0';
                files_list += buf;
            }
            files_list += "  " + info.get_date() + (g_manager.is_conffile(*it, filename) ? " *" : "  ") + filename;
            if(info.get_file_type() == memfile::memory_file::file_info::symbolic_link) {
                files_list += " -> " + info.get_link();
            }
            files_list += "\n";
        }
        files_list += "</pre>";
        replace(o, "@FILES@", files_list);

        // add this entry to the body
        body += o;
    }

    // final output
    out = header + body + footer;

    // write to output file
    memfile::memory_file out_file;
    out_file.create(memfile::memory_file::file_format_other);

    out_file.write(out.c_str(), 0, static_cast<int>(out.length()));

    const wpkg_filename::uri_filename html_filename(output_directory.append_child("package_").append_path(package).append_path(".html"));
    out_file.write_file(html_filename);
}



int main(int argc, char *argv[])
{
    static const advgetopt::getopt::option options[] =
    {
        {
            '\0',
            0,
            NULL,
            NULL,
            "Usage: deb2html [-<opt>] <package> ...",
            advgetopt::getopt::help_argument
        },
        {
            '\0',
            0,
            "admindir",
            "var/lib/wpkg",
            "define the administration directory (i.e. wpkg database folder), default is /var/lib/wpkg",
            advgetopt::getopt::required_argument
        },
        {
            'f',
            0,
            "filename",
            NULL,
            NULL, // hidden argument in --help screen
            advgetopt::getopt::default_multiple_argument
        },
        {
            'h',
            0,
            "help",
            NULL,
            "print this help message",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "help-nobr",
            NULL,
            NULL,
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "instdir",
            "",
            "specify the installation directory, where files get unpacked, by default the root is used",
            advgetopt::getopt::required_argument
        },
        {
            'o',
            0,
            "output",
            NULL,
            "define a directory where the HTML files are saved (one file per package)",
            advgetopt::getopt::required_argument
        },
        {
            'q',
            0,
            "quiet",
            NULL,
            "keep the software quiet",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "root",
            "/",
            "define the root directory (i.e. where everything is installed), default is /",
            advgetopt::getopt::required_argument
        },
        {
            't',
            0,
            "template",
            NULL,
            "filename of the HTML template to use to generate the output",
            advgetopt::getopt::required_argument
        },
        {
            '\0',
            0,
            "version",
            NULL,
            "show the version of deb2html",
            advgetopt::getopt::no_argument
        },
        {
            'v',
            0,
            "verbose",
            NULL,
            "print additional information as available",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "license",
            NULL,
            "displays the license of this tool",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "licence", // French spelling
            NULL,
            NULL, // hidden argument in --help screen
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            NULL,
            NULL,
            NULL,
            advgetopt::getopt::end_of_options
        }
    };

    wpkg_output::set_output(&g_output);

    std::vector<std::string> configuration_files;
    advgetopt::getopt opt(argc, argv, options, configuration_files, "");

    if(opt.is_defined("help") || opt.is_defined("help-nobr"))
    {
        opt.usage(opt.is_defined("help-nobr") ? advgetopt::getopt::no_error_nobr : advgetopt::getopt::no_error, "Usage: deb2html [-<opt>] <package> ...");
        /*NOTREACHED*/
    }

    if(opt.is_defined("version"))
    {
        printf("%s\n", debian_packages_version_string());
        exit(1);
    }

    if(opt.is_defined("license") || opt.is_defined("licence"))
    {
        license::license();
        exit(1);
    }

    if(!opt.is_defined("output"))
    {
        opt.usage(advgetopt::getopt::error, "The --output | -o <directory> option is required");
        /*NOTREACHED*/
    }
    wpkg_filename::uri_filename output_directory(opt.get_string("output"));
    if(!output_directory.exists())
    {
        if(errno != ENOENT)
        {
            opt.usage(advgetopt::getopt::error,
                            "error: file \"%s\" exists but it is not a directory or it is not accessible", output_directory.original_filename().c_str());
            return 1;
        }
        output_directory.os_mkdir_p();
    }
    else
    {
        if(!output_directory.is_dir())
        {
            opt.usage(advgetopt::getopt::error,
                            "error: file name \"%s\" is not a directory", output_directory.original_filename().c_str());
            return 1;
        }
    }

    bool verbose(opt.is_defined("verbose"));
    bool quiet(opt.is_defined("quiet"));

    g_manager.set_root_path(opt.get_string("root"));
    g_manager.set_inst_path(opt.get_string("instdir"));
    g_manager.set_database_path(opt.get_string("admindir"));
    g_output.set_program_name(opt.get_program_name());
    if(verbose)
    {
        g_output.set_level(wpkg_output::level_info);
    }
    else if(quiet)
    {
        g_output.set_level(wpkg_output::level_error);
    }

    // get the size, if zero it's undefined
    int max(opt.size("filename"));
    if(max == 0)
    {
        // if no .deb, try to check for installed packages instead
        wpkgar::wpkgar_lock lock_wpkg(&g_manager, "Listing");
        wpkgar::wpkgar_manager::package_list_t list;
        g_manager.list_installed_packages(list);
        for(wpkgar::wpkgar_manager::package_list_t::const_iterator it(list.begin());
                it != list.end(); ++it)
        {
            wpkg_output::log("found %1")
                    .quoted_arg(*it)
                .package(*it)
                .action("loading");
            wpkgar::wpkgar_manager::package_status_t status(g_manager.package_status(*it));
            switch(status)
            {
            case wpkgar::wpkgar_manager::config_files:
            case wpkgar::wpkgar_manager::unpacked:
            case wpkgar::wpkgar_manager::installed:
                {
                    package_t p;
                    p.f_package_name = *it;
                    p.f_filenames.push_back(*it);
                    g_packages[*it] = p;
                }
                break;

            default:
                // ignore all the other packages
                break;

            }
        }
    }
    else
    {
        // create the list of files from the ones specified on the
        // command line; if we have a directory, recursively search
        // for .deb files
        for(int i = 0; i < max; ++i)
        {
            load_packages(opt.get_string("filename", i));
        }
    }

    if(opt.is_defined("template"))
    {
        // read the user supplied template
        const std::string template_filename(opt.get_string("template"));
        memfile::memory_file template_data;
        template_data.read_file(template_filename);
        std::vector<char> data;
        data.reserve(template_data.size());
        template_data.read(&data[0], 0, template_data.size());
        html_template.assign(&data[0], template_data.size());
    }

    std::string index("<html><head><title>Index</title></head><body><h1>Index</h1><ul>");
    for(package_list_t::const_iterator it(g_packages.begin());
            it != g_packages.end(); ++it)
    {
        package_to_html(output_directory, it->second, index);
    }
    index += "</ul><div style=\"border-top: 1px solid black; margin-top: 10px; padding-top: 5px; text-align: center; font-size: 80%; color: #666666;\">Index Generated by deb2html<br/>"
             "See the <a href=\"http://windowspackager.org/\" style=\"color: #6666ff\">Windows Packager</a> website for additional details.</div>"
             "</body></html>";

    // write to index file
    // (note that the index doesn't work very well if we do not include all the .deb
    // at once... i.e. call deb2html once at the end once your repository is ready)
    memfile::memory_file out_index;
    out_index.create(memfile::memory_file::file_format_other);

    out_index.write(index.c_str(), 0, static_cast<int>(index.length()));

    const wpkg_filename::uri_filename index_filename(output_directory.append_child("index.html"));
    out_index.write_file(index_filename);

    return 0;
}

// vim: ts=4 sw=4 et
