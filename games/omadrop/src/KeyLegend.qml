import OmaGames

OmaKeyLegend {
    property bool startScreen: false

    model: startScreen ? [
        { key: qsTr("Enter"), label: qsTr("play") },
        { key: qsTr("H"), label: qsTr("scores") },
        { key: qsTr("Ctrl+Q"), label: qsTr("quit") }
    ] : [
        { key: qsTr("← →"), label: qsTr("or h/l to aim") },
        { key: qsTr("Space"), label: qsTr("launch") },
        { key: qsTr("P"), label: qsTr("pause") },
        { key: qsTr("R"), label: qsTr("restart") },
        { key: qsTr("Esc"), label: qsTr("leave") }
    ]
}
