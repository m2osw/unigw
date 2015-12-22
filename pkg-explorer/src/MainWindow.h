//===============================================================================
// Copyright:	Copyright (c) 2013-2015 Made to Order Software Corp.
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

#include "InitThread.h"
#include "InstallDialog.h"
#include "InstallThread.h"
#include "LicenseBox.h"
#include "Manager.h"
#include "ProcessDialog.h"
#include "UpdateThread.h"

#include "ui_MainWindow.h"


class MainWindow
    : public QMainWindow
    , public std::enable_shared_from_this<MainWindow>
    , private Ui::MainWindow
{
    Q_OBJECT
    
public:
    MainWindow( const bool showSysTray = true );
    ~MainWindow();

    void SetInstallPackages( const QStringList& list );
    void SetDoUpgrade( const bool val = true );

    static std::weak_ptr<QSystemTrayIcon> GetSysTray();
    static bool StopClicked();

protected:
    void showEvent ( QShowEvent * event );
    void hideEvent ( QHideEvent * event );
    void closeEvent( QCloseEvent* event );
    
private:
    QStandardItemModel						f_packageModel;
    QItemSelectionModel						f_selectModel;
    std::shared_ptr<InitThread>             f_initThread;
    std::shared_ptr<InstallThread>          f_installThread;
    std::shared_ptr<UpdateThread>           f_updateThread;
	QScopedPointer<LicenseBox>				f_license_box;
    bool									f_showInstalledPackagesOnly;
    QStringList                             f_immediateInstall;
    InstallDialog::Mode                     f_installMode;
    QSharedPointer<ProcessDialog>           f_procDlg;
    std::shared_ptr<LogOutput>              f_logOutput;
    bool                                    f_doUpgrade;
    QLabel                                  f_statusLabel;
    QLabel                                  f_logLabel;
    QProgressBar                            f_progressBar;
    Manager::pointer_t                      f_manager;

    struct message_t
    {
        QString f_message;
        wpkgar::wpkgar_install::progress_record_t f_record;
    };

    QVector<message_t>     	                f_messageFifo;
    QTimer                                  f_timer;

    static QMutex        					f_mutex;
    static std::shared_ptr<QSystemTrayIcon>	f_sysTray;
    static bool                             f_stopClicked;

    typedef QMap<wpkg_output::level_t,QAction*> level_to_action_t;
	level_to_action_t	f_levelToAction;

	typedef QList<QAction*> ActionList;
	ActionList f_actionList;

    void EnableStopButton( const bool enabled = true );
	void LoadSettings();
	void SaveSettings();

    void LogDebug   ( const QString& msg );
	void LogInfo    ( const QString& msg );
	void LogWarning ( const QString& msg );
	void LogError   ( const QString& msg );
	void LogFatal   ( const QString& msg );

    void HandleFailure();

    void UpdateWindowCaption();
    void InitManager();
    void RefreshListing();
    void SelectFromModel( const QString& package_name );
	void UpdateActions();

    void ResetLogChecks( QAction* except );

    void StartInstallThread( const QStringList& packages_list );

    void DisplayPackage( const QString& package_name );

    void OnProgressChange( wpkgar::wpkgar_install::progress_record_t record ); // wpkgar_install

public slots:
    void OnStartImportOperation();
    void OnEndImportOperation();
    void OnStartRemoveOperation();
    void OnEndRemoveOperation();

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
    void OnFsTimeout();
    void OnAddLogMessage( const QString& message );
    void OnDisplayMessages();
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
    void on_actionStop_triggered();
};

// vim: ts=4 sw=4 noet
