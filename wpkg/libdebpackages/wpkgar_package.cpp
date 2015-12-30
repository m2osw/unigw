/*    wpkgar.cpp -- implementation of the wpkg archive
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

#include "libdebpackages/wpkgar_package.h"
#include "libdebpackages/wpkgar_exception.h"

#include <unistd.h>

namespace wpkgar
{

/** \brief File in an archive.
 *
 * This class records the offset where the named file is found in an
 * archive. It is used so we can immediately access the file without
 * having to scan the entire archive again and again.
 *
 * \param[in] offset  The offset where the file starts in the archive.
 * \param[in] info  The file information (name, mode, etc.)
 */
wpkgar_package::wpkgar_file::wpkgar_file(int offset, const memfile::memory_file::file_info& info)
    //: f_modified -- auto-init
    : f_offset(offset)
    //, f_data_dir_pos -- auto-init
    , f_info(info)
{
}

/** \brief Get the file offset.
 *
 * This function is used to retrieve the offset where this file data
 * start. This is the offset as specified to the constructor.
 *
 * \return The offset passed to the constructor.
 */
int wpkgar_package::wpkgar_file::get_offset() const
{
    return f_offset;
}

/** \brief Set the data position.
 *
 * When creating a wpkg archive, the offset represents the position of
 * the file info in the wpkg archive, not the data in the source data.tar.gz
 * file. This function is used to save that extraneous offset.
 *
 * \param[in] pos  The byte position of the data in the data archive.
 */
void wpkgar_package::wpkgar_file::set_data_dir_pos(int pos)
{
    // this position is in an archive (ar or tar) and thus
    // we cannot really check its validity here
    f_data_dir_pos = pos;
}

/** \brief Retrieve the data position.
 *
 * This function is used to retrieve the data found in the data.tar.gz
 * file without having to scan all the data to get to it.
 *
 * \return The data position as defined by the set_data_dir_pos() function.
 */
int wpkgar_package::wpkgar_file::get_data_dir_pos() const
{
    return f_data_dir_pos;
}


wpkgar_package::wpkgar_package(
        const wpkg_filename::uri_filename& fullname,
        std::shared_ptr<wpkg_control::control_file::control_file_state_t> control_file_state)
    //, f_package_path -- auto-init (to invalid)
    : f_fullname(fullname)
    //, f_modified -- auto-init
    //, f_files -- auto-init
    //, f_wpkgar_file -- auto-init
    , f_control_file(control_file_state)
    , f_status_file()
{
	// This doesn't seem to do anything in the manager, so remarked out.
    //manager->set_control_variables(f_control_file);
}

void wpkgar_package::get_wpkgar_file(memfile::memory_file *& wpkgar_file_out)
{
    wpkgar_file_out = &f_wpkgar_file;
}

void wpkgar_package::set_package_path(const wpkg_filename::uri_filename& path)
{
    f_package_path = path;
}

const wpkg_filename::uri_filename& wpkgar_package::get_package_path() const
{
    return f_package_path;
}

const wpkg_filename::uri_filename& wpkgar_package::get_fullname() const
{
    return f_fullname;
}

void wpkgar_package::read_package()
{
    if(f_wpkgar_file.size() != 0)
    {
        throw wpkgar_exception_invalid("this package was already read (size != 0)");
    }
    if(f_package_path.empty())
    {
        throw wpkgar_exception_invalid("database package path is still undefined");
    }

    // wpkgar index
    f_wpkgar_file.read_file(f_package_path.append_child("index.wpkgar"));
    f_wpkgar_file.dir_rewind();
    for(;;)
    {
        memfile::memory_file::file_info info;
        int p(f_wpkgar_file.dir_pos());
        if(!f_wpkgar_file.dir_next(info, NULL))
        {
            break;
        }
        std::shared_ptr<wpkgar_file> file(new wpkgar_file(p, info));
        f_files[info.get_filename()] = file;
        // we don't have the offset in the data file so that's it here
    }

    // control file
    {
        memfile::memory_file data;
        data.read_file(f_package_path.append_child("control"));
        f_control_file.set_input_file(&data);
        f_control_file.read();
        f_control_file.set_input_file(NULL);
    }

    // status file
    {
        memfile::memory_file data;
        data.read_file(f_package_path.append_child("wpkg-status"));
        f_status_file.set_input_file(&data);
        f_status_file.read();
        f_status_file.set_input_file(NULL);
    }
}

