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

#include "RemoveDialog.h"

using namespace wpkgar;

RemoveDialog::RemoveDialog( QWidget *p, QSharedPointer<wpkgar_manager> manager, LogForm* logForm )
    : QDialog(p)
    , f_model(this)
    , f_selectModel(static_cast<QAbstractItemModel*>(&f_model))
    , f_manager(manager)
    , f_remover(QSharedPointer<wpkgar_remove>(new wpkgar_remove(f_manager.data())))
	, f_logForm(logForm)
{
    setupUi(this);
    f_listView->setModel( &f_model );
	f_listView->setSelectionModel( &f_selectModel );

    QObject::connect
        ( &f_selectModel, SIGNAL( selectionChanged  ( const QItemSelection&, const QItemSelection& ))
        , this          , SLOT  ( OnSelectionChanged( const QItemSelection&, const QItemSelection& ))
        );

    QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
	Q_ASSERT( applyBtn != NULL );
    applyBtn->setEnabled( true );

    f_optionsFrame->hide();
}


RemoveDialog::~RemoveDialog()
{
}


void RemoveDialog::SetPackagesToRemove( const QStringList& list )
{
	f_model.setStringList( list );
}


void RemoveDialog::OnSelectionChanged( const QItemSelection &/*selected*/, const QItemSelection& /*deselected*/ )
{
    QModelIndexList selrows = f_selectModel.selectedRows();
    //
    //QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
    //Q_ASSERT( applyBtn != NULL );
    //applyBtn->setEnabled( false );
}


void RemoveDialog::on_f_forceAllCB_clicked()
{
	const bool all_checked( f_forceAllCB->checkState() == Qt::Checked );
	QList<QCheckBox*> cb_list;
    cb_list << f_forceDepsCB;
    cb_list << f_forceHoldCB;
    cb_list << f_forceRemoveEssentialCB;

	foreach( QCheckBox* cb, cb_list )
	{
		cb->setCheckState( all_checked ? Qt::Checked : Qt::Unchecked );
	}
}


void RemoveDialog::ChangeAllChecked()
{
	QList<QCheckBox*> cb_list;
	cb_list << f_forceDepsCB;
	cb_list << f_forceHoldCB;
	cb_list << f_forceRemoveEssentialCB;

	bool all_checked = true;
	foreach( QCheckBox* cb, cb_list )
	{
		if( cb->checkState() == Qt::Unchecked )
		{
			all_checked = false;
			break;
		}
	}

	f_forceAllCB->setCheckState( all_checked ? Qt::Checked : Qt::Unchecked );
}


void RemoveDialog::on_f_forceDepsCB_clicked()
{
	ChangeAllChecked();
}


void RemoveDialog::on_f_forceRemoveEssentialCB_clicked()
{
	ChangeAllChecked();
}


void RemoveDialog::on_f_purgeCB_clicked()
{
    ChangeAllChecked();
}


void RemoveDialog::on_f_recursiveCB_clicked()
{
	ChangeAllChecked();
}


void RemoveDialog::SetSwitches()
{
	QMap<wpkgar_remove::parameter_t,QCheckBox*> cb_map;
	cb_map[wpkgar_remove::wpkgar_remove_force_depends]           = f_forceDepsCB;
	cb_map[wpkgar_remove::wpkgar_remove_force_hold]              = f_forceHoldCB;
	cb_map[wpkgar_remove::wpkgar_remove_force_remove_essentials] = f_forceRemoveEssentialCB;
	cb_map[wpkgar_remove::wpkgar_remove_recursive]               = f_recursiveCB;

	foreach( wpkgar_remove::parameter_t key, cb_map.keys() )
	{
		f_remover->set_parameter
			( key
			, cb_map[key]->checkState() == Qt::Checked
			);
    }

    if( f_purgeCB->checkState() == Qt::Checked )
    {
        f_remover->set_purging();
    }
    //else -- you don't want that for --remove or --purge, only --deconfigure which we do not support yet
    //{
    //    f_remover->set_deconfiguring();
    //}
}

