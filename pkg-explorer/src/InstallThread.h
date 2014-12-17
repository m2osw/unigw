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
#include <libdebpackages/wpkgar_install.h>

class InstallThread : public QThread
{
public:
	typedef enum { ThreadStopped, ThreadRunning, ThreadFailed, ThreadSucceeded } State;
	typedef enum { ThreadValidateOnly, ThreadInstallOnly, ThreadFullInstall } Mode;

    InstallThread( QObject* p, wpkgar::wpkgar_manager* manager, wpkgar::wpkgar_install* installer, const Mode mode = ThreadFullInstall );

    virtual void run();

	State get_state() const;


private:
    wpkgar::wpkgar_manager* f_manager;
    wpkgar::wpkgar_install* f_installer;
    State                   f_state;
	Mode					f_mode;
    mutable QMutex        	f_mutex;

	bool Validate();
	bool Preconfigure();
	void InstallFiles();

	void set_state( const State new_state );
};


// vim: ts=4 sw=4 noet
