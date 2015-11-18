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

#include "include_qt4.h"
#include "LogOutput.h"

#include <libdebpackages/wpkgar.h>
#include <libdebpackages/wpkgar_install.h>

#include <memory>

class Manager
{
public:
    typedef std::weak_ptr<Manager> pointer_t;

    Manager( std::weak_ptr<LogOutput> log );

    std::weak_ptr<wpkgar::wpkgar_manager>   GetManager() const;
    std::weak_ptr<wpkgar::wpkgar_install>   GetInstaller() const;
    std::weak_ptr<wpkgar::wpkgar_remove>    GetRemover() const;

private:
    void Init( std::weak_ptr<wpkgar::wpkgar_lock> log );
    bool CreateLock();

    mutable QMutex        					f_mutex;
    std::shared_ptr<wpkgar::wpkgar_lock>	f_lock;
    std::shared_ptr<wpkgar::wpkgar_manager>	f_manager;
    std::shared_ptr<wpkgar::wpkgar_install> f_installer;
    std::shared_ptr<wpkgar::wpkgar_remove>  f_remover;
};

// vim: ts=4 sw=4 et
