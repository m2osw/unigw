#pragma once

#include <QString>

namespace Database
{
	QString GetUnameArch();
	QString GetCanonalizedArch();
    QString GetDefaultDbRoot();
	void    SetDefaultDbRoot( const QString& new_root );
    void    InitDatabase();
}

