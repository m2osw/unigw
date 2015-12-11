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


InstallThread::InstallThread
        ( QObject* p
		, const Mode mode
		)
	: QThread(p)
	, f_state(ThreadStopped)
	, f_mode(mode)
    , f_manager(Manager::WeakInstance())
    , f_mutex(QMutex::Recursive)
{
}


InstallThread::State InstallThread::get_state() const
{
    QMutexLocker locker( &f_mutex );
	return f_state;
}


void InstallThread::set_state( const State new_state )
{
	f_state = new_state;
}


bool InstallThread::Validate( std::shared_ptr<wpkgar_manager> manager, std::shared_ptr<wpkgar_install> installer )
{
    const bool succeeded = installer->validate();
	if( !succeeded )
	{
		set_state( ThreadFailed );
	}
    else if( manager->is_self() )
	{
        wpkg_output::log( "Unfortunately, you cannot manage the pkg-explorer installation from itself! To update pkg-explorer use the pkg-explorer-setup or wpkg in a console." ).level(wpkg_output::level_error );
		set_state( ThreadFailed );
		return false;
	}
	return succeeded;
}


bool InstallThread::Preconfigure( std::shared_ptr<wpkgar_install> installer )
{
    const bool succeeded = installer->pre_configure();
	if( !succeeded )
	{
		set_state( ThreadFailed );
	}
	return succeeded;
}


void InstallThread::InstallFiles( std::shared_ptr<wpkgar_install> installer )
{
	for(;;)
	{
		const std::string package_name( installer->get_package_name(0) );
		//
		const int i( installer->unpack() );
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
		if(!installer->configure(i))
		{
            wpkg_output::log( "Configuration failed!" ).level( wpkg_output::level_error );
            set_state( ThreadFailed );
			break;
		}
	}
}


void InstallThread::run()
{
    try
    {
        QMutexLocker locker( &f_mutex );
        QMutexLocker mgr_locker( &(f_manager->GetMutex()) );

        set_state( ThreadRunning );

        // Lock these in place, which is thread-safe, during the thread lifetime.
        //
        auto manager     ( f_manager->GetManager().lock()   );
        auto installer   ( f_manager->GetInstaller().lock() );

        // Load the installed packages into memory
        //
        manager->load_installed_packages();

        if( f_mode == ThreadValidateOnly || f_mode == ThreadFullInstall )
        {
            if( Validate( manager, installer ) )
            {
                // Stop here if we are in full install mode. Don't delete the manager instance.
                //
                return;
            }
        }
        if( f_mode == ThreadInstallOnly || f_mode == ThreadFullInstall )
        {
            if( Preconfigure( installer ) )
            {
                InstallFiles( installer );
            }
        }
    }
    catch( const std::runtime_error& x )
    {
        qCritical() << "std::runtime_error caught! what=" << x.what();
        LogOutput::Instance()->OutputToLog( wpkg_output::level_error, x.what() );
        set_state( ThreadFailed );
    }
}


// vim: ts=4 sw=4 noet
