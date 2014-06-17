/*    dep2graph.cpp -- create a dot file of dependencies
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
 * \brief Implementation of the Debian to Graph tool.
 *
 * The deb2graph tool is used to generate a graphical tree of all the
 * Debian packages as specified on the command line. The output can then
 * be parsed by dot to generate the actual tree. Note that such a tree
 * with a very large number of packages can be enormous. We strongly
 * suggest that you at least use SVG as the output file format of DOT.
 */
#include    "libdebpackages/wpkgar.h"
#include    "libdebpackages/wpkg_control.h"
#include    "libdebpackages/advgetopt.h"
#include    "libdebpackages/debian_packages.h"
#include    "license.h"
#include    <stdio.h>
#include    <stdlib.h>

/** \brief Brief information about a package.
 *
 * This structure holds information about a package:
 *
 * \li Its name
 * \li Its filename
 * \li Its number (node count).
 */
struct package_info_t
{
    std::string     f_package;
    std::string     f_filename;
    int             f_nodecount;
};

typedef std::vector<package_info_t> node_names_t;
namespace {
wpkgar::wpkgar_manager manager;
memfile::memory_file dot;
int node_count = 0;
} // noname namespace

node_names_t find_nodes(node_names_t names, const std::string& n)
{
    node_names_t result;
    for(node_names_t::const_iterator it(names.begin()); it != names.end(); ++it) {
        if(it->f_package == n || it->f_filename == n) {
            result.push_back(*it);
        }
    }
    return result;
}

void add_nodes(node_names_t& nodes, node_names_t& deps, const std::string field_name)
{
    dot.printf("/* Field: %s */\n", field_name.c_str());
    for(node_names_t::const_iterator it(nodes.begin()); it != nodes.end(); ++it) {
        if(manager.field_is_defined(it->f_filename, field_name)) {
            wpkg_dependencies::dependencies depends(manager.get_dependencies(it->f_filename, field_name));
            for(int i(0); i < depends.size(); ++i) {
                const wpkg_dependencies::dependencies::dependency_t& dep(depends.get_dependency(i));
                node_names_t packages(find_nodes(nodes, dep.f_name));
                if(packages.empty()) {
                    packages = find_nodes(deps, dep.f_name);
                    if(packages.empty()) {
                        // it's not defined yet, add it as a dependency
                        package_info_t info;
                        info.f_package = dep.f_name;
                        info.f_filename = dep.f_name;
                        info.f_nodecount = node_count;
                        deps.push_back(info);
                        dot.printf("n%d [label=\"%s\",shape=ellipse];\n", node_count, dep.f_name.c_str());
                        ++node_count;
                        packages = find_nodes(deps, dep.f_name);
                    }
                }
                if(dep.f_operator == wpkg_dependencies::dependencies::operator_any) {
                    // if any operator, then version doesn't apply,
                    // put an empty string instead
                    dot.printf("edge [headlabel=\"\"];\n");
                }
                else {
                    std::string op(dep.operator_to_string());
                    if(op.length() == 0) {
                        op = "=";
                    }
                    std::string version(op + " " + dep.f_version);
                    dot.printf("edge [headlabel=\"\\rversion %s\"];\n", version.c_str());
                }
                for(node_names_t::const_iterator pt(packages.begin()); pt != packages.end(); ++pt) {
                    dot.printf("n%d -> n%d;\n", it->f_nodecount, pt->f_nodecount);
                }
            }
        }
    }
}



int main(int argc, char *argv[])
{
    static const advgetopt::getopt::option options[] = {
        {
            '\0',
            0,
            NULL,
            NULL,
            "Usage: dep2graph [-<opt>] <package> ...",
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
            '\0',
            0,
            "instdir",
            "",
            "specify the installation directory, where files get unpacked, by default the root is used",
            advgetopt::getopt::required_argument
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
            'o',
            0,
            "output",
            NULL,
            "define a filename where the final PNG is saved",
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
            "version",
            NULL,
            "show the version of deb2graph",
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

    std::vector<std::string> configuration_files;
    advgetopt::getopt opt(argc, argv, options, configuration_files, "");

    if(opt.is_defined("help") || opt.is_defined("help-nobr"))
    {
        opt.usage(opt.is_defined("help-nobr") ? advgetopt::getopt::no_error_nobr : advgetopt::getopt::no_error, "Usage: deb2graph [-<opt>] <package> ...");
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

    bool verbose(opt.is_defined("verbose"));

    // get the size, if zero it's undefined
    int max(opt.size("filename"));
    if(max == 0)
    {
        opt.usage(advgetopt::getopt::error, "at least one debian package must be specified on the command line");
        /*NOTREACHED*/
    }

    // all these directories have a default if not specified on the command line
    manager.set_root_path(opt.get_string("root"));
    manager.set_inst_path(opt.get_string("instdir"));
    manager.set_database_path(opt.get_string("admindir"));
    manager.set_control_file_state(std::shared_ptr<wpkg_control::control_file::control_file_state_t>(new wpkg_control::control_file::contents_control_file_state_t));

    // start creating the .dot file
    dot.create(memfile::memory_file::file_format_other);
    dot.printf("digraph {\nrankdir=BT;\nlabel=\"Debian Package Dependency Graph\";\n");

    node_names_t nodes;
    node_names_t deps; // dependencies not found on the command line

    // load all the packages
    dot.printf("/* Explicit Packages */\n");
    for(int i = 0; i < max; ++i)
    {
        std::string package_filename(opt.get_string("filename", i));
        // avoid adding the exact same package more than once
        if(verbose)
        {
            printf("Package \"%s\" loaded.\n", package_filename.c_str());
        }
        manager.load_package(package_filename);
        std::string package(manager.get_field(package_filename, "Package"));
        package_info_t info;
        info.f_package = package;
        info.f_filename = package_filename;
        info.f_nodecount = node_count;
        nodes.push_back(info);
        std::string version(manager.get_field(package_filename, "Version"));
        std::string architecture(manager.get_field(package_filename, "Architecture"));
        dot.printf("n%d [label=\"%s %s\\n%s\",shape=box];\n", node_count, package.c_str(), version.c_str(), architecture.c_str());
        ++node_count;
    }

    // edges font size to small
    dot.printf("edge [fontsize=8,fontcolor=\"#990033\",color=\"#cccccc\"];\n");

    // use the dependency fields to define all the nodes of the graph
    dot.printf("edge [style=dashed];\n");
    add_nodes(nodes, deps, "Build-Depends");
    dot.printf("edge [style=bold,color=\"#8888ff\"];\n");
    add_nodes(nodes, deps, "Pre-Depends");
    dot.printf("edge [style=solid,color=\"#aaaaaa\"];\n");
    add_nodes(nodes, deps, "Depends");
    dot.printf("edge [color=\"#ff8888\"];\n");
    add_nodes(nodes, deps, "Breaks");
    dot.printf("edge [style=bold,arrowhead=tee];\n");
    add_nodes(nodes, deps, "Conflicts");

    // close the digraph
    dot.printf("}\n");

    dot.write_file("deb2graph.dot");

    std::string cmd("dot -Tsvg deb2graph.dot >");
    if(opt.is_defined("output"))
    {
        cmd += opt.get_string("output");
    }
    else
    {
        cmd += "deb2graph.svg";
    }

    const int status = system(cmd.c_str());
    if( status == -1 )
    {
        // TODO: Add a message here stating the command could not be launched...
        return 1;
    }

    return status;
}

// vim: ts=4 sw=4 et

