import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Window
import QtQuick.Layouts 
import QtQuick.Dialogs
import QtCore

Pane
{
	id: rootItem
	anchors.fill: parent
	anchors.topMargin: rootItem.Window.window ? rootItem.Window.window.SafeArea.margins.top : 0
	
	FolderDialog
	{
		id: folderDialog
		currentFolder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
		property bool clearDB: true

		onAccepted:
		{
			MainQmlBinder.parseFolder(currentFolder, clearDB);
		}
	}

	MouseArea 
	{
		id: catchMouseEvents
		anchors.fill: parent
		preventStealing:true
		hoverEnabled:   true
		onWheel:        (pWheel)=>{ pWheel.accepted = true; }
		onPressed:      (pMouse)=>{ pMouse.accepted = true; }
		onReleased:     (pMouse)=>{ pMouse.accepted = true; }
	}

	ColumnLayout
	{
		anchors.fill: parent
		
		RowLayout
		{
			id: settings
			height: 50
			
			RoundButton
			{
				icon.source: "qrc:/Images/Add.png"
				display: AbstractButton.IconOnly
				radius: 0
				padding: 0

				onReleased:
				{
					folderDialog.clearDB = false;
					folderDialog.open();
				}
			}
			
			RoundButton
			{
				icon.source: "qrc:/Images/Refresh.png"
				display: AbstractButton.IconOnly
				radius: 0
				padding: 0

				onReleased:
				{
					MainQmlBinder.refresh(false);
				}
			}
			RoundButton
			{
				icon.source: "qrc:/Images/Settings.png"
				display: AbstractButton.IconOnly
				radius: 0
				padding: 0

				onReleased: settingsPopup.open()
			}
		}
		
		ProgressBar
		{
			id: processingProgressBar
			Layout.preferredWidth: parent.width
			Layout.preferredHeight: 10
			value: MainQmlBinder.ProcessingProgress
			opacity: MainQmlBinder.Processing ? 1.0 : 0.0
			height: 10
		}
		
		ListView
		{
			id: consoleStrList
			model: DebugQmlBinder.mConsoleLines
			Layout.fillHeight: true
			Layout.fillWidth: true
			clip: true
			delegate: TextArea
			{
				text: display
				textFormat: Text.PlainText
				wrapMode: Text.Wrap
				font.pixelSize: 12
				readOnly: true
				background: null
				padding: 0
			}
			onCountChanged:
			{
				if(atYEnd)
				{
					Qt.callLater( consoleStrList.positionViewAtEnd )
				}
			}
			
			flickableDirection: Flickable.VerticalFlick
			boundsBehavior: Flickable.StopAtBounds
			ScrollBar.vertical: ScrollBar{}
		}
	}
		
	Button
	{
		id: scrollDown
		font.pixelSize: 15
		text: "↓"
		width: height
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		anchors.leftMargin: 10
		anchors.bottomMargin: 10
		anchors.topMargin: rootItem.Window.window ? rootItem.Window.window.SafeArea.margins.top : 0
		onReleased:
		{
			Qt.callLater( consoleStrList.positionViewAtEnd )
		}
	}
	
	Button
	{
		id: scrollUp
		font.pixelSize: 15
		text: "↑"
		width: height
		anchors.right: scrollDown.left
		anchors.bottom: parent.bottom
		anchors.leftMargin: 10
		anchors.rightMargin: 10
		anchors.bottomMargin: 10
		onReleased:
		{
			Qt.callLater( consoleStrList.positionViewAtBeginning )
		}
	}
	
	ServerSettings
	{
		id: settingsPopup
	}
}