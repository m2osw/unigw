//===============================================================================
// Copyright:	Copyright (c) 2013-2015 Made to Order Software Corp.
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

#include <QtGui>
#include <libdebpackages/wpkgar.h>

class LogOutput
        : public QObject
        , public wpkg_output::output
{
	Q_OBJECT

public:
    LogOutput();

    virtual void           log_message( const wpkg_output::message_t& msg ) const;
    wpkg_output::level_t   get_level() const;
	bool                   pending_messages() const;
	wpkg_output::message_t pop_next_message();
    void                   set_level( wpkg_output::level_t level );
	void                   clear();

signals:
	void                   AddProcessMessage( const QString& msg ) const;

private:
    wpkg_output::level_t	                    f_logLevel;
	//
    mutable QMutex								f_mutex;
    mutable QVector<wpkg_output::message_t>		f_messageFifo;
};

// vim: ts=4 sw=4 noet
