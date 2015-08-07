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
#include "InstallDialog.h"
#include "RemoveDialog.h"
#include "PrefsDialog.h"
#include "ProcessDialog.h"
#include "SourcesDialog.h"
#include "LicenseBox.h"

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
	class ActionsDisable
	{
	public:
		ActionsDisable( QList<QAction*> actionList, const bool enable_on_destroy = true )
			: f_actionList(actionList)
			, f_enableOnDestroy(enable_on_destroy)
		{
			std::for_each( f_actionList.begin(), f_actionList.end(), []( QAction* act ) { act->setEnabled(false); } );
		}
		~ActionsDisable()
		{
			if( f_enableOnDestroy )
			{
				std::for_each( f_actionList.begin(), f_actionList.end(), []( QAction* act ) { act->setEnabled(true); } );
			}
		}

	private:
		QList<QAction*> f_actionList;
		bool			f_enableOnDestroy;
	};

	class ProcessInterrupt : public wpkgar::wpkgar_interrupt
	{
		public:
			virtual bool stop_now()
			{
				return ProcessDialog::CancelClicked();
			}
	};

	ProcessInterrupt interrupt;
}
//namespace


MainWindow::MainWindow( const bool showSysTray )
	: QMainWindow(0)
	, f_packageModel(this)
	, f_selectModel(&f_packageModel)
{
    setupUi(this);

    if( QSystemTrayIcon::isSystemTrayAvailable() && showSysTray )
    {
        f_sysTray = QSharedPointer<QSystemTrayIcon>( new QSystemTrayIcon( this ) );
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
    connect( actionQuit, SIGNAL(triggered()),   qApp, SLOT(quit())          );
    connect( qApp,           SIGNAL(aboutToQuit()), this, SLOT(OnAboutToQuit()) );

	// Listen for selection changes in the view (either via mouse or keyboard)
	//
    QObject::connect
        ( &f_selectModel, SIGNAL( selectionChanged  ( const QItemSelection&, const QItemSelection& ))
        , this          , SLOT  ( OnSelectionChanged( const QItemSelection&, const QItemSelection& ))
        );

    actionHistoryBack->setEnabled    ( false );
    actionHistoryForward->setEnabled ( false );

    QObject::connect
        ( f_webForm, SIGNAL( StackStatus     ( bool, bool ) )
        , this     , SLOT  ( OnStackStatus   ( bool, bool ) )
        );
    QObject::connect
        ( f_webForm, SIGNAL( HistoryChanged  ( const QString& ) )
        , this     , SLOT  ( OnHistoryChanged( const QString& ) )
        );
    QObject::connect
        ( f_webForm, SIGNAL( PackageClicked  ( const QString& ) )
        , this     , SLOT  ( OnPackageClicked( const QString& ) )
        );
    QObject::connect
        ( f_webForm, SIGNAL( WebPageClicked  ( const QString& ) )
        , this     , SLOT  ( OnWebPageClicked( const QString& ) )
        );

    QObject::connect
        ( f_logForm, SIGNAL( SetSystrayMessage(const QString&) )
        , this     , SLOT  ( OnSystrayMessage (const QString&) )
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

	QTimer::singleShot( 100, this, SLOT( OnInitTimer() ) );
}


MainWindow::~MainWindow()
{
}


void MainWindow::RunCommand( const QString& command )
{
	if( command == "import" )
	{
		on_actionFileImport_triggered();
	}
	else if( command == "remove" )
	{
		on_actionRemove_triggered();
	}
	else if( command == "install" )
	{
		on_actionInstall_triggered();
	}
	else if( command == "update" )
	{
		on_actionUpdate_triggered();
	}
	else if( command == "upgrade" )
	{
		on_actionUpgrade_triggered();
	}
	else if( command == "manage" )
	{
		on_actionManageRepositories_triggered();
	}
	else if( command == "database" )
	{
		on_actionDatabaseRoot_triggered();
	}
	else
	{
		LogError( tr("Unknown command '") + command + tr("'!") );
	}
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
    f_logForm->SetLogLevel( level );
	f_levelToAction[level]->setChecked( true );
}


void MainWindow::SaveSettings()
{
    QSettings settings;
    settings.setValue( "geometry",            saveGeometry()                       );
    settings.setValue( "state",               saveState()                          );
    settings.setValue( "show_installed",      actionShowInstalled->isChecked()     );
    settings.setValue( "show_log",            actionShowLog->isChecked()           );
    settings.setValue( "log_level",           f_logForm->GetLogLevel()             );
    settings.setValue( "minimize_to_systray", actionMinimizeToSystray->isChecked() );
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
    connect( actionShowApplication, SIGNAL(triggered()), this, SLOT(show()) );
}


void MainWindow::showEvent( QShowEvent * /*evt*/ )
{
    actionShowApplication->setText( tr("&Hide Application") );
    connect( actionShowApplication, SIGNAL(triggered()), this, SLOT(hide()) );
}


void MainWindow::OutputToLog( wpkg_output::level_t level, const QString& msg )
{
    wpkg_output::output* out = f_logForm->GetLogOutput();
	Q_ASSERT(out);
	//
	wpkg_output::message_t msg_obj;
	msg_obj.set_level( level );
	msg_obj.set_raw_message( msg.toStdString() );
    out->log( msg_obj );
}


void MainWindow::LogDebug( const QString& msg )
{
	OutputToLog( wpkg_output::level_debug, msg );
}


void MainWindow::LogInfo( const QString& msg )
{
	OutputToLog( wpkg_output::level_info, msg );
}


void MainWindow::LogWarning( const QString& msg )
{
	OutputToLog( wpkg_output::level_warning, msg );
}


void MainWindow::LogError( const QString& msg )
{
	OutputToLog( wpkg_output::level_error, msg );
}


void MainWindow::LogFatal( const QString& msg )
{
    OutputToLog( wpkg_output::level_fatal, msg );
    QMessageBox::critical
        ( this
          , tr("Application Terminated!")
          , msg
          , QMessageBox::Ok
        );
    qFatal( "%s", msg.toStdString().c_str() );
}



void MainWindow::InitManager()
{
    f_lock.clear();
    if( f_manager.data() != NULL )
    {
        LogDebug( "Closing database..." );
    }
    f_manager = QSharedPointer<wpkgar_manager>( new wpkgar_manager );

    f_manager->set_interrupt_handler( &interrupt );

	// TODO: add the Qt packages we depend on once ready
	//       (specifically for MS-Windows)
    f_manager->add_self("wpkg-gui");
    f_manager->add_self("wpkgguiqt4");

    wpkg_output::output* out = f_logForm->GetLogOutput();
    Q_ASSERT( out );
    wpkg_output::set_output( out );
    out->set_debug_flags( wpkg_output::debug_flags::debug_progress );

	QSettings settings;
	const QString root_path = settings.value( "root_path" ).toString();
    const QString database_path = QString("%1/var/lib/wpkg").arg(root_path);

    LogDebug( QString("Opening WPKG database %1").arg(root_path) );

    f_manager->set_root_path( root_path.toStdString() );
    f_manager->set_database_path( database_path.toStdString() );

    bool lock_file_created = false;
    while( !lock_file_created )
	{
		try
		{
			f_lock = QSharedPointer<wpkgar_lock>(
					new wpkgar_lock( f_manager.data(), "Package Explorer" )
					);
			lock_file_created = true;
		}
        catch( const wpkgar_exception_locked& except )
		{
            LogError( except.what() );
			QMessageBox::StandardButton result = QMessageBox::critical
				( this
				  , tr("Database locked!")
				  , tr("The database is locked. "
					  "This means that either pkg-explorer terminated unexpectantly, "
					  "or there is another instance accessing the database. Do you want to remove the lock?")
				  , QMessageBox::Yes | QMessageBox::No
				);
			if( result == QMessageBox::Yes )
			{
				try
				{
					f_manager->remove_lock();
                    LogDebug( "Lock file removed." );
				}
               catch( const std::runtime_error& _xcpt )
				{
                    LogFatal( _xcpt.what() );
                    break;
				}
			}
			else
			{
				// Quit the application ungracefully.
				//
                LogFatal( "Not removing the lock, so exiting application." );
                break;
			}
        }
        catch( const std::runtime_error& except )
		{
            LogFatal( except.what() );
            break;
		}
	}

    f_webForm->SetManager( f_manager );
    if( lock_file_created )
    {
        RefreshListing();
    }
    else
    {
        MainWindow::close();
        QApplication::quit();
    }
}


class InitThread : public QThread
{
public:
    InitThread( QObject* p, wpkgar_manager* manager, const bool show_installed_only )
        : QThread(p)
		, f_manager(manager)
		, f_showInstalledOnly(show_installed_only)
	{}

	typedef QList<QString>				ItemList;
	typedef QList<ItemList>				PackageList;
	typedef QMap<QString,PackageList>	SectionMap;
	SectionMap GetSectionMap() const { return f_sectionMap; }

    virtual void run();

private:
	wpkgar_manager*	f_manager;
	SectionMap		f_sectionMap;
	bool			f_showInstalledOnly;
};


namespace
{
	QString StatusToQString( wpkgar_manager::package_status_t status )
	{
		switch( status )
		{
			case wpkgar_manager::no_package		 : return QObject::tr("no_package");
			case wpkgar_manager::unknown 		 : return QObject::tr("unknown");
			case wpkgar_manager::not_installed	 : return QObject::tr("not_installed");
			case wpkgar_manager::config_files	 : return QObject::tr("config_files");
			case wpkgar_manager::installing		 : return QObject::tr("installing");
			case wpkgar_manager::upgrading		 : return QObject::tr("upgrading");
			case wpkgar_manager::half_installed	 : return QObject::tr("half_installed");
			case wpkgar_manager::unpacked		 : return QObject::tr("unpacked");
			case wpkgar_manager::half_configured : return QObject::tr("half_configured");
			case wpkgar_manager::installed		 : return QObject::tr("installed");
			case wpkgar_manager::removing		 : return QObject::tr("removing");
			case wpkgar_manager::purging		 : return QObject::tr("purging");
			case wpkgar_manager::listing		 : return QObject::tr("listing");
			case wpkgar_manager::verifying		 : return QObject::tr("verifying");
			case wpkgar_manager::ready			 : return QObject::tr("ready");
		}

		return QObject::tr("Unknown!");
	}

	void ResetErrorCount()
	{
		wpkg_output::output* output = wpkg_output::get_output();
		if( output )
		{
			output->reset_error_count();
		}
	}
}


void InitThread::run()
{
	wpkgar_manager::package_list_t list;
	try
    {
		ResetErrorCount();
        f_manager->list_installed_packages( list );

		Q_FOREACH( std::string package_name, list )
		{
			wpkgar_manager::package_status_t status( f_manager->package_status( package_name ) );

			bool show_package = true;
			//
            if( f_showInstalledOnly && (status != wpkgar_manager::installed) )
			{
				show_package = false;
			}
			//
			if( show_package )
			{
				const std::string version( f_manager->get_field( package_name, "Version" ) );
				std::string section("base"); // use a valid default
				if( f_manager->field_is_defined( package_name, "Section" ) )
				{
					section = f_manager->get_field( package_name, "Section" );
				}
				ItemList pkg;
				pkg << package_name.c_str();
				pkg << StatusToQString( status );
				pkg << version.c_str();
				f_sectionMap[section.c_str()] << pkg;
			}
		}
    }
    catch( const std::runtime_error& except )
    {
        qCritical() << "std::runtime_error caught! what=" << except.what();
		wpkg_output::log( except.what() ).level( wpkg_output::level_error );
    }
}


void MainWindow::RefreshListing()
{
	ActionsDisable ad( f_actionList, false /*enable_remove_action*/ );

    f_logForm->ShowProcessDialog( true, false /*enable_cancel*/ );
	f_packageModel.setRowCount( 0 );

	f_thread = QSharedPointer<QThread>( static_cast<QThread*>(
        new InitThread( this, f_manager.data(), actionShowInstalled->isChecked() )
		) );
	f_thread->start();

    connect
        ( f_thread.data(), SIGNAL(finished())
        , this           , SLOT(OnRefreshListing())
        );
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


void MainWindow::OnRefreshListing()
{
	ActionsDisable ad( f_actionList );

    InitThread* _thread = dynamic_cast<InitThread*>(f_thread.data());
    Q_ASSERT(_thread);
    InitThread::SectionMap map = _thread->GetSectionMap();
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
    f_logForm->ShowProcessDialog( false, true );
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


void MainWindow::OnPackageClicked( const QString& package_name )
{
    SelectFromModel( package_name );
    f_webForm->DisplayPackage( package_name );
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
				f_webForm->DisplayPackage( package_name );
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


void MainWindow::on_actionFileImport_triggered()
{
    ActionsDisable ad( f_actionList );
	ResetErrorCount();
    ImportDialog dlg( this, f_manager );
    connect
        ( &dlg      , SIGNAL(ShowProcessDialog(bool,bool))
        , f_logForm , SLOT  (ShowProcessDialog(bool,bool))
        );
    if( dlg.exec() == QDialog::Accepted )
    {
        InitManager();
    }
}


void MainWindow::on_actionInstall_triggered()
{
    ActionsDisable ad( f_actionList );
    InstallDialog dlg( this, f_manager );
    connect
        ( &dlg      , SIGNAL(ShowProcessDialog(bool,bool))
        , f_logForm , SLOT  (ShowProcessDialog(bool,bool))
        );
    if( dlg.exec() == QDialog::Accepted )
    {
        InitManager();
    }
}


void MainWindow::on_actionRemove_triggered()
{
    ActionsDisable ad( f_actionList );
	QStringList packages_to_remove;
	QModelIndexList selrows = f_selectModel.selectedRows();
	foreach( QModelIndex index, selrows )
	{
		QStandardItem* item = f_packageModel.itemFromIndex( index );
		Q_ASSERT(item);
		packages_to_remove << item->text();
	}

	ResetErrorCount();
    RemoveDialog dlg( this, f_manager );
    connect
        ( &dlg      , SIGNAL(ShowProcessDialog(bool,bool))
        , f_logForm , SLOT  (ShowProcessDialog(bool,bool))
        );
    dlg.SetPackagesToRemove( packages_to_remove );
    if( dlg.exec() == QDialog::Accepted )
    {
        f_webForm->ClearHistory();
        InitManager();
    }
}


void MainWindow::on_actionDatabaseRoot_triggered()
{
    ActionsDisable ad( f_actionList );
    PrefsDialog prefsDlg;
    if( prefsDlg.exec() == QDialog::Accepted )
    {
        // Recreate and refresh...
        //
        InitManager();
    }
}


void MainWindow::on_actionReload_triggered()
{
    InitManager();
}


void MainWindow::on_actionHistoryBack_triggered()
{
    f_webForm->Back();
}


void MainWindow::on_actionHistoryForward_triggered()
{
    f_webForm->Forward();
}


class UpdateThread : public QThread
{
public:
    UpdateThread( QObject* p, wpkgar_manager* manager )
        : QThread(p)
		, f_manager(manager)
	{}

    virtual void run();

private:
	wpkgar_manager*	f_manager;
};


void UpdateThread::run()
{
    wpkgar_repository repository( f_manager );
	try
	{
		repository.update();
	}
    catch( const std::runtime_error& except )
	{
        qCritical() << "std::runtime_error caught! what=" << except.what();
        wpkg_output::log( except.what() ).level( wpkg_output::level_error );
	}
}


void MainWindow::on_actionUpdate_triggered()
{
	ActionsDisable ad( f_actionList, false /*enable_on_destroy*/ );
    f_logForm->ShowProcessDialog( true, false /*enable_cancel*/ );

	f_thread = QSharedPointer<QThread>( static_cast<QThread*>( new UpdateThread( this, f_manager.data() ) ) );
	f_thread->start();

    connect
        ( f_thread.data(), SIGNAL(finished())
        , this           , SLOT(OnUpdateFinished())
        );
}


void MainWindow::OnUpdateFinished()
{
	ActionsDisable ad( f_actionList );
    f_logForm->ShowProcessDialog( false, true );
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
    ActionsDisable ad( f_actionList );
    InstallDialog dlg( this, f_manager, InstallDialog::UpgradeMode );
    connect
        ( &dlg      , SIGNAL(ShowProcessDialog(bool,bool))
        , f_logForm , SLOT  (ShowProcessDialog(bool,bool))
        );
    if( dlg.exec() == QDialog::Accepted )
    {
        InitManager();
    }
}


void MainWindow::on_actionManageRepositories_triggered()
{
    ActionsDisable ad( f_actionList );
	SourcesDialog dlg( this );
	dlg.SetManager( f_manager );
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
    f_logForm->Clear();
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
    f_logForm->SetLogLevel( wpkg_output::level_debug );
    f_statusbar->showMessage( tr("Debug Log Level Set") );
}


void MainWindow::on_actionViewLogInfo_triggered()
{
    ResetLogChecks( actionViewLogInfo );
    f_logForm->SetLogLevel( wpkg_output::level_info );
    f_statusbar->showMessage( tr("Info Log Level Set") );
}


void MainWindow::on_actionViewLogWarning_triggered()
{
    ResetLogChecks( actionViewLogWarning );
    f_logForm->SetLogLevel( wpkg_output::level_warning );
    f_statusbar->showMessage( tr("Warning Log Level Set") );
}


void MainWindow::on_actionViewLogError_triggered()
{
    ResetLogChecks( actionViewLogError );
    f_logForm->SetLogLevel( wpkg_output::level_error );
    f_statusbar->showMessage( tr("Error Log Level Set") );
}


// vim: ts=4 sw=4 noet
