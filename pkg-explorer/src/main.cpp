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

#include "MainWindow.h"
#include "PrefsDialog.h"

#include <iostream>
#include <memory>

// Qt4
//
#include <QtGui>

int main( int argc, char *argv[] )
{
	// The main Qt4 application object
	//
	QApplication  app(argc, argv);

	QStringList args(app.arguments());
	if( args.contains("--help") || args.contains("-h") )
	{
		printf("Usage: pkg-explorer [--help | --version]\n\n");
		printf("  Run pkg-explorer by itself and use the menus to do work.\n\n");
		printf("  If you are looking for a command line tool to manage your installation\n");
		printf("  environment, use wpkg instead.\n");
		exit(0);
	}
	if( args.contains("--version") || args.contains("-v") )
	{
		printf("pkg-explorer %s\n", VERSION);
		exit(0);
	}

    // Set-up core application info
	//
	QApplication::setOrganizationName   ( "M2OSW"        );
	QApplication::setOrganizationDomain ( "m2osw.com"    );
	QApplication::setApplicationName    ( "pkg-explorer" );

	// Make sure the wpkg database is created and initialized.
	//
	PrefsDialog::InitDatabase();

	// Attempt to create shared memory segment--if created, then there is another instance running...
	//
	QSharedMemory sm("pkg_explorer");
	std::cout << sm.nativeKey().toStdString() << std::endl;
	bool have_db = false;
	if( sm.create( 1, QSharedMemory::ReadOnly ) )
	{
		have_db = true;
	}
	else
	{
		if( sm.error() == QSharedMemory::AlreadyExists )
		{
			std::cerr << "pkg_explorer is already running!" << std::endl;
			QMessageBox::critical( 0, QObject::tr("Error"), QObject::tr("Package Explorer is already running!") );
		}
		else
		{
			std::cerr << "Cannot create shared memory!" << std::endl;
			QMessageBox::critical( 0, QObject::tr("Error"), QObject::tr("Cannot create shared memory!") );
		}
		if( sm.isAttached() )
		{
			sm.detach();
		}
		//
		return 1;
	}

	// Create and show main window
	//
	int retval = 0;
#if 0
	if( argc == 2 )
	{
		MainWindow mainWnd( false /*showSysTray*/ );
		mainWnd.RunCommand( argv[1] );
    }
	else
	{
#endif
		MainWindow mainWnd;
		mainWnd.show();
		//
        app.setQuitOnLastWindowClosed( false ); // We need this to keep the app from qutting when
                                                // we are minimized to systray and a dialog closes
		retval = app.exec();
    //}

	sm.detach();

	return retval;
}


// vim: ts=4 sw=4 noexpandtab syntax=cpp.doxygen
