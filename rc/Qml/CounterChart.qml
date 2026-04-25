import QtQuick
import QtQuick.Window
import QtQuick.Dialogs
import Qt.labs.platform
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Shapes
import QtQuick.Layouts
import ExifStats

Pane
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
		
		Material.theme: Material.theme
		
		Connections
		{
			target: barChartItem.parent.parent.parent.Material
			function onThemeChanged()
			{
				barChartItem.Material.theme = barChartItem.parent.parent.parent.Material.theme;
				barChartItem.update();
			}
		}
		
		mInvertAxis: height > width
		mXAxisHeight: 0
		mXAxisHeightAuto: true
		mXAxisMaxHeightAuto: 150
		mYAxisWidth: 40
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

		PinchArea 
		{
			anchors.fill: parent
			
			property real startDistX: 0
			property real startDistY: 0
			property real startScaleX: 1.0
			property real startScaleY: 1.0
			property real startOffsetX: 0
			property real pinchStartX: 0
			property real oldFullWidth: 0

			onPinchStarted: (pinch) =>
			{
				valueToltip.visible = false;
				startDistX = Math.max(1, Math.abs(pinch.point1.x - pinch.point2.x));
				startDistY = Math.max(1, Math.abs(pinch.point1.y - pinch.point2.y));
				
				startScaleX = barChartItem.mInvertAxis ? barChartItem.mYScale : barChartItem.mXScale;
				startScaleY = barChartItem.mInvertAxis ? barChartItem.mXScale : barChartItem.mYScale;
				startOffsetX = barChartItem.mXOffset;
				oldFullWidth = barChartItem.getChartFullWidth();
				
				pinchStartX = barChartItem.mapToPlotArea(pinch.center.x, 0).x;
			}

			onPinchUpdated: (pinch) =>
			{
				var currentDistX = Math.abs(pinch.point1.x - pinch.point2.x);
				var currentDistY = Math.abs(pinch.point1.y - pinch.point2.y);
				
				if (startDistX > 20 && currentDistX > 20) 
				{
					var newScale = startScaleX * (currentDistX / startDistX);
					if(barChartItem.mInvertAxis)
						barChartItem.mYScale = newScale;
					else
						barChartItem.mXScale = newScale;
					var newFullWidth = barChartItem.getChartFullWidth();
					barChartItem.mXOffset = (startOffsetX - pinchStartX) * newFullWidth / oldFullWidth + pinchStartX;
				}
				
				if (startDistY > 50 && currentDistY > 50) 
				{
					var newScale = startScaleY * (currentDistY / startDistY);
					if(barChartItem.mInvertAxis)
						barChartItem.mXScale = newScale;
					else
						barChartItem.mYScale
				}
			}

			MouseArea
			{
				anchors.fill: parent
				property real lastX: 0
				property real lastY: 0
				property real pressX: 0
				property real pressY: 0
				
				onPressed: (mouse) =>
				{
					lastX = mouse.x;
					lastY = mouse.y;
					pressX = mouse.x;
					pressY = mouse.y;
				}
				
				onPositionChanged: (mouse) =>
				{
					if (pressed) 
					{
						valueToltip.visible = false;
						if(barChartItem.mInvertAxis)
							barChartItem.mXOffset += (mouse.y - lastY);
						else
							barChartItem.mXOffset += (mouse.x - lastX);
						lastX = mouse.x;
						lastY = mouse.y;
					}
				}
				
				onDoubleClicked: (mouse) =>
				{
					rootItem.resetView();
				}
				
				onClicked: (mouse) =>
				{
					if (Math.abs(mouse.x - pressX) > 5 || Math.abs(mouse.y - pressY) > 5) return;

					var p = barChartItem.mapToValue(barChartItem.mInvertAxis ? mouse.y : mouse.x);
					valueToltip.text = rootItem.categories[p.x] + " => " + p.y;
					valueToltip.x = mouse.x - valueToltip.width/2;
					valueToltip.y = mouse.y - valueToltip.height;
					valueToltip.visible = true;
				}
			}
		}
	}
}