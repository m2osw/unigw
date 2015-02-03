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

#include "ImportDialog.h"
#include "InstallThread.h"

using namespace wpkgar;

ImportDialog::ImportDialog( QWidget *p, QSharedPointer<wpkgar_manager> manager )
	: QDialog(p)
    , f_model(this)
    , f_selectModel(static_cast<QAbstractItemModel*>(&f_model))
    , f_manager(manager)
    , f_installer(QSharedPointer<wpkgar_install>(new wpkgar_install(f_manager.data())))
{
    setupUi(this);
    f_listView->setModel( &f_model );
    f_listView->setSelectionModel( &f_selectModel );

    QObject::connect
        ( &f_model, SIGNAL( modelReset  ())
        , this    , SLOT  ( OnModelReset())
        );

    QObject::connect
        ( &f_selectModel, SIGNAL( selectionChanged  ( const QItemSelection&, const QItemSelection& ))
        , this          , SLOT  ( OnSelectionChanged( const QItemSelection&, const QItemSelection& ))
        );

    QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
	Q_ASSERT( applyBtn != NULL );
    applyBtn->setEnabled( false );

    f_optionsFrame->hide();
    f_logFrame->hide();
}


ImportDialog::~ImportDialog()
{
}


void ImportDialog::ShowLogPane( const bool show_pane )
{
    if( show_pane )
	{
		connect
			( this      , SIGNAL(ShowProcessDialog(bool,bool))
			, f_logForm , SLOT  (ShowProcessDialog(bool,bool))
			);
		//
		wpkg_output::output* out = f_logForm->GetLogOutput();
		Q_ASSERT( out );
		wpkg_output::set_output( out );
        out->set_debug_flags( wpkg_output::debug_flags::debug_progress );

        f_logFrame->show();
	}
	else
	{
        f_logFrame->hide();

		wpkg_output::set_output( 0 );
		//
		connect
			( this      , SIGNAL(ShowProcessDialog(bool,bool))
			, f_logForm , SLOT  (ShowProcessDialog(bool,bool))
			);
	}
}


void ImportDialog::AddPackages( const QStringList& package_list, const bool clear )
{
    // Append to the end of the model list of packages.
    //
    QStringList contents( clear? QStringList(): f_model.stringList() );
    f_model.setStringList( contents + package_list );

    QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
    Q_ASSERT( applyBtn != NULL );
    applyBtn->setEnabled( !(contents + package_list).empty() );
}


void ImportDialog::on_f_addButton_clicked()
{
    //QStringList contents = f_model.stringList();
	QSettings settings;
	QFileDialog importDlg( this
						, tr("Select one or more WPKG files to import.")
						);
    importDlg.restoreState( settings.value( "import_add_dialog" ).toByteArray() );
	importDlg.setFileMode( QFileDialog::ExistingFiles );
	importDlg.setNameFilter( tr("WPKG Files (*.deb)") );

    if( importDlg.exec() )
    {
        AddPackages( importDlg.selectedFiles() );
	}

	settings.setValue( "import_add_dialog", importDlg.saveState() );
}


void ImportDialog::on_f_removeButton_clicked()
{
	QModelIndexList selrows = f_selectModel.selectedRows();
	if( selrows.size() > 0 )
	{
		QStringList contents = f_model.stringList();
		foreach( QModelIndex index, selrows )
        {
            const QVariant _data = f_model.data( index, Qt::EditRole );
            contents.removeOne( _data.toString() );
		}
		f_model.setStringList( contents );

		if( contents.empty() )
		{
            QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
			Q_ASSERT( applyBtn != NULL );
			applyBtn->setEnabled( false );
		}
	}
	else
	{
        f_removeButton->setEnabled( false );
	}
}


void ImportDialog::OnSelectionChanged( const QItemSelection &/*selected*/, const QItemSelection& /*deselected*/ )
{
	QModelIndexList selrows = f_selectModel.selectedRows();
    f_removeButton->setEnabled( selrows.size() > 0 );
}


void ImportDialog::OnModelReset()
{
    const bool model_empty = f_model.stringList().empty();
    QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
    Q_ASSERT( applyBtn != NULL );
    applyBtn->setEnabled( !model_empty );
    f_removeButton->setEnabled( false );
}


void ImportDialog::on_f_forceAllCB_clicked()
{
    const bool all_checked( f_forceAllCB->checkState() == Qt::Checked );
	QList<QCheckBox*> cb_list;
    cb_list << f_forceArchCB;
    cb_list << f_forceBreaksCB;
    cb_list << f_forceConfCB;
    cb_list << f_forceConflictsCB;
    cb_list << f_forceDepVerCB;
    cb_list << f_forceDependsCB;
    cb_list << f_forceDowngradeCB;
    cb_list << f_forceFileInfoCB;
    cb_list << f_forceOverwriteCB;
    cb_list << f_forceOverwriteDirCB;

	foreach( QCheckBox* cb, cb_list )
	{
		cb->setCheckState( all_checked? Qt::Checked: Qt::Unchecked );
	}
}


