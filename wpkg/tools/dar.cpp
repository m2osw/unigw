/*    dar -- create ar or tar archives (like the ar/tar tools)
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
 * \brief The implementation of the dar tool.
 *
 * The dar tool can be used to read and generate archives of all the
 * different types supported by wpkg. This includes tar, ar, and wpkgar.
 * It also supports the different compressions, which at this point are gz
 * and bz2.
 *
 * There is much to do to ameliorate and make this tool much more user
 * friendly than it currently is.
 */
#include    "libdebpackages/advgetopt.h"
#include    "libdebpackages/memfile.h"
#include    "libdebpackages/debian_packages.h"
#include    "license.h"
#include    <stdio.h>
#include    <stdlib.h>



void show_md5sum(const memfile::memory_file& m, const memfile::memory_file::file_info& info, const memfile::memory_file& data)
{
    md5::raw_md5sum raw;
    memfile::memory_file::file_info::file_type_t type(info.get_file_type());
    if(m.get_format() != memfile::memory_file::file_format_wpkg)
    {
        if(type == memfile::memory_file::file_info::regular_file
        || type == memfile::memory_file::file_info::continuous)
        {
            data.raw_md5sum(raw);
        }
    }
    else
    {
        raw = info.get_raw_md5sum();
    }
    switch(type)
    {
    case memfile::memory_file::file_info::regular_file:
    case memfile::memory_file::file_info::continuous:
        printf("%s  ", md5::md5sum::sum(raw).c_str());
        break;

    default:
        printf("--------------------------------  ");
        break;

    }
}

