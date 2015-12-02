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

#include "InitThread.h"

#include <libdebpackages/wpkgar.h>
#include <libdebpackages/wpkg_output.h>
#include <libdebpackages/wpkgar_repository.h>

using namespace wpkgar;

namespace
{
    QString StatusToQString( wpkgar_manager::package_status_t status )
	{
		switch( status )
		{
			case wpkgar_manager::no_package		 : return QObject::tr("no_package");
			case wpkgar_manager::unknown 		 : return QObject::tr("unknown");
			case wpkgar_manager::not_installed	 : return QObject::tr("not_installed");
			case wpkgar_manager::config_files	 : return QObject::tr("config_files");
			case wpkgar_manager::installing		 : return QObject::tr("installing");
			case wpkgar_manager::upgrading		 : return QObject::tr("upgrading");
			case wpkgar_manager::half_installed	 : return QObject::tr("half_installed");
			case wpkgar_manager::unpacked		 : return QObject::tr("unpacked");
			case wpkgar_manager::half_configured : return QObject::tr("half_configured");
			case wpkgar_manager::installed		 : return QObject::tr("installed");
			case wpkgar_manager::removing		 : return QObject::tr("removing");
			case wpkgar_manager::purging		 : return QObject::tr("purging");
			case wpkgar_manager::listing		 : return QObject::tr("listing");
			case wpkgar_manager::verifying		 : return QObject::tr("verifying");
			case wpkgar_manager::ready			 : return QObject::tr("ready");
		}

		return QObject::tr("Unknown!");
	}

	void ResetErrorCount()
	{
		wpkg_output::output* output = wpkg_output::get_output();
		if( output )
		{
			output->reset_error_count();
		}
	}
}


InitThread::InitThread( QObject* p, const bool show_installed_only )
    : QThread(p)
    , f_showInstalledOnly(show_installed_only)
    , f_manager(Manager::WeakInstance())
    , f_mutex(QMutex::Recursive)
{}


InitThread::~InitThread()
{
    f_manager.reset();
}


void InitThread::run()
{
	try
    {
        QMutexLocker locker( &f_mutex );
        QMutexLocker mgr_locker( &f_manager->GetMutex() );

        wpkgar_manager::package_list_t list;

        auto manager( f_manager->GetManager().lock() );
        ResetErrorCount();
        manager->list_installed_packages( list );

        Q_FOREACH( std::string package_name, list )
        {
            wpkgar_manager::package_status_t status( manager->package_status( package_name ) );

            bool show_package = true;
            //
            if( f_showInstalledOnly && (status != wpkgar_manager::installed) )
            {
                switch( status )
                {
                case wpkgar_manager::installed:
                case wpkgar_manager::half_installed:
                case wpkgar_manager::half_configured:
                    show_package = true;
                    break;
                default:
                    show_package = false;
                }
            }
            //
            if( show_package )
            {
                const std::string version( manager->get_field( package_name, "Version" ) );
                std::string section("base"); // use a valid default
                if( manager->field_is_defined( package_name, "Section" ) )
                {
                    section = manager->get_field( package_name, "Section" );
                }
                ItemList pkg;
                pkg << package_name.c_str();
                pkg << StatusToQString( status );
                pkg << version.c_str();
                f_sectionMap[section.c_str()] << pkg;
            }
        }
    }
    catch( const std::runtime_error& except )
    {
        qCritical() << "std::runtime_error caught! what=" << except.what();
		wpkg_output::log( except.what() ).level( wpkg_output::level_error );
    }
}


InitThread::SectionMap InitThread::GetSectionMap() const
{
    QMutexLocker locker( &f_mutex );
    return f_sectionMap;
}

// vim: ts=4 sw=4 et