class RemoveThread : public QThread
{
public:
	typedef enum { ThreadStopped, ThreadRunning, ThreadFailed, ThreadSucceeded } State;

    RemoveThread( QObject* p, wpkgar_manager* manager, wpkgar_remove* remover )
        : QThread(p)
        , f_manager(manager)
		, f_remover(remover)
		, f_state(ThreadStopped)
    {}

    virtual void run();

	State get_state() const
	{
		QMutexLocker locker( &f_mutex );
		return f_state;
	}

private:
    wpkgar_manager*	f_manager;
	wpkgar_remove*  f_remover;
	State			f_state;
	mutable QMutex	f_mutex;

	void set_state( const State new_state )
	{
		QMutexLocker locker( &f_mutex );
		f_state = new_state;
	}
};


void RemoveThread::run()
{
	set_state( ThreadRunning );

    try
    {
        for(;;)
		{
			const int i( f_remover->remove() );
			if(i < 0)
			{
				if( i == wpkgar_remove::WPKGAR_EOP )
				{
					wpkg_output::log( "Removal of packages complete!" ).level( wpkg_output::level_info );
					set_state( ThreadSucceeded );
				}
				else
				{
					wpkg_output::log( "Removal of packages failed!" ).level( wpkg_output::level_error );
					set_state( ThreadFailed );
				}
				break;
			}
			if(f_remover->get_purging())
			{
				if( !f_remover->deconfigure(i) )
				{
					wpkg_output::log( "Removal failed deconfiguration!" ).level( wpkg_output::level_error );
					set_state( ThreadFailed );
					break;
				}
			}
		}
    }
    catch( const std::runtime_error& except )
    {
        qCritical() << "std::runtime_error caught! what=" << except.what();
		wpkg_output::log( except.what() ).level( wpkg_output::level_error );
		set_state( ThreadFailed );
    }
}


void RemoveDialog::on_f_buttonBox_clicked(QAbstractButton *button)
{
    QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
	Q_ASSERT( applyBtn );
    QPushButton* closeBtn = f_buttonBox->button( QDialogButtonBox::Discard );
	Q_ASSERT( closeBtn );
	//
    if( button == applyBtn )
	{
		f_logForm->ShowProcessDialog( true, false /*cancel_button*/ );
        SetSwitches();

        QMap<QString,int> folders;
        const QStringList contents = f_model.stringList();
		foreach( QString file, contents )
		{
            f_remover->add_package( file.toStdString() );
        }

        if( f_remover->validate() )
		{
			f_thread = QSharedPointer<QThread>( static_cast<QThread*>( new RemoveThread( this, f_manager.data(), f_remover.data() ) ) );
			f_thread->start();

			connect
				( f_thread.data(), SIGNAL(finished())
				, this           , SLOT(OnRemoveComplete())
				);
		}
		else
		{
			QMessageBox::critical
				( this
				  , tr("Package Validation Error!")
				  , tr("One or more packages failed to validate for removal! See log pane for details...")
				  , QMessageBox::Ok
				);
			f_logForm->ShowProcessDialog( false );
            reject();
        }
    }
    else if( button == closeBtn )
    {
        reject();
    }
}


void RemoveDialog::OnRemoveComplete()
{
	if( dynamic_cast<RemoveThread*>(f_thread.data())->get_state() == RemoveThread::ThreadFailed )
	{
		QMessageBox::critical
			( this
			, tr("Package Removal Error!")
			, tr("One or more packages failed to remove! See log pane for details...")
			, QMessageBox::Ok
			);
	}

	f_logForm->ShowProcessDialog( false );
	accept();
}


void RemoveDialog::on_f_optionsButton_toggled(bool checked)
{
    f_optionsButton->setText(
                checked? tr(">> &Options")
                       : tr("<< &Options")
                         );
    f_optionsFrame->setShown( checked );
}


// vim: ts=4 sw=4 noet
