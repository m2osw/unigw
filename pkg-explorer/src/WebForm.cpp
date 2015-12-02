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

#include "WebForm.h"
#include "Manager.h"

#include <QtWebKit>

#include <libdebpackages/wpkgar.h>

WebForm::WebForm(QWidget *p)
    : QWidget(p)
{
    setupUi(this);

	f_webView->page()->setLinkDelegationPolicy( QWebPage::DelegateAllLinks );

	QObject::connect
        ( f_webView, SIGNAL( linkClicked  ( const QUrl& ))
        , this         , SLOT  ( OnLinkClicked( const QUrl& ))
		);
}

WebForm::~WebForm()
{
}


void WebForm::Back()
{
    if( !f_backStack.isEmpty() )
    {
        const QString package_name = f_backStack.pop();
        f_fwdStack.push( f_currentPackage );
        f_currentPackage = package_name;
    }

    PrivateDisplayPackage();
    emit HistoryChanged( f_currentPackage );
    emit StackStatus( f_backStack.isEmpty(), f_fwdStack.isEmpty() );
}


void WebForm::Forward()
{
    if( !f_fwdStack.isEmpty() )
    {
        const QString package_name = f_fwdStack.pop();
        f_backStack.push( f_currentPackage );
        f_currentPackage = package_name;
    }

    PrivateDisplayPackage();
    emit HistoryChanged( f_currentPackage );
    emit StackStatus( f_backStack.isEmpty(), f_fwdStack.isEmpty() );
}


void WebForm::DisplayPackage( const QString& package_name )
{
    f_fwdStack.clear();
    //
    if( !f_currentPackage.isEmpty() )
    {
        f_backStack.push( f_currentPackage );
    }
    //
    f_currentPackage = package_name;

    PrivateDisplayPackage();
    emit StackStatus( f_backStack.isEmpty(), f_fwdStack.isEmpty() );
}


void WebForm::ClearDisplay()
{
    f_webView->load( QUrl("about:blank") );
}


void WebForm::ClearHistory()
{
    f_backStack.clear();
    f_fwdStack.clear();

    emit StackStatus( f_backStack.isEmpty(), f_fwdStack.isEmpty() );
}


void WebForm::OnLinkClicked( const QUrl& url )
{
    if( url.scheme() == "package" )
    {
        emit PackageClicked( url.host() );
    }
    else if( url.scheme() == "http" )
    {
        emit WebPageClicked( url.toString() );
    }
}


void WebForm::PrivateDisplayPackage()
{
    if( !f_thread )
    {
        f_thread.reset( new DisplayThread( this, f_currentPackage ) );
        f_thread->start();

        connect
            ( f_thread.get(), &QThread::finished
            , this          , &WebForm::OnPrivateDisplayPackage
            );
    }
}


void WebForm::OnPrivateDisplayPackage()
{
    f_webView->setHtml( f_thread->GetHtml().c_str() );

    // Destroy manager object now that we're finished.
    //
    f_thread->wait();
    f_thread.reset();
    //Manager::Release();
}

// vim: ts=4 sw=4 noet
