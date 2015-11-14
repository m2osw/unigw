#include "SourceInfoDialog.h"

#include <algorithm>

#include <QtGui>
#include <QUrl>

#include <algorithm>

SourceInfoDialog::SourceInfoDialog( QWidget *p )
    : QDialog(p)
{
    setupUi( this );
}


SourceInfoDialog::~SourceInfoDialog()
{
}


wpkgar::source SourceInfoDialog::GetSource() const
{
	wpkgar::source src;
	src.set_type         ( f_typeCB->currentText().toStdString() );
	src.set_uri          ( f_uriEdit->text().toStdString()       );
    src.set_distribution ( f_distEdit->text().toStdString()      );

    QStringList components( f_sourceEdit->text().split(" ") );
    std::for_each( components.begin(), components.end(), [&](const QString& comp)
    {
        src.add_component( comp.toStdString() );
    });
	return src;
}


void SourceInfoDialog::SetSource( const wpkgar::source& src )
{
	const int id = f_typeCB->findText( src.get_type().c_str() );
    f_typeCB->setCurrentIndex( id );

    QStringList components;
    for( int i(0); i < src.get_component_size(); ++i )
    {
        components << src.get_component( i ).c_str();
    }

	f_uriEdit->setText    ( src.get_uri().c_str()          );
	f_distEdit->setText   ( src.get_distribution().c_str() );
	f_sourceEdit->setText ( components.join(" ")           );
}


void SourceInfoDialog::on_f_uriButton_clicked()
{
    QSettings settings;

	QFileDialog uriDlg( this
						, tr("Select repository folder.")
						);
    uriDlg.restoreState( settings.value( "uri_add_dialog" ).toByteArray() );
	uriDlg.setFileMode( QFileDialog::Directory );
	uriDlg.setOptions( QFileDialog::ShowDirsOnly );

    bool accept_it = false;
    if( uriDlg.exec() )
    {
		QUrl url;
		url.setPath( QDir::fromNativeSeparators(uriDlg.directory().absolutePath()) );
		url.setScheme( "file" );

        f_uriEdit->setText( url.path() );
        f_sourceEdit->setText( "./" );
        f_distEdit->setText( "" );

        accept_it = true;
	}

	settings.setValue( "uri_add_dialog", uriDlg.saveState() );

    if( accept_it )
    {
        accept();
    }
}

void SourceInfoDialog::on_f_buttonBox_clicked(QAbstractButton *button)
{
    QPushButton* okayBtn = f_buttonBox->button( QDialogButtonBox::Ok );
    Q_ASSERT( okayBtn );
    QPushButton* cancelBtn = f_buttonBox->button( QDialogButtonBox::Cancel );
    Q_ASSERT( cancelBtn );
    //
    if( button == okayBtn )
    {
        if( !f_uriEdit->text().isEmpty()
         && !f_distEdit->text().isEmpty()
         && !f_sourceEdit->text().isEmpty()
         )
        {
            accept();
        }
        else
        {
            QMessageBox::critical( this
                                   , tr("Validation Error")
                                   , tr("You must fill out each field before submitting.")
                                   );
        }
    }
    else if( button == cancelBtn )
    {
        reject();
    }
}

// vim: ts=4 sw=4 et
