import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtLocation
import QtCore

Popup
{
	id: rootItem
	anchors.centerIn: Overlay.overlay
	width: 300
	height: mainSettingsLayout.implicitHeight + 50
	modal: true
	focus: true
	
	Dialog
	{
		id: setPasswordDialog
		title: "Password: "
		standardButtons: MessageDialog.Ok | MessageDialog.Cancel
		anchors.centerIn: Overlay.overlay
		modal: true
		
		TextField
		{
			id: password
		}
		
		onAccepted:
		{
			MainQmlBinder.setServerPassword(password.text);
		}
	}
	
	ColumnLayout
	{
		id: mainSettingsLayout
		anchors.centerIn: parent
		spacing: 15
		width: parent.width - 40

		RegularButton
		{
			text:"Set Password"
			Layout.alignment: Qt.AlignHCenter
			
			onReleased:
			{
				setPasswordDialog.open();
			}
		}
		RegularButton
		{
			text:"Full Refresh"
			Layout.alignment: Qt.AlignHCenter
			
			onReleased:
			{
				MainQmlBinder.refresh(true);
			}
		}
		RegularButton
		{
			id: clearButton
			text:"Clear Database"
			Layout.alignment: Qt.AlignHCenter
			
			onReleased:
			{
				MainQmlBinder.clear();
			}
		}
	}
}