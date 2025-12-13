import QtQuick
import QtQuick.Window
import QtQuick.Dialogs
import Qt.labs.platform
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Shapes
import QtQuick.Layouts
import ExifStats

Rectangle
{
	id:rootItem
	clip:true
	
	property var title: ""
	property var categories: []
	property var values: []
	property var min: 0
	property var max: 1
	property var totalHorizontalScroll: 0
	
	property alias barChartChild: barChartItem
	
	function resetView()
	{
		valueToltip.visible = false;
		barChartItem.mXOffset = 0;
		barChartItem.mXScale = 1.0;
		barChartItem.mYScale = 1.0;
	}
	
	ESBarChartQuickItem
	{
		id: barChartItem
		height: parent.height
		width: parent.width
		x:0
		y:0
		mXAxisHeight: 0
		mXAxisHeightAuto: true
		mYAxisWidth: 30
		mMargin: 5
		
		mBarSpacing: 1
		mCategorySpacing: 10
		
		mCategories: rootItem.categories
		mValues: rootItem.values
		
		ToolTip
		{
			id: valueToltip
			contentItem: Text
			{
				color: "white"
				text: valueToltip.text
			}
			background: Rectangle
			{
				color: "black"
			}
		}
		
		MouseArea
		{
			anchors.fill: parent
			onDoubleClicked:
			{
				rootItem.resetView();
			}
			
			onClicked: (cursorPos)=>
			{
				var p = barChartItem.mapToValue(cursorPos.x)
				valueToltip.text = rootItem.categories[p.x] + " => " + p.y
				valueToltip.x = cursorPos.x - valueToltip.width/2
				valueToltip.y = cursorPos.y - valueToltip.height
				
				//valueToltip.timeout = 1000
				valueToltip.visible = true
			}
		}
		
		WheelHandler
		{
			onWheel: (event)=>
			{
				valueToltip.visible = false;
				var scale = event.angleDelta.y > 0 ? 1.1 : 0.9;
				if(MainQmlBinder.isCtrlPressed())
				{
					barChartItem.mYScale *= scale;
				}
				else
				{
					var mouseX = barChartItem.mapToPlotArea(event.x, event.y).x;
					var oldChartFullWidth = barChartItem.getChartFullWidth();
					barChartItem.mXScale *= scale;
					var newChartFullWidth = barChartItem.getChartFullWidth();
					barChartItem.mXOffset = (barChartItem.mXOffset - mouseX) * newChartFullWidth/oldChartFullWidth + mouseX;
				}
			}
		}
		
		DragHandler
		{
			id: drag
			target: null
			onTranslationChanged: (delta) =>
			{
				valueToltip.visible = false;
				barChartItem.mXOffset += delta.x;
			}
		}
	}
}