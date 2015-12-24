/*    disk.cpp -- disk utilities for non-bsd systems and non-sunos
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
 *	   Doug Barbieri  doug@m2osw.com
 */

#include "details/disk.h"

#if !defined(MO_DARWIN) && !defined(MO_SUNOS) && !defined(MO_FREEBSD)
namespace wpkgar
{

namespace details
{


disk_t::disk_t(const wpkg_filename::uri_filename& path)
    : f_path(path)
    //, f_size(0) -- auto-init
    //, f_block(0) -- auto-init
    //, f_free_space(0) -- auto-init
    //, f_readonly(false) -- auto-init
{
}

const wpkg_filename::uri_filename& disk_t::get_path() const
{
    return f_path;
}

bool disk_t::match(const wpkg_filename::uri_filename& path)
{
    return f_path.full_path() == path.full_path().substr(0, f_path.full_path().length());
}

void disk_t::add_size(int64_t size)
{
    if(f_readonly)
    {
        throw wpkgar_exception_io("package cannot be installed on " + f_path.original_filename() + " since it is currently mounted as read-only");
    }
    // use the ceiling of (size / block size)
    // Note: size may be negative when the file is being removed or upgraded
    f_size += (size + f_block_size - 1) / f_block_size;

    // note: we do not add anything for the directory entry which is most
    // certainly wrong although I think that the size very much depends
    // on the file system and for very small files it may even use the
    // directory entry to save the file data (instead of a file chain
    // as usual)
}

//int64_t disk_t::size() const
//{
//    return f_size * f_block_size;
//}

void disk_t::set_block_size(uint64_t block_size)
{
    f_block_size = block_size;
}

void disk_t::set_free_space(uint64_t space)
{
    f_free_space = space;
}

//uint64_t disk_t::free_space() const
//{
//    return f_free_space;
//}

void disk_t::set_readonly()
{
    // size should still be zero when we call this function, but if not
    // we still want an error
    if(0 != f_size)
    {
        throw wpkgar_exception_io("package cannot be installed on " + f_path.original_filename() + " since it is currently mounted as read-only");
    }
    f_readonly = true;
}

bool disk_t::is_valid() const
{
    // if we're saving space then it's always valid
    if(f_size <= 0)
    {
        return true;
    }
    // leave a 10% margin for all the errors in our computation
    return static_cast<uint64_t>(f_size * f_block_size) < f_free_space * 9 / 10;
}



disk_list_t::disk_list_t( wpkgar_manager::pointer_t manager, std::shared_ptr<wpkgar_install> install )
    : f_manager(manager),
      f_install(install),
      //f_disks() -- auto-init
      f_default_disk(NULL)
      //f_filenames() -- auto-init
{
    wpkg_output::log("Enumerating available volumes and drives on the current system.")
        .level(wpkg_output::level_info)
        .debug(wpkg_output::debug_flags::debug_progress)
        .module(wpkg_output::module_validate_installation);

#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
    // limit ourselves to regular drives as local drives are all
    // defined in this way
    DWORD drives(GetLogicalDrives());
    for(int d(0); d < 26; ++d)
    {
        if((drives & (1 << d)) != 0)
        {
            // drive is defined!
            std::string letter;
            letter = static_cast<char>('a' + d);
            letter += ":/";
            if(GetDriveTypeA(letter.c_str()) == DRIVE_FIXED)
            {
                // only accept hard drives
                DWORD sectors_per_cluster;
                DWORD bytes_per_sector;
                DWORD number_of_free_clusters;
                DWORD total_number_of_clusters;
                if(GetDiskFreeSpace(letter.c_str(), &sectors_per_cluster, &bytes_per_sector, &number_of_free_clusters, &total_number_of_clusters))
                {
                    // we could gather the total size, keep this entry
                    disk_t disk(letter);
                    disk.set_free_space(bytes_per_sector * sectors_per_cluster * number_of_free_clusters);
                    disk.set_block_size(bytes_per_sector);

                    // check whether that partition is read-only
                    DWORD volume_serial_number;
                    DWORD maximum_component_length;
                    DWORD file_system_flags;
                    if(GetVolumeInformation(letter.c_str(), NULL, 0, &volume_serial_number, &maximum_component_length, &file_system_flags, NULL, 0))
                    {
                        if((file_system_flags & FILE_READ_ONLY_VOLUME) != 0)
                        {
                            disk.set_readonly();
                        }
                    }

                    // save that disk in our vector
                    f_disks.push_back(disk);
                }
            }
        }
    }

    // include the / folder using the correct information
    wchar_t cwd[4096];
    if(GetCurrentDirectoryW(sizeof(cwd) / sizeof(cwd[0]), cwd) == 0)
    {
        throw std::runtime_error("failed reading current working directory (more than 4096 character long?)");
    }
    if(wcslen(cwd) < 3)
    {
        throw std::runtime_error("the name of the current working directory is too short");
    }
    if(cwd[1] != L':' || cwd[2] != L'\\'
    || ((cwd[0] < L'a' || cwd[0] > L'z') && (cwd[0] < L'A' || cwd[0] > L'Z')))
    {
        // TODO: add support for \\foo\blah (network drives)
        throw std::runtime_error("the name of the current working directory does not start with a drive name (are you on a network drive? this is not currently supported.)");
    }
    cwd[3] = L'\0';     // end the string
    cwd[2] = L'/';      // change \\ to /
    cwd[0] |= 0x20;     // lowercase (this works in UCS-2 as well since this is an ASCII letter)
    disk_t *def(find_disk(libutf8::wcstombs(cwd)));
    if(def == NULL)
    {
        throw std::runtime_error("the name of the drive found in the current working directory \"" + libutf8::wcstombs(cwd) + "\" is not defined in the list of existing directories");
    }
    f_default_disk = def;

#elif defined(MO_LINUX)
    // RAII on f_mounts
    class mounts
    {
    public:
        mounts()
            : f_mounts(setmntent("/etc/mtab", "r")),
              f_entry(NULL)
        {
            if(f_mounts == NULL)
            {
                throw std::runtime_error("packager could not open /etc/mtab for reading");
            }
        }

        ~mounts()
        {
            endmntent(f_mounts);
        }

        struct mntent *next()
        {
            f_entry = getmntent(f_mounts);
            // TODO: if NULL we reached the end or had an error
            return f_entry;
        }

        bool has_option(const char *opt)
        {
            if(f_entry == NULL)
            {
                std::logic_error("has_option() cannot be called before next()");
            }
            return hasmntopt(f_entry, opt) != NULL;
        }

    private:
        FILE *              f_mounts;
        struct mntent *     f_entry;
    };

    mounts m;
    for(;;)
    {
        struct mntent *e(m.next());
        if(e == NULL)
        {
            // EOF
            break;
        }
        // ignore unusable disks and skip network disks too
        if(strcmp(e->mnt_type, MNTTYPE_IGNORE) != 0
        && strcmp(e->mnt_type, MNTTYPE_NFS) != 0
        && strcmp(e->mnt_type, MNTTYPE_SWAP) != 0)
        {
            struct statvfs s;
            if(statvfs(e->mnt_dir, &s) == 0)
            {
                if(s.f_bfree != 0)
                {
                    disk_t disk(e->mnt_dir);
                    // Note: f_bfree is larger than f_bavail and most certainly
                    //       includes kernel reserved blocks that even root
                    //       cannot access while installing packages
                    disk.set_free_space(s.f_bavail * s.f_bsize);
                    disk.set_block_size(s.f_bsize);
                    if(m.has_option("ro"))
                    {
                        disk.set_readonly();
                    }
//printf("Got disk [%s] %ld, %ld, %s\n", e->mnt_dir, s.f_bavail, s.f_frsize, m.has_option("ro") ? "ro" : "r/w");
                    f_disks.push_back(disk);
                }
            }
        }
    }
#else
#   error This platform is not yet supported!
#endif
}

disk_t *disk_list_t::find_disk(const std::string& path)
{
    // we want to keep the longest as it represents the real mount point
    // (i.e. we must select /usr and not /)
#if defined(MO_WINDOWS) || defined(MO_CYGWIN)
    std::string drive(path.substr(0, 3));
    if(drive.length() < 3
    || drive[1] != ':' || drive[2] != '/'
    || ((drive[0] < 'a' || drive[0] > 'z') && (drive[0] < 'A' || drive[0] > 'Z')))
    {
        // no drive is specified in that path, use the default drive instead!
        // TODO: support //network/folder syntax for network drives
        return f_default_disk;
    }
#endif
    std::string::size_type max(0);
    std::vector<disk_t>::size_type found(static_cast<std::vector<disk_t>::size_type>(-1));
    for(std::vector<disk_t>::size_type i(0); i < f_disks.size(); ++i)
    {
        std::string::size_type l(f_disks[i].get_path().full_path().length());
        if(l > max && f_disks[i].match(path))
        {
            max = l;
            found = i;
        }
    }
    if(found != static_cast<std::vector<disk_t>::size_type>(-1))
    {
        return &f_disks[found];
    }
    return NULL;
}

void disk_list_t::add_size(const std::string& path, int64_t size)
{
    disk_t *d(find_disk(path));
    if(d != NULL)
    {
        d->add_size(size);
    }
    else
    {
        wpkg_output::log("cannot find partition for %1.")
                .quoted_arg(path)
            .level(wpkg_output::level_error)
            .module(wpkg_output::module_validate_installation)
            .action("install-validation");
    }
}

void disk_list_t::compute_size_and_verify_overwrite(const wpkgar_install::wpkgar_package_list_t::size_type idx,
                                                    const installer::package_item_t& item,
                                                    const wpkg_filename::uri_filename& root,
                                                    memfile::memory_file *data,
                                                    memfile::memory_file *upgrade,
                                                    const int factor)
{
    const wpkg_filename::uri_filename package_name(item.get_filename());
    memfile::memory_file::file_info info;

    // if we have an upgrade package then we want to get all the filenames
    // first to avoid searching that upgrade package for every file we find
    // in the new package being installed; we use that data file only to
    // determine whether an overwrite is normal or not
    std::map<std::string, memfile::memory_file::file_info> upgrade_files;
    if(upgrade != NULL)
    {
        upgrade->dir_rewind();
        while(upgrade->dir_next(info))
        {
            upgrade_files[info.get_filename()] = info;
        }
    }

    // placed here because VC++ "needs" an initialization (which is
    // wrong, but instead of hiding the warning...)
    wpkg_filename::uri_filename::file_stat s;

    data->dir_rewind();
    while(data->dir_next(info))
    {
        std::string path(info.get_filename());
        if(path[0] != '/')
        {
            // files that do not start with a slash are part of the database
            // only so we ignore them here
            continue;
        }
        if(factor == 1)
        {
            filename_info_map_t::const_iterator it(f_filenames.find(path));
            if(it != f_filenames.end())
            {
                // this is not an upgrade (downgrade) so the filename must be
                // unique otherwise two packages being installed are in conflict
                // note that in this case we do not check for the
                // --force-overwrite flags... (should we allow such here?)
                if(info.get_file_type() != memfile::memory_file::file_info::directory
                || it->second.get_file_type() != memfile::memory_file::file_info::directory)
                {
                    wpkg_output::log("file %1 from package %2 also exists in %3.")
                            .quoted_arg(info.get_filename())
                            .quoted_arg(package_name)
                            .quoted_arg(it->second.get_package_name())
                        .level(wpkg_output::level_error)
                        .module(wpkg_output::module_validate_installation)
                        .package(package_name)
                        .action("install-validation");
                }
            }
            else
            {
                info.set_package_name(package_name.original_filename());
                f_filenames[path] = info;
            }
        }
        int64_t size(info.get_size());
        switch(info.get_file_type())
        {
        case memfile::memory_file::file_info::regular_file:
        case memfile::memory_file::file_info::continuous:
            // if regular, then the default size works
            break;

        case memfile::memory_file::file_info::directory:
            // at this time we may not remove directories so
            // we keep this size if upgrading
            if(factor < 0)
            {
                size = 0;
                break;
            }
        default:
            // otherwise use at least 1 block
            if(size < 512)
            {
                // note that this adds 1 block whether it is 512,
                // 1024, 2048, 4096, etc. (we assume blocks are
                // never less than 512 though.)
                size = 512;
            }
            break;

        }
        // note that we want to call add_size() even if the
        // size is zero because the add_size() function
        // verifies that path is writable
        add_size(path, size * factor);

        // check whether the file already exists, and if so whether
        // we're upgrading because if so, we're fine--note that we
        // allow an overwrite only of a file from the same package
        // (same Package field name); later we may support a Replace
        // in which case the names could differ
        //
        // IMPORTANT NOTE: if the file is a configuration file, then
        // it shouldn't exist if we are installing that package for
        // the first time and if that's an upgrade then we need
        // the file to be present in the package being upgraded
        //
        // note that any number of packages can have the same
        // directory defined and that is silently "overwritten";
        // however, a directory cannot be overwritten by a regular
        // file and vice versa unless you have the --force-overwrite-dir
        // and that's certainly a very bad idea
        //
        // We run that test only against explicit and implicit packages
        // since installed (upgrade) packages are... installed and thus
        // their files exist on the target!
        if(factor > 0 && root.append_child(path).os_stat(s) == 0)
        {
            // it already exists, so we're overwriting it...
            bool a(info.get_file_type() != memfile::memory_file::file_info::directory);
            bool b(!s.is_dir());
            if(a && b)
            {  // both are regular files
                // are we upgrading?
                if(upgrade_files.find(info.get_filename()) == upgrade_files.end())
                {
                    // first check whether this is a file in an Essential package
                    // because if so we ALWAYS prevent the overwrite
                    if(f_install->find_essential_file(info.get_filename(), idx))
                    {
                        // use a fatal error because that's pretty much what it is
                        // (i.e. there isn't a way to prevent the error from occurring)
                        wpkg_output::log("file %1 from package %2 already exists on your target system and it cannot be overwritten because the owner is an essential package.")
                                .quoted_arg(info.get_filename())
                                .quoted_arg(package_name)
                            .level(wpkg_output::level_fatal)
                            .module(wpkg_output::module_validate_installation)
                            .package(package_name)
                            .action("install-validation");
                    }
                    else
                    {
                        // last chance, is that a configuration file?
                        // if so we deal with those later...
                        if(!item.is_conffile(path))
                        {
                            // bad bad bad!
                            if(f_install->get_parameter(wpkgar_install::wpkgar_install_force_overwrite, false))
                            {
                                wpkg_output::log("file %1 from package %2 already exists on your target system and it will get overwritten.")
                                        .quoted_arg(info.get_filename())
                                        .quoted_arg(package_name)
                                    .level(wpkg_output::level_warning)
                                    .module(wpkg_output::module_validate_installation)
                                    .package(package_name)
                                    .action("install-validation");
                            }
                            else
                            {
                                wpkg_output::log("file %1 from package %2 already exists on your target system.")
                                        .quoted_arg(info.get_filename())
                                        .quoted_arg(package_name)
                                    .level(wpkg_output::level_error)
                                    .module(wpkg_output::module_validate_installation)
                                    .package(package_name)
                                    .action("install-validation");
                            }
                        }
                    }
                }
            }
            else if(a ^ b)
            {  // one is a directory and the other is not
                // are we upgrading?
                if(upgrade_files.find(info.get_filename()) == upgrade_files.end())
                {
                    if(f_install->get_parameter(wpkgar_install::wpkgar_install_force_overwrite_dir, false))
                    {
                        // super bad!
                        if(a)
                        {
                            // TODO:
                            // forbid this no matter what when the directory to
                            // be overwritten is defined in an essential package
                            wpkg_output::log("file %1 from package %2 will replace directory of the same name (IMPORTANT NOTE: the contents of that directory will be lost!)")
                                    .quoted_arg(info.get_filename())
                                    .quoted_arg(package_name)
                                .level(wpkg_output::level_warning)
                                .module(wpkg_output::module_validate_installation)
                                .package(package_name)
                                .action("install-validation");
                        }
                        else
                        {
                            wpkg_output::log("directory %1 from package %2 will replace the regular file of the same name.")
                                    .quoted_arg(info.get_filename())
                                    .quoted_arg(package_name)
                                .level(wpkg_output::level_warning)
                                .module(wpkg_output::module_validate_installation)
                                .package(package_name)
                                .action("install-validation");
                        }
                    }
                    else
                    {
                        if(b)
                        {
                            wpkg_output::log("file %1 already exists on your target system and package %2 would like to create a directory in its place.")
                                    .quoted_arg(path)
                                    .quoted_arg(package_name)
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_validate_installation)
                                .package(package_name)
                                .action("install-validation");
                        }
                        else
                        {
                            wpkg_output::log("directory %1 already exists on your target system and package %2 would like to create a regular file in its place.")
                                    .quoted_arg(path)
                                    .quoted_arg(package_name)
                                .level(wpkg_output::level_error)
                                .module(wpkg_output::module_validate_installation)
                                .package(package_name)
                                .action("install-validation");
                        }
                    }
                }
                else
                {
                    // in this case we emit a warning because a package
                    // should not transform a file into a directory or
                    // vice versa (bad practice!) but we still allow it
                    if(a)
                    {
                        wpkg_output::log("package %1 is requesting directory %2 to be replaced by a regular file.")
                                .quoted_arg(package_name)
                                .quoted_arg(info.get_filename())
                            .level(wpkg_output::level_warning)
                            .module(wpkg_output::module_validate_installation)
                            .package(package_name)
                            .action("install-validation");
                    }
                    else
                    {
                        wpkg_output::log("package %1 is requesting file %2 to be replaced by a directory.")
                                .quoted_arg(package_name)
                                .quoted_arg(info.get_filename())
                            .level(wpkg_output::level_warning)
                            .module(wpkg_output::module_validate_installation)
                            .package(package_name)
                            .action("install-validation");
                    }
                }
            }
            // else -- both are directories so we can ignore the "overwrite"
        }
    }
}

bool disk_list_t::are_valid() const
{
    for(std::vector<disk_t>::size_type i(0); i < f_disks.size(); ++i)
    {
        if(!f_disks[i].is_valid())
        {
            return false;
        }
    }
    return true;
}


}   // details namespace

}   // namespace wpkgar

#endif

// vim: ts=4 sw=4 expandtab syntax=cpp.doxygen fdm=marker
