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

#include "MainWindow.h"
#include "ImportDialog.h"
#include "RemoveDialog.h"
#include "PrefsDialog.h"
#include "LicenseBox.h"
#include "SourcesDialog.h"

#include <QtWidgets>

#include <libdebpackages/wpkgar.h>
#include <libdebpackages/wpkgar_repository.h>
#include <libdebpackages/debian_packages.h>

#include <algorithm>

#ifdef WIN32
#	undef open
#	undef close
#endif

using namespace wpkgar;

namespace
{
    class ProcessInterrupt : public wpkgar::wpkgar_interrupt
    {
        public:
            virtual bool stop_now()
            {
                return MainWindow::StopClicked();
            }
    };

    ProcessInterrupt interrupt;

    void ResetErrorCount()
    {
        auto output = wpkg_output::get_output().lock();
        if( output )
        {
            output->reset_error_count();
        }
    }
}
//namespace


std::shared_ptr<QSystemTrayIcon>    MainWindow::f_sysTray;
QMutex                              MainWindow::f_mutex;
bool                                MainWindow::f_stopClicked = false;


MainWindow::MainWindow( const bool showSysTray )
	: QMainWindow(0)
	, f_packageModel(this)
	, f_selectModel(&f_packageModel)
    , f_installMode(InstallDialog::InstallMode)
    , f_doUpgrade(false)
    , f_timer(this)
{
    setupUi(this);

    f_logOutput = LogOutput::Instance();
    f_logForm->SetLogOutput( f_logOutput );

    if( QSystemTrayIcon::isSystemTrayAvailable() && showSysTray )
    {
        f_sysTray.reset( new QSystemTrayIcon( this ) );
        //
        QMenu* stMenu = new QMenu( this );
        stMenu->addAction( actionShowApplication );
        stMenu->addSeparator();
        stMenu->addAction( actionFileImport );
        stMenu->addSeparator();
        stMenu->addAction( actionUpdate  );
        stMenu->addAction( actionInstall );
        stMenu->addAction( actionUpgrade );
        stMenu->addSeparator();
        stMenu->addAction( actionQuit );
        //
        f_sysTray->setContextMenu( stMenu );
        f_sysTray->setIcon( QIcon(":/icons/systray_icon") );
        f_sysTray->show();

        //f_sysTray->showMessage( tr("This is a test message"), tr("Look at this awesome message!"), QSystemTrayIcon::Information, 5000 );
    }

	LoadSettings();

    QStringList _headers;
    _headers << "Package Name" << "Status" << "Version";
    f_packageModel.setHorizontalHeaderLabels( _headers );
    f_treeView->setModel( static_cast<QAbstractItemModel*>(&f_packageModel) );
    f_treeView->setSelectionModel( &f_selectModel );
    f_treeView->header()->setSectionResizeMode( QHeaderView::ResizeToContents );

    // Action mapping
    //
    connect( actionQuit, &QAction::triggered,        qApp, &QApplication::quit        );
    connect( qApp,       &QApplication::aboutToQuit, this, &MainWindow::OnAboutToQuit );

	// Listen for selection changes in the view (either via mouse or keyboard)
	//
    QObject::connect
        ( &f_selectModel, &QItemSelectionModel::selectionChanged
        , this          , &MainWindow::OnSelectionChanged
        );

    actionHistoryBack->setEnabled    ( false );
    actionHistoryForward->setEnabled ( false );

    QObject::connect
        ( f_webForm, &WebForm::StackStatus
        , this     , &MainWindow::OnStackStatus
        );
    QObject::connect
        ( f_webForm, &WebForm::HistoryChanged
        , this     , &MainWindow::OnHistoryChanged
        );
    QObject::connect
        ( f_webForm, &WebForm::PackageClicked
        , this     , &MainWindow::OnPackageClicked
        );
    QObject::connect
        ( f_webForm, &WebForm::WebPageClicked
        , this     , &MainWindow::OnWebPageClicked
        );

    QObject::connect
        ( f_logForm, &LogForm::SetSystrayMessage
        , this     , &MainWindow::OnSystrayMessage
        );

	f_actionList
        << actionQuit
        << actionDatabaseRoot
        << actionInstall
        << actionRemove
        << actionReload
        << actionHistoryBack
        << actionHistoryForward
        << actionShowLog
        << actionFileImport
        << actionUpgrade
        << actionManageRepositories
        << actionAbout
        << actionHelp
        << actionUpdate
        << actionShowInstalled
        << actionViewLogDebug
        << actionViewLogInfo
        << actionViewLogWarning
        << actionViewLogError
        << actionClearLog
        << actionAboutWindowsPackager
        << actionPackageExplorerLicense
        << actionMinimizeToSystray
        << actionShowApplication
		;

    connect
        ( f_logOutput.get()
        , &LogOutput::AddProcessMessage
        , this
        , &MainWindow::OnAddLogMessage
        );

    statusBar()->addWidget( &f_logLabel );
    statusBar()->addPermanentWidget( &f_progressBar );
    statusBar()->addPermanentWidget( &f_statusLabel );
    f_statusLabel.setText( "Please wait, initializing..." );

    f_progressBar.setMaximum( 100 );
    f_progressBar.setValue( 0 );
    f_progressBar.hide();

    setWindowTitle( tr("WPKG Package Explorer") );

    connect( &f_timer, &QTimer::timeout, this, &MainWindow::OnDisplayMessages );
    f_timer.start( 100 );

    QTimer::singleShot( 100, this, &MainWindow::OnInitTimer );
}


