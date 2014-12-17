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

#include "MainWindow.h"
#include "ui_MainWindow.h"

#include <QtCore>
#include <QtGui>

#ifdef WIN32
#	undef open
#	undef close
#endif


MainWindow::MainWindow(QWidget *p)
	: QMainWindow(p)
    , ui(new Ui::MainWindow)
	, f_tray( new QSystemTrayIcon(this) )
{
	setWindowIcon( QIcon( ":/icons/m2osw_logo" ) );
    ui->setupUi(this);
	f_tray->setIcon( QIcon( ":/icons/m2osw_logo" ) );
	f_tray->show();
}


MainWindow::~MainWindow()
{
    delete ui;
}


// vim: ts=4 sw=4 noet
