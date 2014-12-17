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

// Qt4
//
#include <QtGui>

int main( int argc, char *argv[] )
{
	// The main Qt4 application object
	//
	QApplication app(argc, argv);

	// Set-up core application infor
	//
	QApplication::setOrganizationName   ( "M2OSW"     );
	QApplication::setOrganizationDomain ( "m2osw.com" );
	QApplication::setApplicationName    ( "systray"   );

	// Create and show main window
	//
	MainWindow mainWnd( 0 );
	mainWnd.show();

	// Start the application
	//
	return app.exec();
}


// vim: ts=4 sw=4 noexpandtab syntax=cpp.doxygen