void wpkgar_package::read_archive(memfile::memory_file& p)
{
    if(f_wpkgar_file.size() != 0)
    {
        throw wpkgar_exception_invalid("this package was already read (size != 0)");
    }
    if(f_package_path.empty())
    {
        throw wpkgar_exception_invalid("database package path is still undefined");
    }
    f_wpkgar_file.create(memfile::memory_file::file_format_wpkg);
    f_wpkgar_file.set_package_path(f_package_path);

    // reading the ar file (top level)
    p.dir_rewind();
    bool has_debian_binary(false);
    bool has_control_tar_gz(false);
    bool has_data_tar_gz(false);
    for(;;)
    {
        memfile::memory_file::file_info info;
        memfile::memory_file data;
        if(!p.dir_next(info, &data))
        {
            break;
        }
        std::string filename(info.get_filename());
        if(filename.find('/') != std::string::npos)
        {
            // this should never happen since it's not allowed in 'ar'
            throw wpkgar_exception_invalid("the .deb file includes a file with a slash (/) character");
        }
        if(f_files.find(filename) != f_files.end())
        {
            throw wpkgar_exception_invalid("the .deb control files include two files with the same name");
        }
        wpkgar::wpkgar_block_t::wpkgar_compression_t compression(wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_NONE);
        switch(data.get_format())
        {
        case memfile::memory_file::file_format_gz:
            compression = wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_GZ;
            break;

        case memfile::memory_file::file_format_bz2:
            compression = wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_BZ2;
            break;

        case memfile::memory_file::file_format_lzma:
            compression = wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_LZMA;
            break;

        case memfile::memory_file::file_format_xz:
            compression = wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_XZ;
            break;

        default:
            // keep as is, not viewed as compressed
            break;

        }
        info.set_original_compression(compression);
        std::shared_ptr<wpkgar_file> file(new wpkgar_file(f_wpkgar_file.size(), info));
        if(filename == "debian-binary")
        {
            f_files[filename] = file;
            // this marks the package as a Debian package (i.e. if not present it's a bug in the package)
            // (actually this should be the very first file too!)
            if(data.size() != 4)
            {
                throw wpkgar_exception_invalid("invalid debian-binary file size, it must be 4 bytes");
            }
            char buf[4];
            if(data.read(buf, 0, 4) != 4)
            {
                throw wpkgar_exception_invalid("reading the debian-binary file 4 bytes failed");
            }
            char debian_binary[4] = { '2', '.', '0', '\n' };
            if(memcmp(buf, debian_binary, 4) != 0)
            {
                throw wpkgar_exception_invalid("the debian-binary file is not version 2.0");
            }
            has_debian_binary = true;
        }
        else if(filename.substr(0, 11) == "control.tar")
        {
            f_files["control.tar"] = file;
            // this is the control file, read its contents
            if(data.is_compressed())
            {
                memfile::memory_file d;
                data.copy(d);
                d.decompress(data);
            }
            // we save the file uncompressed (this is to support the -x option)
            info.set_filename("control.tar");
            f_wpkgar_file.append_file(info, data);
            read_control(data);
            has_control_tar_gz = true;
        }
        else if(filename.substr(0, 8) == "data.tar")
        { // ignore compression extension
            f_files["data.tar"] = file;
            // this is the data file, read its contents
            if(data.is_compressed())
            {
                memfile::memory_file d;
                data.copy(d);
                d.decompress(data);
            }
            // we save the file uncompressed in our db
            info.set_filename("data.tar");
            f_wpkgar_file.append_file(info, data);
            read_data(data);
            has_data_tar_gz = true;
        }
        else
        {
            f_files[filename] = file;
            // err on other files? at this point all the files I've seen
            // do not include anything else for now we save them in our
            // index, just in case
            f_wpkgar_file.append_file(info, data);
        }
    }

    if(!has_debian_binary)
    {
        throw wpkgar_exception_invalid("the debian-binary file was not found in this package");
    }
    if(!has_control_tar_gz)
    {
        throw wpkgar_exception_invalid("the control.tar.gz file was not found in this package");
    }
    if(!has_data_tar_gz)
    {
        throw wpkgar_exception_invalid("the data.tar.gz file was not found in this package");
    }

    // it worked, save the wpkgar file too
    f_wpkgar_file.write_file(f_package_path.append_child("index.wpkgar"), true);
}