MainWindow::~MainWindow()
{
    f_sysTray.reset();
    wpkg_output::set_output( 0 );
}


std::weak_ptr<QSystemTrayIcon> MainWindow::GetSysTray()
{
    return f_sysTray;
}


bool MainWindow::StopClicked()
{
    QMutexLocker locker( &f_mutex );
    return f_stopClicked;
}


void MainWindow::EnableStopButton( const bool enabled )
{
    QMutexLocker locker( &f_mutex );
    f_stopClicked = false;

    auto systray( MainWindow::GetSysTray().lock() );
    if( systray )
    {
        systray->setIcon( enabled
            ? QIcon(":/icons/locked_logo")
            : QIcon(":/icons/systray_icon")
            );
    }

    std::for_each( f_actionList.begin(), f_actionList.end(), [enabled]( QAction* act )
    {
        act->setEnabled(!enabled);
    });

    actionStop->setEnabled( enabled );
}


void MainWindow::SetInstallPackages( const QStringList& list )
{
    f_immediateInstall = list;
}


void MainWindow::SetDoUpgrade( const bool val )
{
    f_doUpgrade = val;
}


void MainWindow::LoadSettings()
{
    QSettings settings;
    const bool minimize = settings.value( "minimize_to_systray", false ).toBool();
    actionMinimizeToSystray->setChecked( minimize );

    restoreGeometry( settings.value( "geometry", saveGeometry() ).toByteArray() );
    restoreState   ( settings.value( "state"   , saveState()    ).toByteArray() );
    actionShowInstalled->setChecked( settings.value("show_installed", true).toBool() );
    actionShowLog->setChecked( settings.value("show_log", true).toBool() );

    f_levelToAction[wpkg_output::level_debug]   = actionViewLogDebug;
    f_levelToAction[wpkg_output::level_info]    = actionViewLogInfo;
    f_levelToAction[wpkg_output::level_warning] = actionViewLogWarning;
    f_levelToAction[wpkg_output::level_error]   = actionViewLogError;

    const wpkg_output::level_t level = static_cast<wpkg_output::level_t>(settings.value( "log_level", wpkg_output::level_info ).toInt());
    f_logOutput->set_level( level );
	f_levelToAction[level]->setChecked( true );
}


void MainWindow::SaveSettings()
{
    QSettings settings;
    settings.setValue( "geometry",            saveGeometry()                       );
    settings.setValue( "state",               saveState()                          );
    settings.setValue( "show_installed",      actionShowInstalled->isChecked()     );
    settings.setValue( "show_log",            actionShowLog->isChecked()           );
    settings.setValue( "log_level",           f_logOutput->get_level()             );
    settings.setValue( "minimize_to_systray", actionMinimizeToSystray->isChecked() );
}


void MainWindow::OnStartImportOperation()
{
    EnableStopButton();
}


void MainWindow::OnEndImportOperation()
{
    EnableStopButton( false );
    f_webForm->ClearHistory();
    RefreshListing();
}


void MainWindow::OnStartRemoveOperation()
{
    EnableStopButton();
}


void MainWindow::OnEndRemoveOperation()
{
    EnableStopButton( false );
    f_webForm->ClearHistory();
    RefreshListing();
}


