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

#include "LogForm.h"
#include "include_qt4.h"
#include "ui_ImportDialog.h"

#include <libdebpackages/wpkgar_install.h>

class ImportDialog
        : public QDialog
        , private Ui::ImportDialog
{
    Q_OBJECT
    
public:
    ImportDialog( QWidget *p, QSharedPointer<wpkgar::wpkgar_manager> manager, LogForm* logForm );
    ~ImportDialog();
    
private slots:
    void on_f_addButton_clicked();
    void on_f_removeButton_clicked();
	void OnSelectionChanged( const QItemSelection &, const QItemSelection& );
    void on_f_buttonBox_clicked(QAbstractButton *button);
    void on_f_forceAllCB_clicked();
	void on_f_forceArchCB_clicked();
	void on_f_forceBreaksCB_clicked();
	void on_f_forceConfCB_clicked();
	void on_f_forceConflictsCB_clicked();
	void on_f_forceDepVerCB_clicked();
	void on_f_forceDependsCB_clicked();
	void on_f_forceDowngradeCB_clicked();
	void on_f_forceFileInfoCB_clicked();
	void on_f_forceOverwriteCB_clicked();
	void on_f_forceOverwriteDirCB_clicked();
    void OnInstallComplete();

    void on_f_optionsButton_toggled(bool checked);

private:
    QStringListModel                      	f_model;
    QItemSelectionModel                   	f_selectModel;
    QSharedPointer<wpkgar::wpkgar_manager>	f_manager;
    QSharedPointer<wpkgar::wpkgar_install>	f_installer;
    QSharedPointer<QThread>               	f_thread;
    LogForm*                              	f_logForm;

	void SetSwitches();
	void ChangeAllChecked();
};

// vim: ts=4 sw=4 noet
