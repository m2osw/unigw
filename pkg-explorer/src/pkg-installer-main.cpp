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

//===============================================================================
/// \brief Main Function
//===============================================================================

#include "database.h"
#include "ImportDialog.h"
#include "LogOutput.h"
#include "ProcessDialog.h"

#include <libdebpackages/wpkgar.h>

#include <iostream>
#include <memory>

// Qt4
//
#include <QtGui>

#include "ProcessWindow.h"

using namespace wpkgar;

int main( int argc, char *argv[] )
{
	// The main Qt4 application object
	//
	QApplication  app(argc, argv);

	QStringList args(app.arguments());
	if( args.contains("--help") || args.contains("-h") )
	{
        printf("Usage: pkg-installer [--help | --version] [package1] [package2] ... [packageN]\n\n");
        printf("  Run pkg-installer to import WPKG packages into the database, but with a graphical meter.\n\n");
        printf("  Useful for the Windows Explorer shell, Nautilis or Mac OS/X Finder.\n");
        printf("  If you are looking for a command line tool to manage your installation\n");
		printf("  environment, use wpkg instead.\n");
		exit(0);
	}
	if( args.contains("--version") || args.contains("-v") )
	{
        printf("pkg-installer %s\n", VERSION);
		exit(0);
	}

	QStringList full_args( args );
    args.clear();
    std::for_each( full_args.begin(), full_args.end(), [&args]( QString arg )
	{
        if( arg.endsWith( ".deb" ) )
        {
            args << arg;
        }
    }
    );

	// Set-up core application info
	//
	QApplication::setOrganizationName   ( "M2OSW"        );
	QApplication::setOrganizationDomain ( "m2osw.com"    );
    QApplication::setApplicationName    ( "pkg-explorer" );

	// Make sure the wpkg database is created and initialized.
	//
    Database::InitDatabase();

    QSharedPointer<wpkgar::wpkgar_manager>	manager( new wpkgar_manager );
    //manager->set_interrupt_handler( &interrupt );
    manager->add_self("pkg-explorer");

	QSettings settings;
	const QString root_path     ( settings.value( "root_path" ).toString()  );
    const QString database_path ( QString("%1/var/lib/wpkg").arg(root_path) );
    manager->set_root_path      ( root_path.toStdString()     );
    manager->set_database_path  ( database_path.toStdString() );
	//
    QSharedPointer<wpkgar::wpkgar_lock>		lock( new wpkgar_lock( manager.data(), "Package Installer" ) );

	LogOutput	log_output;
    wpkg_output::set_output( &log_output );
    log_output.set_debug( wpkg_output::debug_flags::debug_progress );

	// Create and show main window
	//
    ImportDialog import_dlg( 0, manager );
	import_dlg.ShowLogPane();

    ProcessWindow proc_dlg( &import_dlg );

    QObject::connect
        ( &import_dlg , SIGNAL(ShowProcessDialog(bool,bool))
        , &proc_dlg   , SLOT  (ShowProcessDialog(bool,bool))
        );

    import_dlg.AddPackages( args );
    import_dlg.show();

	return app.exec();
}


// vim: ts=4 sw=4 noexpandtab syntax=cpp.doxygen