void MainWindow::OnAboutToQuit()
{
	SaveSettings();
}


void MainWindow::OnInitTimer()
{
    InitManager();
}


void MainWindow::closeEvent( QCloseEvent* evt )
{
    if( actionMinimizeToSystray->isChecked() )
    {
        hide();
        evt->ignore();
    }
    else
    {
        QApplication::quit();
        evt->accept();
    }
}


void MainWindow::hideEvent( QHideEvent * /*evt*/ )
{
    actionShowApplication->setText( tr("&Show Application") );
    connect( actionShowApplication, &QAction::triggered, this, &MainWindow::show );
}


void MainWindow::showEvent( QShowEvent * /*evt*/ )
{
    actionShowApplication->setText( tr("&Hide Application") );
    connect( actionShowApplication, &QAction::triggered, this, &MainWindow::hide );
}


void MainWindow::LogDebug( const QString& msg )
{
    f_logOutput->OutputToLog( wpkg_output::level_debug, msg );
}


void MainWindow::LogInfo( const QString& msg )
{
    f_logOutput->OutputToLog( wpkg_output::level_info, msg );
}


void MainWindow::LogWarning( const QString& msg )
{
    f_logOutput->OutputToLog( wpkg_output::level_warning, msg );
}


void MainWindow::LogError( const QString& msg )
{
    f_logOutput->OutputToLog( wpkg_output::level_error, msg );
}


void MainWindow::LogFatal( const QString& msg )
{
    f_logOutput->OutputToLog( wpkg_output::level_fatal, msg );
    QMessageBox::critical
        ( this
          , tr("Application Terminated!")
          , msg
          , QMessageBox::Ok
        );
    qFatal( "%s", msg.toStdString().c_str() );
}


void MainWindow::UpdateWindowCaption()
{
    QSettings settings;
    const QString root_path = settings.value( "root_path" ).toString();
    const QString rootMsg( QObject::tr("Database root: [%1]").arg(root_path) );
    f_statusLabel.setText( rootMsg );
}

void MainWindow::InitManager()
{
    f_manager = Manager::WeakInstance();
    if( !f_manager->GetLock().lock() )
    {
        QMessageBox::StandardButton result = QMessageBox::critical
              ( this
              , QObject::tr("Database locked!")
              , QObject::tr("The database is locked. "
                            "This means that either pkg-explorer terminated unexpectantly, "
                            "or there is another instance accessing the database. Do you want to remove the lock?")
              , QMessageBox::Yes | QMessageBox::No
              );
        if( result == QMessageBox::Yes )
        {
            try
            {
                f_manager->ResetLock();
                LogDebug( "Lock file removed and reset." );

                if( !f_manager->GetLock().lock() )
                {
                    LogFatal( "Lock file is stubbornly refusing to be created!" );
                }
            }
            catch( const std::runtime_error& _xcpt )
            {
                LogFatal( _xcpt.what() );
            }
        }
        else
        {
            // Quit the application ungracefully.
            //
            LogFatal( "Not removing the lock, so exiting application." );
        }
    }

    f_logLabel.setText( tr("Please wait...") );

    UpdateWindowCaption();

    if( f_immediateInstall.isEmpty() )
    {
        RefreshListing();
    }
    else
    {
        f_procDlg.reset( new ProcessDialog( this ) );
        connect
            ( f_logOutput.get()
            , &LogOutput::AddProcessMessage
            , f_procDlg.data()
            , &ProcessDialog::AddMessage
            );
        f_procDlg->ShowLogPane(true);
        f_procDlg->show();

        // Start install
        f_installMode = InstallDialog::InstallMode;
        StartInstallThread( f_immediateInstall );
    }
}


void MainWindow::RefreshListing()
{
    EnableStopButton();

    f_packageModel.setRowCount( 0 );

    if( !f_initThread )
    {
        f_initThread.reset( new InitThread( this, actionShowInstalled->isChecked() ) );
        f_initThread->start();

        connect
           ( f_initThread.get(), &QThread::finished
           , this              , &MainWindow::OnRefreshListing
           );
    }
}


