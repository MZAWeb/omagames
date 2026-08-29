import QtQuick
import QtQuick.Layouts
import OmaGames

// Minimal themed yes/no prompt (Escape with a puzzle in progress).
Rectangle {
    id: root

    property string message: ""
    property string acceptText: qsTr("Yes")
    property string rejectText: qsTr("Cancel")
    signal accepted()
    signal rejected()

    color: theme.alpha(theme.background, 0.85)
    focus: true

    MouseArea {
        anchors.fill: parent
        onClicked: root.rejected()
    }

    Keys.onPressed: function(event) {
        if (event.key === Qt.Key_Escape || event.key === Qt.Key_N)
            root.rejected();
        else if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter || event.key === Qt.Key_Y)
            root.accepted();
        event.accepted = true;
    }

    OmaPanel {
        anchors.centerIn: parent
        width: Math.min(parent.width - 48 * theme.textScale, 300 * theme.textScale)
        height: column.implicitHeight + 2 * padding

        ColumnLayout {
            id: column
            anchors.fill: parent
            spacing: 12 * theme.textScale

            Text {
                Layout.fillWidth: true
                text: root.message
                color: theme.foreground
                font.pixelSize: 15 * theme.textScale
                wrapMode: Text.WordWrap
                horizontalAlignment: Text.AlignHCenter
            }
            RowLayout {
                Layout.fillWidth: true
                spacing: 8 * theme.textScale
                HintButton {
                    Layout.fillWidth: true
                    text: root.rejectText
                    keyHint: qsTr("Esc")
                    onClicked: root.rejected()
                }
                HintButton {
                    Layout.fillWidth: true
                    text: root.acceptText
                    primary: true
                    keyHint: qsTr("Enter")
                    onClicked: root.accepted()
                }
            }
        }
    }
}
