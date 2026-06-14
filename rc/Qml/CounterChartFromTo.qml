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
	property var categoriesRealValues: []
	
	property var selectedValueFrom: function(pValue) { return pValue; }
	property var selectedValueTo: function(pValue) { return pValue; }
	
	property var selectedRealValueFrom: function(pValue) { return pValue; }
	property var selectedRealValueTo: function(pValue) { return pValue; }
	
	popupMenuFunction: function(pMouse)
	{
		var cursorPos = parent.mapToItem(rootItem.barChartChild, pMouse);
		var p = rootItem.barChartChild.mapToValue(rootItem.barChartChild.mInvertAxis ? cursorPos.y : cursorPos.x);
		pMouse.accepted = true;
		if(p.y > 0)
		{
			var selectedCategory = rootItem.categories[Math.round(p.x)];
			var selectedCategoryRealValue = categoriesRealValues.length > 0 ? rootItem.categoriesRealValues[Math.round(p.x)] : selectedCategory;
						
			setFromTo.from = selectedValueFrom(selectedCategory);
			setFromTo.to = selectedValueTo(selectedCategory);
			
			setFromTo.fromRealValue = selectedRealValueFrom(selectedCategoryRealValue);
			setFromTo.toRealValue = selectedRealValueTo(selectedCategoryRealValue);
			
			setFrom.from = setFromTo.from;
			setTo.to = setFromTo.to;
			
			setFrom.fromRealValue = setFromTo.fromRealValue;
			setTo.toRealValue = setFromTo.toRealValue;
			
			barsContextMenu.oneBarSelected = true;
		}
		else
		{
			barsContextMenu.oneBarSelected = false;
		}
		barsContextMenu.popup();
	}
	
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
			property string fromRealValue
			text: "Filter from '" + from + "'"
			visible: barsContextMenu.oneBarSelected
			height: barsContextMenu.oneBarSelected ? resetAllItem.height : 0
			onTriggered:
			{
				MainQmlBinder[fromPropertyName] = setFrom.fromRealValue;
			}
		}
		MenuItem
		{
			id: setTo
			property string to
			property string toRealValue
			text: "Filter to '" + to + "'"
			visible: barsContextMenu.oneBarSelected
			height: barsContextMenu.oneBarSelected ? resetAllItem.height : 0
			onTriggered:
			{
				MainQmlBinder[toPropertyName] = setTo.toRealValue;
			}
		}
		MenuItem
		{
			id: setFromTo
			property string from
			property string to
			property string fromRealValue
			property string toRealValue
			text: "Filter from '" + from + "' to '" + to + "'"
			height: barsContextMenu.oneBarSelected ? resetAllItem.height : 0
			visible: barsContextMenu.oneBarSelected
			onTriggered:
			{
				MainQmlBinder[fromPropertyName] = setFromTo.fromRealValue;
				MainQmlBinder[toPropertyName] = setFromTo.toRealValue;
			}
		}
	}
}