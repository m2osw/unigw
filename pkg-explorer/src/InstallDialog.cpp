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

#include "InstallDialog.h"
#include "RepoUtils.h"

#include <libdebpackages/wpkgar_repository.h>

#include <algorithm>

#include <QtWidgets>
#include <QtGui>

using namespace wpkgar;

InstallDialog::InstallDialog(
            QWidget *p,
            Mode mode
            )
    : QDialog(p)
    , f_model(this)
    , f_selectModel(static_cast<QAbstractItemModel*>(&f_model))
    , f_mode(mode)
{
    setupUi(this);

    connect
        ( &f_model, SIGNAL( itemChanged  ( QStandardItem* ))
        , this    , SLOT  ( OnItemChanged( QStandardItem* ))
        );

    QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
	Q_ASSERT( applyBtn != NULL );
    applyBtn->setEnabled( false );

	// Initialize tree view
	//
    f_treeView->setModel( static_cast<QAbstractItemModel*>(&f_model) );
    f_treeView->setSelectionModel( &f_selectModel );
    f_treeView->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

    QString column_1;
    switch( f_mode )
    {
        case InstallMode:
            column_1 = tr("Install");
            f_label->setText( tr(
                                  "The following packages are available to be installed from the package sources. "
                                  "Check one or more packages to install, and then click on the \"Apply\" button to "
                                  "install them."
                                 ));
            break;
        case UpgradeMode:
            column_1 = tr("Upgrade");
            f_label->setText( tr(
                                  "The following packages are available to be upgraded from the package sources. "
                                  "Uncheck those packages you do not wish to upgrade, and then click on the \"Apply\" button to "
                                  "install the updates."
                                 ));
            break;
        default:
            qFatal("Undefined value at InstallDialog()!");
    };

	QStringList column_labels;
    column_labels << column_1 << tr("Package Name") << tr("Version");
    f_model.setHorizontalHeaderLabels( column_labels );

    PopulateTree();

    f_searchBox->setFocus( Qt::OtherFocusReason );
}


InstallDialog::~InstallDialog()
{
}


void InstallDialog::PopulateTree( const QString& filterText )
{
    f_model.removeRows( 0, f_model.rowCount() );

    wpkgar_repository::package_item_t::package_item_status_t itemStatus = wpkgar_repository::package_item_t::not_installed;
    Qt::CheckState checkState = Qt::Unchecked;
    if( f_mode == UpgradeMode )
    {
        itemStatus = wpkgar_repository::package_item_t::need_upgrade;
        checkState = Qt::Checked;
    };

    // Load up tree with packages that can be installed/upgraded
    //
    auto manager( Manager::Instance()->GetManager().lock() );
    wpkgar_repository repository( manager.get() );
    const wpkgar_repository::wpkgar_package_list_t& list( repository.upgrade_list() );
    size_t _max(list.size());
    for( size_t i = 0; i < _max; ++i )
    {
        if( list[i].get_status() == itemStatus )
        {
            // Yes, I know this isn't exception safe, but these items shouldn't throw at all.
            //
            const QString name( list[i].get_name().c_str() );
            bool add_item(true);
            //
            if( !filterText.isEmpty() )
            {
                add_item = name.left( filterText.size() ) == filterText;
            }
            //
            if( add_item )
            {
                QList<QStandardItem*> itemList;
                QStandardItem* install_item = new QStandardItem;
                install_item->setCheckable( true );
                install_item->setCheckState( checkState );
                install_item->setData( name );
                itemList << install_item;
                itemList << new QStandardItem( QIcon(":/icons/file"), name );
                itemList << new QStandardItem( list[i].get_version().c_str() );
                f_model.appendRow( itemList );

                f_buttonBox->button( QDialogButtonBox::Apply )->setEnabled( f_mode == UpgradeMode );
            }
        }
    }
}


void InstallDialog::OnItemChanged( QStandardItem* _item )
{
    QModelIndexList selrows = f_selectModel.selectedRows();
    for( int row(0); row < selrows.size(); ++row )
    {
        QStandardItem* item = f_model.itemFromIndex( selrows[row] );
        if( item )
        {
            item->setCheckState( _item->checkState() );
        }
    }

    QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
	Q_ASSERT( applyBtn );
	applyBtn->setEnabled( false );
	for( int row(0); row < f_model.rowCount(); ++row )
	{
		QStandardItem* item = f_model.item( row );
		if( item->checkState() == Qt::Checked )
		{
			applyBtn->setEnabled( true );
			break;
		}
	}
}


void InstallDialog::on_f_treeView_pressed(const QModelIndex &/*index*/)
{
	f_selectModel.clearSelection();
}


void InstallDialog::on_f_buttonBox_clicked(QAbstractButton *button)
{
    QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
    Q_ASSERT( applyBtn );
    QPushButton* closeBtn = f_buttonBox->button( QDialogButtonBox::Discard );
    Q_ASSERT( closeBtn );
    QPushButton* resetBtn = f_buttonBox->button( QDialogButtonBox::Reset );
    Q_ASSERT( resetBtn );
    //
    if( button == applyBtn )
    {
        accept();
    }
    else if( button == closeBtn )
    {
        reject();
    }
	else if( button == resetBtn )
	{
		// Reset all of the checks
		//
        for( int row(0); row < f_model.rowCount(); ++row )
		{
			QStandardItem* item = f_model.item( row );
			item->setCheckState( Qt::Unchecked );
		}
		//
		applyBtn->setEnabled( false );
	}
}


void InstallDialog::GetPackageList( QStringList& package_list ) const
{
    for( int row(0); row < f_model.rowCount(); ++row )
    {
        const QStandardItem* item = f_model.item( row );
        if( item )
        {
            if( item->checkState() == Qt::Checked )
            {
                const QString filename = item->data().toString();
                package_list << filename;
            }
        }
    }
}


void InstallDialog::on_f_searchBox_textEdited(const QString &arg1)
{
    PopulateTree( arg1 );
}


// vim: ts=4 sw=4 et
