/*
    Copyright (C) 2008 Nikolas Zimmermann <zimmermann@kde.org>

    This file is part of the KDE project

    This library is free software you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    aint with this library see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "DGMLIconTagHandler.h"

#include "DGMLElementDictionary.h"
#include "DGMLAttributeDictionary.h"
#include "GeoParser.h"
#include "GeoSceneHead.h"
#include "GeoSceneItem.h"

using namespace GeoSceneElementDictionary;
using namespace GeoSceneAttributeDictionary;

DGML_DEFINE_TAG_HANDLER(Icon)

DGMLIconTagHandler::DGMLIconTagHandler()
    : GeoTagHandler()
{
}

DGMLIconTagHandler::~DGMLIconTagHandler()
{
}

GeoNode* DGMLIconTagHandler::parse(GeoParser& parser) const
{
    // Check whether the tag is valid
    Q_ASSERT(parser.isStartElement() && parser.isValidElement(dgmlTag_Icon));

    QString pixmap = parser.attribute(dgmlAttr_pixmap).trimmed();
    QString color  = parser.attribute(dgmlAttr_color).trimmed();

    GeoSceneIcon *icon = 0;

    // Checking for parent item
    GeoStackItem parentItem = parser.parentElement();
    if (parentItem.represents(dgmlTag_Head)) {
        icon = parentItem.nodeAs<GeoSceneHead>()->icon();
        icon->setPixmap( pixmap );
        icon->setColor( color );
    }
    if (parentItem.represents(dgmlTag_Item)) {
        icon = parentItem.nodeAs<GeoSceneItem>()->icon();
        icon->setPixmap( pixmap );
        icon->setColor( color );
    }

    return 0;
}
