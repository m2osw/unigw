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

#include "UpdateThread.h"

#include <libdebpackages/wpkg_output.h>
#include <libdebpackages/wpkgar_repository.h>

using namespace wpkgar;

UpdateThread::UpdateThread( QObject* p )
    : QThread(p)
    , f_manager(Manager::WeakInstance())
{
}

void UpdateThread::run()
{
    try
    {
        QMutexLocker locker( &f_manager->GetMutex() );
        auto manager( f_manager->GetManager().lock() );

        // Load the installed packages into memory
        //
        wpkgar_manager::package_list_t list;
        manager->list_installed_packages( list );
        for( auto pkg : list )
        {
            manager->load_package( pkg );
        }

        wpkgar_repository repository( manager.get() );
        repository.update();
    }
    catch( const std::runtime_error& except )
    {
        qCritical() << "std::runtime_error caught! what=" << except.what();
        LogOutput::Instance()->OutputToLog( wpkg_output::level_error, except.what() );
    }
}

// vim: ts=4 sw=4 et
