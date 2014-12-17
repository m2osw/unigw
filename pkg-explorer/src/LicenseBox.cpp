//===============================================================================
// Package Explorer -- Package Explorer License
// Copyright (C) 2011  Made to Order Software Corp.
// All Rights Reserved.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
//===============================================================================

#include "LicenseBox.h"
#include "include_qt4.h"

LicenseBox::LicenseBox(QWidget *parent_widget)
	: QDialog(parent_widget)
{
	setWindowModality(Qt::ApplicationModal);
	setupUi(this);

	QObject::connect
        ( copyrightNoice, SIGNAL( linkActivated  ( const QString& ))
        , this          , SLOT  ( onLinkActivated( const QString& ))
		);
}

void LicenseBox::onLinkActivated( const QString& webpage_url )
{
	//LogInfo( QString("Browse to %1").arg(webpage_url) );
	QDesktopServices::openUrl(webpage_url);
}

// vim: ts=4 sw=4 noet
