#include "database.h"

#if defined(MO_LINUX)
#   pragma GCC diagnostic push
#   pragma GCC diagnostic ignored "-Wctor-dtor-privacy"
#   pragma GCC diagnostic ignored "-Wsign-promo"
#endif
#include <QtWidgets>
#if defined(MO_LINUX)
#   pragma GCC diagnostic pop
#endif

#include <libdebpackages/wpkgar.h>


namespace Database
{

#if defined(MO_LINUX) || defined(MO_DARWIN)
QString GetUnameArch()
{
	QProcess uname;
	uname.start( "uname", QStringList() << "-m" );
	uname.waitForFinished();
	const QString arch = uname.readAll().constData();
	//
	if( arch == "x86_64\n" )
	{
		return "amd64";
	}
	//
	return "i386";
}
#endif

QString GetCanonalizedArch()
{
	QString carch;
#if defined(MO_WINDOWS)
	SYSTEM_INFO si;
	GetNativeSystemInfo( &si  );
	switch( si.wProcessorArchitecture )
	{
		case PROCESSOR_ARCHITECTURE_AMD64:
			carch = "win64-amd64";
			break;

		case PROCESSOR_ARCHITECTURE_INTEL:
			carch =  "win32-i386";
			break;

		default:
			carch = "unknown-unknown";
	}
#elif defined(MO_LINUX)
	carch = QString("linux-%1").arg( GetUnameArch() );
#elif defined(MO_DARWIN)
	carch = QString("darwin-%1").arg( GetUnameArch() );
#else
#	error "Unsupported architecture!"
#endif
	return carch;
}


QString GetDefaultDbRoot()
{
    return QStandardPaths::writableLocation( QStandardPaths::AppLocalDataLocation ) + "/WPKG_ROOT";
}


void SetDbRoot( const QString& new_root )
{
    QSettings settings;
    settings.setValue( "root_path", new_root );
}


void InitDatabase()
{
    QSettings settings;
    const QString root_path       ( settings.value( "root_path", GetDefaultDbRoot() ).toString() );
    const QString wpkg_admin_path ( QString("%1/var/lib/wpkg").arg(root_path) );
    const QString control_file	  ( QString("%1/core/control").arg(wpkg_admin_path) );

    QFileInfo finfo( QDir::toNativeSeparators( control_file ) );
    if( !finfo.exists() )
    {
        const QString admindir_init_file =
           QDir::toNativeSeparators( QStandardPaths::writableLocation( QStandardPaths::TempLocation )
                                    + "/admindir_init.txt");
        {
            QFile file( admindir_init_file );
            file.open( QIODevice::WriteOnly | QIODevice::Text );
            QTextStream out( &file );
            out << "# Auto-generated by pkg-explorer; do not modify!\n#\nArchitecture: "
                << GetCanonalizedArch()
                <<"\nMaintainer: Made to Order Software Corporation <contact@m2osw.com>\n";
        }

        QDir rdir;
        rdir.mkpath( QDir::toNativeSeparators(root_path) );

        wpkgar::wpkgar_manager manager;
        const QString database_path( QDir::toNativeSeparators( wpkg_admin_path ) );
        manager.set_database_path( database_path.toStdString() );
        manager.create_database( admindir_init_file.toStdString() );

        settings.setValue( "root_path", root_path );
    }
}


}
// namespace Database
