/*    wpkg_file_item.cpp -- implementation of the control file item
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
 * \brief Implementation of file related fields of control files.
 *
 * The control format supports different \em file fields each supporting
 * a different format by default. This implementation allows for the
 * format to be specified or deduced by the number of parameters on
 * each line of data.
 *
 * These formats are used by the Files, ConfFiles, Sha256, and some other
 * similar fields.
 */
#include    "libdebpackages/wpkg_control.h"

#include    <sstream>
#include    <ctime>
#include    <string.h>


namespace wpkg_control
{




file_list_t control_file::get_files(const std::string& name) const
{
    file_list_t list(name);
    std::string files(get_field(name));
    list.set(files);
    return list;
}

file_list_t::file_list_t(const std::string& name)
    : f_name(name)
{
}


void file_list_t::set(const std::string& files)
{
    struct words
    {
        const char *read_line(const char *s)
        {
            f_words.clear();
            ++f_line;
            for(; isspace(*s); ++s);
            const char *start(s);
            while(*s != '\0' && *s != '\r' && *s != '\n')
            {
                if(*s == '"')
                {
                    // in this case all the characters up to the next " are included in the word
                    if(s != start)
                    {
                        std::stringstream ss;
                        ss << "word cannot include a quote (\") character on line #" << f_line;
                        throw wpkg_control_exception_invalid(ss.str());
                    }
                    for(++s, start = s; *s != '"'; ++s)
                    {
                        if(*s == '\0' || *s == '\r' || *s == '\n')
                        {
                            std::stringstream ss;
                            ss << "word starting with a quote (\") must end with a quote, end quote is missing on line #" << f_line;
                            throw wpkg_control_exception_invalid(ss.str());
                        }
                    }
                    std::string w(start, s - start);
                    f_words.push_back(w);
                    for(++s; isspace(*s); ++s);
                    start = s;
                }
                else if(isspace(*s))
                {
                    std::string w(start, s - start);
                    f_words.push_back(w);
                    for(++s; isspace(*s); ++s);
                    start = s;
                }
                else
                {
                    ++s;
                }
            }
            if(start != s)
            {
                std::string w(start, s - start);
                f_words.push_back(w);
            }
            for(; *s == '\r' || *s == '\n'; ++s);
            return s;
        }

        controlled_vars::zint32_t   f_line;
        std::vector<std::string>    f_words;
    };

    //file_list_t d;
    //std::string files(get_field(name));
    const char *s(files.c_str());

    // first get the name of the format if any
    const char *start(s);
    file_item::format_t format(file_item::format_unknown);
    for(; *s != '\0' && *s != '\n' && *s != '\r'; ++s);
    if(s == start)
    {
        case_insensitive::case_insensitive_string nm(f_name);

        if(nm == control_file::field_checksumssha1_factory_t::canonicalized_name())
        {
            format = file_item::format_sha1;
        }
        else if(nm == control_file::field_checksumssha256_factory_t::canonicalized_name())
        {
            format = file_item::format_sha256;
        }
        else if(nm == control_file::field_conffiles_factory_t::canonicalized_name())
        {
            format = file_item::format_conffiles;
        }
        else if(nm == control_file::field_files_factory_t::canonicalized_name())
        {
            format = file_item::format_md5sum;
        }
        else
        {
            format = file_item::format_not_specified;
        }
    }
    else
    {
        case_insensitive::case_insensitive_string fmt(start, s - start);
        if(fmt == "conffiles")
        {
            format = file_item::format_conffiles;
        }
        else if(fmt == "list")
        {
            format = file_item::format_list;
        }
        else if(fmt == "modelist")
        {
            format = file_item::format_modelist;
        }
        else if(fmt == "longlist")
        {
            format = file_item::format_longlist;
        }
        else if(fmt == "md5sum" || fmt == "sources")
        {
            format = file_item::format_md5sum;
        }
        else if(fmt == "sha1")
        {
            format = file_item::format_sha1;
        }
        else if(fmt == "sha256")
        {
            format = file_item::format_sha256;
        }
        else
        {
            // we cannot read the data if a format is specified
            // and we do not know how to parse it
            throw wpkg_control_exception_invalid("unknown format \"" + fmt + "\"");
        }
    }
    for(; *s == '\r' || *s == '\n'; ++s);

    words w;
    switch(format)
    {
    case file_item::format_not_specified:
        while(*s != '\0')
        {
            s = w.read_line(s);
            file_item item;
            switch(w.f_words.size())
            {
            case 1:
                item.set_format(file_item::format_list);
                item.set_filename(w.f_words[0]);
                break;

            case 2:
                item.set_format(file_item::format_conffiles);
                item.set_filename(w.f_words[0]);
                item.set_checksum(w.f_words[1]);
                break;

            case 3:
                // TODO: test the size of f_words[0] to determine the format
                item.set_format(file_item::format_md5sum); // md5 by default, but could be sha1 or sha256
                item.set_checksum(w.f_words[0]);
                item.set_size(w.f_words[1]);
                item.set_filename(w.f_words[2]);
                break;

            case 4:
                item.set_format(file_item::format_longlist);
                item.set_mode(w.f_words[0]);
                item.set_size(w.f_words[1]);
                item.set_checksum(w.f_words[2]);
                item.set_filename(w.f_words[3]);
                break;

            case 6:
                item.set_format(file_item::format_metadata);
                item.set_mode(w.f_words[0]);
                item.set_user_uid(w.f_words[1]);
                item.set_group_gid(w.f_words[2]);
                if(w.f_words[3].find_first_of(',') != std::string::npos)
                {
                    item.set_dev(w.f_words[3]);
                }
                else
                {
                    item.set_size(w.f_words[3]);
                }
                item.set_mtime(w.f_words[4]);
                item.set_filename(w.f_words[5]);
                break;

            default:
                throw wpkg_control_exception_invalid("invalid entry for a Files field, we expect one to four words");

            }
            push_back(item);
        }
        break;

    case file_item::format_list:
        while(*s != '\0')
        {
            start = s;
            for(; *s != '\0' && *s != '\r' && *s != '\n'; ++s);
            if(start != s)
            {
                file_item item;
                item.set_format(format);
                if(*start == '"' && s - start > 2 && s[-1] == '"')
                {
                    std::string filename(start + 1, s - start - 2);
                    item.set_filename(filename);
                }
                else
                {
                    std::string filename(start, s - start);
                    item.set_filename(filename);
                }
                push_back(item);
            }
            for(; *s == '\r' || *s == '\n'; ++s);
        }
        break;

    case file_item::format_modelist:
        while(*s != '\0')
        {
            s = w.read_line(s);
            if(w.f_words.size() != 2)
            {
                throw wpkg_control_exception_invalid("invalid entry for a modelist formatted list of files, we expect exactly two words");
            }
            file_item item;
            item.set_format(format);
            item.set_mode(w.f_words[0]);
            item.set_filename(w.f_words[1]);
            push_back(item);
        }
        break;

    case file_item::format_conffiles:
        while(*s != '\0')
        {
            s = w.read_line(s);
            if(w.f_words.size() != 2)
            {
                throw wpkg_control_exception_invalid("invalid entry for a conffiles formatted list of files, we expect exactly two words");
            }
            file_item item;
            item.set_format(format);
            item.set_filename(w.f_words[0]);
            item.set_checksum(w.f_words[1]);
            push_back(item);
        }
        break;

    case file_item::format_md5sum:
    case file_item::format_sha1:
    case file_item::format_sha256:
        while(*s != '\0')
        {
            s = w.read_line(s);
            if(w.f_words.size() != 3)
            {
                throw wpkg_control_exception_invalid("invalid entry for a sources / md5sum formatted list of files, we expect exactly three words");
            }
            file_item item;
            item.set_format(format);
            item.set_checksum(w.f_words[0]);
            item.set_size(w.f_words[1]);
            item.set_filename(w.f_words[2]);
            push_back(item);
        }
        break;

    case file_item::format_longlist:
        while(*s != '\0')
        {
            s = w.read_line(s);
            if(w.f_words.size() != 4)
            {
                throw wpkg_control_exception_invalid("invalid entry for a long list of files, we expect exactly four words");
            }
            file_item item;
            item.set_format(format);
            item.set_mode(w.f_words[0]);
            item.set_size(w.f_words[1]);
            item.set_checksum(w.f_words[2]);
            item.set_filename(w.f_words[3]);
            push_back(item);
        }
        break;

    case file_item::format_metadata:
        while(*s != '\0')
        {
            s = w.read_line(s);
            if(w.f_words.size() != 6)
            {
                throw wpkg_control_exception_invalid("invalid entry for metadata of files, we expect exactly six words");
            }
            file_item item;
            item.set_format(format);
            item.set_mode(w.f_words[0]);
            item.set_user_uid(w.f_words[1]);
            item.set_group_gid(w.f_words[2]);
            if(w.f_words[3].find_first_of(',') != std::string::npos)
            {
                item.set_dev(w.f_words[3]);
            }
            else
            {
                item.set_size(w.f_words[3]);
            }
            item.set_mtime(w.f_words[4]);
            item.set_filename(w.f_words[5]);
            push_back(item);
        }
        break;

    case file_item::format_unknown:
    case file_item::format_choose_best:
        throw wpkg_control_exception_invalid("invalid format for a Files field");

    }
}








void file_item::set_format(format_t format)
{
    f_format = static_cast<int>(format);
}

void file_item::set_filename(const std::string& filename)
{
    f_filename = filename;
}

void file_item::set_mode(const std::string& line)
{
    f_mode = 0;
    if(line == "-")
    {
        // empty mode
        return;
    }
    int i(0);
    const char *l(line.c_str());
    for(; !isspace(*l); ++i, ++l)
    {
        if(*l == '\0')
        {
            throw wpkg_control_exception_invalid("file mode and permission field too short: \"" + line + "\"; it must be exactly 10 characters");
        }
        if(i > 10)
        {
            throw wpkg_control_exception_invalid("file mode and permission field has to be exactly 10 characters; \"" + line + "\"");
        }
        switch(i)
        {
        case 0:
            // type
            switch(*l)
            {
            case '-': // regular file
            case 'C': // continuous
                f_mode = (f_mode & ~S_IFMT) | S_IFREG;
                break;

            case 'd': // directory
                f_mode = (f_mode & ~S_IFMT) | S_IFDIR;
                break;

            case 'c': // character special
                f_mode = (f_mode & ~S_IFMT) | S_IFCHR;
                break;

#ifndef MO_WINDOWS
            case 'l': // symbolic link
                f_mode = (f_mode & ~S_IFMT) | S_IFLNK;
                break;

            case 'b': // block special
                f_mode = (f_mode & ~S_IFMT) | S_IFBLK;
                break;

            case 'p': // fifo
                f_mode = (f_mode & ~S_IFMT) | S_IFIFO;
                break;
#endif

            default:
                throw wpkg_control_exception_invalid("unknown file type in this mode: \"" + line + "\"");

            }
            continue;

        case 1: // owner read
        case 4: // group read
        case 7: // other read
            if(*l != '-' && *l != 'r')
            {
                throw wpkg_control_exception_invalid("a read flag in your mode must either be 'r' or '-', \"" + line + "\"");
            }
            break;

        case 2: // owner write
        case 5: // group write
        case 8: // other write
            if(*l != '-' && *l != 'w')
            {
                throw wpkg_control_exception_invalid("a write flag in your mode must either be 'w' or '-', \"" + line + "\"");
            }
            break;

        case 3: // owner execute
            if(*l == 's')
            {
                f_mode |= 04100;
                continue;
            }
            if(*l == 'S')
            {
                f_mode |= 04000;
                continue;
            }
        case 6: // group execute
            if(*l == 's')
            {
                f_mode |= 02010;
                continue;
            }
            if(*l == 'S')
            {
                f_mode |= 02000;
                continue;
            }
        case 9: // other execute
            if(i == 9)
            {
                if(*l == 't')
                {
                    f_mode |= 01001;
                    continue;
                }
                if(*l == 'T')
                {
                    f_mode |= 01000;
                    continue;
                }
            }
            if(*l != '-' && *l != 'x')
            {
                throw wpkg_control_exception_invalid("an execute flag in your mode must either be 'x' or '-', \"" + line + "\"");
            }
            break;

        }
        if(*l != '-')
        {
            f_mode |= 1 << (9 - i);
        }
    }
    if(i != 10)
    {
        throw wpkg_control_exception_invalid("file type and permission field has to be exactly 10 characters: \"" + line + "\"");
    }
}

void file_item::set_mode(mode_t mode)
{
    f_mode = mode;
}

void file_item::set_user(const std::string& user)
{
    f_user = user == "-" ? "" : user;
}

void file_item::set_uid(int uid)
{
    f_uid = uid;
}

void file_item::set_uid(const std::string& uid)
{
    if(uid == "-")
    {
        f_uid = undefined_uid;
        return;
    }
    int u(0);
    for(const char *s(uid.c_str()); *s != '\0'; ++s)
    {
        if(*s < '0' || *s > '9')
        {
            throw wpkg_control_exception_invalid("the user uid can only be composed of digits");
        }
        int o(u);
        u = u * 10 + *s - '0';
        if(u < o)
        {
            throw wpkg_control_exception_invalid("the user uid is too large");
        }
    }
    f_uid = u;
}

void file_item::set_user_uid(const std::string& user_uid)
{
    std::string::size_type p(user_uid.find_first_of('/'));
    set_user(user_uid.substr(0, p));
    set_uid(user_uid.substr(p + 1));
}

void file_item::set_group(const std::string& group)
{
    f_group = group == "-" ? "" : group;
}

void file_item::set_gid(int gid)
{
    f_gid = gid;
}

void file_item::set_gid(const std::string& gid)
{
    if(gid == "-")
    {
        f_gid = undefined_gid;
        return;
    }
    int g(0);
    for(const char *s(gid.c_str()); *s != '\0'; ++s)
    {
        if(*s < '0' || *s > '9')
        {
            throw wpkg_control_exception_invalid("the group gid can only be composed of digits");
        }
        int o(g);
        g = g * 10 + *s - '0';
        if(g < o)
        {
            throw wpkg_control_exception_invalid("the group gid is too large");
        }
    }
    f_gid = g;
}

void file_item::set_group_gid(const std::string& group_gid)
{
    std::string::size_type p(group_gid.find_first_of('/'));
    set_group(group_gid.substr(0, p));
    set_gid(group_gid.substr(p + 1));
}

void file_item::set_mtime(time_t time)
{
    f_mtime = time;
}

void file_item::set_mtime(const std::string& date)
{
    if(date == "-")
    {
        f_mtime = 0;
        return;
    }

    // at this point we only support YYYYMMDDTHHMMSS as the date format
    // (i.e. 20121231T23:59:59 for the last second of 2012.)
    struct tm t;
    memset(&t, 0, sizeof(t));
    if(date.length() == 15)
    {
        if(date[8] != 'T')
        {
            throw wpkg_control_exception_invalid("the date and time format must have a 'T' at position 8 (YYYYmmDDTHHMMSS)");
        }

        t.tm_hour = date[9] * 10 + date[10] - '0' * 11;
        if(t.tm_hour < 0 || t.tm_hour > 23)
        {
            throw wpkg_control_exception_invalid("the hours must be between 0 and 23 inclusive");
        }

        t.tm_min  = date[11] * 10 + date[12] - '0' * 11;
        if(t.tm_min < 0 || t.tm_min > 59)
        {
            throw wpkg_control_exception_invalid("the minutes must be between 0 and 59 inclusive");
        }

        t.tm_sec  = date[13] * 10 + date[14] - '0' * 11;
        if(t.tm_sec < 0 || t.tm_sec > 59)
        {
            throw wpkg_control_exception_invalid("the seconds must be between 0 and 59 inclusive");
        }
    }
    else if(date.length() != 8)
    {
        throw wpkg_control_exception_invalid("the date size is not compatible with the expected formats (YYYYmmDD or YYYYmmDDTHHMMSS)");
    }
    const std::string d(date.substr(0, 8) + date.substr(9));
    if(d.find_first_not_of("0123456789") != std::string::npos)
    {
        throw wpkg_control_exception_invalid("the date and time must only be composed of digits");
    }

    t.tm_year = date[0] * 1000 + date[1] * 100 + date[2] * 10 + date[3] - '0' * 1111;
    if(t.tm_year < 1970 || t.tm_year > 2067)
    {
        throw wpkg_control_exception_invalid("the year must be between 1970 and 2067 inclusive");
    }
    t.tm_year -= 1900;

    t.tm_mon  = date[4] * 10 + date[5] - '0' * 11 - 1;
    if(t.tm_mon < 0 || t.tm_mon > 11)
    {
        throw wpkg_control_exception_invalid("the month must be between 1 and 12 inclusive");
    }

    t.tm_mday  = date[6] * 10 + date[7] - '0' * 11;
    if(t.tm_mday < 1 || t.tm_mday > 31)
    {
        throw wpkg_control_exception_invalid("the day of the month must be between 1 and 31 inclusive");
    }

    f_mtime = mktime(&t);
    if(-1LL == f_mtime)
    {
        // undefined time is represented with zero
        f_mtime = 0;
    }
}

void file_item::set_dev(int dev_major, int dev_minor)
{
    f_dev_major = dev_major;
    f_dev_minor = dev_minor;
}

void file_item::set_dev(const std::string& dev)
{
    std::string::size_type p(dev.find_first_of(','));
    std::string dev_major(dev.substr(0, p));
    std::string dev_minor(dev.substr(p + 1));

    if(dev_major.length() == 0 || dev_minor.length() == 0)
    {
        throw wpkg_control_exception_invalid("invalid device specification; major and minor cannot be empty; if undefined use '-' instead of empty");
    }

    int mj(0);
    if(dev_major == "-")
    {
        mj = undefined_device;
    }
    else
    {
        for(const char *s(dev_major.c_str()); *s != '\0'; ++s)
        {
            if(*s < '0' || *s > '9')
            {
                throw wpkg_control_exception_invalid("major device number must be composed of digits only (0-9)");
            }
            int old(mj);
            mj = mj * 10 + *s - '0';
            if(mj < old)
            {
                throw wpkg_control_exception_invalid("major device number too large");
            }
        }
    }

    int mn(0);
    if(dev_minor == "-")
    {
        mn = undefined_device;
    }
    else
    {
        for(const char *s(dev_minor.c_str()); *s != '\0'; ++s)
        {
            if(*s < '0' || *s > '9')
            {
                throw wpkg_control_exception_invalid("minor device number must be composed of digits only (0-9)");
            }
            int old(mn);
            mn = mn * 10 + *s - '0';
            if(mn < old)
            {
                throw wpkg_control_exception_invalid("minor device number too large");
            }
        }
    }

    set_dev(mj, mn);
}

void file_item::set_size(size_t size)
{
    f_size = size;
}

void file_item::set_size(const std::string& size)
{
    if(size == "-")
    {
        f_size = 0;
    }
    else
    {
        int sz(0);
        for(const char *s(size.c_str()); *s != '\0'; ++s)
        {
            if(*s < '0' || *s > '9')
            {
                throw wpkg_control_exception_invalid("file size was expected to be a valid decimal number");
            }
            int old(sz);
            sz = sz * 10 + *s - '0';
            if(sz < old)
            {
                throw wpkg_control_exception_invalid("file size too large");
            }
        }
        f_size = sz;
    }
}

void file_item::set_checksum(const std::string& checksum)
{
    f_checksum = checksum;
}

file_item::format_t file_item::get_format() const
{
    return f_format;
}

const std::string& file_item::get_filename() const
{
    return f_filename;
}

mode_t file_item::get_mode() const
{
    return f_mode;
}

size_t file_item::get_size() const
{
    return f_size;
}

const std::string& file_item::get_checksum() const
{
    return f_checksum;
}

std::string file_item::to_string(format_t format) const
{
    std::string filename(f_filename);
    if(filename.find_first_of(" ") != std::string::npos)
    {
        // filename includes spaces, add quotes
        filename = '"' + filename + '"';
    }
    if(filename.empty())
    {
        filename = "-";
    }

    std::string checksum(f_checksum);
    if(checksum.empty())
    {
        checksum = "-";
    }

    std::string user(f_user);
    if(user.empty() && f_uid == -1)
    {
        user = "-";
    }
    else
    {
        if(user.empty())
        {
            user = "-";
        }
        user += "/";
        if(f_uid == -1)
        {
            user += "-";
        }
        else
        {
            std::stringstream ss;
            ss << f_uid;
            user += ss.str();
        }
    }

    std::string group(f_group);
    if(group.empty() && f_gid == -1)
    {
        group = "-";
    }
    else
    {
        if(group.empty())
        {
            group = "-";
        }
        group += "/";
        if(f_gid == -1)
        {
            group += "-";
        }
        else
        {
            std::stringstream ss;
            ss << f_gid;
            group += ss.str();
        }
    }

    std::string mode;
    if(f_mode != 0)
    {
        switch(f_mode & S_IFMT)
        {
        case S_IFDIR:
            mode += "d";
            break;

        case S_IFREG:
            mode += "-";
            break;

        case S_IFCHR:
            mode += "c";
            break;

#ifndef MO_WINDOWS
        case S_IFIFO:
            mode += "f";
            break;

        case S_IFLNK:
            mode += "l";
            break;

        case S_IFBLK:
            mode += "b";
            break;
#endif

        default:
            throw wpkg_control_exception_invalid("file item has an unsupported file type in its mode");

        }
        // owner
        if(f_mode & 0400)
        {
            mode += 'r';
        }
        else
        {
            mode += '-';
        }
        if(f_mode & 0200)
        {
            mode += 'w';
        }
        else
        {
            mode += '-';
        }
        if(f_mode & 04000)
        {
            if(f_mode & 0100)
            {
                mode += 's';
            }
            else
            {
                mode += 'S';
            }
        }
        else
        {
            if(f_mode & 0100)
            {
                mode += 'x';
            }
            else
            {
                mode += '-';
            }
        }
        // group
        if(f_mode & 0040)
        {
            mode += 'r';
        }
        else
        {
            mode += '-';
        }
        if(f_mode & 0020)
        {
            mode += 'w';
        }
        else
        {
            mode += '-';
        }
        if(f_mode & 02000)
        {
            if(f_mode & 0010)
            {
                mode += 's';
            }
            else
            {
                mode += 'S';
            }
        }
        else
        {
            if(f_mode & 0010)
            {
                mode += 'x';
            }
            else
            {
                mode += '-';
            }
        }
        // other
        if(f_mode & 0004)
        {
            mode += 'r';
        }
        else
        {
            mode += '-';
        }
        if(f_mode & 0002)
        {
            mode += 'w';
        }
        else
        {
            mode += '-';
        }
        if(f_mode & 01000)
        {
            if(f_mode & 0001)
            {
                mode += 't';
            }
            else
            {
                mode += 'T';
            }
        }
        else
        {
            if(f_mode & 0001)
            {
                mode += 'x';
            }
            else
            {
                mode += '-';
            }
        }
    }

    std::stringstream ss;
    if(f_dev_major != undefined_device || f_dev_minor != undefined_device)
    {
        ss << f_dev_major << "," << f_dev_minor;
    }
    else
    {
        ss << f_size;
    }
    std::string size(ss.str());

    char mtime[32];
    time_t t(f_mtime);
    struct tm *m(gmtime(&t)); // should this be localtime() or gmtime()?
    // WARNING: remember that format needs to work on MS-Windows
    strftime_utf8(mtime, sizeof(mtime) - 1, "%Y%m%dT%H%M%S", m);
    mtime[sizeof(mtime) / sizeof(mtime[0]) - 1] = '\0';
    if(strcmp(mtime + 8, "T000000") == 0)
    {
        // remove the time as it is set to the default
        mtime[8] = '\0';
    }

    switch(format_unknown == format ? static_cast<format_t>(f_format) : format)
    {
    case format_unknown:
    case format_choose_best:
    case format_not_specified:
        throw wpkg_control_exception_invalid("file item does not have a valid format to be transformed to a string");

    case format_modelist:           // <mode> <filename>
        return mode + " " + filename;

    case format_conffiles:          // <filename> <md5sum>
        return filename + " " + checksum;

    case format_md5sum:             // <md5sum> <size> <filename>
    case format_sha1:               // <sha1> <size> <filename>
    case format_sha256:             // <sha256> <size> <filename>
        return checksum + " " + size + " " + filename;

    case format_list:               // <filename>
        return filename;

    case format_longlist:           // <mode> <size> <md5sum> <filename>
        if(mode.empty() || checksum == "-")
        {
            throw wpkg_control_exception_invalid("file mode or checksum missing for a long list");
        }
        return mode + " " + size + " " + checksum + " " + filename;

    case format_metadata:           // <mode> <user/uid> <group/gid> <size>|<major,minor> <mtime> <filename>
        return mode + " " + user + " " + group + " " + size + " " + mtime + " " + filename;

    }
    /*NOTREACHED*/
    throw std::logic_error("internal error: statement should never be reached");
}

namespace
{
    // set of flags to determine what data is defined based on the format
    const long file_item_flag_filename = 0x0001;
    const long file_item_flag_mode     = 0x0002;
    const long file_item_flag_md5sum   = 0x0004;
    const long file_item_flag_size     = 0x0008;
    const long file_item_flag_sha1     = 0x0010;
    const long file_item_flag_sha256   = 0x0020;
    const long file_item_flag_user     = 0x0040;
    const long file_item_flag_group    = 0x0080;
    const long file_item_flag_device   = 0x0100;
    const long file_item_flag_mtime    = 0x0200;

