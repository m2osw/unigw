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

#include "DisplayThread.h"
#include "ProcessDialog.h"
#include "include_qt4.h"
#include "ui_WebForm.h"

#include <memory>

namespace wpkgar
{
    class wpkgar_manager;
}

class WebForm : public QWidget, private Ui::WebForm
{
    Q_OBJECT
    
public:
    WebForm( QWidget *p = 0 );
    ~WebForm();

    void DisplayPackage( const QString& package_name );
    void ClearDisplay();
    void ClearHistory();

    void Back();
    void Forward();
    
private:
    QString									f_currentPackage;
    QStack<QString>							f_backStack;
    QStack<QString>							f_fwdStack;
    ProcessDialog							f_processDlg;
    std::shared_ptr<DisplayThread>			f_thread;

    void PrivateDisplayPackage();

signals:
    void StackStatus( bool back_empty, bool fwd_empty );
    void PackageClicked( const QString& package_name );
    void HistoryChanged( const QString& package_name );
    void WebPageClicked( const QString& webpage_url  );

private slots:
	void OnLinkClicked( const QUrl& url );
	void OnPrivateDisplayPackage();
};

// vim: ts=4 sw=4 noet
