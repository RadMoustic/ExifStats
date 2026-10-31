import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Window

Pane
{
	id: rootItem
	
	property alias model: consoleStrList.model
	property alias closeButtonVisible: closeButton.visible

	MouseArea 
	{
		id: catchMouseEvents
		anchors.fill: parent
		preventStealing:true
		hoverEnabled:   true
		onWheel:        { wheel.accepted = true; }
		onPressed:      { mouse.accepted = true; }
		onReleased:     { mouse.accepted = true; }
	}

	ListView
	{
		id: consoleStrList
		model: DebugQmlBinder.mConsoleLines
		anchors.fill: parent
		anchors.topMargin: rootItem.Window.window ? rootItem.Window.window.SafeArea.margins.top : 0
		clip: true
		delegate: TextArea
		{
			text: display
			textFormat: Text.PlainText
			wrapMode: Text.NoWrap
			width: implicitWidth
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
		flickableDirection: Flickable.HorizontalAndVerticalFlick
		contentWidth: Math.max(width, contentItem.childrenRect.width)
		boundsBehavior: Flickable.StopAtBounds
		ScrollBar.vertical: ScrollBar{}
		ScrollBar.horizontal: ScrollBar{}
	}
	
	RegularButton
	{
		id: scrollDown
		font.pixelSize: 15
		text: "↓"
		width: height
		anchors.right: parent.right
		anchors.bottom: parent.bottom
		anchors.rightMargin: 20
		anchors.bottomMargin: 15
		anchors.topMargin: rootItem.Window.window ? rootItem.Window.window.SafeArea.margins.top : 0
		onReleased:
		{
			Qt.callLater( consoleStrList.positionViewAtEnd )
		}
	}
	
	RegularButton
	{
		id: scrollUp
		font.pixelSize: 15
		text: "↑"
		width: height
		anchors.right: scrollDown.left
		anchors.bottom: parent.bottom
		anchors.rightMargin: 10
		anchors.bottomMargin: 15
		onReleased:
		{
			Qt.callLater( consoleStrList.positionViewAtBeginning )
		}
	}
	
	RegularButton
	{
		id: closeButton
		visible: false
		text: "X"
		anchors.top: parent.top
		anchors.right: parent.right
		anchors.margins: 15
		
		implicitWidth: 40
		implicitHeight: 40

		onReleased:
		{
			rootItem.visible = false;
		}
	}
}