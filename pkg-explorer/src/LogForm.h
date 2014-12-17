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
#include "ProcessDialog.h"
#include "LogForm.h"
#include "ui_LogForm.h"

#include <libdebpackages/wpkg_output.h>

class SystrayMessageCB
{
public:
    virtual void OnSystrayMessage( const QString& msg ) = 0;
};


class LogForm
    : public QWidget
    , private SystrayMessageCB
    , private Ui::LogForm
{
    Q_OBJECT
    
public:
    LogForm( QWidget *p = 0 );
    ~LogForm();

    wpkg_output::output* GetLogOutput() const;

    void ShowProcessDialog( const bool show_it, const bool enable_cancel = true );

    wpkg_output::level_t GetLogLevel() const;
    void SetLogLevel( wpkg_output::level_t level );

	void clear();

private:
    ProcessDialog			f_procDlg;
    wpkg_output::output *	f_logOutput;
    QTimer					f_timer;

    virtual void OnSystrayMessage( const QString& msg );

signals:
    void SystrayMessage( const QString& msg );

private slots:
	void OnDisplayText();
};

// vim: ts=4 sw=4 noet
