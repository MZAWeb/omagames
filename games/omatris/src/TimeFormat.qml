import QtQuick

// One place that turns milliseconds into a clock, for the header, the result
// overlay and the Sprint table.
QtObject {
    // "1:23.45"; the hundredths matter when a Sprint is the score.
    function text(millis) {
        var total = Math.max(0, Math.round(millis / 10));
        var hundredths = total % 100;
        var seconds = Math.floor(total / 100) % 60;
        var minutes = Math.floor(total / 6000);
        return minutes + ":" + (seconds < 10 ? "0" : "") + seconds
             + "." + (hundredths < 10 ? "0" : "") + hundredths;
    }
}
