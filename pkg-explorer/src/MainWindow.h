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
#include <libdebpackages/wpkgar.h>
#include <libdebpackages/wpkgar_install.h>

#include "InstallDialog.h"
#include "InstallThread.h"
#include "LicenseBox.h"
#include "ProcessDialog.h"

#include "ui_MainWindow.h"

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT
    
public:
    MainWindow( const bool showSysTray = true );
    ~MainWindow();

    void SetInstallPackages( const QStringList& list );
    void SetDoUpgrade( const bool val = true );
    void RunCommand( const QString& command );

protected:
    void showEvent ( QShowEvent * event );
    void hideEvent ( QHideEvent * event );
    void closeEvent( QCloseEvent* event );
    
private:
    QStandardItemModel						f_packageModel;
    QItemSelectionModel						f_selectModel;
    QSharedPointer<wpkgar::wpkgar_manager>	f_manager;
    QSharedPointer<wpkgar::wpkgar_install>  f_installer;
    QSharedPointer<wpkgar::wpkgar_lock>		f_lock;
    QSharedPointer<QThread>                 f_thread;
	QScopedPointer<LicenseBox>				f_license_box;
    bool									f_showInstalledPackagesOnly;
	QSharedPointer<QSystemTrayIcon>			f_sysTray;
    QStringList                             f_immediateInstall;
    InstallDialog::Mode                     f_installMode;
    ProcessDialog                           f_procDlg;
    QSharedPointer<LogOutput>               f_logOutput;
    bool                                    f_doUpgrade;

    typedef QMap<wpkg_output::level_t,QAction*> level_to_action_t;
	level_to_action_t	f_levelToAction;

	typedef QList<QAction*> ActionList;
	ActionList f_actionList;

	void LoadSettings();
	void SaveSettings();

	void OutputToLog( wpkg_output::level_t level, const QString& msg );
	void LogDebug   ( const QString& msg );
	void LogInfo    ( const QString& msg );
	void LogWarning ( const QString& msg );
	void LogError   ( const QString& msg );
	void LogFatal   ( const QString& msg );

    void HandleFailure();

    void InitManager();
    void RefreshListing();
    void SelectFromModel( const QString& package_name );
	void UpdateActions();

    void ResetLogChecks( QAction* except );

    void StartInstallThread( const QStringList& packages_list );

public slots:
    void OnShowProcessDialog( const bool show_it, const bool enable_cancel );

private slots:
	void OnAboutToQuit();
	void OnInitTimer();
    void OnStackStatus( bool, bool );
	void OnHistoryChanged( const QString& );
    void OnPackageClicked( const QString& );
	void OnWebPageClicked( const QString& );
    void OnSelectionChanged( const QItemSelection &selected, const QItemSelection& deselected );
	void OnRefreshListing();
	void OnUpdateFinished();
    void OnSystrayMessage( const QString& );
    void OnInstallValidateComplete();
    void OnInstallComplete();
    void on_actionFileImport_triggered();
    void on_actionRemove_triggered();
    void on_actionDatabaseRoot_triggered();
    void on_actionInstall_triggered();
    void on_actionReload_triggered();
    void on_actionHistoryBack_triggered();
    void on_actionHistoryForward_triggered();
    void on_actionUpgrade_triggered();
    void on_actionManageRepositories_triggered();
	void on_actionHelp_triggered();
	void on_actionPackageExplorerLicense_triggered();
    void on_actionAbout_triggered();
    void on_actionAboutWindowsPackager_triggered();
	void on_actionClearLog_triggered();
    void on_actionUpdate_triggered();
    void on_actionShowInstalled_triggered();
    void on_actionViewLogDebug_triggered();
    void on_actionViewLogInfo_triggered();
    void on_actionViewLogWarning_triggered();
    void on_actionViewLogError_triggered();
};

// vim: ts=4 sw=4 noet
