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
#pragma once

#include "include_qt4.h"
#include "ui_ProcessDialog.h"

class ProcessDialog : public QDialog, private Ui::ProcessDialog
{
    Q_OBJECT
    
public:
    ProcessDialog( QWidget *p = 0 );
    virtual ~ProcessDialog();

    // Thread-safe methods
    //
    void AddProgressValue( const int value );
    static bool CancelClicked();

    // Non-thread-safe methods (accesses the gui directly)
    //
    void EnableCancelButton( const bool enable );
	void ShowProgressBar( const bool show );
    void SetProgressRange( const int min, const int max );

public slots:
    void AddMessage( const QString& message );

protected:
	virtual void hideEvent( QHideEvent* evt );

private:
    QVector<QString>     	f_messageFifo;
    QVector<int>         	f_progressFifo;
    QTimer           		f_timer;

    static QMutex           f_mutex;
    static bool				f_cancelClicked;

private slots:
    void DisplayMessages();
    void on_f_cancelButton_clicked();
};

// vim: ts=4 sw=4 noet
