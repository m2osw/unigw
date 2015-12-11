//===============================================================================
// Copyright:	Copyright (c) 2015 Made to Order Software Corp.
//
// All Rights Reserved.
//
// The source code in this file ("Source Code") is provided by Made to Order Software Corporation
// to you under the terms of the GNU General Public License, version 2.0
// ("GPL").  Terms of the GPL can be found in doc/GPL-license.txt in this distribution.
// 
// By copying, modifying or distributing this software, you acknowledge
// that you have read and understood your obligations described above,
// and agree to abide by those obligations.
// 
// ALL SOURCE CODE IN THIS DISTRIBUTION IS PROVIDED "AS IS." THE AUTHOR MAKES NO
// WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
// COMPLETENESS OR PERFORMANCE.
//===============================================================================

#include "DisplayThread.h"

#include <libdebpackages/wpkg_output.h>

#include <time.h>

using namespace wpkgar;

namespace
{
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
            "<tr><td class=\"field-name\">Primary Section:</td><td class=\"field-value\">@PRIMARY_SECTION@</td></tr>"
            "<tr><td class=\"field-name\">Secondary Section:</td><td class=\"field-value\">@SECONDARY_SECTION</td></tr>"
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
            "<div style=\"border-top: 1px solid black; margin-top: 10px; padding-top: 5px; text-align: center; font-size: 80%; color: #666666;\">Package File Generated by Package Explorer on @NOW@<br/>"
            "See the <a href=\"http://windowspackager.org/\" style=\"color: #6666ff\">Windows Packager</a> website for additional details.</div>"
            "</body>"
            "</html>"
            );


    void replace( std::string& out, const std::string& pattern, const std::string& replacement )
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
    std::string str_to_html( const std::string& str )
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

    bool is_letter( const char c )
    {
        return (c >= 'a' && c <= 'z')
            || (c >= 'A' && c <= 'Z');
    }
}
// namespace

#include <iostream>

DisplayThread::DisplayThread
    ( QObject* p
    , QString currentPkg
    )
        : QThread(p)
        , f_currentPackage(currentPkg)
        , f_mainManager(Manager::WeakInstance())
        , f_manager(f_mainManager->GetManager().lock() )
{
}


void DisplayThread::run()
{
	try
	{
        QMutexLocker locker( &Manager::WeakInstance()->GetMutex() );

        // Load the installed packages into memory
        //
        wpkgar_manager::package_list_t list;
        f_manager->list_installed_packages( list );
        for( auto pkg : list )
        {
            f_manager->load_package( pkg );
        }

        GeneratePackageHtml();
	}
    catch( const wpkgar_exception& except )
    {
        qCritical() << "wpkgar_exception caught! what=" << except.what();
        LogOutput::Instance()->OutputToLog( wpkg_output::level_error, except.what() );
    }
    catch( ... )
    {
        qCritical() << "unknown exception caught!";
        LogOutput::Instance()->OutputToLog( wpkg_output::level_error, "unknown exception!" );
    }
}


