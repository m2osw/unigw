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

#include "ProcessDialog.h"

#include <QtWidgets>


bool ProcessDialog::f_cancelClicked = false;
QMutex ProcessDialog::f_mutex;


ProcessDialog::ProcessDialog(QWidget *p)
    : QDialog(p)
{
    setupUi(this);
    f_progressBar->setVisible( false );

    ShowLogPane( false );

    connect( &f_timer, SIGNAL(timeout()), this, SLOT(DisplayMessages()));
    f_timer.start( 100 );
}

ProcessDialog::~ProcessDialog()
{
}


void ProcessDialog::hideEvent( QHideEvent* /*evt*/ )
{
    QMutexLocker locker( &f_mutex );
	f_cancelClicked = false;
	f_cancelButton->setText( tr("&Cancel") );
}


void ProcessDialog::AddMessage( const QString& message )
{
    QMutexLocker locker( &f_mutex );
    f_messageFifo.push_back( message );
}


void ProcessDialog::AddProgressValue( int value )
{
    QMutexLocker locker( &f_mutex );
    f_progressFifo.push_back( value );
}


void ProcessDialog::ShowLogPane( const bool val )
{
    if( val )
    {
        setWindowFlags( Qt::Window );
    }
    else
    {
        setWindowFlags( Qt::Dialog
                        | Qt::CustomizeWindowHint	// Turn off the system menu, title bar, and max/min buttons
                        );
    }
    //
    f_dockWidget->setVisible( val );
}


void ProcessDialog::DisplayMessages()
{
    QMutexLocker locker( &f_mutex );
    while( !f_messageFifo.isEmpty() )
    {
        f_label->setText( f_messageFifo.front() );
		f_messageFifo.pop_front();
    }

    while( !f_progressFifo.isEmpty() )
    {
        f_progressBar->setValue( f_progressFifo.front() );
		f_progressFifo.pop_front();
    }
}


void ProcessDialog::EnableCancelButton( const bool enable )
{
    QMutexLocker locker( &f_mutex );
	f_cancelClicked = false;
	f_cancelButton->setEnabled( enable );
}


bool ProcessDialog::CancelClicked()
{
    QMutexLocker locker( &f_mutex );
	return f_cancelClicked;
}


void ProcessDialog::ShowProgressBar( const bool show_it )
{
    f_progressBar->setVisible( show_it );
}


void ProcessDialog::SetProgressRange( const int min, const int max )
{
	f_progressBar->setMinimum( min );
	f_progressBar->setMaximum( max );
}


void ProcessDialog::on_f_cancelButton_clicked()
{
    QMessageBox::StandardButton answer = QMessageBox::question
		( this
		, tr("Cancel Operation")
		, tr("Are you sure you want to cancel the current operation?")
		, QMessageBox::Yes | QMessageBox::No
		);
	if( answer == QMessageBox::Yes )
	{
		QMutexLocker locker( &f_mutex );
		f_cancelClicked = true;
        f_cancelButton->setText( tr("Cancelling...please wait...") );
		f_cancelButton->setEnabled( false );
	}
}


// vim: ts=4 sw=4 noet
