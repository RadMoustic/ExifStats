import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Window

Item
{
	anchors.fill: parent

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
		clip: true
		delegate: TextEdit
		{
			anchors
			{
				left: parent.left
				right: parent.right
			}
			text: display
			textFormat: Text.PlainText
			wrapMode: Text.Wrap
			font.pixelSize: 12
			color: "black"
			readOnly: true
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
}