void wpkgar_package::read_control(memfile::memory_file& p)
{
    p.dir_rewind();
    bool has_control(false);
    bool has_md5sums(false);
    for(;;)
    {
        memfile::memory_file::file_info info;
        memfile::memory_file data;
        if(!p.dir_next(info, &data))
        {
            if(!has_control)
            {
                throw wpkgar_exception_invalid("the control file was not found in this package");
            }
            if(!has_md5sums)
            {
                throw wpkgar_exception_invalid("the md5sums file was not found in this package");
            }
            break;
        }
        // here we're dealing with a tarball and often it includes "./"
        // at the beginning which we do not support in the wpkgar_file
        // plus we may have the directory "./"!
        std::string filename(info.get_filename());
        if(info.get_file_type() != memfile::memory_file::file_info::regular_file)
        {
            if(info.get_file_type() == memfile::memory_file::file_info::directory)
            {
                if(filename == "." || filename == "./")
                {
                    // top directory is often represented here, just ignore it
                    continue;
                }
            }
            throw wpkgar_exception_invalid("unexpected file in control.tar.gz (unsupported type)");
        }
        if(filename.length() > 2 && filename[0] == '.' && filename[1] == '/')
        {
            // remove ./ if present and not by itself
            filename.erase(0, 2);
        }
        if(filename.find_first_of('/') != std::string::npos)
        {
            // this may be legal in some cases, but at this point we do not support such
            throw wpkgar_exception_invalid("unexpected file in control.tar.gz (included in a sub-directory)");
        }
        if(f_files.find(filename) != f_files.end())
        {
            throw wpkgar_exception_invalid("the .deb control files include two files with the same name");
        }
        info.set_filename(filename);
        std::shared_ptr<wpkgar_file> file(new wpkgar_file(f_wpkgar_file.size(), info));
        f_files[filename] = file;
        // the append_file() has the side effect of saving the files
        // in the database (automatically!)
        f_wpkgar_file.append_file(info, data);
        if(filename == "control")
        {
            f_control_file.set_input_file(&data);
            f_control_file.read();
            f_control_file.set_input_file(NULL);
            has_control = true;
        }
        else if(filename == "md5sums")
        {
            // TODO: verify the contents of the md5sums file?
            //       verify so as we read the data file?
            has_md5sums = true;
        }
        // other files are optional
    }

    {
        // add our status file (this one is read/write)
        memfile::memory_file status;
        std::string status_field("X-Status: unknown\n");
        status.create(memfile::memory_file::file_format_other);
        status.write(status_field.c_str(), 0, static_cast<int>(status_field.length()));
        memfile::memory_file::file_info info;
        info.set_filename("wpkg-status");
        info.set_file_type(memfile::memory_file::file_info::regular_file);
        info.set_mode(0644);
        info.set_uid(getuid());
        info.set_gid(getgid());
        info.set_size(status.size());
        info.set_mtime(time(NULL));
        f_wpkgar_file.append_file(info, status);
        //status.write_file(f_package_path + "/wpkg-status", true);
        f_status_file.set_input_file(&status);
        f_status_file.read();
        f_status_file.set_input_file(NULL);
    }
}

