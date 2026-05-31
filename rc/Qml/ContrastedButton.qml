import QtQuick
import QtQuick.Controls
import QtQuick.Effects

RoundButton
{
	id: rootItem
	radius: 4
	
	implicitWidth: 40
	implicitHeight: 40

	contentItem: Text
	{
		text: rootItem.text
		font.pixelSize: 15
		font.bold: true
		color: "#666666"
		horizontalAlignment: Text.AlignHCenter
		verticalAlignment: Text.AlignVCenter
	}

	background: Item
	{
		Rectangle
		{
			id: bgRect
			anchors.fill: parent
			color: rootItem.down ? "#222222" : "#111111"
			border.color: "#666666"
			border.width: 2
			radius: rootItem.radius
			visible: false
		}

		MultiEffect
		{
			source: bgRect
			anchors.fill: parent
			shadowEnabled: true
			shadowColor: "#CC000000"
			shadowBlur: 1.0
		}
	}
}