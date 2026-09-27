import QtQuick
import QtQuick.Layouts
import OmaGames
import Omadrop // qmllint disable import

FocusScope {
    id: root

    focus: true
    signal leaveRequested()

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Left || event.key === Qt.Key_H)
            game.nudgeAim(-1);
        else if (event.key === Qt.Key_Right || event.key === Qt.Key_L)
            game.nudgeAim(1);
        else if (event.key === Qt.Key_Space || event.key === Qt.Key_Return || event.key === Qt.Key_Enter)
            game.launch();
        else if (event.key === Qt.Key_P)
            game.togglePause();
        else if (event.key === Qt.Key_R)
            game.restart();
        else if (event.key === Qt.Key_Escape)
            root.leaveRequested();
        else
            return;
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

            Item {
                id: board
                anchors.centerIn: parent
                height: Math.min(arena.height, arena.width / 0.62)
                width: height * 0.62

                // qmllint disable import unresolved-type missing-type incompatible-type missing-property
                DropFieldView {
                    anchors.fill: parent
                    source: game
                    fieldColor: theme.mix(theme.background, theme.darkerBackground, 0.35)
                    borderColor: theme.mix(theme.background, theme.foreground, 0.65)
                    guideColor: theme.mix(theme.background, theme.foreground, 0.72)
                    ballColor: theme.brightForeground
                    textColor: theme.darkerBackground
                    faintColor: theme.alpha(theme.foreground, 0.055)
                    oneColor: theme.orange
                    twoColor: theme.yellow
                    threeColor: theme.green
                    fourColor: theme.blue
                    fiveColor: theme.cyan
                    sixColor: theme.magenta
                    sevenColor: theme.red
                }
                // qmllint enable import unresolved-type missing-type incompatible-type missing-property

                MouseArea {
                    anchors.fill: parent
                    enabled: game.ready && !game.paused
                    cursorShape: enabled ? Qt.CrossCursor : Qt.ArrowCursor
                    onPressed: function(mouse) {
                        game.aimAt(mouse.x / width, mouse.y / height);
                    }
                    onPositionChanged: function(mouse) {
                        if (pressed)
                            game.aimAt(mouse.x / width, mouse.y / height);
                    }
                    onReleased: function(mouse) {
                        game.aimAt(mouse.x / width, mouse.y / height);
                        game.launch();
                    }
                }

                Item { id: popups; anchors.fill: parent }
                Component {
                    id: popupComponent
                    OmaBonusPopup { onFinished: destroy() }
                }
                Connections {
                    target: game
                    function onScored(text, x, y) {
                        popupComponent.createObject(popups, {
                            text: text,
                            anchorX: x / 0.62 * board.width,
                            anchorY: y * board.height
                        });
                    }
                }
            }
        }

        KeyLegend {
            Layout.fillWidth: true
            compact: true
        }
    }
}
