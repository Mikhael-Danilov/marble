//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2007        Inge Wallin   <ingwa@kde.org>
// Copyright 2007-2012   Torsten Rahn  <rahn@kde.org>
// Copyright 2009        Patrick Spendrin <ps_ml@gmx.de>
// Copyright 2012        Cezar Mocan   <mocancezar@gmail.com>
//

// Local
#include "SphericalProjection.h"
#include "AbstractProjection_p.h"

#include "MarbleDebug.h"

// Marble
#include "ViewportParams.h"
#include "GeoDataPoint.h"
#include "GeoDataLineString.h"
#include "GeoDataCoordinates.h"
#include "MarbleGlobal.h"

#define SAFE_DISTANCE

// Maximum amount of nodes that are created automatically between actual nodes.
static const int maxTessellationNodes = 200;

namespace Marble
{

class SphericalProjectionPrivate : public AbstractProjectionPrivate
{
  public:
    enum LineStringFlag {
        PlainLineString = 0x00,
        ClosedLineString = 0x01
    };

    explicit SphericalProjectionPrivate( SphericalProjection * parent );

    // This method tessellates a line segment in a way that the line segment
    // follows great circles. The count parameter specifies the
    // number of nodes generated for the polygon. If the
    // clampToGround flag is added the polygon contains count + 2
    // nodes as the clamped down start and end node get added.

    void tessellateLineSegment(  const GeoDataCoordinates &aCoords,
                                qreal ax, qreal ay,
                                const GeoDataCoordinates &bCoords,
                                qreal bx, qreal by,
                                LineStringFlag lineStringFlag,
                                QVector<QPolygonF*> &polygons,
                                const ViewportParams *viewport,
                                TessellationFlags f = 0 ) const;

    void processTessellation(   const GeoDataCoordinates &previousCoords,
                               const GeoDataCoordinates &currentCoords,
                               int count,
                               LineStringFlag lineStringFlag,
                               QVector<QPolygonF*> &polygons,
                               const ViewportParams *viewport,
                               TessellationFlags f = 0 ) const;

    void crossHorizon( const GeoDataCoordinates & bCoord,
                       LineStringFlag lineStringFlag,
                       QVector<QPolygonF*> &polygons,
                       const ViewportParams *viewport ) const;

    virtual bool lineStringToPolygon( const GeoDataLineString &lineString,
                              const ViewportParams *viewport,
                              QVector<QPolygonF*> &polygons ) const;

    void horizonToPolygon( const ViewportParams *viewport,
                           const GeoDataCoordinates & disappearCoords,
                           const GeoDataCoordinates & reappearCoords,
                           QPolygonF* ) const;

    GeoDataCoordinates findHorizon( const GeoDataCoordinates & previousCoords,
                                    const GeoDataCoordinates & currentCoords,
                                    const ViewportParams *viewport,
                                    TessellationFlags f = 0,
                                    int recursionCounter = 0 ) const;

    bool globeHidesPoint( const GeoDataCoordinates &coordinates,
                          const ViewportParams *viewport ) const;

