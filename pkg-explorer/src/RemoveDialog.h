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

#include "include_qt4.h"
#include "ui_RemoveDialog.h"
#include "RemoveThread.h"
#include "Manager.h"

#include <memory>

class RemoveDialog : public QDialog, private Ui::RemoveDialog
{
    Q_OBJECT
    
public:
    RemoveDialog( QWidget *p, Manager::pointer_t manager );
    ~RemoveDialog();

	void SetPackagesToRemove( const QStringList& list );

signals:
    void ShowProcessDialog( bool show_it, bool enable_cancel );

private slots:
	void OnSelectionChanged( const QItemSelection &, const QItemSelection& );
    void on_f_buttonBox_clicked(QAbstractButton *button);
    void on_f_forceAllCB_clicked();
	void on_f_forceDepsCB_clicked();
	void on_f_forceRemoveEssentialCB_clicked();
	void on_f_recursiveCB_clicked();
    void OnRemoveComplete();
    void on_f_optionsButton_toggled(bool checked);

    void on_f_purgeCB_clicked();

private:
    QStringListModel                f_model;
    QItemSelectionModel             f_selectModel;
    Manager::pointer_t              f_manager;
    std::shared_ptr<RemoveThread>	f_thread;

	void SetSwitches();
	void ChangeAllChecked();
};

// vim: ts=4 sw=4 noet
