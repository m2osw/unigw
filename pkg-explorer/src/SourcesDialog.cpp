#include "SourcesDialog.h"
#include "SourceInfoDialog.h"
#include "RepoUtils.h"

#include <algorithm> 

#include <libdebpackages/wpkgar_repository.h>

using namespace wpkgar;

SourcesDialog::SourcesDialog( QWidget *p )
    : QDialog(p)
    , f_model(this)
    , f_selectModel(static_cast<QAbstractItemModel*>(&f_model))
{
	setupUi(this);
	f_listView->setModel( &f_model );
	f_listView->setSelectionModel( &f_selectModel );

	// Listen for selection changes in the view (either via mouse or keyboard)
	//
	QObject::connect
		( &f_selectModel, SIGNAL( selectionChanged  ( const QItemSelection&, const QItemSelection& ))
		  , this          , SLOT  ( OnSelectionChanged( const QItemSelection&, const QItemSelection& ))
		);

	QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
	Q_ASSERT( applyBtn != NULL );
	applyBtn->setEnabled( false );
}

SourcesDialog::~SourcesDialog()
{
}


void SourcesDialog::SetManager( QSharedPointer<wpkgar::wpkgar_manager> mgr )
{
	f_manager = mgr;
	Q_ASSERT( f_manager.data() );

    try
    {
        f_model.setStringList( RepoUtils::ReadSourcesList( f_manager.data() ) );
    }
    catch( const wpkgar::wpkgar_exception& _x )
    {
        QMessageBox::critical( this, tr("Error!"), tr("Error reading sources list: %1").arg(_x.what()) );
    }
}


void SourcesDialog::OnSelectionChanged( const QItemSelection&, const QItemSelection& )
{
	QModelIndexList selrows = f_selectModel.selectedRows();
	f_removeButton->setEnabled( selrows.size() > 0 );
}

void SourcesDialog::on_f_addButton_clicked()
{
    SourceInfoDialog dlg( this );

	QModelIndexList selrows = f_selectModel.selectedRows();
	if( selrows.size() == 1 )
	{
        const QVariant _data = f_model.data( selrows[0], Qt::EditRole );
        dlg.SetSource( RepoUtils::QStringToSource( _data.toString() ) );
	}

    if( dlg.exec() == QDialog::Accepted )
	{
		QStringList contents = f_model.stringList();
		contents << RepoUtils::SourceToQString( dlg.GetSource() );
		f_model.setStringList( contents );
#if 1
		// TODO: This should be done via events from the model...
		//
		QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
		Q_ASSERT( applyBtn != NULL );
		applyBtn->setEnabled( true );
#endif
	}
}


void SourcesDialog::on_f_removeButton_clicked()
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
	}
	else
	{
        f_removeButton->setEnabled( false );
	}
#if 1
		// TODO: This should be done via events from the model...
		//
		QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
		Q_ASSERT( applyBtn != NULL );
		applyBtn->setEnabled( true );
#endif
}

void SourcesDialog::on_f_buttonBox_clicked(QAbstractButton *button)
{
    QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
	Q_ASSERT( applyBtn );
    QPushButton* discardBtn = f_buttonBox->button( QDialogButtonBox::Discard );
	Q_ASSERT( discardBtn );
	//
    if( button == applyBtn )
    {
        try
        {
            RepoUtils::WriteSourcesList( f_manager.data(), f_model.stringList() );
            accept();
        }
        catch( const wpkgar::wpkgar_exception& _x )
        {
            QMessageBox::critical( this, tr("Error!"), tr("Error writing sources list: %1").arg(_x.what()) );
        }
    }
    else if( button == discardBtn )
    {
        reject();
    }
}

void SourcesDialog::on_f_listView_doubleClicked(const QModelIndex &index)
{
    QPushButton* applyBtn = f_buttonBox->button( QDialogButtonBox::Apply );
	Q_ASSERT( applyBtn );
	applyBtn->setEnabled( true );
}

// vim: ts=4 sw=4 et
