//===============================================================================
// Copyright:	Copyright (c) 2015 Made to Order Software Corp.
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
#include "Manager.h"

#include <memory>

class InitThread : public QThread
{
public:
    InitThread( QObject* p, const bool show_installed_only );
    ~InitThread();

	typedef QList<QString>				ItemList;
	typedef QList<ItemList>				PackageList;
	typedef QMap<QString,PackageList>	SectionMap;
    SectionMap GetSectionMap() const;

    virtual void run();

private:
    SectionMap               f_sectionMap;
	bool                     f_showInstalledOnly;
    Manager::pointer_t       f_manager;
    mutable QMutex           f_mutex;
};


// vim: ts=4 sw=4 noet
