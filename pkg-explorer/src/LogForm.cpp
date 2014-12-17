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

#include <QtCore>
#include <libdebpackages/wpkgar.h>

using namespace wpkgar;

class LogOutput
        : public wpkg_output::output
{
public:
    LogOutput( Ui::LogForm *_ui, ProcessDialog* dlg, SystrayMessageCB* systrayCB )
        : ui(_ui)
		, f_procDlg(dlg)
        , f_logLevel(wpkg_output::level_info)
        , f_systrayCb(systrayCB)
	{
		set_program_name(QApplication::applicationName().toStdString());
	}

    virtual void log_message( const wpkg_output::message_t& msg ) const
    {
		QMutexLocker locker( &f_mutex );
        if( compare_levels(msg.get_level(), f_logLevel) >= 0 )
		{
            f_messageFifo.push_back( msg );
        }

		if( msg.get_debug_flags() & wpkg_output::debug_flags::debug_progress )
        {
            f_procDlg->AddMessage( msg.get_raw_message().c_str() );
        }
    }


    wpkg_output::level_t get_level() const
    {
        return f_logLevel;
    }

    void set_level( wpkg_output::level_t level )
	{
		QMutexLocker locker( &f_mutex );
		f_logLevel = level;
	}

	void display_text()
	{
		QMutexLocker locker( &f_mutex );
		while( !f_messageFifo.isEmpty() )
		{
			const wpkg_output::message_t message = f_messageFifo.first();
			f_messageFifo.pop_front();
            //
            if( f_systrayCb )
            {
                if( message.get_module() == wpkg_output::module_tool )
                {
                    const QString raw_text( message.get_raw_message().c_str() );
                    f_systrayCb->OnSystrayMessage( raw_text );
                }
            }
			//
            QTextEdit *pTextEdit = ui->f_textEdit;
            Q_ASSERT(pTextEdit);
			//
            const QString msg_text( message.get_full_message( true ).c_str() );
            const QString text( QString("%1\n").arg(msg_text) );
			//
			// Insert the text, but move the cursor to the end.
			// This forces the track bar to scroll to the end of the log
			// when new lines are appended...
			//
			if(message.get_level() >= wpkg_output::level_error)
			{
            	pTextEdit->insertHtml( "<span style='color: #880000;'>" + Qt::escape(text) + "<br/></span>" );
			}
			else
			{
            	pTextEdit->insertHtml( "<span style='color: #000000;'>" + Qt::escape(text) + "<br/></span>" );
			}
            QTextCursor c = pTextEdit->textCursor();
            c.movePosition( QTextCursor::End );
            pTextEdit->setTextCursor(c);
        }
	}

	void clear()
	{
		QMutexLocker locker( &f_mutex );
		QTextEdit *pTextEdit = ui->f_textEdit;
		Q_ASSERT(pTextEdit);
		pTextEdit->setText( "" );
	}

private:
    Ui::LogForm*					ui;
    ProcessDialog*				    f_procDlg;
    wpkg_output::level_t	f_logLevel;
    SystrayMessageCB*            	f_systrayCb;
	//
    mutable QMutex										f_mutex;
    mutable QVector<wpkg_output::message_t>		f_messageFifo;
};


LogForm::LogForm(QWidget *p)
    : QWidget(p)
    , f_procDlg(p)  // Make sure parent is this object's parent, in case LogForm is hidden.
    , f_logOutput( new LogOutput( this, &f_procDlg, this ) )
{
    setupUi(this);
    f_textEdit->ensureCursorVisible();
    connect( &f_timer, SIGNAL(timeout()), this, SLOT(OnDisplayText()) );
	f_timer.start( 100 );
}


LogForm::~LogForm()
{
	delete f_logOutput;
}


void LogForm::ShowProcessDialog( const bool show_it, const bool enable_cancel )
{
    if( show_it )
    {
        f_procDlg.show();
		f_procDlg.EnableCancelButton( enable_cancel );
    }
    else
    {
        f_procDlg.hide();
    }
}


wpkg_output::level_t LogForm::GetLogLevel() const
{
    return dynamic_cast<LogOutput*>(f_logOutput)->get_level();
}

void LogForm::SetLogLevel( wpkg_output::level_t level )
{
    dynamic_cast<LogOutput*>(f_logOutput)->set_level( level );
}


wpkg_output::output* LogForm::GetLogOutput() const
{
	return f_logOutput;
}


void LogForm::OnDisplayText()
{
    dynamic_cast<LogOutput*>(f_logOutput)->display_text();
}


void LogForm::clear()
{
    dynamic_cast<LogOutput*>(f_logOutput)->clear();
}


void LogForm::OnSystrayMessage( const QString& msg )
{
    emit SystrayMessage( msg );
}


// vim: ts=4 sw=4 noet
