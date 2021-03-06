//
// This file is part of the Marble Virtual Globe.
//
// This program is free software licensed under the GNU LGPL. You can
// find a copy of this license in LICENSE.txt in the top directory of
// the source code.
//
// Copyright 2015      Gábor Péterffy <peterffy95@gmail.com>
//


import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Window 2.2

import org.kde.marble 0.20

Item {
    id: root
    property var marbleItem: null
    property var tts: null
    property alias snappedPositionMarkerScreenPosition: navigation.screenPosition
    property bool guidanceMode: false
    property alias screenAccuracy: navigation.screenAccuracy
    property alias deviated: navigation.deviated

    onGuidanceModeChanged: {
        if (guidanceMode) {
            marbleItem.setZoomToMaximumLevel();
            marbleItem.centerOnCurrentPosition();
        }

        navigation.guidanceModeEnabled = guidanceMode;
    }

    Settings {
        id: settings
        Component.onDestruction: {
            settings.setValue("Navigation", "muted", muteButton.muted)
        }
    }

    BorderImage {
        anchors.fill: infoBar
        anchors.margins: -14
        border { top: 14; left: 14; right: 14; bottom: 14 }
        source: "qrc:///border_shadow.png"
    }

    NavigationInfoBar {
        id: infoBar
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }
        instructionIcon: navigation.nextInstructionImage.replace("qrc:/", "qrc:///");
        distance: navigation.nextInstructionDistance;
        destinationDistance: navigation.destinationDistance
    }

    CircularButton {
        id: muteButton

        property bool muted: settings.value("Navigation", "muted") === "true"

        anchors.right: infoBar.right
        anchors.rightMargin: Screen.pixelDensity * 3
        anchors.top: infoBar.bottom
        anchors.topMargin: Screen.pixelDensity * 5
        iconSource: muted ? "qrc:///material/volume-off.svg" : "qrc:///material/volume-on.svg"
        onClicked: muted = !muted
    }

    Navigation {
        id: navigation
        marbleQuickItem: marbleItem

        onVoiceNavigationAnnouncementChanged: {
            if (root.guidanceMode && !muteButton.muted) {
                textToSpeechClient.readText(voiceNavigationAnnouncement);
            }
        }
    }
}
