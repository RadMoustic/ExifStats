import QtQuick
import QtQuick.Window
import QtQuick.Dialogs
import Qt.labs.platform
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Shapes
import QtQuick.Layouts

RoundButton
{
	id: bt
	radius: 0
	Material.accent: Material.Green
	
	contentItem: Label
	{
		text: bt.text
		font: bt.font
		verticalAlignment: Text.AlignVCenter
		horizontalAlignment: Text.AlignHCenter
	}
}