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
#include "InstallThread.h"
#include "RepoUtils.h"

#include <libdebpackages/wpkgar_repository.h>

#include <algorithm>

using namespace wpkgar;

InstallDialog::InstallDialog(
            QWidget *p,
            QSharedPointer<wpkgar::wpkgar_manager> manager,
            Mode mode
            )
    : QDialog(p)
    , f_model(this)
    , f_selectModel(static_cast<QAbstractItemModel*>(&f_model))
    , f_manager(manager)
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
    f_treeView->header()->setResizeMode( QHeaderView::ResizeToContents );
    //
    QString column_1;
    wpkgar_repository::package_item_t::package_item_status_t itemStatus = wpkgar_repository::package_item_t::not_installed;
    Qt::CheckState checkState = Qt::Unchecked;
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
            itemStatus = wpkgar_repository::package_item_t::need_upgrade;
            checkState = Qt::Checked;
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

    // Load up tree with packages that can be installed/upgraded
	//
	wpkgar_repository repository( f_manager.data() );
    const wpkgar_repository::wpkgar_package_list_t& list( repository.upgrade_list() );
    size_t _max(list.size());
    for( size_t i = 0; i < _max; ++i )
	{
        if( list[i].get_status() == itemStatus )
		{
            // Yes, I know this isn't exception safe, but these items shouldn't throw at all.
			//
			QList<QStandardItem*> itemList;
            QStandardItem* install_item = new QStandardItem;
			install_item->setCheckable( true );
            install_item->setCheckState( checkState );
            install_item->setData( list[i].get_name().c_str() );
			itemList << install_item;
            itemList << new QStandardItem( QIcon(":/icons/file"), list[i].get_name().c_str() );
			itemList << new QStandardItem( list[i].get_version().c_str() );
			f_model.appendRow( itemList );

            applyBtn->setEnabled( f_mode == UpgradeMode );
        }
	}
}


InstallDialog::~InstallDialog()
{
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
        StartThread();
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


void InstallDialog::StartThread()
{
    wpkg_output::get_output()->reset_error_count();
    f_installer = QSharedPointer<wpkgar_install>( new wpkgar_install(f_manager.data()) );

    // always force the chown/chmod because under Unix that doesn't work well otherwise
	f_installer->set_parameter( wpkgar_install::wpkgar_install_force_file_info, true );

    for( int row(0); row < f_model.rowCount(); ++row )
    {
        const QStandardItem* item = f_model.item( row );
        if( item )
        {
            if( item->checkState() == Qt::Checked )
            {
                const QString filename = item->data().toString();
                f_installer->add_package( filename.toStdString() );
            }
        }
    }

    QSharedPointer<InstallThread> _thread( new InstallThread( this, f_manager.data(), f_installer.data(), InstallThread::ThreadValidateOnly ) );
    ShowProcessDialog( true, true );

    connect
        ( _thread.data(), SIGNAL(finished())
          , this        , SLOT(OnValidateComplete())
        );

    f_thread = _thread.staticCast<QThread>();
    f_thread->start();
}


void InstallDialog::OnValidateComplete()
{
    ShowProcessDialog( false, true );

    if( f_thread.dynamicCast<InstallThread>()->get_state() == InstallThread::ThreadFailed )
    {
        QMessageBox::critical
            ( this
              , tr("Package Validation Error!")
              , tr("One or more packages failed to validate! See log pane for details...")
              , QMessageBox::Ok
            );
            return;
    }

    wpkgar_install::install_info_list_t install_list = f_installer->get_install_list();

    QStringList msg;
    switch( f_mode )
    {
        case InstallMode:
            {
                QStringList explicit_packages;
                QStringList implicit_packages;
                std::for_each( install_list.begin(), install_list.end(),
                    [&](const wpkgar_install::install_info_t& info )
                    {
                        const QString package_name = QString("%1: %2").arg(info.get_name().c_str()).arg(info.get_version().c_str());
                        switch( info.get_install_type() )
                        {
                        case wpkgar_install::install_info_t::install_type_explicit:
                            explicit_packages << package_name;
                            break;
                        case wpkgar_install::install_info_t::install_type_implicit:
                            implicit_packages << package_name;
                            break;
                        default:
                            qDebug() << "this should not happen!!!!";
                            Q_ASSERT(0);
                        }
                    }
                );

                msg << tr("The following requested packages will be installed:\n\n%1").arg(explicit_packages.join(", "));
                if( !implicit_packages.empty() )
                {
                    msg << tr("The following new packages will be installed to satisfy dependencies:\n\n%1").arg(implicit_packages.join(", "));
                }
            }
            break;

        case UpgradeMode:
            {
                QStringList explicit_packages;
                QStringList implicit_packages;
                QStringList upgrading_packages;
                std::for_each( install_list.begin(), install_list.end(),
                    [&](const wpkgar_install::install_info_t& info )
                    {
                        const QString package_name = QString("%1: %2").arg(info.get_name().c_str()).arg(info.get_version().c_str());
                        switch( info.get_install_type() )
                        {
                        case wpkgar_install::install_info_t::install_type_explicit:
                            explicit_packages << package_name;
                            break;
                        case wpkgar_install::install_info_t::install_type_implicit:
                            implicit_packages << package_name;
                            break;
                        default:
                            qDebug() << "this should not happen!!!!";
                            Q_ASSERT(0);
                        }
                        if( info.is_upgrade() )
                        {
                            upgrading_packages << info.get_name().c_str();
                        }
                    }
                );

                msg << tr("The following packages will be installed:\n\n%1").arg(QString(explicit_packages.join(", ")));
                if( !implicit_packages.empty() )
                {
                    msg << tr("The following new packages will be installed to satisfy dependencies:\n\n%1").arg(implicit_packages.join(", "));
                }
                if( !upgrading_packages.empty() )
                {
                    msg << tr("The packages which will be upgraded are:\n\n%1").arg(upgrading_packages.join(", "));
                }
            }
            break;
    }

    msg << tr("\nDo you want to continue?");
    if( QMessageBox::question
            ( this
            , tr("Package Validation")
            , msg.join("\n\n")
            , QMessageBox::Yes
            , QMessageBox::No
            )
            == QMessageBox::Yes )
    {
        QSharedPointer<InstallThread> _thread( new InstallThread( this, f_manager.data(), f_installer.data(), InstallThread::ThreadInstallOnly ) );

        ShowProcessDialog( true, true );

        connect
            ( _thread.data(), SIGNAL(finished())
              , this          , SLOT(OnInstallComplete())
            );

        f_thread = _thread.staticCast<QThread>();
        f_thread->start();
    }
}


void InstallDialog::OnInstallComplete()
{
    ShowProcessDialog( false, true );

    if( f_thread.dynamicCast<InstallThread>()->get_state() == InstallThread::ThreadFailed )
    {
        QMessageBox::critical
            ( this
            , tr("Package Installation Error!")
            , tr("One or more packages failed to install! See log pane for details...")
            , QMessageBox::Ok
            );
    }
    else
    {
        accept();
    }
}


// vim: ts=4 sw=4 et
