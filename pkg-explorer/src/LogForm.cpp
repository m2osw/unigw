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

#include "LogForm.h"

#include <QtWidgets>
#include <libdebpackages/wpkgar.h>

using namespace wpkgar;

LogForm::LogForm(QWidget *p)
    : QWidget(p)
{
    setupUi(this);
    f_textEdit->ensureCursorVisible();
    connect( &f_timer, SIGNAL(timeout()), this, SLOT(OnDisplayText()) );
	f_timer.start( 100 );
}


LogForm::~LogForm()
{
}


wpkg_output::level_t LogForm::GetLogLevel() const
{
    return f_logOutput.get_level();
}


void LogForm::SetLogLevel( wpkg_output::level_t level )
{
    f_logOutput.set_level( level );
}


wpkg_output::output* LogForm::GetLogOutput()
{
    return &f_logOutput;
}


void LogForm::OnDisplayText()
{
    if( !f_logOutput.pending_messages() )
	{
		return;
	}

    const wpkg_output::message_t message( f_logOutput.pop_next_message() );
	//
    if( message.get_module() == wpkg_output::module_tool )
    {
        const QString raw_text( message.get_raw_message().c_str() );
        SetSystrayMessage( raw_text );
    }
	//
	QTextEdit *pTextEdit = f_textEdit;
	Q_ASSERT(pTextEdit);
	//
	const QString msg_text( message.get_full_message( true ).c_str() );
	const QString text( QString("%1\n").arg(msg_text) );
	//
	// Insert the text, but move the cursor to the end.
	// This forces the track bar to scroll to the end of the log
	// when new lines are appended...
	//
	if( message.get_level() >= wpkg_output::level_error )
	{
        pTextEdit->insertHtml( "<span style='color: #880000;'>" + text.toHtmlEscaped() + "<br/></span>" );
	}
	else
	{
        pTextEdit->insertHtml( "<span style='color: #000000;'>" + text.toHtmlEscaped() + "<br/></span>" );
	}

	QTextCursor c = pTextEdit->textCursor();
	c.movePosition( QTextCursor::End );
	pTextEdit->setTextCursor(c);
}


void LogForm::Clear()
{
    f_logOutput.clear();
}


// vim: ts=4 sw=4 noet
