#pragma once

#include <QString>

namespace Database
{
	QString GetUnameArch();
	QString GetCanonalizedArch();
    QString GetDefaultDbRoot();
    void    InitDatabase();
}

