//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2009      Andrew Manson <g.real.ate@gmail.com>
//

#include <QObject>

#include "GeoDataParser.h"
#include "GeoDataDocument.h"
#include "GeoDataColorStyle.h"
#include "GeoWriter.h"

#include <QDir>
#include <QFile>
#include <QTest>
#include <QTemporaryFile>
#include <QTextStream>

using namespace Marble;

class TestGeoDataWriter : public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void countFeatures_data();
    void countFeatures();
    void saveFile_data();
    void saveFile();
    void saveAndLoad_data();
    void saveAndLoad();
    void saveAndCompare_data();
    void saveAndCompare();
    void cleanupTestCase();
private:
    QDir dataDir;
    QMap<QString, QSharedPointer<GeoDataParser> > parsers;
    QStringList m_testFiles;
};

Q_DECLARE_METATYPE( QSharedPointer<GeoDataParser> )

void TestGeoDataWriter::initTestCase()
{
    dataDir = QDir( TESTSRCDIR );
    dataDir.cd( "data" );
    //check there are files in the data dir
    QVERIFY( dataDir.count() > 0 );

    //test the loading of each file in the data dir
    foreach( const QString &filename, dataDir.entryList(QStringList() << "*.kml", QDir::Files) ){
        QFile file( dataDir.filePath(filename));
        QVERIFY( file.exists() );

        // Create parsers
        GeoDataParser* parser = new GeoDataParser( GeoData_KML );
        QSharedPointer<GeoDataParser>parserPointer ( parser );

        // Open the files and verify
        QVERIFY( file.open( QIODevice::ReadOnly ) );
        QVERIFY2( parser->read( &file ), filename.toLatin1() );

        parsers.insert( filename, parserPointer );
        m_testFiles << filename;
        file.close();
    }
}

void TestGeoDataWriter::countFeatures_data()
{
    QTest::addColumn<QSharedPointer<GeoDataParser> >("parser");
    foreach( const QString &file, m_testFiles ) {
        QTest::newRow(file.toStdString().c_str()) << parsers.value(file);
    }
}

void TestGeoDataWriter::countFeatures()
{
    //count the features in the loaded KML file
    QFETCH(QSharedPointer<GeoDataParser>, parser);
    GeoDataDocument* document = dynamic_cast<GeoDataDocument*>(parser->activeDocument());
    QVERIFY( document );

    // there should be at least one child in each document
    QVERIFY( document->size() > 0 );
}

void TestGeoDataWriter::saveFile_data()
{
    QTest::addColumn<QSharedPointer<GeoDataParser> >( "parser" );
    foreach( const QString &file, m_testFiles ) {
        QTest::newRow(file.toStdString().c_str()) << parsers.value(file);
    }
}

void TestGeoDataWriter::saveFile()
{
    QFETCH( QSharedPointer<GeoDataParser>, parser );

    //attempt to save a file using the GeoWriter
    QTemporaryFile tempFile;

    GeoWriter writer;
    //FIXME: a better way to do this?
    writer.setDocumentType( "http://earth.google.com/kml/2.2" );

    // Open file in right mode
    QVERIFY( tempFile.open() );

    QVERIFY( writer.write( &tempFile, &(*dynamic_cast<GeoDataFeature*>(parser->activeDocument() ) ) ) );

}

void TestGeoDataWriter::saveAndLoad_data()
{
    QTest::addColumn<QSharedPointer<GeoDataParser> >("parser");
    foreach( const QString &file, m_testFiles ) {
        QTest::newRow(file.toStdString().c_str()) << parsers.value(file);
    }
}

void TestGeoDataWriter::saveAndLoad()
{
    //Save the file and then verify loading it again
    QFETCH( QSharedPointer<GeoDataParser>, parser );

    QTemporaryFile tempFile;
    GeoWriter writer;
    //FIXME: a better way to do this?
    writer.setDocumentType( "http://earth.google.com/kml/2.2" );

    // Open file in right mode
    QVERIFY( tempFile.open() );

    QVERIFY( writer.write( &tempFile, &( *dynamic_cast<GeoDataFeature*>(parser->activeDocument() ) ) ) );

    GeoDataParser resultParser( GeoData_KML );

    tempFile.reset();
    QVERIFY( resultParser.read( &tempFile ) );
}

void TestGeoDataWriter::saveAndCompare_data()
{
    QTest::addColumn<QSharedPointer<GeoDataParser> >("parser");
    QTest::addColumn<QString>("original");

    /** @todo Look into why these two files fail */
    QStringList const blacklist = QStringList() << "CDATATest.kml" << "TimeStamp.kml";
    foreach( const QString &file, m_testFiles ) {
        if ( !blacklist.contains( file ) ) {
            QTest::newRow(file.toStdString().c_str()) << parsers.value(file) << file;
        }
    }
}

void TestGeoDataWriter::saveAndCompare()
{
    //save the file and compare it to the original
    QFETCH( QSharedPointer<GeoDataParser>, parser );
    QFETCH( QString, original );

    //attempt to save a file using the GeoWriter
    QTemporaryFile tempFile;

    GeoWriter writer;
    //FIXME: a better way to do this?
    writer.setDocumentType( "http://earth.google.com/kml/2.2" );

    // Open file in right mode
    QVERIFY( tempFile.open() );

    QVERIFY( writer.write( &tempFile, &( *dynamic_cast<GeoDataFeature*>(parser->activeDocument() ) ) ) );

    QFile file( dataDir.filePath( original ) );
    QVERIFY( file.open( QIODevice::ReadOnly ) );
    QVERIFY( tempFile.reset() );
    QTextStream oldFile( &file );
    QTextStream newFile( &tempFile );

    QCOMPARE( newFile.readAll().simplified(), oldFile.readAll().simplified() );
}

void TestGeoDataWriter::cleanupTestCase()
{
    QMap<QString, QSharedPointer<GeoDataParser> >::iterator itpoint = parsers.begin();
    QMap<QString, QSharedPointer<GeoDataParser> >::iterator const endpoint = parsers.end();
    for (; itpoint != endpoint; ++itpoint ) {
        delete itpoint.value()->releaseDocument();
    }
}

QTEST_MAIN( TestGeoDataWriter )
#include "TestGeoDataWriter.moc"

