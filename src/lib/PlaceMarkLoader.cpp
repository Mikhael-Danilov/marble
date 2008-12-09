//
// This file is part of the Marble Desktop Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2008      Patrick Spendrin <ps_ml@gmx.de>
//

#include "PlaceMarkLoader.h"

#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QThread>

#include "GeoDataParser.h"
#include "GeoSceneDocument.h"
#include "GeoDataDocument.h"
#include "GeoDataPlacemark.h"
#include "MarbleDirs.h"
#include "MarblePlacemarkModel.h"
#include "PlaceMarkContainer.h"


namespace Marble {

PlaceMarkLoader::PlaceMarkLoader( QObject* parent, MarblePlacemarkModel* model, const QString& file )
 : QThread( parent ), 
   m_model( model ),
   filepath( file ) {
}

void PlaceMarkLoader::run() {
    Q_ASSERT( m_model != 0 && "You have called addPlaceMarkFile before creating a model!" );

    QString defaultcachename;
    QString defaultsrcname;
    QString defaulthomecache;

    if ( !filepath.contains( "\\" ) && !filepath.contains( '/' ) ) {
        defaultcachename = MarbleDirs::path( "placemarks/" + filepath + ".cache" );
        defaultsrcname   = MarbleDirs::path( "placemarks/" + filepath + ".kml");
        defaulthomecache = MarbleDirs::localPath() + "/placemarks/" + filepath + ".cache";
    } else
        exit(0);

    if ( QFile::exists( defaultcachename ) ) {
        qDebug() << "Loading Default Placemark Cache File:" + defaultcachename;

        bool      cacheoutdated = false;
        QDateTime sourceLastModified;
        QDateTime cacheLastModified;

        if ( QFile::exists( defaultsrcname ) ) {
            sourceLastModified = QFileInfo( defaultsrcname ).lastModified();
            cacheLastModified  = QFileInfo( defaultcachename ).lastModified();

            if ( cacheLastModified < sourceLastModified )
                cacheoutdated = true;
        }

        bool loadok = false;

        if ( cacheoutdated == false ) {
            PlaceMarkContainer container;
            loadok = loadFile( defaultcachename, &container );
            if ( loadok )
                m_model->addPlaceMarks( container );
        }
        qDebug() << "Loading ended" << loadok;
        if ( loadok == true )
            return;
    }

    qDebug() << "No recent Default Placemark Cache File available for " << filepath;

    if ( QFile::exists( defaultsrcname ) ) {
        PlaceMarkContainer container;

        // Read the KML file.
        importKml( defaultsrcname, &container );

        qDebug() << "ContainerSize for" << filepath << ":" << container.size();
        // Save the contents in the efficient cache format.
        saveFile( defaulthomecache, &container );

        // ...and finally add it to the PlaceMarkContainer
        m_model->addPlaceMarks( container );
    }
    else {
        qDebug() << "No Default Placemark Source File for " << filepath;
    }
}

static const quint32 MarbleMagicNumber = 0x31415926;

void PlaceMarkLoader::importKml( const QString& filename,
                                 PlaceMarkContainer* placeMarkContainer )
{
    GeoDataParser parser( GeoData_KML );

    QFile file( filename );
    if ( !file.exists() ) {
        qWarning( "File does not exist!" );
        return;
    }

    // Open file in right mode
    file.open( QIODevice::ReadOnly );

    if ( !parser.read( &file ) ) {
        qWarning( "Could not parse file!" );
        return;
    }
    GeoDocument* document = parser.releaseDocument();
    Q_ASSERT( document );

    GeoDataDocument *dataDocument = static_cast<GeoDataDocument*>( document );
    *placeMarkContainer = PlaceMarkContainer( dataDocument->placemarks(), 
                                              QFileInfo( filename ).baseName() );

}

void PlaceMarkLoader::saveFile( const QString& filename,
                                 PlaceMarkContainer* placeMarkContainer )
{
    if ( QDir( MarbleDirs::localPath() + "/placemarks/" ).exists() == false )
        ( QDir::root() ).mkpath( MarbleDirs::localPath() + "/placemarks/" );

    QFile file( filename );
    file.open( QIODevice::WriteOnly );
    QDataStream out( &file );

    // Write a header with a "magic number" and a version
    // out << (quint32)0xA0B0C0D0;
    out << (quint32)MarbleMagicNumber;
    out << (qint32)014;

    out.setVersion( QDataStream::Qt_4_2 );

    qreal lon;
    qreal lat;
    qreal alt;

    PlaceMarkContainer::const_iterator it = placeMarkContainer->constBegin();
    PlaceMarkContainer::const_iterator const end = placeMarkContainer->constEnd();
    for (; it != end; ++it )
    {
        out << (*it)->name();
        (*it)->coordinate( lon, lat, alt );

        out << lon << lat << alt;
        out << QString( (*it)->role() );
        out << QString( (*it)->description() );
        out << QString( (*it)->countryCode() );
        out << (qreal)(*it)->area();
        out << (qint64)(*it)->population();
}
    }

bool PlaceMarkLoader::loadFile( const QString& filename,
                                 PlaceMarkContainer* placeMarkContainer )
{
    QFile file( filename );
    file.open( QIODevice::ReadOnly );
    QDataStream in( &file );

    // Read and check the header
    quint32 magic;
    in >> magic;
    if ( magic != MarbleMagicNumber ) {
        qDebug( "Bad file format!" );
        return false;
    }

    // Read the version
    qint32 version;
    in >> version;
    if ( version < 014 ) {
        qDebug( "Bad file - too old!" );
        return false;
    }
    /*
      if (version > 002) {
      qDebug( "Bad file - too new!" );
      return;
      }
    */

    in.setVersion( QDataStream::Qt_4_2 );

    // Read the data itself
    qreal   lon;
    qreal   lat;
    qreal   alt;
    qreal   area;

    QString  tmpstr;
    qint64   tmpint64;

    QString testo;

    GeoDataPlacemark  *mark;
    while ( !in.atEnd() ) {
        mark = new GeoDataPlacemark;

        in >> tmpstr;
        mark->setName( tmpstr );
        testo = tmpstr;
        in >> lon >> lat >> alt;
        mark->setCoordinate( lon, lat, alt );
        in >> tmpstr;
        mark->setRole( tmpstr.at(0) );
        in >> tmpstr;
        mark->setDescription( tmpstr );
        in >> tmpstr;
        mark->setCountryCode( tmpstr );
        in >> area;
        mark->setArea( area );
        in >> tmpint64;
        mark->setPopulation( tmpint64 );

        placeMarkContainer->append( mark );
    }

    return true;
}

#include "PlaceMarkLoader.moc"
} // namespace Marble
