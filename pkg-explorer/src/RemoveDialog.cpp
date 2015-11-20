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

#include "RemoveDialog.h"

#include <libdebpackages/wpkgar_remove.h>

#include <QtWidgets>

using namespace wpkgar;

RemoveDialog::RemoveDialog( QWidget *p )
    : QDialog(p)
    , f_model(this)
    , f_selectModel(static_cast<QAbstractItemModel*>(&f_model))
{
    setupUi(this);
    f_listView->setModel( &f_model );
	f_listView->setSelectionModel( &f_selectModel );

    QObject::connect
        ( &f_selectModel, SIGNAL( selectionChanged  ( const QItemSelection&, const QItemSelection& ))
        , this          , SLOT  ( OnSelectionChanged( const QItemSelection&, const QItemSelection& ))
        );

    QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
	Q_ASSERT( applyBtn != NULL );
    applyBtn->setEnabled( true );

    f_optionsFrame->hide();
}


RemoveDialog::~RemoveDialog()
{
}


void RemoveDialog::SetPackagesToRemove( const QStringList& list )
{
	f_model.setStringList( list );
}


void RemoveDialog::OnSelectionChanged( const QItemSelection &/*selected*/, const QItemSelection& /*deselected*/ )
{
    QModelIndexList selrows = f_selectModel.selectedRows();
    //
    //QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
    //Q_ASSERT( applyBtn != NULL );
    //applyBtn->setEnabled( false );
}


void RemoveDialog::on_f_forceAllCB_clicked()
{
	const bool all_checked( f_forceAllCB->checkState() == Qt::Checked );
	QList<QCheckBox*> cb_list;
    cb_list << f_forceDepsCB;
    cb_list << f_forceHoldCB;
    cb_list << f_forceRemoveEssentialCB;

	foreach( QCheckBox* cb, cb_list )
	{
		cb->setCheckState( all_checked ? Qt::Checked : Qt::Unchecked );
	}
}


void RemoveDialog::ChangeAllChecked()
{
	QList<QCheckBox*> cb_list;
	cb_list << f_forceDepsCB;
	cb_list << f_forceHoldCB;
	cb_list << f_forceRemoveEssentialCB;

	bool all_checked = true;
	foreach( QCheckBox* cb, cb_list )
	{
		if( cb->checkState() == Qt::Unchecked )
		{
			all_checked = false;
			break;
		}
	}

	f_forceAllCB->setCheckState( all_checked ? Qt::Checked : Qt::Unchecked );
}


void RemoveDialog::on_f_forceDepsCB_clicked()
{
	ChangeAllChecked();
}


void RemoveDialog::on_f_forceRemoveEssentialCB_clicked()
{
	ChangeAllChecked();
}


void RemoveDialog::on_f_purgeCB_clicked()
{
    ChangeAllChecked();
}


void RemoveDialog::on_f_recursiveCB_clicked()
{
	ChangeAllChecked();
}


void RemoveDialog::SetSwitches()
{
	QMap<wpkgar_remove::parameter_t,QCheckBox*> cb_map;
	cb_map[wpkgar_remove::wpkgar_remove_force_depends]           = f_forceDepsCB;
	cb_map[wpkgar_remove::wpkgar_remove_force_hold]              = f_forceHoldCB;
	cb_map[wpkgar_remove::wpkgar_remove_force_remove_essentials] = f_forceRemoveEssentialCB;
	cb_map[wpkgar_remove::wpkgar_remove_recursive]               = f_recursiveCB;

    auto remover( Manager::Instance()->GetRemover().lock() );

	foreach( wpkgar_remove::parameter_t key, cb_map.keys() )
	{
        remover->set_parameter
			( key
			, cb_map[key]->checkState() == Qt::Checked
			);
    }

    if( f_purgeCB->checkState() == Qt::Checked )
    {
        remover->set_purging();
    }
    //else -- you don't want that for --remove or --purge, only --deconfigure which we do not support yet
    //{
    //    remover->set_deconfiguring();
    //}
}


void RemoveDialog::on_f_buttonBox_clicked(QAbstractButton *button)
{
    QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
	Q_ASSERT( applyBtn );
    QPushButton* closeBtn = f_buttonBox->button( QDialogButtonBox::Discard );
	Q_ASSERT( closeBtn );
	//
    if( button == applyBtn )
	{
        ShowProcessDialog( true, false /*cancel_button*/ );
        SetSwitches();

        auto remover( Manager::Instance()->GetRemover().lock() );

        QMap<QString,int> folders;
        const QStringList contents = f_model.stringList();
		foreach( QString file, contents )
		{
            remover->add_package( file.toStdString() );
        }

        if( remover->validate() )
		{
            f_thread.reset( new RemoveThread( this ) );
			f_thread->start();

			connect
				( f_thread.get(), SIGNAL(finished())
				, this          , SLOT(OnRemoveComplete())
				);
		}
		else
		{
			QMessageBox::critical
				( this
				  , tr("Package Validation Error!")
				  , tr("One or more packages failed to validate for removal! See log pane for details...")
				  , QMessageBox::Ok
				);
            ShowProcessDialog( false, true );
            reject();
        }
    }
    else if( button == closeBtn )
    {
        reject();
    }
}


void RemoveDialog::OnRemoveComplete()
{
	if( f_thread->get_state() == RemoveThread::ThreadFailed )
	{
		QMessageBox::critical
			( this
			, tr("Package Removal Error!")
			, tr("One or more packages failed to remove! See log pane for details...")
			, QMessageBox::Ok
			);
	}

    ShowProcessDialog( false, true );
	accept();
}


void RemoveDialog::on_f_optionsButton_toggled(bool checked)
{
    f_optionsButton->setText(
                checked? tr(">> &Options")
                       : tr("<< &Options")
                         );
    f_optionsFrame->setVisible( checked );
}


// vim: ts=4 sw=4 noet
