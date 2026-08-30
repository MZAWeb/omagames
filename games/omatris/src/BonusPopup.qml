import QtQuick

// A short label that rises and fades from the spot a piece locked at, then
// removes itself. `anchorX`/`anchorY` are the spot in the parent's pixels;
// the label centres on it and stays inside the parent. One placement can earn
// three of these at once: `stackIndex` lifts each later label above the one
// before it, or below it when the spot is at the top edge.
Text {
    id: popup

    property real anchorX: 0
    property real anchorY: 0
    property int stackIndex: 0
    signal finished()

    readonly property real stackStep: 22 * theme.textScale
    readonly property real baseY: anchorY - height

    x: Math.min(Math.max(0, anchorX - width / 2), parent.width - width)
    y: baseY - stackIndex * stackStep >= 0
        ? Math.min(baseY - stackIndex * stackStep, parent.height - height)
        : Math.max(0, baseY) + stackIndex * stackStep

    color: theme.yellow
    font.pixelSize: 15 * theme.textScale
    font.bold: true
    style: Text.Outline
    styleColor: theme.alpha(theme.background, 0.9)

    transform: Translate { id: rise }

    ParallelAnimation {
        running: true
        NumberAnimation { target: rise; property: "y"; from: 0; to: -34 * theme.textScale; duration: 1000; easing.type: Easing.OutCubic }
        SequentialAnimation {
            PauseAnimation { duration: 600 }
            NumberAnimation { target: popup; property: "opacity"; to: 0; duration: 400 }
        }
        onFinished: popup.finished()
    }
}
