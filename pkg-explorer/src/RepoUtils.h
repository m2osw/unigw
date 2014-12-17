//===============================================================================
// Copyright:   Copyright (c) 2013 Made to Order Software Corp.
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

#include <libdebpackages/wpkgar.h>
#include <libdebpackages/wpkgar_repository.h>

namespace RepoUtils
{
    QString     SourceToQString( const wpkgar::wpkgar_repository::source& src, const bool uri_only = false );
    wpkgar::wpkgar_repository::source
                QStringToSource( const QString& str );
    QStringList ReadSourcesList( wpkgar::wpkgar_manager* manager, const bool uri_only = false );
    void        WriteSourcesList( wpkgar::wpkgar_manager* manager, const QStringList& contents );
}
// namespace


// vim: ts=4 sw=4 et

