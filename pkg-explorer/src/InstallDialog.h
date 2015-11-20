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
#include "Manager.h"

#include <libdebpackages/wpkgar_install.h>

#include "ui_InstallDialog.h"

class InstallDialog : public QDialog, private Ui::InstallDialog
{
    Q_OBJECT
    
public:
    typedef enum { InstallMode, UpgradeMode } Mode;
    InstallDialog(
            QWidget *p,
            Mode mode = InstallMode
            );
    ~InstallDialog();

    void GetPackageList( QStringList& package_list ) const;
    Mode GetMode() const { return f_mode; }

private:
    QStandardItemModel                      f_model;
    QItemSelectionModel                     f_selectModel;
    Mode                                    f_mode;

    void PopulateTree( const QString& filterText = QString() );

private slots:
    void OnItemChanged( QStandardItem* );
    void on_f_treeView_pressed(const QModelIndex &index);
    void on_f_buttonBox_clicked(QAbstractButton *button);
    void on_f_searchBox_textEdited(const QString &arg1);
};


// vim: ts=4 sw=4 et
