//===============================================================================
// Copyright:	Copyright (c) 2013-2015 Made to Order Software Corp.
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

#include "Manager.h"

namespace
{
	class ProcessInterrupt : public wpkgar::wpkgar_interrupt
	{
		public:
			virtual bool stop_now()
			{
				return ProcessDialog::CancelClicked();
			}
	};

	ProcessInterrupt g_interrupt;
}


Manager::Manager( std::weak_ptr<LogOutput> log )
{
    Init( log );
}


std::weak_ptr<wpkgar::wpkgar_manager> Manager::GetManager() const
{
	QMutexLocker locker( &f_mutex );
    return f_manager;
}


std::weak_ptr<wpkgar::wpkgar_install>   GetInstaller() const
{
	QMutexLocker locker( &f_mutex );
    return f_installer;
}


void Manager::Init( std::weak_ptr<wpkgar::wpkgar_lock> log )
{
	QMutexLocker locker( &f_mutex );

    f_manager.reset( new wpkgar_manager );

    f_manager->set_interrupt_handler( &g_interrupt );

	// TODO: add the Qt packages we depend on once ready
	//       (specifically for MS-Windows)
    f_manager->add_self("wpkg-gui");
    f_manager->add_self("wpkgguiqt4");

    wpkg_output::set_output( log.lock().get() );
    log.lock()->set_debug_flags( wpkg_output::debug_flags::debug_progress );

	QSettings settings;
	const QString root_path = settings.value( "root_path" ).toString();
    const QString database_path = QString("%1/var/lib/wpkg").arg(root_path);

    LogDebug( QString("Opening WPKG database %1").arg(root_path) );

    const QString rootMsg( tr("Database root: [%1]").arg(root_path) );
    f_statusLabel.setText( rootMsg );

    f_manager->set_root_path( root_path.toStdString() );
    f_manager->set_database_path( database_path.toStdString() );
	f_manager->add_sources_list();

    if( !CreateLock() )
    {
        throw std::exception( "Lock file not created!" );
    }
}


bool Manager::CreateLock()
{
    bool lock_file_created = false;
    while( !lock_file_created )
    {
        try
        {
            f_lock = reset( 
                        new wpkgar_lock( f_manager.get(), "Package Explorer" )
                        );
            lock_file_created = true;
        }
        catch( const wpkgar_exception_locked& except )
        {
            LogError( except.what() );
            QMessageBox::StandardButton result = QMessageBox::critical
                    ( this
                      , tr("Database locked!")
                      , tr("The database is locked. "
                           "This means that either pkg-explorer terminated unexpectantly, "
                           "or there is another instance accessing the database. Do you want to remove the lock?")
                      , QMessageBox::Yes | QMessageBox::No
                      );
            if( result == QMessageBox::Yes )
            {
                try
                {
                    f_manager->remove_lock();
                    LogDebug( "Lock file removed." );
                }
                catch( const std::runtime_error& _xcpt )
                {
                    LogFatal( _xcpt.what() );
                    break;
                }
            }
            else
            {
                // Quit the application ungracefully.
                //
                LogFatal( "Not removing the lock, so exiting application." );
                break;
            }
        }
        catch( const std::runtime_error& except )
        {
            LogFatal( except.what() );
            break;
        }
    }

    return lock_file_created;
}

// vim: ts=4 sw=4 et