void ImportDialog::ChangeAllChecked()
{
	QList<QCheckBox*> cb_list;
    cb_list << f_forceArchCB;
    cb_list << f_forceBreaksCB;
    cb_list << f_forceConfCB;
    cb_list << f_forceConflictsCB;
    cb_list << f_forceDepVerCB;
    cb_list << f_forceDependsCB;
    cb_list << f_forceDowngradeCB;
    cb_list << f_forceFileInfoCB;
    cb_list << f_forceOverwriteCB;
    cb_list << f_forceOverwriteDirCB;

	bool all_checked = true;
	foreach( QCheckBox* cb, cb_list )
	{
		if( cb->checkState() == Qt::Unchecked )
		{
			all_checked = false;
			break;
		}
	}

    f_forceAllCB->setCheckState( all_checked? Qt::Checked: Qt::Unchecked );
}


void ImportDialog::on_f_forceArchCB_clicked()
{
	ChangeAllChecked();
}

void ImportDialog::on_f_forceBreaksCB_clicked()
{
	ChangeAllChecked();
}

void ImportDialog::on_f_forceConfCB_clicked()
{
	ChangeAllChecked();
}

void ImportDialog::on_f_forceConflictsCB_clicked()
{
	ChangeAllChecked();
}

void ImportDialog::on_f_forceDepVerCB_clicked()
{
	ChangeAllChecked();
}

void ImportDialog::on_f_forceDependsCB_clicked()
{
	ChangeAllChecked();
}

//void ImportDialog::on_f_forceDistCB_clicked()
//{
//	ChangeAllChecked();
//}

void ImportDialog::on_f_forceDowngradeCB_clicked()
{
	ChangeAllChecked();
}

void ImportDialog::on_f_forceFileInfoCB_clicked()
{
	ChangeAllChecked();
}

void ImportDialog::on_f_forceOverwriteCB_clicked()
{
	ChangeAllChecked();
}

void ImportDialog::on_f_forceOverwriteDirCB_clicked()
{
	ChangeAllChecked();
}


void ImportDialog::SetSwitches()
{
	QMap<wpkgar_install::parameter_t,QCheckBox*> cb_map;
    cb_map[wpkgar_install::wpkgar_install_force_architecture]     = f_forceArchCB;
    cb_map[wpkgar_install::wpkgar_install_force_breaks]           = f_forceBreaksCB;
    cb_map[wpkgar_install::wpkgar_install_force_configure_any]    = f_forceConfCB;
    cb_map[wpkgar_install::wpkgar_install_force_conflicts]        = f_forceConflictsCB;
    cb_map[wpkgar_install::wpkgar_install_force_depends]          = f_forceDependsCB;
    cb_map[wpkgar_install::wpkgar_install_force_downgrade]        = f_forceDowngradeCB;
    cb_map[wpkgar_install::wpkgar_install_force_file_info]        = f_forceFileInfoCB;
    cb_map[wpkgar_install::wpkgar_install_force_overwrite]        = f_forceOverwriteCB;
    cb_map[wpkgar_install::wpkgar_install_force_overwrite_dir]    = f_forceOverwriteDirCB;
    cb_map[wpkgar_install::wpkgar_install_force_depends_version]  = f_forceDepVerCB;

	foreach( wpkgar_install::parameter_t key, cb_map.keys() )
	{
		f_installer->set_parameter
			( key
			, cb_map[key]->checkState() == Qt::Checked
			);
	}

	f_installer->set_parameter( wpkgar_install::wpkgar_install_skip_same_version,
        f_skipSameVersCB->checkState() == Qt::Checked );
}


void ImportDialog::on_f_buttonBox_clicked(QAbstractButton *button)
{
    QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
	Q_ASSERT( applyBtn );
    QPushButton* closeBtn = f_buttonBox->button( QDialogButtonBox::Discard );
	Q_ASSERT( closeBtn );
	//
    if( button == applyBtn )
    {
        ShowProcessDialog( true, true );
        SetSwitches();

		QMap<QString,int> folders;
		const QStringList contents = f_model.stringList();
		foreach( QString file, contents )
		{
			f_installer->add_package( file.toStdString() );
			QFileInfo info(file);
			folders[info.path()]++;
		}

        f_thread = QSharedPointer<QThread>( static_cast<QThread*>( new InstallThread( this, f_manager.data(), f_installer.data(), InstallThread::ThreadFullInstall ) ) );
		f_thread->start();

		connect
			( f_thread.data(), SIGNAL(finished())
			  , this           , SLOT(OnInstallComplete())
			);
    }
    else if( button == closeBtn )
    {
        reject();
    }
}


void ImportDialog::OnInstallComplete()
{
    if( dynamic_cast<InstallThread*>(f_thread.data())->get_state() == InstallThread::ThreadFailed )
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
        QMessageBox::information
            ( this
              , tr("Package Installation Succeeded!")
              , tr("Your package(s) install successfully!" )
              , QMessageBox::Ok
            );
        accept();
    }

    ShowProcessDialog( false, true );
}


void ImportDialog::on_f_optionsButton_toggled(bool checked)
{
    f_optionsButton->setText(
                checked? tr(">> &Options")
                       : tr("<< &Options")
                         );
    f_optionsFrame->setShown( checked );
}


// vim: ts=4 sw=4 noet
