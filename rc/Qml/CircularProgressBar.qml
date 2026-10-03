import QtQuick
import QtQuick.Controls
import QtQuick.Shapes

Item
{
    id: root

    property real progress: 0.0
    property color statusColor: "#2196F3"
    property string statusText: ""
    property bool loading: false

    readonly property bool isIndeterminate: root.loading && root.progress === 0.0

    implicitWidth: 40
    implicitHeight: 40

    HoverHandler
    {
        id: hoverHandler
    }

    ToolTip
    {
        text: root.statusText
        visible: hoverHandler.hovered && root.statusText !== ""
        delay: 300
    }

    Shape
    {
        id: shape
        anchors.fill: parent
        layer.enabled: true
        layer.samples: 4
        rotation: 0

        RotationAnimation on rotation
        {
            from: 0
            to: 360
            duration: 1200
            loops: Animation.Infinite
            running: root.isIndeterminate
        }

        ShapePath
        {
            strokeColor: Qt.darker(root.statusColor, 1.6)
            strokeWidth: 4
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc
            {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: (root.width - 4) / 2
                radiusY: (root.height - 4) / 2
                startAngle: 0
                sweepAngle: 360
            }
        }

        ShapePath
        {
            strokeColor: root.statusColor
            strokeWidth: 4
            fillColor: "transparent"
            capStyle: ShapePath.RoundCap

            PathAngleArc
            {
                centerX: root.width / 2
                centerY: root.height / 2
                radiusX: (root.width - 4) / 2
                radiusY: (root.height - 4) / 2
                startAngle: -90
                sweepAngle: root.isIndeterminate ? 90 : root.progress * 360
            }
        }
    }
}