int dar(int argc, char *argv[])
{
    static const advgetopt::getopt::option options[] = {
        {
            '\0',
            0,
            NULL,
            NULL,
            "Usage: dar [-<opt>] <archive> <member> ...",
            advgetopt::getopt::help_argument
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
            NULL, // don't show in output
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "version",
            NULL,
            "show the version of dar",
            advgetopt::getopt::no_argument
        },
        {
            'c',
            0,
            "create",
            NULL,
            "create an archive from a set of members or a folder",
            advgetopt::getopt::no_argument
        },
        {
            'x',
            0,
            "extract",
            NULL,
            "extract the archive contents",
            advgetopt::getopt::no_argument
        },
        {
            't',
            0,
            "list",
            NULL,
            "list the archive contents",
            advgetopt::getopt::no_argument
        },
        {
            '\0',
            0,
            "verify",
            NULL,
            "check the archive validity",
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
            'q',
            0,
            "quiet",
            NULL,
            "don't print errors about invalid archives (useful with --verify)",
            advgetopt::getopt::no_argument
        },
        {
            's',
            0,
            "md5sums",
            NULL,
            "display the md5sums before the filename",
            advgetopt::getopt::no_argument
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

    if(opt.is_defined("help"))
    {
        opt.usage(advgetopt::getopt::no_error, "Usage: dar [-<opt>] <archive> <member> ...");
        /*NOTREACHED*/
    }
    if(opt.is_defined("help-nobr"))
    {
        opt.usage(advgetopt::getopt::no_error_nobr, "Usage: dar [-<opt>] <archive> <member> ...");
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

    bool create(opt.is_defined("create"));
    bool extract(opt.is_defined("extract"));
    bool list(opt.is_defined("list"));
    bool verify(opt.is_defined("verify"));
    bool quiet(opt.is_defined("quiet"));
    bool verbose(opt.is_defined("verbose"));
    bool md5sums(opt.is_defined("md5sums"));

    if(create + extract + list + verify != 1)
    {
        opt.usage(advgetopt::getopt::no_error, "exactly one of --create, --extract, --list, or --check must be specified");
    }

    // get the size, if zero it's undefined
    int max(opt.size("filename"));
    if(max == 0)
    {
        opt.usage(advgetopt::getopt::error, "archive filename is necessary");
        /*NOTREACHED*/
    }
    std::string archive(opt.get_string("filename", 0));

    if(list + verify + extract)
    {
        memfile::memory_file m;
        m.read_file(archive);
        if(m.is_compressed())
        {
            memfile::memory_file d;
            m.copy(d);
            d.decompress(m);
        }
        if(m.get_format() != memfile::memory_file::file_format_ar
        && m.get_format() != memfile::memory_file::file_format_tar
        && m.get_format() != memfile::memory_file::file_format_zip
        && m.get_format() != memfile::memory_file::file_format_7z
        && m.get_format() != memfile::memory_file::file_format_wpkg)
        {
            if(!quiet || !verify)
            {
                fprintf(stderr, "error:%s: %s is not an ar or tar archive.\n", opt.get_program_name().c_str(), archive.c_str());
            }
            exit(1);
        }
        std::map<std::string, int> member;
        for(int i = 1; i < max; ++i)
        {
            std::string file(opt.get_string("filename", i));
            member[file] = 1;
        }
        m.dir_rewind();
        for(;;)
        {
            memfile::memory_file::file_info info;
            memfile::memory_file data;
            if(extract)
            {
                if(!m.dir_next(info, &data))
                {
                    break;
                }
                if(member.empty() || member.count(info.get_filename()) != 0)
                {
                    if(verbose)
                    {
                        printf("x - %s\n", info.get_filename().c_str());
                    }
                    // TODO: support for symbolic links
                    data.write_file(info.get_filename());
                    // we need to also setup the user, group, mode of the file
                    //info.apply_attributes();
                }
            }
            else
            {
                if(md5sums && m.get_format() != memfile::memory_file::file_format_wpkg)
                {
                    if(info.get_file_type() == memfile::memory_file::file_info::regular_file
                    || info.get_file_type() == memfile::memory_file::file_info::continuous)
                    {
                        if(!m.dir_next(info, &data))
                        {
                            break;
                        }
                    }
                }
                else
                {
                    if(!m.dir_next(info))
                    {
                        break;
                    }
                }
                std::string filename(info.get_filename());
                if(member.empty() || member.count(filename) != 0)
                {
                    if(list)
                    {
                        if(verbose)
                        {
                            // TODO? to properly format all the widths we
                            // would need to save all the data and all the
                            // max. length, then we could write the perfect
                            // output using all those max. length
                            printf("%s ", info.get_mode_flags().c_str());
                            std::string user(info.get_user());
                            std::string group(info.get_group());
                            if(user.empty() || group.empty())
                            {
                                printf("%d/%d",
                                        info.get_uid(), info.get_gid());
                            }
                            else
                            {
                                printf("%8.8s/%-8.8s",
                                    user.c_str(), group.c_str());
                            }
                            printf(" %6d  %s  ",
                                    info.get_size(),
                                    info.get_date().c_str());
                            if(md5sums)
                            {
                                show_md5sum(m, info, data);
                            }
                            printf("%s", filename.c_str());
                            wpkgar::wpkgar_block_t::wpkgar_compression_t original_compression(info.get_original_compression());
                            switch(original_compression)
                            {
                            case wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_GZ:
                                printf("[.gz]");
                                break;

                            case wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_BZ2:
                                printf("[.bz2]");
                                break;

                            case wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_LZMA:
                                printf("[.lzma]");
                                break;

                            case wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_XZ:
                                printf("[.xz]");
                                break;

                            case wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_NONE:
                                // no default extension
                                break;

                            }
                            if(info.get_file_type() == memfile::memory_file::file_info::symbolic_link)
                            {
                                printf(" -> %s", info.get_link().c_str());
                            }
                            printf("\n");
                        }
                        else if(md5sums)
                        {
                            switch(info.get_file_type())
                            {
                            case memfile::memory_file::file_info::regular_file:
                            case memfile::memory_file::file_info::continuous:
                                show_md5sum(m, info, data);
                                printf("%s\n", filename.c_str());
                                break;

                            default:
                                // special files and directories do not have an md5sum
                                break;

                            }
                        }
                        else
                        {
                            printf("%s\n", filename.c_str());
                        }
                    }
                    else if(verbose)
                    {
                        printf("%s\n", filename.c_str());
                    }
                    member.erase(filename);
                }
            }
        }
        if(verify && !member.empty())
        {
            fprintf(stderr, "error:%s: the following members are not present in this archive:\n", opt.get_program_name().c_str());
            for(std::map<std::string, int>::const_iterator it(member.begin());
                        it != member.end(); ++it)
            {
                fprintf(stderr, "  [%s]\n", it->first.c_str());
            }
            // this is considered an error
            exit(1);
        }
    }
    else if(create)
    {
        // in this case archive is the output filename and the
        // members are the input filenames
        memfile::memory_file dar;
        memfile::memory_file::file_format_t ar_format(memfile::memory_file::filename_extension_to_format(archive, true));
        switch(ar_format)
        {
        case memfile::memory_file::file_format_ar:
        case memfile::memory_file::file_format_tar:
        case memfile::memory_file::file_format_zip:
        case memfile::memory_file::file_format_7z:
        case memfile::memory_file::file_format_wpkg:
            break;

        default:
            opt.usage(advgetopt::getopt::no_error, "unsupported archive file extension (we support .deb, .a, .tar)");
            /*NOTREACHED*/
            break;

        }
        dar.create(ar_format);
        dar.set_package_path(".");
        for(int i = 1; i < max; ++i)
        {
            std::string file(opt.get_string("filename", i));
            memfile::memory_file data;
            memfile::memory_file::file_info info;
            data.read_file(file, &info);
            while(ar_format == memfile::memory_file::file_format_tar
            && info.get_filename().find_first_of('/') == 0)
            {
                // tar generally forbids / as the first character(s)
                info.set_filename(info.get_filename().substr(1));
            }
            dar.append_file(info, data);
        }
        dar.end_archive();
        memfile::memory_file::file_format_t format(memfile::memory_file::filename_extension_to_format(archive));
        switch(format)
        {
        case memfile::memory_file::file_format_gz:
        case memfile::memory_file::file_format_bz2:
        case memfile::memory_file::file_format_lzma:
        case memfile::memory_file::file_format_xz:
            {
                memfile::memory_file compressed;
                dar.compress(compressed, format);
                compressed.write_file(archive);
            }
            break;

        default: // we don't prevent any extension here
            dar.write_file(archive);
            break;

        }
    }
    else
    {
        throw std::logic_error("this case should never be reached");
    }

    return 0;
}


int main(int argc, char *argv[])
{
    try
    {
        dar(argc, argv);
    }
    catch(const std::exception& e)
    {
        fprintf(stderr, "exception: %s\n", e.what());
        exit(1);
    }

    exit(0);
}


// vim: ts=4 sw=4 et
