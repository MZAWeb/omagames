import QtQuick
import QtQuick.Layouts
import OmaGames

// The paused screen: a heading, a line of where the run stands, and whatever
// buttons the game offers, which it adds as its own children. The keys stay
// with the game too, since only it knows what R or G should do.
OmaOverlayPanel {
    id: root

    property string title: qsTr("Paused")
    property string subtitle: ""

    Text {
        Layout.alignment: Qt.AlignHCenter
        text: root.title
        color: theme.foreground
        font.pixelSize: 30 * theme.textScale
        font.bold: true
    }
    Text {
        Layout.alignment: Qt.AlignHCenter
        visible: root.subtitle !== ""
        text: root.subtitle
        color: theme.mix(theme.background, theme.foreground, 0.7)
        font.pixelSize: 14 * theme.textScale
    }
}