    Q_DECLARE_PUBLIC( SphericalProjection )
};

SphericalProjection::SphericalProjection()
    : AbstractProjection( new SphericalProjectionPrivate( this ) )
{
    setMinLat( minValidLat() );
    setMaxLat( maxValidLat() );
}

SphericalProjection::SphericalProjection( SphericalProjectionPrivate *dd )
        : AbstractProjection( dd )
{
    setMinLat( minValidLat() );
    setMaxLat( maxValidLat() );
}

SphericalProjection::~SphericalProjection()
{
}


SphericalProjectionPrivate::SphericalProjectionPrivate( SphericalProjection * parent )
        : AbstractProjectionPrivate( parent )
{

}


bool SphericalProjection::repeatableX() const
{
    return false;
}

qreal SphericalProjection::maxValidLat() const
{
    return +90.0 * DEG2RAD;
}

qreal SphericalProjection::minValidLat() const
{
    return -90.0 * DEG2RAD;
}

bool SphericalProjection::screenCoordinates( const GeoDataCoordinates &coordinates, 
                                             const ViewportParams *viewport,
                                             qreal &x, qreal &y, bool &globeHidesPoint ) const
{
    qreal       absoluteAltitude = coordinates.altitude() + EARTH_RADIUS;
    Quaternion  qpos             = coordinates.quaternion();

    qpos.rotateAroundAxis( viewport->planetAxisMatrix() );

    qreal      pixelAltitude = ( ( viewport->radius() ) 
                                  / EARTH_RADIUS * absoluteAltitude );
    if ( coordinates.altitude() < 10000 ) {
        // Skip placemarks at the other side of the earth.
        if ( qpos.v[Q_Z] < 0 ) {
            globeHidesPoint = true;
            return false;
        }
    }
    else {
        qreal  earthCenteredX = pixelAltitude * qpos.v[Q_X];
        qreal  earthCenteredY = pixelAltitude * qpos.v[Q_Y];
        qreal  radius         = viewport->radius();

        // Don't draw high placemarks (e.g. satellites) that aren't visible.
        if ( qpos.v[Q_Z] < 0
             && ( ( earthCenteredX * earthCenteredX
                    + earthCenteredY * earthCenteredY )
                  < radius * radius ) ) {
            globeHidesPoint = true;
            return false;
        }
    }

    // Let (x, y) be the position on the screen of the placemark..
    x = ((qreal)(viewport->width())  / 2 + pixelAltitude * qpos.v[Q_X]);
    y = ((qreal)(viewport->height()) / 2 - pixelAltitude * qpos.v[Q_Y]);

    // Skip placemarks that are outside the screen area
    if ( x < 0 || x >= viewport->width() || y < 0 || y >= viewport->height() ) {
        globeHidesPoint = false;
        return false;
    }

    globeHidesPoint = false;
    return true;
}

bool SphericalProjection::screenCoordinates( const GeoDataCoordinates &coordinates,
                                             const ViewportParams *viewport,
                                             qreal *x, qreal &y,
                                             int &pointRepeatNum,
                                             const QSizeF& size,
                                             bool &globeHidesPoint ) const
{
    pointRepeatNum = 0;
    bool visible = screenCoordinates( coordinates, viewport, *x, y, globeHidesPoint );

    // Skip placemarks that are outside the screen area
    if ( *x + size.width() / 2.0 < 0.0 || *x >= viewport->width() + size.width() / 2.0 
         || y + size.height() / 2.0 < 0.0 || y >= viewport->height() + size.height() / 2.0 )
    {
        globeHidesPoint = false;
        return false;
    }

    // This projection doesn't have any repetitions, 
    // so the number of screen points referring to the geopoint is one.
    pointRepeatNum = 1;
    return visible;
}


bool SphericalProjection::geoCoordinates( const int x, const int y,
                                          const ViewportParams *viewport,
                                          qreal& lon, qreal& lat,
                                          GeoDataCoordinates::Unit unit ) const
{
    const qreal  inverseRadius = 1.0 / (qreal)(viewport->radius());

    const qreal qx = +(qreal)( x - viewport->width()  / 2 ) * inverseRadius;
    const qreal qy = -(qreal)( y - viewport->height() / 2 ) * inverseRadius;

    if ( 1 <= qx * qx + qy * qy ) {
        return false;
    }

    const qreal qz = sqrt( 1 - qx * qx - qy * qy );

    Quaternion  qpos( 0.0, qx, qy, qz );
    qpos.rotateAroundAxis( viewport->planetAxis() );
    qpos.getSpherical( lon, lat );

    if ( unit == GeoDataCoordinates::Degree ) {
        lon *= RAD2DEG;
        lat *= RAD2DEG;
    }

    return true;
}

GeoDataLatLonAltBox SphericalProjection::latLonAltBox( const QRect& screenRect,
                                                       const ViewportParams *viewport ) const
{
    // For the case where the whole viewport gets covered there is a 
    // pretty dirty and generic detection algorithm:
    GeoDataLatLonAltBox latLonAltBox = AbstractProjection::latLonAltBox( screenRect, viewport );

    // If the whole globe is visible we can easily calculate
    // analytically the lon-/lat- range.
    qreal pitch = GeoDataCoordinates::normalizeLat( viewport->planetAxis().pitch() );

    if ( 2.0 * viewport->radius() <= viewport->height()
         &&  2.0 * viewport->radius() <= viewport->width() )
    { 
        // Unless the planetaxis is in the screen plane the allowed longitude range
        // covers full -180 deg to +180 deg:
        if ( pitch > 0.0 && pitch < +M_PI ) {
            latLonAltBox.setNorth( +fabs( M_PI / 2.0 - fabs( pitch ) ) );
            latLonAltBox.setSouth( -M_PI / 2.0 );
        }
        if ( pitch < 0.0 && pitch > -M_PI ) {
            latLonAltBox.setWest(  -M_PI );
            latLonAltBox.setEast(  +M_PI );
            latLonAltBox.setNorth( +M_PI / 2.0 );
            latLonAltBox.setSouth( -fabs( M_PI / 2.0 - fabs( pitch ) ) );
        }

        // Last but not least we deal with the rare case where the
        // globe is fully visible and pitch = 0.0 or pitch = -M_PI or
        // pitch = +M_PI
        if ( pitch == 0.0 || pitch == -M_PI || pitch == +M_PI ) {
            qreal yaw = viewport->planetAxis().yaw();
            latLonAltBox.setWest( GeoDataCoordinates::normalizeLon( yaw - M_PI / 2.0 ) );
            latLonAltBox.setEast( GeoDataCoordinates::normalizeLon( yaw + M_PI / 2.0 ) );
            latLonAltBox.setNorth( +M_PI / 2.0 );
            latLonAltBox.setSouth( -M_PI / 2.0 );
        }
    }

    // Now we check whether maxLat (e.g. the north pole) gets displayed
    // inside the viewport to get more accurate values for east and west.

    // We need a point on the screen at maxLat that definitely gets displayed:
    qreal averageLongitude = ( latLonAltBox.west() + latLonAltBox.east() ) / 2.0;

    GeoDataCoordinates maxLatPoint( averageLongitude, maxLat(), 0.0, GeoDataCoordinates::Radian );
    GeoDataCoordinates minLatPoint( averageLongitude, minLat(), 0.0, GeoDataCoordinates::Radian );

    qreal dummyX, dummyY; // not needed
    bool dummyVal;

    if ( screenCoordinates( maxLatPoint, viewport, dummyX, dummyY, dummyVal ) ||
         screenCoordinates( minLatPoint, viewport, dummyX, dummyY, dummyVal ) ) {
        latLonAltBox.setWest( -M_PI );
        latLonAltBox.setEast( +M_PI );
    }

    return latLonAltBox;
}


bool SphericalProjection::mapCoversViewport( const ViewportParams *viewport ) const
{
    qint64  radius = viewport->radius();
    qint64  width  = viewport->width();
    qint64  height = viewport->height();

    // This first test is a quick one that will catch all really big
    // radii and prevent overflow in the real test.
    if ( radius > width + height )
        return true;

    // This is the real test.  The 4 is because we are really
    // comparing to width/2 and height/2.
    if ( 4 * radius * radius >= width * width + height * height )
        return true;

    return false;
}

QPainterPath SphericalProjection::mapShape( const ViewportParams *viewport ) const
{
    int  radius    = viewport->radius();
    int  imgWidth  = viewport->width();
    int  imgHeight = viewport->height();

    QPainterPath fullRect;
    fullRect.addRect(  0 , 0 , imgWidth, imgHeight );

    // If the globe covers the whole image, then the projected region represents
    // all of the image.
    // Otherwise the active region has got the shape of the visible globe.

    if ( !viewport->mapCoversViewport() ) {
        QPainterPath mapShape;
        mapShape.addEllipse(
            imgWidth  / 2 - radius,
            imgHeight / 2 - radius,
            2 * radius,
            2 * radius );
        return mapShape.intersected( fullRect );
    }

    return fullRect;
}

bool SphericalProjection::screenCoordinates( const GeoDataLineString &lineString,
                                                  const ViewportParams *viewport,
                                                  QVector<QPolygonF *> &polygons ) const
{

    Q_D( const SphericalProjection );
    // Compare bounding box size of the line string with the angularResolution
    // Immediately return if the latLonAltBox is smaller.
    if ( !viewport->resolves( lineString.latLonAltBox() ) ) {
//      mDebug() << "Object too small to be resolved";
        return false;
    }

    d->lineStringToPolygon( lineString, viewport, polygons );
    return true;
}

void SphericalProjectionPrivate::tessellateLineSegment( const GeoDataCoordinates &aCoords,
                                                qreal ax, qreal ay,
                                                const GeoDataCoordinates &bCoords,
                                                qreal bx, qreal by,
                                                LineStringFlag lineStringFlag,
                                                QVector<QPolygonF*> &polygons,
                                                const ViewportParams *viewport,
                                                TessellationFlags f) const
{
    // We take the manhattan length as a distance approximation
    // that can be too big by a factor of sqrt(2)
    qreal distance = fabs((bx - ax)) + fabs((by - ay));
#ifdef SAFE_DISTANCE
    // Interpolate additional nodes if the line segment that connects the
    // current or previous nodes might cross the viewport.
    // The latter can pretty safely be excluded for most projections if both points
    // are located on the same side relative to the viewport boundaries and if they are
    // located more than half the line segment distance away from the viewport.
    const qreal safeDistance = - 0.5 * distance;
    if (   !( bx < safeDistance && ax < safeDistance )
        || !( by < safeDistance && ay < safeDistance )
        || !( bx + safeDistance > viewport->width()
            && ax + safeDistance > viewport->width() )
        || !( by + safeDistance > viewport->height()
            && ay + safeDistance > viewport->height() )
    )
    {
#endif
        bool const smallScreen = MarbleGlobal::getInstance()->profiles() & MarbleGlobal::SmallScreen;
        int const finalTessellationPrecision = smallScreen ? 3 * tessellationPrecision : tessellationPrecision;

        // Let the line segment follow the spherical surface
        // if the distance between the previous point and the current point
        // on screen is too big

        if ( distance > finalTessellationPrecision ) {
            const int tessellatedNodes = qMin<int>( distance / finalTessellationPrecision, maxTessellationNodes );

            processTessellation( aCoords, bCoords,
                                 tessellatedNodes,
                                 lineStringFlag,
                                 polygons,
                                 viewport,
                                 f );
        }
        else {
            crossHorizon( bCoords, lineStringFlag, polygons, viewport );
        }
#ifdef SAFE_DISTANCE
    }
#endif
}


void SphericalProjectionPrivate::processTessellation( const GeoDataCoordinates &previousCoords,
                                                    const GeoDataCoordinates &currentCoords,
                                                    int tessellatedNodes,
                                                    LineStringFlag lineStringFlag,
                                                    QVector<QPolygonF*> &polygons,
                                                    const ViewportParams *viewport,
                                                    TessellationFlags f) const
{

    const bool clampToGround = f.testFlag( FollowGround );
    const bool followLatitudeCircle = f.testFlag( RespectLatitudeCircle )
                                      && previousCoords.latitude() == currentCoords.latitude();

    // Calculate steps for tessellation: lonDiff and altDiff
    qreal lonDiff = 0.0;
    if ( followLatitudeCircle ) {
        const int previousSign = previousCoords.longitude() > 0 ? 1 : -1;
        const int currentSign = currentCoords.longitude() > 0 ? 1 : -1;

        lonDiff = currentCoords.longitude() - previousCoords.longitude();
        if ( previousSign != currentSign
             && fabs(previousCoords.longitude()) + fabs(currentCoords.longitude()) > M_PI ) {
            if ( previousSign > currentSign ) {
                // going eastwards ->
                lonDiff += 2 * M_PI ;
            } else {
                // going westwards ->
                lonDiff -= 2 * M_PI;
            }
        }
    }

    const qreal altDiff = currentCoords.altitude() - previousCoords.altitude();

    // Create the tessellation nodes.
    GeoDataCoordinates previousTessellatedCoords = previousCoords;
    for ( int i = 1; i <= tessellatedNodes; ++i ) {
        const qreal t = (qreal)(i) / (qreal)( tessellatedNodes + 1 );

        // interpolate the altitude, too
        const qreal altitude = clampToGround ? 0 : altDiff * t + previousCoords.altitude();

        qreal lon = 0.0;
        qreal lat = 0.0;
        if ( followLatitudeCircle ) {
            // To tessellate along latitude circles use the
            // linear interpolation of the longitude.
            lon = lonDiff * t + previousCoords.longitude();
            lat = previousTessellatedCoords.latitude();
        }
        else {
            // To tessellate along great circles use the
            // normalized linear interpolation ("NLERP") for latitude and longitude.
            const Quaternion itpos = Quaternion::nlerp( previousCoords.quaternion(), currentCoords.quaternion(), t );
            itpos. getSpherical( lon, lat );
        }

        const GeoDataCoordinates currentTessellatedCoords( lon, lat, altitude );
        crossHorizon( currentTessellatedCoords, lineStringFlag, polygons, viewport );
        previousTessellatedCoords = currentTessellatedCoords;
    }

    // For the clampToGround case add the "current" coordinate after adding all other nodes.
    GeoDataCoordinates currentModifiedCoords( currentCoords );
    if ( clampToGround ) {
        currentModifiedCoords.setAltitude( 0.0 );
    }
    crossHorizon( currentModifiedCoords, lineStringFlag, polygons, viewport );
}

void SphericalProjectionPrivate::crossHorizon( const GeoDataCoordinates & bCoord,
                                              LineStringFlag lineStringFlag,
                                              QVector<QPolygonF*> &polygons,
                                              const ViewportParams *viewport ) const
{
    qreal x, y;
    bool globeHidesPoint;

    Q_Q( const AbstractProjection );

    q->screenCoordinates( bCoord, viewport, x, y, globeHidesPoint );

    if( !globeHidesPoint ) {
        *polygons.last() << QPointF( x, y );
    }
    else {
        if ( !polygons.last()->isEmpty() && lineStringFlag != ClosedLineString  ) {
            QPolygonF *path = new QPolygonF;
            polygons.append( path );
        }
    }
}

bool SphericalProjectionPrivate::lineStringToPolygon( const GeoDataLineString &lineString,
                                              const ViewportParams *viewport,
                                              QVector<QPolygonF *> &polygons ) const
{
    Q_Q( const SphericalProjection );

    const TessellationFlags f = lineString.tessellationFlags();

    qreal x = 0;
    qreal y = 0;
    bool globeHidesPoint = false;

    qreal previousX = -1.0;
    qreal previousY = -1.0;
    bool previousGlobeHidesPoint = false;

    qreal horizonX = -1.0;
    qreal horizonY = -1.0;

    polygons.append( new QPolygonF );

    GeoDataLineString::ConstIterator itCoords = lineString.constBegin();
    GeoDataLineString::ConstIterator itPreviousCoords = lineString.constBegin();

    // Some projections display the earth in a way so that there is a
    // foreside and a backside.
    // The horizon is the line (usually a circle) which separates both
    // sides from each other and resembles the map shape.
    GeoDataCoordinates horizonCoords;

    // A horizon pair is a pair of two subsequent horizon crossings:
    // The first one describes the point where a line string disappears behind the horizon.
    // and where horizonPair is set to true.
    // The second one describes the point where the line string reappears.
    // In this case the two points are connected and horizonPair is set to false again.
    bool horizonPair = false;
    GeoDataCoordinates horizonDisappearCoords;

    // If the first horizon crossing in a line string describes the appearance of
    // a line string then we call it a "horizon orphan" and horizonOrphan is set to true.
    // In this case once the last horizon crossing in the line string is reached
    // it needs to be connected to the orphan.
    bool horizonOrphan = false;
    GeoDataCoordinates horizonOrphanCoords;

    GeoDataLineString::ConstIterator itBegin = lineString.constBegin();
    GeoDataLineString::ConstIterator itEnd = lineString.constEnd();

    bool processingLastNode = false;

    // We use a while loop to be able to cover linestrings as well as linear rings:
    // Linear rings require to tessellate the path from the last node to the first node
    // which isn't really convenient to achieve with a for loop ...

    const bool isLong = lineString.size() > 50;
    const int maximumDetail = ( viewport->radius() > 5000 ) ? 5 :
                              ( viewport->radius() > 2500 ) ? 4 :
                              ( viewport->radius() > 1000 ) ? 3 :
                              ( viewport->radius() >  600 ) ? 2 :
                              ( viewport->radius() >   50 ) ? 1 :
                                                              0;

    while ( itCoords != itEnd )
    {

        // Optimization for line strings with a big amount of nodes
        bool skipNode = itCoords != itBegin && isLong && !processingLastNode &&
                ( (*itCoords).detail() > maximumDetail
                  || viewport->resolves( *itPreviousCoords, *itCoords ) );

        if ( !skipNode ) {

            q->screenCoordinates( *itCoords, viewport, x, y, globeHidesPoint );

            // Initializing variables that store the values of the previous iteration
            if ( !processingLastNode && itCoords == itBegin ) {
                previousGlobeHidesPoint = globeHidesPoint;
                itPreviousCoords = itCoords;
                previousX = x;
                previousY = y;
            }

            // Check for the "horizon case" (which is present e.g. for the spherical projection
            const bool isAtHorizon = ( globeHidesPoint || previousGlobeHidesPoint ) &&
                                     ( globeHidesPoint !=  previousGlobeHidesPoint );
     
            if ( isAtHorizon ) {
                // Handle the "horizon case"
                horizonCoords = findHorizon( *itPreviousCoords, *itCoords, viewport, f );

                if ( lineString.isClosed() ) {
                    if ( horizonPair ) {
                        horizonToPolygon( viewport, horizonDisappearCoords, horizonCoords, polygons.last() );
                        horizonPair = false;
                    }
                    else {
                        if ( globeHidesPoint ) {
                            horizonDisappearCoords = horizonCoords;
                            horizonPair = true;
                        }
                        else {
                            horizonOrphanCoords = horizonCoords;
                            horizonOrphan = true;
                        }
                    }
                }

                q->screenCoordinates( horizonCoords, viewport, horizonX, horizonY );

                // If the line appears on the visible half we need
                // to add an interpolated point at the horizon as the previous point.
                if ( previousGlobeHidesPoint ) {
                    *polygons.last() << QPointF( horizonX, horizonY );
                }
            }

            // This if-clause contains the section that tessellates the line
            // segments of a linestring. If you are about to learn how the code of
            // this class works you can safely ignore this section for a start.

            if ( lineString.tessellate() ) {
                LineStringFlag lineStringFlag = lineString.isClosed() ? ClosedLineString : PlainLineString;
                if ( !isAtHorizon ) {
                    tessellateLineSegment( *itPreviousCoords, previousX, previousY,
                                           *itCoords, x, y,
                                           lineStringFlag, polygons, viewport,
                                           f );
                }
                else {
                    // Connect the interpolated  point at the horizon with the
                    // current or previous point in the line. 
                    if ( previousGlobeHidesPoint ) {
                        tessellateLineSegment( horizonCoords, horizonX, horizonY,
                                               *itCoords, x, y,
                                               lineStringFlag, polygons, viewport,
                                               f );
                    }
                    else {
                        tessellateLineSegment( *itPreviousCoords, previousX, previousY,
                                               horizonCoords, horizonX, horizonY,
                                               lineStringFlag, polygons, viewport,
                                               f );
                    }
                }
            }
            else {
                if ( !globeHidesPoint ) {
                    *polygons.last() << QPointF( x, y );
                }
                else {
                    if ( !previousGlobeHidesPoint && isAtHorizon ) {
                        *polygons.last() << QPointF( horizonX, horizonY );
                    }
                }
            }

            if ( globeHidesPoint ) {
                if (   !previousGlobeHidesPoint
                    && !lineString.isClosed()
                    ) {
                    polygons.append( new QPolygonF );
                }
            }

            previousGlobeHidesPoint = globeHidesPoint;
            itPreviousCoords = itCoords;
            previousX = x;
            previousY = y;
        }

        // Here we modify the condition to be able to process the
        // first node after the last node in a LinearRing.

        if ( processingLastNode ) {
            break;
        }
        ++itCoords;

        if ( itCoords == itEnd  && lineString.isClosed() ) {
            itCoords = itBegin;
            processingLastNode = true;
        }
    }

    // In case of horizon crossings, make sure that we always get a
    // polygon closed correctly.
    if ( horizonOrphan && lineString.isClosed() ) {
        horizonToPolygon( viewport, horizonCoords, horizonOrphanCoords, polygons.last() );
    }

    if ( polygons.last()->size() <= 1 ){
        polygons.pop_back(); // Clean up "unused" empty polygon instances
    }

    return polygons.isEmpty();
}

void SphericalProjectionPrivate::horizonToPolygon( const ViewportParams *viewport,
                                           const GeoDataCoordinates & disappearCoords,
                                           const GeoDataCoordinates & reappearCoords,
                                           QPolygonF * polygon ) const
{
    qreal x, y;

    const qreal imageHalfWidth  = viewport->width() / 2;
    const qreal imageHalfHeight = viewport->height() / 2;

    bool dummyGlobeHidesPoint = false;

    Q_Q( const SphericalProjection );
    // Calculate the angle of the position vectors of both coordinates
    q->screenCoordinates( disappearCoords, viewport, x, y, dummyGlobeHidesPoint );
    qreal alpha = atan2( y - imageHalfHeight,
                         x - imageHalfWidth );

    q->screenCoordinates( reappearCoords, viewport, x, y, dummyGlobeHidesPoint );
    qreal beta =  atan2( y - imageHalfHeight,
                         x - imageHalfWidth );

    // Calculate the difference between both
    qreal diff = GeoDataCoordinates::normalizeLon( beta - alpha );

    qreal sgndiff = diff < 0 ? -1 : 1;

    const qreal arcradius = viewport->radius();
    const int itEnd = fabs(diff * RAD2DEG);

    // Create a polygon that resembles an arc between the two position vectors
    for ( int it = 1; it <= itEnd; ++it ) {
        const qreal angle = alpha + DEG2RAD * sgndiff * it;
        const qreal itx = imageHalfWidth  +  arcradius * cos( angle );
        const qreal ity = imageHalfHeight +  arcradius * sin( angle );
        *polygon << QPointF( itx, ity );
    }
}


GeoDataCoordinates SphericalProjectionPrivate::findHorizon( const GeoDataCoordinates & previousCoords,
                                                    const GeoDataCoordinates & currentCoords,
                                                    const ViewportParams *viewport,
                                                    TessellationFlags f,
                                                    int recursionCounter ) const
{
    bool currentHide = globeHidesPoint( currentCoords, viewport ) ;

    if ( recursionCounter > 20 ) {
        return currentHide ? previousCoords : currentCoords;
    }
    ++recursionCounter;

    bool followLatitudeCircle = false;

    // Calculate steps for tessellation: lonDiff and altDiff
    qreal lonDiff = 0.0;
    qreal previousLongitude = 0.0;
    qreal previousLatitude = 0.0;

    if ( f.testFlag( RespectLatitudeCircle ) ) {
        previousCoords.geoCoordinates( previousLongitude, previousLatitude );
        qreal previousSign = previousLongitude > 0 ? 1 : -1;

        qreal currentLongitude = 0.0;
        qreal currentLatitude = 0.0;
        currentCoords.geoCoordinates( currentLongitude, currentLatitude );
        qreal currentSign = currentLongitude > 0 ? 1 : -1;

        if ( previousLatitude == currentLatitude ) {
            followLatitudeCircle = true;

            lonDiff = currentLongitude - previousLongitude;
            if ( previousSign != currentSign
                 && fabs(previousLongitude) + fabs(currentLongitude) > M_PI ) {
                if ( previousSign > currentSign ) {
                    // going eastwards ->
                    lonDiff += 2 * M_PI ;
                } else {
                    // going westwards ->
                    lonDiff -= 2 * M_PI;
                }
            }

        }
        else {
//            mDebug() << "Don't FollowLatitudeCircle";
        }
    }

    qreal  lon = 0.0;
    qreal  lat = 0.0;

    qreal altDiff = currentCoords.altitude() - previousCoords.altitude();

    if ( followLatitudeCircle ) {
        // To tessellate along latitude circles use the
        // linear interpolation of the longitude.
        lon = lonDiff * 0.5 + previousLongitude;
        lat = previousLatitude;
    }
    else {
        // To tessellate along great circles use the
        // normalized linear interpolation ("NLERP") for latitude and longitude.
        const Quaternion itpos = Quaternion::nlerp( previousCoords.quaternion(), currentCoords.quaternion(), 0.5 );
        itpos. getSpherical( lon, lat );
    }

    qreal altitude = previousCoords.altitude() + 0.5 * altDiff;

    GeoDataCoordinates horizonCoords( lon, lat, altitude );

    bool horizonHide = globeHidesPoint( horizonCoords, viewport );

    if ( horizonHide != currentHide ) {
        return findHorizon( horizonCoords, currentCoords, viewport, f, recursionCounter );
    }

    return findHorizon( previousCoords, horizonCoords, viewport, f, recursionCounter );
}


bool SphericalProjectionPrivate::globeHidesPoint( const GeoDataCoordinates &coordinates,
                                          const ViewportParams *viewport ) const
{
    qreal       absoluteAltitude = coordinates.altitude() + EARTH_RADIUS;
    Quaternion  qpos             = coordinates.quaternion();

    qpos.rotateAroundAxis( viewport->planetAxisMatrix() );

    qreal      pixelAltitude = ( ( viewport->radius() )
                                  / EARTH_RADIUS * absoluteAltitude );
    if ( coordinates.altitude() < 10000 ) {
        // Skip placemarks at the other side of the earth.
        if ( qpos.v[Q_Z] < 0 ) {
            return true;
        }
    }
    else {
        qreal  earthCenteredX = pixelAltitude * qpos.v[Q_X];
        qreal  earthCenteredY = pixelAltitude * qpos.v[Q_Y];
        qreal  radius         = viewport->radius();

        // Don't draw high placemarks (e.g. satellites) that aren't visible.
        if ( qpos.v[Q_Z] < 0
             && ( ( earthCenteredX * earthCenteredX
                    + earthCenteredY * earthCenteredY )
                  < radius * radius ) ) {
            return true;
        }
    }

    return false;
}



}
