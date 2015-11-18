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
#include "ui_SourcesDialog.h"
#include "MainWindow.h"

#include <libdebpackages/wpkgar.h>

class SourcesDialog : public QDialog, private Ui::SourcesDialog
{
    Q_OBJECT
    
public:
    SourcesDialog( QWidget *p = 0 );
    ~SourcesDialog();

    void SetManager( Manager::pointer_t mgr );
 
private:
    QStringListModel     f_model;
    QItemSelectionModel  f_selectModel;
    Manager::pointer_t   f_manager;

private slots:
    void OnSelectionChanged( const QItemSelection&, const QItemSelection& );
    void on_f_addButton_clicked();
    void on_f_removeButton_clicked();
    void on_f_buttonBox_clicked(QAbstractButton *button);
    void on_f_listView_doubleClicked(const QModelIndex &index);
};

