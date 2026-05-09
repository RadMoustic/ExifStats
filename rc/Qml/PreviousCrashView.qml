import QtQuick
import QtQuick.Window
import QtQuick.Dialogs
import Qt.labs.platform
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Shapes
import QtQuick.Layouts

Pane
{
	id: rootItem
	visible: false
	
	Component.onCompleted:
	{
		if(MainQmlBinder.hasPreviousCrash())
		{
			showPreviousCrashLogDialog.open();
		}
	}

	Dialog
	{
		id: showPreviousCrashLogDialog
		title: "Show crash logs ?"
		standardButtons: MessageDialog.Yes | MessageDialog.No
		anchors.centerIn: Overlay.overlay
		modal: true
		
		onAccepted:
		{
			rootItem.visible = true;
			crashLogsTextArea.text = MainQmlBinder.getPreviousCrashLogs();
			MainQmlBinder.resetPreviousCrash();
		}
		onRejected:
		{
			rootItem.visible = false;
			MainQmlBinder.resetPreviousCrash();
		}
	}
	
	ScrollView
	{
		id: crashLogsScrollView
		anchors.fill: parent
		clip: true

		TextArea
		{
			id: crashLogsTextArea

			readOnly: true
			wrapMode: TextArea.Wrap
			selectByMouse: true
			selectionColor: "lightblue"
		}
	}
	
	RoundButton
	{
		id: closeButton
		text: "X"
		anchors.top: parent.top
		anchors.right: parent.right
		anchors.margins: 15
		radius: 4
		
		implicitWidth: 40
		implicitHeight: 40

		onReleased:
		{
			rootItem.visible = false;
		}
	}
}