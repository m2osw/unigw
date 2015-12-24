/*    wpkg_changelog.h -- declaration of the changelog file format
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
#ifndef WPKG_CHANGELOG_H
#define WPKG_CHANGELOG_H

/** \file
 * \brief Declaration of the necessary class to load changelog files.
 *
 * As we want to be capable to read changelog files in order to use the
 * information found in them for:
 *
 * \li The Changes field with the actual descriptions
 * \li The Version field to make sure it matches the current version of the
 *     project
 * \li The Package field name to see that it matches
 * \li The Distribution field with one or more distributions for which
 *     the package can be compiled
 * \li The Urgency field with the changelog information for the current
 *     version
 */
#include    "memfile.h"
#include    "controlled_vars/controlled_vars_auto_enum_init.h"


namespace wpkg_changelog
{

class wpkg_changelog_exception : public std::runtime_error
{
public:
    wpkg_changelog_exception(const std::string& what_msg) : runtime_error(what_msg) {}
};




class DEBIAN_PACKAGE_EXPORT changelog_file
{
public:
    class state
    {
    public:
                                    state(const memfile::memory_file& input);

        bool                        next_line();
        const std::string&          last_line() const;
        int                         space_count() const;
        void                        restore();
        bool                        has_empty_line() const;

        wpkg_filename::uri_filename get_filename() const;
        int                         get_line() const;

    private:
        const memfile::memory_file& f_input;
        std::string                 f_last_line;
        controlled_vars::zint32_t   f_space_count;
        int                         f_offset; // cannot use a controlled var.
        controlled_vars::zint32_t   f_previous_offset;
        controlled_vars::zint32_t   f_line;
        controlled_vars::fbool_t    f_has_empty_line;
    };

    // one block represents a specific version of the package
    class version
    {
    public:
        class DEBIAN_PACKAGE_EXPORT log
        {
        public:
            bool                        parse(state& s, bool& group);

            bool                        is_group() const;
            std::string                 get_log() const;
            std::string                 get_bug() const;

            wpkg_filename::uri_filename get_filename() const;
            int                         get_line() const;

        private:
            wpkg_filename::uri_filename f_filename;
            controlled_vars::zint32_t   f_line;

            controlled_vars::fbool_t    f_is_group;
            std::string                 f_log;
            std::string                 f_bug;
        };
        typedef std::vector<log> log_list_t;
        typedef std::map<std::string, std::string> parameter_list_t;
        typedef std::vector<std::string> distributions_t;

        bool                        parse(state& s);

        std::string                 get_package() const;
        std::string                 get_version() const;
        const distributions_t&      get_distributions() const;
        bool                        parameter_is_defined(const std::string& name) const;
        std::string                 get_parameter(const std::string& name) const;
        const parameter_list_t&     get_parameters() const;
        std::string                 get_maintainer() const;
        std::string                 get_date() const;
        int                         count_groups() const;
        const log_list_t&           get_logs() const;

        wpkg_filename::uri_filename get_filename() const;
        int                         get_line() const;

    private:
        wpkg_filename::uri_filename f_filename;
        controlled_vars::zint32_t   f_line;

        std::string                 f_package;
        std::string                 f_version;
        distributions_t             f_distributions;
        std::string                 f_maintainer; // name + email
        std::string                 f_date;
        parameter_list_t            f_parameters;
        log_list_t                  f_logs;
    };
    typedef std::vector<version> version_list_t;

                    changelog_file();

    bool            read(const memfile::memory_file& data);

    size_t          get_version_count() const;
    const version&  get_version(int idx) const;

private:
    // avoid copies
    changelog_file(const changelog_file& rhs);
    changelog_file& operator = (const changelog_file& rhs);

    version_list_t      f_versions;
};



}       // namespace wpkg_changelog

#endif
//#ifndef WPKG_CHANGELOG_H
// vim: ts=4 sw=4 et
