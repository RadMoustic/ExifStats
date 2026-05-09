import QtQuick
import QtQuick.Window
import QtQuick.Dialogs
import Qt.labs.platform
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Shapes
import QtQuick.Layouts

Frame
{
	id: rootItem
	clip: true
	
	property var model: []
	property var selectedItems: {}
	property int selectedItemIndex: -1
	property alias listViewChild: listView
	property alias vertScrollBarChild: vertScrollBar
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
	
	ColumnLayout
	{
		anchors.fill: parent
		
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
			
			TextField
			{
				id: searchField
				Layout.fillWidth: true
				implicitHeight: 40
				placeholderText: "Search"
			}
			
			RegularButton
			{
				text:"X"
				
				implicitWidth: 30
				implicitHeight: 30

				onReleased:
				{
					searchField.text = ""
				}
			}
		}
		
		ListView
		{
			id: listView
			interactive: true
			boundsBehavior: Flickable.StopAtBounds
			clip: true
			
			model: rootItem.model.filter(item => item.toLowerCase().includes(searchField.text.toLowerCase())).sort()
			
			Layout.fillWidth: true
			Layout.fillHeight: true
			
			ScrollBar.vertical: ScrollBar
			{
				id: vertScrollBar
				active: true
			}
			
			WheelHandler
			{
				onWheel: (event)=>{listView.flick(0, event.angleDelta.y*5)}
			}

			delegate: Rectangle
			{
				id: listItem
				
				property var selected: Object.keys(rootItem.selectedItems).length === 0 ? true : (rootItem.selectedItems[modelData] !== undefined ? rootItem.selectedItems[modelData] : false)

				width: parent.width - vertScrollBar.width
				height: listItemLabel.contentHeight
				color: selected ? Material.accent : "transparent"
						
				Label
				{
					id: listItemLabel
					anchors.fill: parent
					text: modelData ? modelData : "<empty>"
					color: selected ? (Material.theme == Material.Dark ? "#000000" : "#FFFFFF") : Material.foreground
				}
				
				MouseArea
				{
					width: parent.width - vertScrollBar.width
					height: parent.height
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
					onPressAndHold:
					{
						listItem.selected = !listItem.selected
						rootItem.selectedItems[modelData] = listItem.selected;
						rootItem.selectionHasChanged();
					}
				}
			}
		}
	}
}