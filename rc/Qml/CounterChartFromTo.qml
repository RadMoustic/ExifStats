import QtQuick
import QtQuick.Window
import QtQuick.Dialogs
import Qt.labs.platform
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Shapes
import QtQuick.Layouts
import "."

CounterChart
{
	id: rootItem
	
	property string fromPropertyName: ""
	property string toPropertyName: ""
	property string minPropertyName: ""
	property string maxPropertyName: ""
	
	property var selectedValueFrom: function(pValue) { return pValue; }
	property var selectedValueTo: function(pValue) { return pValue; }
	
	Menu
	{
		id: barsContextMenu
		width: 350
		property var contextItem
		property bool oneBarSelected: false

		MenuItem
		{
			id: resetAllItem
			text: "Reset From/To Filters"
			onTriggered:
			{
				MainQmlBinder[fromPropertyName] = MainQmlBinder[minPropertyName];
				MainQmlBinder[toPropertyName] = MainQmlBinder[maxPropertyName];
			}
		}
		MenuItem
		{
			id: setFrom
			property string from
			text: "Filter from '" + from + "'"
			visible: barsContextMenu.oneBarSelected
			height: barsContextMenu.oneBarSelected ? resetAllItem.height : 0
			onTriggered:
			{
				MainQmlBinder[fromPropertyName] = setFrom.from;
			}
		}
		MenuItem
		{
			id: setTo
			property string to
			text: "Filter to '" + to + "'"
			visible: barsContextMenu.oneBarSelected
			height: barsContextMenu.oneBarSelected ? resetAllItem.height : 0
			onTriggered:
			{
				MainQmlBinder[toPropertyName] = setTo.to;
			}
		}
		MenuItem
		{
			id: setFromTo
			property string from
			property string to
			text: "Filter from '" + from + "' to '" + to + "'"
			height: barsContextMenu.oneBarSelected ? resetAllItem.height : 0
			visible: barsContextMenu.oneBarSelected
			onTriggered:
			{
				MainQmlBinder[fromPropertyName] = setFromTo.from;
				MainQmlBinder[toPropertyName] = setFromTo.to;
			}
		}
	}
			
	MouseArea
	{
		width: parent.width
		height: parent.height
		propagateComposedEvents: true
		acceptedButtons: Qt.RightButton

		onClicked: (pMouse)=>
		{
			var cursorPos = parent.mapToItem(rootItem.barChartChild, pMouse);
			var p = rootItem.barChartChild.mapToValue(cursorPos.x);
			pMouse.accepted = true;
			if(p.y > 0)
			{
				var selectedValue = rootItem.categories[Math.round(p.x)];
							
				setFromTo.from = selectedValueFrom(selectedValue);
				setFromTo.to = selectedValueTo(selectedValue);
				
				setFrom.from = setFromTo.from;
				setTo.to = setFromTo.to;
				
				barsContextMenu.oneBarSelected = true;
			}
			else
			{
				barsContextMenu.oneBarSelected = false;
			}
			barsContextMenu.popup();
		}
	}
}