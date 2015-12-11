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

#include "RemoveThread.h"

#include <libdebpackages/wpkg_output.h>
#include <libdebpackages/wpkgar_repository.h>

using namespace wpkgar;

RemoveThread::RemoveThread( QObject* p )
        : QThread(p)
		, f_state(ThreadStopped)
        , f_manager(Manager::WeakInstance())
        , f_mutex(QMutex::Recursive)
{
}

RemoveThread::State RemoveThread::get_state() const
{
    QMutexLocker locker( &f_mutex );
    return f_state;
}

void RemoveThread::set_state( const State new_state )
{
    f_state = new_state;
}


void RemoveThread::run()
{
    try
    {
        QMutexLocker locker( &f_mutex );
        QMutexLocker mgr_locker( &(f_manager->GetMutex()) );

        auto manager( f_manager->GetManager().lock() );
        auto remover( f_manager->GetRemover().lock() );

        set_state( ThreadRunning );

        // Load the installed packages into memory
        //
        wpkgar_manager::package_list_t list;
        manager->list_installed_packages( list );
        for( auto pkg : list )
        {
            manager->load_package( pkg );
        }

        for(;;)
		{
            const int i( remover->remove() );
			if(i < 0)
			{
                if( i == wpkgar_remove::WPKGAR_EOP )
				{
					wpkg_output::log( "Removal of packages complete!" ).level( wpkg_output::level_info );
					set_state( ThreadSucceeded );
				}
				else
				{
					wpkg_output::log( "Removal of packages failed!" ).level( wpkg_output::level_error );
					set_state( ThreadFailed );
				}
				break;
			}
            if(remover->get_purging())
			{
                if( !remover->deconfigure(i) )
				{
					wpkg_output::log( "Removal failed deconfiguration!" ).level( wpkg_output::level_error );
					set_state( ThreadFailed );
					break;
				}
			}
		}
    }
    catch( const std::runtime_error& except )
    {
        qCritical() << "std::runtime_error caught! what=" << except.what();
        LogOutput::Instance()->OutputToLog( wpkg_output::level_error, except.what() );
        set_state( ThreadFailed );
    }
}

// vim: ts=4 sw=4 et