    const long required_fields[file_item::format_choose_best] =
    {
        /* format_not_specified */ 0,
        /* format_list          */ file_item_flag_filename,
        /* format_modelist      */ file_item_flag_filename | file_item_flag_mode,
        /* format_conffiles     */ file_item_flag_filename | file_item_flag_md5sum,
        /* format_md5sum        */ file_item_flag_md5sum | file_item_flag_size | file_item_flag_filename,
        /* format_sha1          */ file_item_flag_sha1 | file_item_flag_size | file_item_flag_filename,
        /* format_sha256        */ file_item_flag_sha256 | file_item_flag_size | file_item_flag_filename,
        /* format_longlist      */ file_item_flag_mode | file_item_flag_size | file_item_flag_md5sum | file_item_flag_filename,
        /* format_metadata      */ file_item_flag_mode | file_item_flag_user | file_item_flag_group | file_item_flag_size | file_item_flag_device | file_item_flag_mtime | file_item_flag_filename,
    };
}

file_item::format_t file_item::best_format(format_t b) const
{
    format_t a(determine_format());

    // we cannot determine the format if totally unknown
    if(a == format_unknown || b == format_unknown)
    {
        return format_unknown;
    }

    if(a == b)
    {
        return a;
    }

    long flags(required_fields[a] | required_fields[b]);
    for(format_t i(format_list); i < format_choose_best; i = static_cast<format_t>(static_cast<int>(i) + 1))
    {
        if((required_fields[i] & flags) == flags)
        {
            return i;
        }
    }

    // no best format (some formats cannot be merged together: i.e. SHA1 and MD5SUM are not compatible and they cannot both be defined in the same field)
    return file_item::format_unknown;
}

file_item::format_t file_item::determine_format() const
{
    if(f_format == format_not_specified || f_format == format_choose_best)
    {
        if(f_mode == 0)
        {
            if(f_size == 0)
            {
                if(f_checksum == "")
                {
                    return format_list;
                }
                return format_conffiles;
            }
            // TODO: check the length of the checksum to determine the real format
            return format_md5sum;
        }
        return format_longlist;
    }

    return f_format;
}

std::string file_list_t::to_string(file_item::format_t format, bool print_format) const
{
    // with an empty list, avoid any extra work
    if(empty())
    {
        return "";
    }

    file_list_t::size_type max(size());

    // first determine the best format
    file_item::format_t f(file_item::format_list);
    if(format == file_item::format_choose_best)
    {
        for(file_list_t::size_type i(0); i < max && f != file_item::format_unknown; ++i)
        {
            f = at(i).best_format(f);
        }
    }
    else
    {
        f = format;
    }
    if(f == file_item::format_unknown)
    {
        // this should never happen unless you add an undefined item to a list
        throw wpkg_control_exception_invalid("cannot find a valid format for this list of files");
    }

    // now generate each entry using the best format
    std::string result;
    if(print_format)
    {
        switch(f)
        {
        case file_item::format_conffiles:
            result = "conffiles";
            break;

        case file_item::format_list:
            result = "list";
            break;

        case file_item::format_modelist:
            result = "modelist";
            break;

        case file_item::format_longlist:
            result = "longlist";
            break;

        case file_item::format_md5sum:
            result = "sources";
            break;

        case file_item::format_sha1:
            result = "sha1";
            break;

        case file_item::format_sha256:
            result = "sha256";
            break;

        default:
            throw wpkg_control_exception_invalid("could not determine a valid format for your list of files");

        }
    }
    for(file_list_t::size_type i(0); i < max; ++i)
    {
        if(!result.empty())
        {
            result += "\n";
        }
        result += at(i).to_string(f);
    }

    return result;
}







}   // namespace wpkg_control

// vim: ts=4 sw=4 et
