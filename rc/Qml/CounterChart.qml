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
	
	property var popupMenuFunction: function(pMouse) {}
	
	property alias barChartChild: barChartItem
	
	function resetView()
	{
		valueToltip.visible = false;
		barChartItem.mCategoryAxisOffset = 0;
		barChartItem.mCategoryAxisScale = 1.0;
		barChartItem.mValueAxisScale = 1.0;
	}
	
	onVisibleChanged:
	{
		barChartItem.Material.theme = barChartItem.parent.parent.parent.Material.theme;
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
		mCategoryAxisSize: 0
		mCategoryAxisSizeAuto: true
		mCategoryAxisMaxSizeAuto: 150
		mValueAxisSize: 40
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
			onWheel: (pEvent)=>
			{
				valueToltip.visible = false;
				var scale = pEvent.angleDelta.y > 0 ? 1.1 : 0.9;
				if(MainQmlBinder.isCtrlPressed())
				{
					barChartItem.mValueAxisScale *= scale;
				}
				else
				{
					var p = barChartItem.mapToPlotArea(pEvent.x, pEvent.y);
					var mouseX = barChartItem.mInvertAxis ? p.y : p.x;
					var oldChartFullSize = barChartItem.getChartFullSize();
					barChartItem.mCategoryAxisScale *= scale;
					var newChartFullSize = barChartItem.getChartFullSize();
					barChartItem.mCategoryAxisOffset = (barChartItem.mCategoryAxisOffset - mouseX) * newChartFullSize / oldChartFullSize + mouseX;
				}
			}
		}

		PinchArea 
		{
			anchors.fill: parent
			
			property real startDistX: 0
			property real startDistY: 0
			property real startScaleCategory: 1.0
			property real startScaleValue: 1.0
			property real startOffset: 0
			property real pinchStart: 0
			property real oldFullSize: 0
			
			onPinchStarted: (pPinch) =>
			{
				valueToltip.visible = false;
				startDistX = Math.max(1, Math.abs(pPinch.point1.x - pPinch.point2.x));
				startDistY = Math.max(1, Math.abs(pPinch.point1.y - pPinch.point2.y));
				
				if(barChartItem.mInvertAxis)
				{
					var tmp = startDistY;
					startDistY = startDistX;
					startDistX = tmp;
				}
				
				startScaleCategory = barChartItem.mCategoryAxisScale;
				startScaleValue = barChartItem.mValueAxisScale;
				startOffset = barChartItem.mCategoryAxisOffset;
				oldFullSize = barChartItem.getChartFullSize();
				
				var pinchCenterInPlotArea = barChartItem.mapToPlotArea(pPinch.center.x, pPinch.center.y);
				if(barChartItem.mInvertAxis)
					pinchStart = pinchCenterInPlotArea.y;
				else
					pinchStart = pinchCenterInPlotArea.x;
			}

			onPinchUpdated: (pPinch) =>
			{
				var currentDistX = Math.abs(pPinch.point1.x - pPinch.point2.x);
				var currentDistY = Math.abs(pPinch.point1.y - pPinch.point2.y);
				
				if(barChartItem.mInvertAxis)
				{
					var tmp = currentDistY;
					currentDistY = currentDistX;
					currentDistX = tmp;
				}
				
				if (startDistX > 20 && currentDistX > 20) 
				{
					var newScale = startScaleCategory * (currentDistX / startDistX);
					barChartItem.mCategoryAxisScale = newScale;
					var newFullSize = barChartItem.getChartFullSize();
					barChartItem.mCategoryAxisOffset = (startOffset - pinchStart) * newFullSize / oldFullSize + pinchStart;
				}
				
				if (startDistY > 50 && currentDistY > 50) 
				{
					var newScale = startScaleValue * (currentDistY / startDistY);
					barChartItem.mValueAxisScale = newScale;
				}
			}

			MouseArea
			{
				anchors.fill: parent
				
				acceptedButtons: Qt.LeftButton | Qt.RightButton
				
				property real lastX: 0
				property real lastY: 0
				property real pressX: 0
				property real pressY: 0
				
				onPressed: (pMouse) =>
				{
					lastX = pMouse.x;
					lastY = pMouse.y;
					pressX = pMouse.x;
					pressY = pMouse.y;
				}
				
				onPositionChanged: (pMouse) =>
				{
					if (pressed) 
					{
						if(pressedButtons & Qt.LeftButton)
						{
							valueToltip.visible = false;
							if(barChartItem.mInvertAxis)
								barChartItem.mCategoryAxisOffset += (pMouse.y - lastY);
							else
								barChartItem.mCategoryAxisOffset += (pMouse.x - lastX);
							lastX = pMouse.x;
							lastY = pMouse.y;
						}
					}
				}
				
				onDoubleClicked: (pMouse) =>
				{
					if(pMouse.button == Qt.LeftButton)
						rootItem.resetView();
				}
				
				onClicked: (pMouse) =>
				{
					pMouse.accepted = false;
					if (Math.abs(pMouse.x - pressX) > 5 || Math.abs(pMouse.y - pressY) > 5) return;
					
					if(pMouse.button == Qt.LeftButton)
					{
						pMouse.accepted = true;
						var p = barChartItem.mapToValue(barChartItem.mInvertAxis ? pMouse.y : pMouse.x);
						valueToltip.text = rootItem.categories[p.x] + " => " + p.y;
						valueToltip.x = pMouse.x - valueToltip.width/2;
						valueToltip.y = pMouse.y - valueToltip.height;
						valueToltip.visible = true;
					}
					else if (pMouse.button == Qt.RightButton)
					{
						popupMenuFunction(pMouse);
					}
				}

				onPressAndHold: (pMouse)=>
				{
					pMouse.accepted = false;
					if (Math.abs(pMouse.x - pressX) > 5 || Math.abs(pMouse.y - pressY) > 5) return;

					if(MainQmlBinder.isMobile())
						popupMenuFunction(pMouse);
				}
			}
		}
	}
}