void DisplayThread::DependencyToLink(
	std::string& result, const std::string& package_name, const std::string& field_name
	) const
{
    if(f_manager->field_is_defined(package_name, field_name))
    {
        if(!result.empty())
        {
            result += "<br/>";
        }
        result += field_name + ": ";

        wpkg_dependencies::dependencies deps(f_manager->get_dependencies(package_name, field_name));

        std::string comma;
        const int max_deps(deps.size());
        for(int i(0); i < max_deps; ++i)
        {
            const wpkg_dependencies::dependencies::dependency_t& d(deps.get_dependency(i));
            result += comma + "<a href=\"package://" + d.f_name + "\">" + d.f_name + "</a>";
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


void DisplayThread::GeneratePackageHtml()
{
    f_html = html_template;
    emit AddMessage( QString("Reading package %1").arg(f_currentPackage) );
	const std::string package_name = f_currentPackage.toStdString();

    // TODO: sort the filenames with the newest version first

    // first take care of global entries
    const std::string package(f_manager->get_field(package_name, "Package"));
    replace(f_html, "@TITLE@", package);
    time_t tt;
    time(&tt);
    std::string r(ctime(&tt)); // format as per RFC 822?
    replace(f_html, "@NOW@", r);

    std::string package_count;
    if(package_name.size() > 1)
    {
        char buf[16];
        snprintf(buf, sizeof(buf), "%d", static_cast<int>(package_name.size()));
        buf[(sizeof(buf) / sizeof(buf[0])) - 1] = '\0';
        package_count = buf;
        package_count = " (" + package_count + ")";
    }

    std::string long_description;
    std::string description(str_to_html(f_manager->get_description(package_name, "Description", long_description)));
    replace(f_html, "@DESCRIPTION@", description);

    std::string::size_type _start(f_html.find("@START@"));
    std::string::size_type _end(f_html.find("@END@"));
    if(_start == std::string::npos || _end == std::string::npos)
    {
		wpkg_output::message_t msg;
		msg.set_level( wpkg_output::level_error );
		msg.set_package_name( package_name );
        msg.set_raw_message( "error: template does not include the @START@ and/or @END@ markers" );
        wpkg_output::get_output()->log( msg );
        return;
    }

    const std::string header(f_html.substr(0, _start));
    _start += 7;
    const std::string repeat(f_html.substr(_start, _end - _start));
    _end += 5;
    const std::string footer(f_html.substr(_end));

    std::string body;
    {
        std::string o(repeat);

        // Package (mandatory field)
        replace(o, "@PACKAGE@", package_name);

        // Package (mandatory field), Provides (optional), Essential, Priority
        std::string package_names(f_manager->get_field(package_name, "Package"));
        if(f_manager->field_is_defined(package_name, "Provides"))
        {
            package_names += ", " + f_manager->get_field(package_name, "Provides");
        }
        bool required(false);
        if(f_manager->field_is_defined(package_name, "Priority"))
        {
            case_insensitive::case_insensitive_string _priority(f_manager->get_field(package_name, "Priority"));
            required = _priority == "required";
        }
        if(required)
        {
            package_names = "<strong style=\"color: red;\">" + package_names + " (Required)</strong>";
        }
        else if(f_manager->field_is_defined(package_name, "Essential") && f_manager->get_field_boolean(package_name, "Essential"))
        {
            package_names = "<strong>" + package_names + " (Essential)</strong>";
        }
        replace(o, "@PROVIDES@", package_names);

        // Version (mandatory field)
        replace(o, "@VERSION@", str_to_html(f_manager->get_field(package_name, "Version")));

        // Architecture (mandatory field)
        replace(o, "@ARCHITECTURE@", str_to_html(f_manager->get_field(package_name, "Architecture")));

        // Distribution
        if( f_manager->field_is_defined(package_name, "Distribution") )
        {
            replace(o, "@DISTRIBUTION@", str_to_html(f_manager->get_field(package_name, "Distribution")));
        }
        else
        {
            replace(o, "@DISTRIBUTION@", "not specified");
        }

        // Maintainer (mandatory field)
        // TODO transform with a mailto:...
        replace(o, "@MAINTAINER@", str_to_html(f_manager->get_field(package_name, "Maintainer")));

        // Priority
        if(f_manager->field_is_defined(package_name, "Priority"))
        {
            replace(o, "@PRIORITY@", str_to_html(f_manager->get_field(package_name, "Priority")));
        }
        else
        {
            replace(o, "@PRIORITY@", "default (Standard)");
        }

        // Urgency
        if(f_manager->field_is_defined(package_name, "Urgency"))
        {
            // XXX -- only show the first line in this placement?
            replace(o, "@URGENCY@", str_to_html(f_manager->get_field(package_name, "Urgency")));
        }
        else
        {
            replace(o, "@URGENCY@", "default (Low)");
        }

        // Section
        if(f_manager->field_is_defined(package_name, "Section"))
        {
            replace(o, "@SECTION@", str_to_html(f_manager->get_field(package_name, "Section")));
        }
        else
        {
            replace(o, "@SECTION@", "Other");
        }

        // X-PrimarySection
        if(f_manager->field_is_defined(package_name, "X-PrimarySection"))
        {
            replace(o, "@PRIMARY_SECTION@", str_to_html(f_manager->get_field(package_name, "X-PrimarySection")));
        }
        else
        {
            replace(o, "@PRIMARY_SECTION@", "Undefined");
        }

        // X-SecondarySection
        if(f_manager->field_is_defined(package_name, "X-SecondarySection"))
        {
            replace(o, "@SECONDARY_SECTION", str_to_html(f_manager->get_field(package_name, "X-SecondarySection")));
        }
        else
        {
            replace(o, "@SECONDARY_SECTION", "Undefined");
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
        if(f_manager->field_is_defined(package_name, "Homepage"))
        {
            // MAKE SURE TO KEEP THIS ONE FIRST!
            if(f_manager->field_is_defined(package_name, "Origin"))
            {
                links = "<a href=\"" + f_manager->get_field(package_name, "Homepage") + "\">" + str_to_html(f_manager->get_field(package_name, "Origin")) + "</a>";
            }
            else
            {
                links = "<a href=\"" + f_manager->get_field(package_name, "Homepage") + "\">Homepage</a>";
            }
        }
        if(f_manager->field_is_defined(package_name, "Bugs"))
        {
            if(!links.empty())
            {
                links += ", ";
            }
            links += "<a href=\"" + f_manager->get_field(package_name, "Bugs") + "\">Bugs</a>";
        }
        if(f_manager->field_is_defined(package_name, "Vcs-Browser"))
        {
            if(!links.empty())
            {
                links += ", ";
            }
            links += ", <a href=\"" + f_manager->get_field(package_name, "Vcs-Browser") + "\">Source Version Control System</a>";
        }
        if(links.empty())
        {
            links = "no links available";
        }
        replace(o, "@LINKS@", links);

        // Dependencies
        std::string dependencies;
        DependencyToLink(dependencies, package_name, "Depends");
        DependencyToLink(dependencies, package_name, "Pre-Depends");
        DependencyToLink(dependencies, package_name, "Build-Depends");
        DependencyToLink(dependencies, package_name, "Build-Depends-Arch");
        DependencyToLink(dependencies, package_name, "Build-Depends-Indep");
        DependencyToLink(dependencies, package_name, "Built-Using");
        DependencyToLink(dependencies, package_name, "");
        if(dependencies.empty())
        {
            dependencies = "no dependencies";
        }
        replace(o, "@DEPENDENCIES@", dependencies);

        // Conflicts
        std::string conflicts;
        DependencyToLink(conflicts, package_name, "Conflicts");
        DependencyToLink(conflicts, package_name, "Breaks");
        DependencyToLink(conflicts, package_name, "Build-Conflicts");
        DependencyToLink(conflicts, package_name, "Build-Conflicts-Arch");
        DependencyToLink(conflicts, package_name, "Build-Conflicts-Indep");
        if(conflicts.empty())
        {
            conflicts = "no conflicts defined";
        }
        replace(o, "@CONFLICTS@", conflicts);

        // Other Dependencies
        std::string other_dependencies;
        DependencyToLink(other_dependencies, package_name, "Replaces");
        DependencyToLink(other_dependencies, package_name, "Recommends");
        DependencyToLink(other_dependencies, package_name, "Suggests");
        DependencyToLink(other_dependencies, package_name, "Enhances");
        if(other_dependencies.empty())
        {
            other_dependencies = "no other dependencies defined";
        }
        replace(o, "@OTHER_DEPENDENCIES@", other_dependencies);

        // Installed-Size
        if(f_manager->field_is_defined(package_name, "Installed-Size"))
        {
            replace(o, "@INSTALLED_SIZE@", f_manager->get_field(package_name, "Installed-Size") + "Kb");
            uint32_t installed_size(f_manager->get_field_integer(package_name, "Installed-Size") * 1024);
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
        if(f_manager->field_is_defined(package_name, "Packager-Version"))
        {
            replace(o, "@PACKAGER_VERSION@", f_manager->get_field(package_name, "Packager-Version"));
        }
        else
        {
            replace(o, "@PACKAGER_VERSION@", "undefined");
        }

        // Files
        std::string files_list("<pre class=\"files\">");
        memfile::memory_file files;
        std::string data_filename("data.tar");
        f_manager->get_control_file(files, package_name, data_filename, false);
        bool use_drive_letter(false);
        if(f_manager->field_is_defined(package_name, "X-Drive-Letter"))
        {
            use_drive_letter = f_manager->get_field_boolean(package_name, "X-Drive-Letter");
        }
        files.dir_rewind();
        for(;;) {
            memfile::memory_file::file_info info;
            memfile::memory_file data;
            if(!files.dir_next(info, &data)) {
                break;
            }
            std::string filename(info.get_filename());

            emit AddMessage( QString("Processing filename %1").arg(filename.c_str()) );

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
            files_list += "  " + info.get_date() + (f_manager->is_conffile(package_name, filename) ? " *" : "  ") + filename;
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
    f_html = header + body + footer;
}

// vim: ts=4 sw=4 et
