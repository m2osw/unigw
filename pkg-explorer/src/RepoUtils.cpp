//===============================================================================
// Copyright:   Copyright (c) 2013 Made to Order Software Corp.
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

#include "RepoUtils.h"

#include <algorithm> 

using namespace wpkgar;


namespace RepoUtils
{
    wpkg_filename::uri_filename GetSourcesUri( wpkgar_manager* manager )
    {
        wpkg_filename::uri_filename name( manager->get_database_path() );
        return name.append_child( "core/sources.list" );
    }

    QString SourceToQString( const wpkgar::source& src, const bool uri_only )
    {
        QStringList components;
        for( int i = 0; i < src.get_component_size(); i++ )
        {
            components << src.get_component( i ).c_str();
        }

        if( uri_only )
        {
            const QString uri = QString("%2/%3/%4")
                .arg(src.get_uri().c_str())
                .arg(src.get_distribution().c_str())
                .arg(components.join("/"))
                ;
            return uri;
        }

        const QString full_source = QString("%1 %2 %3 %4")
            .arg(src.get_type().c_str())
            .arg(src.get_uri().c_str())
            .arg(src.get_distribution().c_str())
            .arg(components.join(" "))
            ;
        return full_source;
    }


    wpkgar::source QStringToSource( const QString& str )
    {
        wpkgar::source src;

        QStringList list = str.split( " " );
        src.set_type         ( list[0].toStdString() );
        src.set_uri          ( list[1].toStdString() );
        src.set_distribution ( list[2].toStdString() );

        list.erase( list.begin(), list.begin()+3 );
        std::for_each( list.begin(), list.end(), [&](const QString& comp)
        {
            src.add_component( comp.toStdString() );
        });

        return src;
    }

    QStringList ReadSourcesList( wpkgar::wpkgar_manager* manager, const bool uri_only )
    {
        QStringList sourceList;
        wpkg_filename::uri_filename name( GetSourcesUri( manager ) );
        if( name.exists() )
        {
            memfile::memory_file				sources_file;
            wpkgar_repository					repository( manager );
            source_vector_t	sources;

            sources_file.read_file( name );
            repository.read_sources( sources_file, sources );

            std::for_each( sources.begin(), sources.end(),
                [&]( const source& src )
                {
                    sourceList << SourceToQString( src, uri_only );
                }
            );
        }

        return sourceList;
    }

    void WriteSourcesList( wpkgar::wpkgar_manager* manager, const QStringList& contents )
    {
		wpkgar_repository repository( manager );

        wpkg_filename::uri_filename name( GetSourcesUri( manager ) );

        source_vector_t sources;
		memfile::memory_file sources_file;

		sources_file.create( memfile::memory_file::file_format_other );
        std::for_each( contents.begin(), contents.end(), [&]( const QString& entry )
		{
			sources_file.printf( "%s\n", entry.toStdString().c_str() );
		});

		repository.write_sources( sources_file, sources );
		sources_file.write_file( name );
    }
}
// namespace


// vim: ts=4 sw=4 et

