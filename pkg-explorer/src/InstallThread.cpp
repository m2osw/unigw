//===============================================================================
// Copyright:	Copyright (c) 2013 Made to Order Software Corp.
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

#include "InstallThread.h"

#include <libdebpackages/wpkg_output.h>

using namespace wpkgar;


InstallThread::InstallThread( QObject* p, wpkgar::wpkgar_manager* manager, wpkgar::wpkgar_install* installer, const Mode mode )
	: QThread(p)
	, f_manager(manager)
	, f_installer(installer)
	, f_state(ThreadStopped)
	, f_mode(mode)
{
}


InstallThread::State InstallThread::get_state() const
{
	QMutexLocker locker( &f_mutex );
	return f_state;
}


void InstallThread::set_state( const State new_state )
{
	QMutexLocker locker( &f_mutex );
	f_state = new_state;
}


bool InstallThread::Validate()
{
	const bool succeeded = f_installer->validate();
	if( !succeeded )
	{
		set_state( ThreadFailed );
	}
	else if( f_manager->is_self() )
	{
		wpkg_output::log( "Unfortunately, you cannot manage the pkg-explorer installation from itself! To update pkg-explorer use the pkg-explorer-setup or wpkg in a console." ).level(wpkg_output::level_error );
		set_state( ThreadFailed );
		return false;
	}
	return succeeded;
}


bool InstallThread::Preconfigure()
{
	const bool succeeded = f_installer->pre_configure();
	if( !succeeded )
	{
		set_state( ThreadFailed );
	}
	return succeeded;
}


void InstallThread::InstallFiles()
{
	for(;;)
	{
		const std::string package_name( f_installer->get_package_name(0) );
		//
		const int i( f_installer->unpack() );
		//
		if(i < 0)
		{
			if( i == wpkgar_install::WPKGAR_EOP )
			{
				wpkg_output::log( "Install complete!" );
				set_state( ThreadSucceeded );
			}
			else
			{
				wpkg_output::log( "Install failed!" ).level( wpkg_output::level_error );
				set_state( ThreadFailed );
			}
			break;
		}
		//
		if(!f_installer->configure(i))
		{
			wpkg_output::log( "Configuration failed!" ).level( wpkg_output::level_error );
			set_state( ThreadFailed );
			break;
		}
	}
}


void InstallThread::run()
{
	set_state( ThreadRunning );

    try
    {
		if( f_mode == ThreadValidateOnly || f_mode == ThreadFullInstall )
		{
            if( !Validate() )
            {
                // Stop here if we are in full install mode
                //
                return;
            }
        }
		if( f_mode == ThreadInstallOnly || f_mode == ThreadFullInstall )
		{
			if( Preconfigure() )
			{
				InstallFiles();
			}
		}
    }
    catch( const std::runtime_error& x )
    {
        qCritical() << "std::runtime_error caught! what=" << x.what();
        wpkg_output::log( x.what() ).level( wpkg_output::level_error );
		set_state( ThreadFailed );
    }
}


// vim: ts=4 sw=4 noet