void MainWindow::OnRefreshListing()
{
    if( f_initThread )
    {
        InitThread::SectionMap map = f_initThread->GetSectionMap();
        QList<QString> keys = map.keys();
        std::for_each( keys.begin(), keys.end(), [&]( const QString& key )
        {
            QStandardItem* _parent = new QStandardItem( QIcon(":/icons/folder"), key );
            QList<InitThread::PackageList> values = map.values(key);
            std::for_each( values.begin(), values.end(), [&]( const InitThread::PackageList& list )
            {
                std::for_each( list.begin(), list.end(), [&]( const InitThread::ItemList& pkg )
                {
                    QList<QStandardItem*> stdItemList;
                    stdItemList << new QStandardItem( QIcon(":/icons/file"), pkg[0] );
                    stdItemList << new QStandardItem( pkg[1] );
                    stdItemList << new QStandardItem( pkg[2] );
                    _parent->appendRow( stdItemList );
                });
            });
            f_packageModel.appendRow( _parent );
        });

        // Done with the initThread object
        //
        f_initThread->wait();
        f_initThread.reset();
    }
    //
    f_packageModel.sort( 0 );
    f_treeView->expandAll();

    QStandardItem* root_item = f_packageModel.item( 0 );
    if( root_item )
    {
        QStandardItem* item = root_item->child( 0 );
        if( item )
        {
            f_selectModel.select( item->index(), QItemSelectionModel::SelectCurrent );
            const QString package_name = item->text();
            OnPackageClicked( package_name );
        }
    }

    UpdateActions();

    // Destroy now that we're finished. This releases the database lock.
    f_manager.reset();

    EnableStopButton( false );

    if( f_doUpgrade )
    {
        // Force the log to show
        actionShowLog->setChecked( true );

        // Kick off update, then upgrade
        actionUpdate->trigger();
    }
}


void MainWindow::OnFsTimeout()
{
    if( Manager::InUse() )
    {
        f_sysTray->setIcon( QIcon(":/icons/locked_logo") );
        return;
    }

    f_sysTray->setIcon( QIcon(":/icons/systray_icon") );
}


void MainWindow::OnAddLogMessage( const QString& message )
{
    QMutexLocker locker( &f_mutex );

    message_t msg;
    msg.f_message = message;

    f_messageFifo.push_back( msg );
}


void MainWindow::OnProgressChange( wpkgar_install::progress_record_t record )
{
    QMutexLocker locker( &f_mutex );

    message_t msg;
    msg.f_message = record.get_progress_what().c_str();
    msg.f_record  = record;

    f_messageFifo.push_back( msg );
}


void MainWindow::OnDisplayMessages()
{
    QMutexLocker locker( &f_mutex );
    while( !f_messageFifo.isEmpty() )
    {
        auto message( f_messageFifo.front() );
        f_logLabel.setText( message.f_message );

        if( message.f_record.get_progress_max() )
        {
            f_progressBar.show();
            f_progressBar.setMaximum( message.f_record.get_progress_max()     );
            f_progressBar.setValue  ( message.f_record.get_current_progress() );
        }

        f_messageFifo.pop_front();
    }
}


void MainWindow::UpdateActions()
{
	QModelIndexList selrows = f_selectModel.selectedRows();
	bool enable_remove_action = false;
	foreach( QModelIndex index, selrows )
	{
		QStandardItem* item = f_packageModel.itemFromIndex( index );
		if( item->parent() )
		{
			enable_remove_action = true;
		}
		else
		{
			enable_remove_action = false;
			break;
		}
	}
	actionRemove->setEnabled( enable_remove_action );
}


void MainWindow::OnStackStatus( bool back_empty, bool fwd_empty )
{
    actionHistoryBack->setEnabled    ( !back_empty );
    actionHistoryForward->setEnabled ( !fwd_empty  );
}


void MainWindow::SelectFromModel( const QString& package_name )
{
    const QList<QStandardItem*> package_items = f_packageModel.findItems( package_name, Qt::MatchRecursive );
    Q_ASSERT( package_items.size() == 1 );
    const QModelIndex index = f_packageModel.indexFromItem(package_items[0]);
    f_selectModel.clearSelection();
    f_selectModel.select( index, QItemSelectionModel::Select );
}


void MainWindow::OnHistoryChanged( const QString& package_name )
{
    SelectFromModel( package_name );
}


void MainWindow::DisplayPackage( const QString& package_name )
{
    f_webForm->DisplayPackage( package_name );
}

void MainWindow::OnPackageClicked( const QString& package_name )
{
    SelectFromModel( package_name );
    DisplayPackage( package_name );
}


