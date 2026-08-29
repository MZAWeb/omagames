import QtQuick

// A short label that rises and fades from the spot a claim closed at, then
// removes itself. `anchorX`/`anchorY` are the spot in the parent's pixels;
// the label centres on it and stays inside the parent.
Text {
    id: popup

    property real anchorX: 0
    property real anchorY: 0
    signal finished()

    x: Math.min(Math.max(0, anchorX - width / 2), parent.width - width)
    y: Math.min(Math.max(0, anchorY - height), parent.height - height)

    color: theme.yellow
    font.pixelSize: 15 * theme.textScale
    font.bold: true
    style: Text.Outline
    styleColor: theme.alpha(theme.background, 0.9)

    transform: Translate { id: rise }

    ParallelAnimation {
        running: true
        NumberAnimation { target: rise; property: "y"; from: 0; to: -34 * theme.textScale; duration: 1100; easing.type: Easing.OutCubic }
        SequentialAnimation {
            PauseAnimation { duration: 650 }
            NumberAnimation { target: popup; property: "opacity"; to: 0; duration: 450 }
        }
        onFinished: popup.finished()
    }
}