void wpkgar_package::read_data(memfile::memory_file& p)
{
    p.dir_rewind();
    bool has_data(false);
    for(;;)
    {
        memfile::memory_file::file_info info;
        memfile::memory_file data;
        int dir_pos(p.dir_pos()); // save the start position!
        if(!p.dir_next(info, &data))
        {
            if(!has_data)
            {
                // is that true? pseudo packages probably don't even have a data.tar.gz file?
                throw wpkgar_exception_invalid("the data.tar.gz file cannot be empty");
            }
            break;
        }
        // should we consider directories as not being data? (although for them
        // to be there they will get created)
        has_data = true;

        // the data file is expected to have all its files including at
        // least one slash (/) character, we actually add / at the beginning
        // unless the name starts with "./" in which case we remove the
        // period (.) which has the same effect.
        // in effect the wpkgar file has all the data files starting with /
        // and the control and .deb files without any '/'
        std::string filename(info.get_filename());
        if(filename.length() >= 2 && filename[0] == '.' && filename[1] == '/')
        {
            filename.erase(0, 1);
        }
        else
        {
            filename = "/" + filename;
        }
        info.set_filename(filename);
        std::shared_ptr<wpkgar_file> file(new wpkgar_file(f_wpkgar_file.size(), info));
        if(f_files.find(filename) != f_files.end())
        {
            throw wpkgar_exception_invalid("the .deb data file includes two files with the same name (including path)");
        }
        // save offset for very fast retrieval
        file->set_data_dir_pos(dir_pos);
        f_files[filename] = file;
        f_wpkgar_file.append_file(info, data);
    }
}

bool wpkgar_package::has_control_file(const std::string& filename)
{
    file_t::const_iterator it(f_files.find(filename));
    return it != f_files.end();
}

void wpkgar_package::read_control_file(memfile::memory_file& p, std::string& filename, bool compress)
{
    file_t::const_iterator it(f_files.find(filename));
    if(it == f_files.end())
    {
        throw wpkgar_exception_parameter("this control file is not defined in this package");
    }
    wpkgar::wpkgar_block_t header;
    f_wpkgar_file.read(reinterpret_cast<char *>(&header), it->second->get_offset(), sizeof(header));
    p.read_file(f_package_path.append_child(filename));
    if(compress && header.f_original_compression != wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_NONE)
    {
        memfile::memory_file::file_format_t format(memfile::memory_file::file_format_undefined);
        switch(header.f_original_compression)
        {
        case wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_GZ:
            format = memfile::memory_file::file_format_gz;
            filename += ".gz";
            break;

        case wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_BZ2:
            format = memfile::memory_file::file_format_bz2;
            filename += ".bz2";
            break;

        case wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_LZMA:
            format = memfile::memory_file::file_format_lzma;
            filename += ".lzma";
            break;

        case wpkgar::wpkgar_block_t::WPKGAR_COMPRESSION_XZ:
            format = memfile::memory_file::file_format_xz;
            filename += ".xz";
            break;

        default:
            throw wpkgar_exception_compatibility("unknown compression to recompress the control.tar file");

        }
        memfile::memory_file d;
        p.copy(d);
        d.compress(p, format);
    }
}

bool wpkgar_package::validate_fields(const std::string& expression)
{
    return f_control_file.validate_fields(expression);
}

bool wpkgar_package::load_conffiles()
{
    // load the conffiles control file once
    if(!f_conffiles_defined)
    {
        file_t::const_iterator it(f_files.find("conffiles"));
        if(it == f_files.end())
        {
            // if there is no conffiles then filename cannot represents a configuration file
            return false;
        }
        wpkgar::wpkgar_block_t header;
        f_wpkgar_file.read(reinterpret_cast<char *>(&header), it->second->get_offset(), sizeof(header));
        memfile::memory_file c;
        c.read_file(f_package_path.append_child("conffiles"));
        int offset(0);
        std::string confname;
        while(c.read_line(offset, confname))
        {
            // note that configuration names start with "/"
            f_conffiles[confname] = 0;
        }
        f_conffiles_defined = true;
    }

    return true;
}

void wpkgar_package::conffiles(std::vector<std::string>& conf_files)
{
    conf_files.clear();
    if(load_conffiles())
    {
        for( auto conffile : f_conffiles )
        {
            conf_files.push_back(conffile.first);
        }
    }
}

bool wpkgar_package::is_conffile(const std::string& filename)
{
    if(filename.empty())
    {
        return false;
    }
    if(!load_conffiles())
    {
        return false;
    }
    // it is a configuration file if it is included in the f_conffiles map
    return f_conffiles.find(filename[0] != '/' ? "/" + filename : filename) != f_conffiles.end();
}

const wpkg_control::control_file& wpkgar_package::get_control_file_info() const
{
    return f_control_file;
}

wpkg_control::control_file& wpkgar_package::get_status_file_info()
{
    return f_status_file;
}

}
// namespace wpkgar

// vim: ts=4 sw=4 et