void MainWindow::OnWebPageClicked( const QString& webpage_url )
{
    LogDebug( QString("Browse to [%1]").arg(webpage_url) );
	QDesktopServices::openUrl(QUrl(webpage_url));
}


void MainWindow::OnSelectionChanged( const QItemSelection &/*selected*/, const QItemSelection& /*deselected*/ )
{
	UpdateActions();

    try
    {
		QModelIndexList selrows = f_selectModel.selectedRows();
        if( selrows.size() == 1 )
		{
            QStandardItem* item = f_packageModel.itemFromIndex( selrows[0] );
			if( item && item->parent() )
			{
				const QString package_name = item->text();
                DisplayPackage( package_name );
            }
            else
            {
                f_webForm->ClearDisplay();
            }
        }
		else
		{
			f_webForm->ClearDisplay();
		}
    }
    catch( const std::runtime_error& except )
    {
        LogError( except.what() );
    }
}


void MainWindow::StartInstallThread( const QStringList& packages_list )
{
    wpkg_output::get_output().lock()->reset_error_count();
    f_manager = Manager::WeakInstance();
    auto installer( f_manager->GetInstaller().lock() );

    installer->register_progress_notifier(
			[&]( wpkgar::wpkgar_install::progress_record_t rec ) { this->OnProgressChange(rec); }
			); 

    // always force the chown/chmod because under Unix that doesn't work well otherwise
    installer->set_parameter( wpkgar_install::wpkgar_install_force_file_info, true );

    for( auto pkg : packages_list )
    {
        QStringList split_pkg( pkg.split(":") );
        installer->add_package( split_pkg[0].toStdString(), split_pkg[1].toStdString() );
    }

    if( !f_installThread )
    {
        EnableStopButton();

        f_manager = Manager::WeakInstance();
        f_installThread.reset
            (
                new InstallThread
                    ( this
                    , InstallThread::ThreadValidateOnly
                    )
            );

        connect
            ( f_installThread.get(), &QThread::finished
            , this                 , &MainWindow::OnInstallValidateComplete
            );

        f_installThread->start();
    }
}


void MainWindow::HandleFailure()
{
    if( f_immediateInstall.isEmpty() )
    {
        RefreshListing();
        return;
    }

    f_immediateInstall.clear();

    if( f_procDlg.data() )
    {
        f_procDlg->hide();
        f_procDlg.reset();
    }

    show();

    f_webForm->ClearHistory();
    RefreshListing();
    actionShowLog->trigger();
}


void MainWindow::OnInstallValidateComplete()
{
    if( f_installThread->get_state() == InstallThread::ThreadFailed )
    {
        QMessageBox::critical
                ( this
                  , tr("Package Validation Error!")
                  , tr("One or more packages failed to validate! See log pane for details...")
                  , QMessageBox::Ok
                  );
		f_installThread->wait();
		f_installThread.reset();
        HandleFailure();
        RefreshListing();
        return;
    }

    wpkgar_install::install_info_list_t install_list = f_manager->GetInstaller().lock()->get_install_list();

    QStringList msg;
    switch( f_installMode )
    {
        case InstallDialog::InstallMode:
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

        case InstallDialog::UpgradeMode:
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

	f_installThread->wait();
	f_installThread.reset();

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
        f_installThread.reset
			(
				new InstallThread
					( this
					, InstallThread::ThreadInstallOnly
					)
			);

        connect
            ( f_installThread.get(), &QThread::finished
            , this                 , &MainWindow::OnInstallComplete
            );

        f_installThread->start();
    }
    else
    {
        RefreshListing();
    }
}


void MainWindow::OnInstallComplete()
{
    const bool failed = f_installThread->get_state() == InstallThread::ThreadFailed;
    if( failed )
    {
        QMessageBox::critical
            ( this
            , tr("Package Installation Error!")
            , tr("One or more packages failed to install! See log pane for details...")
            , QMessageBox::Ok
            );
    }
	else if( !f_immediateInstall.isEmpty() )
	{
        QMessageBox::information
            ( this
            , tr("Success!")
            , tr("Package(s) have installed successfully!")
            , QMessageBox::Ok
            );
	}

	f_installThread->wait();
	f_installThread.reset();

	if( f_immediateInstall.isEmpty() )
	{
        RefreshListing();
    }
	else
	{
        if( failed )
        {
            HandleFailure();
        }
        else
        {
            actionQuit->triggered();
        }
	}
}


