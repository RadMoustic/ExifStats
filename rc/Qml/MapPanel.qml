import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import ExifStats

Item
{
	id: mapRoot
	
	property alias mapDotsChild: mapDots
	property alias panEnabled: drag.enabled
	
	Component.onCompleted:
	{
		mapDots.setMap(map);
	}
	
	Map
	{
		id: map
		anchors.fill: parent

		color: Material.background
		plugin: mapPlugin
		center: QtPositioning.coordinate(43.61, 3.87)
		zoomLevel: 14
		property geoCoordinate startCentroid
		
		Plugin
		{
			id: mapPlugin
			name: "osm"
			
			PluginParameter
			{
				name: "osm.mapping.custom.host"
				value: "https://basemaps.cartocdn.com/dark_all/"
			}
		}
		
		function updateMapTheme()
		{
			var isDark = (map.Material.theme === Material.Dark);
			for (var i = 0; i < map.supportedMapTypes.length; ++i)
			{
				var type = map.supportedMapTypes[i];
				if (isDark && type.name.includes("Custom"))
				{
					map.activeMapType = type;
					return;
				}
				else if (!isDark && !type.name.includes("custom"))
				{
					map.activeMapType = type;
					return;
				}
			}
		}
		
		Component.onCompleted: updateMapTheme()
		
		Connections
		{
			target: map.Material
			function onThemeChanged()
			{
				map.updateMapTheme()
			}
		}
		
		Timer
		{
			id: updateGeoShapeFilterTimer
			interval: 100
			running: false
			repeat: false
			
			onTriggered:
			{
				if(restrictToViewCheckbox.checked)
				{
					MainQmlBinder.setGeoShapeFilter(map.visibleRegion);
				}
			}
		}
		
		WheelHandler
		{
			id: wheel
			// workaround for QTBUG-87646 / QTBUG-112394 / QTBUG-112432:
			// Magic Mouse pretends to be a trackpad but doesn't work with PinchHandler
			// and we don't yet distinguish mice and trackpads on Wayland either
			acceptedDevices: Qt.platform.pluginName === "cocoa" || Qt.platform.pluginName === "wayland"
							 ? PointerDevice.Mouse | PointerDevice.TouchPad
							 : PointerDevice.Mouse
			onWheel: (event) =>
			{
				const loc = map.toCoordinate(wheel.point.position);
				map.zoomLevel += event.angleDelta.y / 120;
				map.alignCoordinateToPoint(loc, wheel.point.position);
				mapDots.refresh();
				updateGeoShapeFilterTimer.restart();
			}
		}
		DragHandler
		{
			id: drag
			target: null
			onTranslationChanged: (delta) =>
			{
				map.pan(-delta.x, -delta.y);
				mapDots.refresh();
				updateGeoShapeFilterTimer.restart();
			}
		}
		PinchHandler
		{
			id: pinch
			target: null
			
			onActiveChanged:
			{
				if (active)
				{
					map.startCentroid = map.toCoordinate(pinch.centroid.position);
				}
			}
			
			onScaleChanged: (delta) =>
			{
				map.zoomLevel += Math.log2(delta);
				map.alignCoordinateToPoint(map.startCentroid, pinch.centroid.position);
				mapDots.refresh();
				updateGeoShapeFilterTimer.restart();
			}
		}
		MouseArea
		{
			anchors.fill: parent
			onClicked: (pMouse)=>
			{
				const loc = map.toCoordinate(Qt.point(pMouse.x, pMouse.y));
				const precision = 10; // in pixel
				const loc2 = map.toCoordinate(Qt.point(pMouse.x + precision, pMouse.y + precision));
				var files = MainQmlBinder.getFilesAtLocation(Qt.point(loc.latitude, loc.longitude), loc2.distanceTo(loc));
				imageGrid.mImageFiles = files;
			}
		}
		
		CheckBox
		{
			id: restrictToViewCheckbox
			checked: false
			text: "Restrict to view"
			onCheckedChanged:
			{
				if(restrictToViewCheckbox.checked)
				{
					MainQmlBinder.setGeoShapeFilter(map.visibleRegion);
				}
				else
				{
					MainQmlBinder.setGeoShapeFilter(QtPositioning.shape());
				}
			}
		}
	}
	ESMapDotsQuickItem
	{
		id: mapDots
		anchors.fill: parent
	}
}