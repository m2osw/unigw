/*    wpkgar_tracker.h -- declaration of the wpkg tracker to rollback changes
 *    Copyright (C) 2012-2014  Made to Order Software Corporation
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
 * \brief Tracker declaration.
 *
 * The tracker is used to track all the functions run to install or remove
 * packages. This is used in order to allow for a full rollback() in case
 * something goes back.
 *
 * Actually, the primordial reason for the feature is for implementing the
 * --build command which installs a source package, builds it, create the
 * binary packages, and then restore the target system to a pristine state.
 * This already works perfectly under Linux.
 */
#ifndef WPKGAR_TRACKER_H
#define WPKGAR_TRACKER_H
#include    "wpkgar.h"


namespace wpkgar
{



class DEBIAN_PACKAGE_EXPORT wpkgar_tracker : public wpkgar::wpkgar_tracker_interface
{
public:
                                    wpkgar_tracker(wpkgar_manager *manager, const wpkg_filename::uri_filename& tracking_filename);
    virtual                         ~wpkgar_tracker();

    void                            rollback();
    void                            commit();

    void                            keep_file(bool keep);
    wpkg_filename::uri_filename     get_filename() const;

    virtual void                    track(const std::string& command, const std::string& package_name = "");

private:
    wpkgar_manager *                    f_manager;
    controlled_vars::fbool_t            f_keep_file;
    controlled_vars::fbool_t            f_committed;
    const wpkg_filename::uri_filename   f_filename;
};


}       // namespace wpkgar_tracker

#endif
//#ifndef WPKGAR_H
// vim: ts=4 sw=4 et
