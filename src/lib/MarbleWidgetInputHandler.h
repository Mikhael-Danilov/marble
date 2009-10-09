//
// This file is part of the Marble Desktop Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2005-2007 Torsten Rahn <tackat@kde.org>
// Copyright 2007      Inge Wallin  <ingwa@kde.org>
//

//
// The MarbleWidgetInputHandler handles mouse and keyboard input.
//


#ifndef MARBLEWIDGETINPUTHANDLER_H
#define MARBLEWIDGETINPUTHANDLER_H

#include <QtCore/QObject>
#include <QtCore/QString>

#include "marble_export.h"

class QEvent;
class QRubberBand;
class QTimer;

namespace Marble
{

class MarbleModel;
class MarbleWidget;
class MarbleMap;
class MarbleWidgetPopupMenu;
class AbstractDataPluginItem;

class MARBLE_EXPORT MarbleWidgetInputHandler  : public QObject
{
    Q_OBJECT

 public:
    MarbleWidgetInputHandler();
    virtual ~MarbleWidgetInputHandler();

    void setPositionSignalConnected( bool connected ){ m_positionSignalConnected = connected; }
    bool isPositionSignalConnected() const { return m_positionSignalConnected; }

    virtual void init(MarbleWidget*);

    /**
     * @brief  Set whether a popup menu appears on a click (not drag) with the left mouse button
     * @param  enabled True to enable the popup menu (default), false to disable it
     */
    void setMouseButtonPopupEnabled(Qt::MouseButton mouseButton, bool enabled);

    /**
     * @brief  Return whether the left mouse button popup menu is active
     * @return True iff a popup menu is shown on left mouse button clicks
     */
    bool isMouseButtonPopupEnabled(Qt::MouseButton mouseButton) const;

 Q_SIGNALS:
    // Mouse button menus
    void lmbRequest( int, int );
    void rmbRequest( int, int );

    //Gps coordinates
    void mouseClickScreenPosition( int, int );
    void mouseMoveGeoPosition( QString );

 protected Q_SLOTS:
    void restoreViewContext();

 protected:
    MarbleWidget  *m_widget;
    MarbleModel   *m_model;

    bool     m_positionSignalConnected;

    QTimer   *m_mouseWheelTimer;

    Qt::MouseButtons m_disabledMouseButtons;
    
 private:
    Q_DISABLE_COPY( MarbleWidgetInputHandler )
};



class MARBLE_EXPORT MarbleWidgetDefaultInputHandler  : public MarbleWidgetInputHandler
{
 Q_OBJECT

 public:
    MarbleWidgetDefaultInputHandler();
    ~MarbleWidgetDefaultInputHandler();

    static bool keyEvent( MarbleMap * map, QEvent* e );
    
    virtual void init(MarbleWidget*);
    
 protected:
    bool eventFilter( QObject *, QEvent * );

 private Q_SLOTS:
    void showLmbMenu( int, int );

    void showRmbMenu( int, int );

    void openItemToolTip();

    
 private:
    Q_DISABLE_COPY( MarbleWidgetDefaultInputHandler )
    class Private;
    Private * const d;
};

}

#endif // MARBLEWIDGETINPUTHANDLER
