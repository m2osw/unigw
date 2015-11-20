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

/// \class LogOutput
///
/// This class is useful for marshalling incoming messages from the wpkg thread
/// into the GUI thread in a thread-safe, serialized manner.
///

#pragma once 

#include <QtGui>
#include <libdebpackages/wpkgar.h>

class LogOutput
        : public QObject
        , public wpkg_output::output
{
	Q_OBJECT

public:
    typedef std::shared_ptr<LogOutput> pointer_t;

    static pointer_t Instance();
    static void      Release();

    virtual void           log_message( const wpkg_output::message_t& msg ) const;
    wpkg_output::level_t   get_level() const;
	bool                   pending_messages() const;
	wpkg_output::message_t pop_next_message();
    void                   set_level( wpkg_output::level_t level );
	void                   clear();

    void                   OutputToLog( wpkg_output::level_t level, const QString& msg );

signals:
	void                   AddProcessMessage( const QString& msg ) const;

private:
    LogOutput();

    static pointer_t                            f_instance;

    wpkg_output::level_t	                    f_logLevel;
	//
    mutable QMutex								f_mutex;
    mutable QVector<wpkg_output::message_t>		f_messageFifo;
};

// vim: ts=4 sw=4 noet
