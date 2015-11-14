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

#include "PrefsDialog.h"
#include "database.h"

#include <QtWidgets>

#include <libdebpackages/wpkgar.h>

PrefsDialog::PrefsDialog(QWidget *p)
    : QDialog(p)
{
    setupUi(this);

    QSettings settings;
    const QString root_path = settings.value( "root_path" ).toString();
	f_repositoryRootLineEdit->setText( root_path );

	const int size_of_array = settings.beginReadArray( "root_path_mru" );
	for( int idx = 0; idx < size_of_array; ++idx )
	{
		settings.setArrayIndex( idx );
        f_mruList << settings.value( "path" ).toString();
    }
    settings.endArray();
}


PrefsDialog::~PrefsDialog()
{
    QSettings settings;
	settings.beginWriteArray( "root_path_mru" );
	int idx = 0;
    for( auto path : f_mruList )
	{
        settings.setArrayIndex( idx );
		settings.setValue( "path", path );
        idx++;
    }
    settings.endArray();
}


void PrefsDialog::accept()
{
    const QString root_path = f_repositoryRootLineEdit->text();
    QSettings settings;
    settings.setValue( "root_path", root_path );
    Database::InitDatabase();
    QDialog::accept();
}


void PrefsDialog::on_f_repositoryBrowseBtn_clicked()
{
    const QString current_root_path = f_repositoryRootLineEdit->text();
    const QString root_path = QFileDialog::getExistingDirectory( this
							, tr("Select WPKG Database Root")
                            , current_root_path
							, QFileDialog::ShowDirsOnly
							);
    if( !root_path.isEmpty()        )
    {
		f_repositoryRootLineEdit->setText( root_path );
	}
}

void PrefsDialog::on_f_repositoryHistoryBtn_clicked()
{
    QList<QAction*> actionList;

    const QString current_root_path = f_repositoryRootLineEdit->text();

    f_mruMenu = QSharedPointer<QMenu>( new QMenu( this ) );
    for( auto path : f_mruList )
	{
        if( path == current_root_path )
        {
            continue;
        }
        actionList << f_mruMenu->addAction( path );
	}

    if( actionList.size() == 0 )
    {
        // Don't display the menu if there is no history (with the current MRU removed)
        return;
    }

    connect( f_mruMenu.data(), &QMenu::triggered, this, &PrefsDialog::OnMruTrigged );

    f_mruMenu->popup( QWidget::mapToGlobal( f_repositoryHistoryBtn->pos() ), actionList[0] );
}

void PrefsDialog::SetLastRootPath( const QString& root_path )
{
    for( auto path : f_mruList )
    {
        if( path == root_path )
        {
            auto iter = std::find_if( f_mruList.begin(), f_mruList.end(), [&root_path]( const QString& p )
            {
                return p == root_path;
            });
            if( iter != f_mruList.end() )
            {
                f_mruList.erase( iter );
            }
            break;
        }
    }

    // Make sure the most recently used path is at the top.
    //
    f_mruList.push_front( root_path );
}

void PrefsDialog::OnMruTrigged( QAction* act )
{
    f_repositoryRootLineEdit->setText( act->text() );
    SetLastRootPath( act->text() );
}

void PrefsDialog::on_f_buttonBox_clicked(QAbstractButton *button)
{
    QAbstractButton* defaultsBtn =
		static_cast<QAbstractButton*>( f_buttonBox->button( QDialogButtonBox::RestoreDefaults ) );
	if( button == defaultsBtn )
	{
        f_repositoryRootLineEdit->setText( Database::GetDefaultDbRoot() );
        SetLastRootPath( Database::GetDefaultDbRoot() );
    }
}

void PrefsDialog::on_f_repositoryRootLineEdit_editingFinished()
{
    SetLastRootPath( f_repositoryRootLineEdit->text() );
}

// vim: ts=4 sw=4 noet
