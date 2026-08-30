import QtQuick
import QtQuick.Layouts
// FieldView is the C++ item main.cpp registers under `Omanix`; there is no
// qmldir for qmllint to find it, so the import and the one block that uses
// it are excused from the lint that needs one.
import Omanix // qmllint disable import
import OmaGames

// The playing screen: header, the painted field centred at a whole number
// of pixels per cell, and the key legend. All rules live in `game`; this
// only renders state and forwards keys.
FocusScope {
    id: root

    focus: true

    signal leaveRequested()

    function directionFor(key) {
        switch (key) {
        case Qt.Key_Up: case Qt.Key_K: return "up";
        case Qt.Key_Down: case Qt.Key_J: return "down";
        case Qt.Key_Left: case Qt.Key_H: return "left";
        case Qt.Key_Right: case Qt.Key_L: return "right";
        }
        return "";
    }

    Keys.onPressed: function(event) {
        var direction = root.directionFor(event.key);
        if (direction !== "") {
            game.setDirection(direction);
        } else if (event.key === Qt.Key_Space || event.key === Qt.Key_P) {
            game.togglePause();
        } else if (event.key === Qt.Key_R) {
            game.restartLevel();
        } else if (event.key === Qt.Key_Escape) {
            root.leaveRequested();
        } else {
            return;
        }
        event.accepted = true;
    }
    Keys.onReleased: function(event) {
        // Auto-repeat releases are not the finger leaving the key.
        if (event.isAutoRepeat)
            return;
        var direction = root.directionFor(event.key);
        if (direction === "")
            return;
        game.releaseDirection(direction);
        event.accepted = true;
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 14 * theme.textScale
        spacing: 10 * theme.textScale

        PlayHeader { Layout.fillWidth: true }

        Item {
            id: arena
            Layout.fillWidth: true
            Layout.fillHeight: true

            readonly property int cell: Math.max(2, Math.floor(Math.min(width / game.fieldWidth, height / game.fieldHeight)))

            Item {
                id: view
                anchors.centerIn: parent
                width: arena.cell * game.fieldWidth
                height: arena.cell * game.fieldHeight

                // qmllint disable import unresolved-type missing-type incompatible-type missing-property
                FieldView {
                    anchors.fill: parent
                    cellSize: arena.cell
                    source: game
                    openColor: theme.mix(theme.background, theme.darkerBackground, 0.6)
                    claimedColor: theme.mix(theme.background, theme.accent, 0.5)
                    trailColor: theme.yellow
                    ballColor: theme.red
                    chaserColor: theme.magenta
                    markerColor: theme.brightForeground
                    gridColor: theme.alpha(theme.foreground, 0.05)
                    flashColor: theme.red
                    accentColor: theme.accent
                    trailThreatened: game.trailThreatened
                }
                // qmllint enable import unresolved-type missing-type incompatible-type missing-property
            }

            // The field's own frame, so it reads as one object.
            Rectangle {
                anchors.fill: view
                anchors.margins: -2
                radius: 4
                color: "transparent"
                border.width: 2
                border.color: theme.alpha(theme.accent, 0.5)
            }

            Item {
                id: popups
                anchors.fill: view
            }

            LevelBanner {
                anchors.centerIn: view
                shown: game.levelIntro
            }

            Component {
                id: popupComponent
                OmaBonusPopup { onFinished: destroy() }
            }

            Connections {
                target: game
                // One claim can earn two bonuses at once; each new label
                // sits above the ones still showing.
                function onBonusEarned(text, x, y) {
                    popupComponent.createObject(popups, {
                        text: text, anchorX: (x + 0.5) * arena.cell, anchorY: y * arena.cell,
                        stackIndex: popups.children.length });
                }
                function onExtraLife() {
                    popupComponent.createObject(popups, {
                        text: qsTr("Extra life!"), color: theme.green,
                        anchorX: popups.width / 2, anchorY: popups.height * 0.4 });
                }
            }
        }

        KeyLegend {
            Layout.fillWidth: true
            compact: true
        }
    }
}
