import QtQuick
import OmaGames

// Responsive House Table: the spatial oval is primary; when seats reach their
// minimum geometry, it becomes a dealer/human stage plus deal-order roster.
Item {
    id: root
    // Too small for a third seat on the oval: the window reports this to the
    // bridge, which caps the table at two mates while it holds.
    readonly property bool compact: theme.textScale >= 1.6 || width < 1016 || height < 488
    property bool rosterMode: theme.textScale >= 1.6
                              || (game.botCount >= 5 && (width < 1240 || height < 600))
                              || (game.botCount >= 3 && compact)
                              || (game.botCount > 0 && width < 720)
    readonly property var botSeats: {
        var bots = [];
        for (var i = 0; i < game.seats.length; ++i) {
            if (!game.seats[i].human)
                bots.push(game.seats[i]);
        }
        return bots;
    }

    Loader {
        anchors.fill: parent
        sourceComponent: root.rosterMode ? rosterLayout : ovalLayout
    }

    Component {
        id: ovalLayout
        Item {
            Rectangle {
                id: table
                anchors.fill: parent
                anchors.margins: 2 * theme.textScale
                radius: height / 2
                color: theme.mix(theme.darkBackground, theme.accent, 0.035)
                border.width: 2 * theme.textScale
                border.color: theme.alpha(theme.cyan, 0.72)
                antialiasing: true

                Rectangle {
                    visible: theme.textScale < 1.3 && parent.width > 850
                    anchors.fill: parent
                    anchors.margins: 28 * theme.textScale
                    radius: height / 2
                    color: theme.alpha(theme.background, 0)
                    border.width: 1
                    border.color: theme.alpha(theme.foreground, 0.2)
                }

                DealerArea {
                    id: dealer
                    width: implicitWidth
                    height: implicitHeight
                    x: (table.width - width) / 2
                    y: 10 * theme.textScale
                }

                Repeater {
                    model: root.botSeats
                    delegate: TableSeat {
                        required property var modelData
                        required property int index
                        // The seating chart lives in C++ (SeatLayout), so the
                        // spacing that keeps seats off each other is tested.
                        readonly property rect slot: game.seatRect(root.botSeats.length, index,
                                                                   Qt.size(table.width, table.height),
                                                                   Qt.size(targetWidth, targetHeight))
                        seatData: modelData
                        targetWidth: 190 * theme.textScale
                        targetHeight: 132 * theme.textScale
                        cardScale: game.botCount <= 2 ? 0.9 : 0.8
                        x: slot.x
                        y: slot.y
                    }
                }

                MessageLog {
                    width: Math.min(440 * theme.textScale, table.width * 0.42)
                    height: 54 * theme.textScale
                    clip: true
                    x: (table.width - width) / 2
                    y: table.height * 0.43
                }

                Text {
                    visible: theme.textScale < 1.3 && table.height > 500
                    width: table.width
                    y: table.height * 0.4
                    text: game.rulesSummary
                    color: theme.cyan
                    opacity: 0.7
                    font.pixelSize: 11 * theme.textScale
                    horizontalAlignment: Text.AlignHCenter
                }

                TableSeat {
                    id: human
                    seatData: game.seats[game.humanSeat]
                    targetWidth: (game.botCount <= 2 ? 500 : 430) * theme.textScale
                    targetHeight: (game.botCount <= 2 ? 226 : 181) * theme.textScale
                    cardScale: game.botCount <= 2 ? 1.3 : 1.12
                    x: (table.width - width) / 2
                    y: table.height - height - 12 * theme.textScale
                }

                DecisionCoach {
                    compact: table.width < 1000
                    x: table.width > 1120 * theme.textScale ? table.width - width - 20 * theme.textScale
                                                           : 18 * theme.textScale
                    y: table.height - height - 22 * theme.textScale
                }
            }
        }
    }

    Component {
        id: rosterLayout
        Item {
            Row {
                anchors.fill: parent
                spacing: 10 * theme.textScale

                OmaPanel {
                    id: stage
                    width: root.botSeats.length > 0 ? parent.width * 0.59 : parent.width
                    height: parent.height
                    padding: 12 * theme.textScale
                    color: theme.mix(theme.darkBackground, theme.accent, 0.025)
                    border.width: 2
                    border.color: theme.alpha(theme.cyan, 0.6)

                    Text {
                        text: "Dealer and you"
                        color: theme.foreground
                        opacity: 0.75
                        font.pixelSize: 11 * theme.textScale
                    }
                    DealerArea {
                        id: stageDealer
                        cardScale: 0.75
                        width: implicitWidth
                        height: implicitHeight
                        x: parent.width * 0.3 - width / 2
                        y: 22 * theme.textScale
                    }
                    DecisionCoach {
                        compact: true
                        x: Math.max(0, parent.width - width)
                        y: 22 * theme.textScale
                    }
                    HumanStage {
                        seatData: game.seats[game.humanSeat]
                        width: parent.width
                        x: (parent.width - width) / 2
                        y: parent.height - height
                    }
                }

                OmaPanel {
                    id: roster
                    visible: root.botSeats.length > 0
                    width: parent.width - stage.width - parent.spacing
                    height: parent.height
                    padding: 8 * theme.textScale

                    Text {
                        id: rosterLabel
                        text: "Table mates, in deal order"
                        color: theme.foreground
                        opacity: 0.75
                        font.pixelSize: 11 * theme.textScale
                    }
                    Flickable {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: rosterLabel.bottom
                        anchors.topMargin: 6 * theme.textScale
                        anchors.bottom: parent.bottom
                        contentWidth: width
                        contentHeight: rosterColumn.height
                        clip: true
                        boundsBehavior: Flickable.StopAtBounds

                        Column {
                            id: rosterColumn
                            width: parent.width
                            spacing: 2 * theme.textScale
                            Repeater {
                                model: root.botSeats
                                delegate: RosterSeat {
                                    required property var modelData
                                    width: rosterColumn.width
                                    seatData: modelData
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