void MainWindow::on_actionFileImport_triggered()
{
    ResetErrorCount();
    ImportDialog dlg( this );
    connect
        ( &dlg , &ImportDialog::StartOperation
        , this , &MainWindow::OnStartImportOperation
        );
    connect
        ( &dlg , &ImportDialog::EndOperation
        , this , &MainWindow::OnEndImportOperation
        );
    dlg.exec();
}


void MainWindow::on_actionInstall_triggered()
{
    InstallDialog dlg( this );
    if( dlg.exec() == QDialog::Accepted )
    {
        f_installMode = dlg.GetMode();
        QStringList list;
        dlg.GetPackageList( list );
        StartInstallThread( list );
    }
}


void MainWindow::on_actionRemove_triggered()
{
    EnableStopButton();
	QStringList packages_to_remove;
	QModelIndexList selrows = f_selectModel.selectedRows();
	foreach( QModelIndex index, selrows )
	{
		QStandardItem* item = f_packageModel.itemFromIndex( index );
		Q_ASSERT(item);
		packages_to_remove << item->text();
	}

	ResetErrorCount();
    RemoveDialog dlg( this );
    connect
        ( &dlg , &RemoveDialog::StartOperation
        , this , &MainWindow::OnStartRemoveOperation
        );
    connect
        ( &dlg , &RemoveDialog::EndOperation
        , this , &MainWindow::OnEndRemoveOperation
        );
    dlg.SetPackagesToRemove( packages_to_remove );
    dlg.exec();
}


void MainWindow::on_actionDatabaseRoot_triggered()
{
    PrefsDialog prefsDlg;
    if( prefsDlg.exec() == QDialog::Accepted )
    {
        // Recreate and refresh...
        //
        UpdateWindowCaption();
        RefreshListing();
    }
}


void MainWindow::on_actionReload_triggered()
{
    RefreshListing();
}


void MainWindow::on_actionHistoryBack_triggered()
{
    f_webForm->Back();
}


void MainWindow::on_actionHistoryForward_triggered()
{
    f_webForm->Forward();
}


void MainWindow::on_actionUpdate_triggered()
{
	if( !f_updateThread )
	{
        EnableStopButton();
        f_updateThread.reset( new UpdateThread( this ) );
		f_updateThread->start();

		connect
			( f_updateThread.get(), &QThread::finished
			, this                , &MainWindow::OnUpdateFinished
			);
	}
}


void MainWindow::OnUpdateFinished()
{
	f_updateThread->wait();
	f_updateThread.reset();

    if( f_doUpgrade )
    {
        actionUpgrade->trigger();
    }
    else
    {
        f_webForm->ClearHistory();
        RefreshListing();
    }
}


void MainWindow::OnSystrayMessage( const QString& msg )
{
    if( isHidden() && f_sysTray )
    {
        f_sysTray->showMessage( tr("Package Explorer"), msg );
    }
}


void MainWindow::on_actionUpgrade_triggered()
{
    EnableStopButton();

    f_doUpgrade = false;

    InstallDialog dlg( this, InstallDialog::UpgradeMode );
    if( dlg.exec() == QDialog::Accepted )
    {
        QStringList list;
        dlg.GetPackageList( list );
        StartInstallThread( list );
    }
    else
    {
        EnableStopButton( false );
    }
}


void MainWindow::on_actionManageRepositories_triggered()
{
	SourcesDialog dlg( this );
	if( dlg.exec() == QDialog::Accepted )
	{
		QMessageBox::StandardButton response = QMessageBox::question
			( this
			  , tr("Sources Changed!")
			  , tr("You have changed your package sources list, and it is recommended that you update your sources. Do you wish to do this now?")
			  , QMessageBox::Yes | QMessageBox::No
			);
		if( response == QMessageBox::Yes )
		{
			on_actionUpdate_triggered();
		}
	}
}


void MainWindow::on_actionHelp_triggered()
{
    LogDebug( QString("Browse to http://windowspackager.org/documentation/package-explorer") );
	if( QDesktopServices::openUrl(QUrl("http://windowspackager.org/documentation/package-explorer")) )
	{
		QMessageBox::about
			( this
			, tr("Package Explorer Help")
			, tr("Package Explorer just launched your favorite browser with Package Explorer Help.")
			);
	}
	else
	{
		QMessageBox::about
			( this
			, tr("Package Explorer Help")
			, tr("Package Explorer failed launching your browser with Package Explorer Help. Please go to http://windowspackager.org/documentation/package-explorer for help about Package Explorer.")
			);
	}
}

