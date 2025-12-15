import QtQuick
import QtQuick.Window
import QtQuick.Dialogs
import Qt.labs.platform
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Shapes
import QtQuick.Layouts

ColumnLayout
{
	id: rootItem
	
	clip: true

	property var model: []
	property var selectedItems: {}
	property int selectedItemIndex: -1
	property alias listViewChild: listView
	signal selectionHasChanged()
		
	function setAllSelectedItems(pSelected, pEmitSignal=true)
	{
		for(var i = 0 ; i < model.length ; ++i)
		{
			selectedItems[model[i]] = pSelected;
		}
		if(pEmitSignal)
			rootItem.selectionHasChanged();
	}
	
	RowLayout
	{
		id: tools
		
		Layout.fillWidth: true
		Layout.fillHeight: false

		SmallButton
		{
			id: allBt
			text: "All"
			
			onClicked:
			{
				setAllSelectedItems(true);
			}
		}
		SmallButton
		{
			id: noneBt
			text: "None"
			
			onClicked:
			{
				setAllSelectedItems(false);
			}
		}
	}
	
	ListView
	{
		id: listView
		interactive: false
		boundsBehavior: Flickable.StopAtBounds
		
		model: rootItem.model
		
		Layout.fillWidth: true
		Layout.fillHeight: true
		
		ScrollBar.vertical: ScrollBar
		{
			active: true
		}
		
		WheelHandler
		{
			onWheel: (event)=>{listView.flick(0, event.angleDelta.y*event.y*0.1)}
		}

		delegate: Rectangle
		{
			id: listItem
			
			property var selected: Object.keys(rootItem.selectedItems).length === 0 ? true : (rootItem.selectedItems[modelData] !== undefined ? rootItem.selectedItems[modelData] : false)

			width: parent.width
			height: listItemLabel.contentHeight
			color: selected ? "lightsteelblue" : "transparent"
					
			Text
			{
				id: listItemLabel
				anchors.fill: parent
				text: modelData ? modelData : "<empty>"
			}
			
			MouseArea
			{
				anchors.fill: parent
				onClicked:
				{
					if(MainQmlBinder.isCtrlPressed())
					{
						listItem.selected = !listItem.selected
						rootItem.selectedItems[modelData] = listItem.selected;
					}
					else
					{
						setAllSelectedItems(false, false);
						listItem.selected = true;
						rootItem.selectedItems[modelData] = listItem.selected;
					}
					rootItem.selectionHasChanged();
				}
			}
		}
	}
}