void MainWindow::on_actionPackageExplorerLicense_triggered()
{
	f_license_box.reset(new LicenseBox(this));
	f_license_box->show();
}

void MainWindow::on_actionAbout_triggered()
{
    QMessageBox::about
        ( this
        , tr("About Package Explorer")
        , tr("<font size='+3'><b>Package Explorer v%1</b></font>"
		     "<p style='color: #447744;'><font size='-1'>Version %1 (%5)<br/>"
			 "Architecture: %2<br/>"
			 "Vendor: %3<br/>"
			 "Debian Package Library Version: %4</font></p>"
			 "<p>Package Explorer is the GUI front end of the <a href='http://windowspackager.org/documentation/wpkg'>wpkg</a> command line tool that helps you setup and maintain a target installation of software packages.</p>"
		     "<p style='color: #444444;'><font size='-1'>Copyright (c) 2013 <a href='http://www.m2osw.com/'>Made to Order Software</a><br/>"
			 "All Rights Reserved<br/>"
			 "Free and Open Source Software<br/>"
			 "GNU General Puplic License Version 2</font></p>")
          .arg(VERSION)
		  .arg(debian_packages_architecture())
		  .arg(debian_packages_vendor())
		  .arg(debian_packages_version_string())
#ifdef DEBUG
		  .arg("debug")
#else
		  .arg("release")
#endif
        );
}

void MainWindow::on_actionAboutWindowsPackager_triggered()
{
    QMessageBox::about
        ( this
        , tr("About Windows Packager")
        , tr("<font size='+3'>Windows Packager</font><br/>"
		     "<p>The <a href='http://windowspackager.org/'>Windows Packager Project</a> is a software suite offering an advanced and very powerful set of tools to create packages and maintain them in a target system.</p>"
			 "<p>The project includes all the powerful functions in a library called libdebpackages. This library is used by the tools offered in this project such as <a href='http://windowspackager.org/documentation/wpkg'>wpkg</a> and <a href='http://windowspackager.org/documentation/package-explorer'>pkg-explorer</a>.</p>"
			 "<p>The packages generated by wpkg are compatible with Debian packages, however, our suite functions on all Unix (Linux, Darwin, FreeBSD, SunOS, ...) and MS-Windows platforms making it even more useful for software companies who want to distribute their software on many different platforms.</p>"
			 "<p>The usys environment and the Windows Packager projects were created and are maintained by <a href='http://www.m2osw.com/'>Made to Order Software Corporation</a>.</p>"
		    )
        );
}


void MainWindow::on_actionClearLog_triggered()
{
    f_logOutput->clear();
}

void MainWindow::on_actionShowInstalled_triggered()
{
    RefreshListing();
}


void MainWindow::ResetLogChecks( QAction* except )
{
    std::for_each( f_levelToAction.begin(), f_levelToAction.end(),
        [&]( QAction* action)
        {
            if( except != action )
            {
                action->setChecked( false );
            }
		});
}

void MainWindow::on_actionViewLogDebug_triggered()
{
    ResetLogChecks( actionViewLogDebug );
    f_logOutput->set_level( wpkg_output::level_debug );
    f_statusbar->showMessage( tr("Debug Log Level Set") );
}


void MainWindow::on_actionViewLogInfo_triggered()
{
    ResetLogChecks( actionViewLogInfo );
    f_logOutput->set_level( wpkg_output::level_info );
    f_statusbar->showMessage( tr("Info Log Level Set") );
}


void MainWindow::on_actionViewLogWarning_triggered()
{
    ResetLogChecks( actionViewLogWarning );
    f_logOutput->set_level( wpkg_output::level_warning );
    f_statusbar->showMessage( tr("Warning Log Level Set") );
}


void MainWindow::on_actionViewLogError_triggered()
{
    ResetLogChecks( actionViewLogError );
    f_logOutput->set_level( wpkg_output::level_error );
    f_statusbar->showMessage( tr("Error Log Level Set") );
}


void MainWindow::on_actionStop_triggered()
{
    QMutexLocker locker( &f_mutex );
    f_stopClicked = true;
}


// vim: ts=4 sw=4 noet
