import QtQuick
import QtQuick.Controls
import QtLocation
import QtPositioning

import ExifStats

Item
{
	id: mapRoot
	
	property alias mapChild: map
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
				value: "https://basemaps.cartocdn.com/rastertiles/dark_all/%z/%x/%y.png?key=cb1_3xkd_1_dd4cc00460f9d5b97500f455&fake=.png"
			}
		}
		
		function applySelectedLocation(pCoord)
		{
			map.center = pCoord;
			if (!keepZoomCheckbox.checked)
			{
				map.zoomLevel = 14;
			}
			mapDots.refresh();
			updateGeoShapeFilterTimer.restart();
			resultsContainer.visible = false;
			searchInput.focus = false;
		}

		GeocodeModel
		{
			id: geocodeModel
			plugin: map.plugin

			onStatusChanged:
			{
				if (status === GeocodeModel.Error)
				{
					console.warn("GeocodeModel error: " + errorString);
				}
			}

			onLocationsChanged:
			{
				if (count > 0 && searchInput.activeFocus)
				{
					resultsContainer.visible = true;
					resultsView.currentIndex = 0;
				}
				else
				{
					resultsContainer.visible = false;
				}
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
			interval: 30
			running: false
			repeat: false
			
			onTriggered:
			{
				if(restrictToViewCheckbox.checked)
				{
					MainQmlBinder.GeoShapeFilter = map.visibleRegion;
				}
			}
		}
		
		WheelHandler
		{
			id: wheel
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
			anchors.top: parent.top
			anchors.left: parent.left
			anchors.margins: 10
			checked: MainQmlBinder.GeoShapeFilter.isValid
			text: "Filter"
			onCheckedChanged:
			{
				MainQmlBinder.GeoShapeFilter = restrictToViewCheckbox.checked ? map.visibleRegion : QtPositioning.shape();
			}
		}
		CheckBox
		{
			id: showGuessedGeoLocationCheckbox
			anchors.top: restrictToViewCheckbox.bottom
			anchors.left: restrictToViewCheckbox.left
			anchors.topMargin: 4
			checked: MainQmlBinder.ShowGuessedLocation
			text: "+ Guessed Loc"
			onCheckedChanged:
			{
				MainQmlBinder.ShowGuessedLocation = showGuessedGeoLocationCheckbox.checked;
			}
		}

		Rectangle
		{
			id: searchContainer
			anchors.top: parent.top
			anchors.left: restrictToViewCheckbox.right
			anchors.right: parent.right
			anchors.margins: 15
			height: 35
			color: Qt.rgba(Material.background.r, Material.background.g, Material.background.b, 0.85)
			radius: 4

			Row
			{
				anchors.fill: parent
				anchors.leftMargin: 8
				anchors.rightMargin: 8
				spacing: 8

				TextField
				{
					id: searchInput
					width: parent.width - keepZoomCheckbox.width - parent.spacing
					height: parent.height
					placeholderText: "Search location..."
					font.pixelSize: 12
					leftPadding: 0
					rightPadding: 0
					verticalAlignment: TextInput.AlignVCenter

					background: null

					Keys.priority: Keys.Before

					onActiveFocusChanged:
					{
						if (activeFocus)
						{
							if (geocodeModel.count > 0)
							{
								resultsContainer.visible = true;
							}
						}
						else
						{
							resultsContainer.visible = false;
						}
					}
					
					onTextChanged:
					{
						geocodeModel.reset();
					}

					Keys.onDownPressed: (event) =>
					{
						if (resultsContainer.visible && resultsView.count > 0)
						{
							resultsView.currentIndex = Math.min(resultsView.currentIndex + 1, resultsView.count - 1);
							resultsView.positionViewAtIndex(resultsView.currentIndex, ListView.Contain);
							event.accepted = true;
						}
					}

					Keys.onUpPressed: (event) =>
					{
						if (resultsContainer.visible && resultsView.count > 0)
						{
							resultsView.currentIndex = Math.max(resultsView.currentIndex - 1, 0);
							resultsView.positionViewAtIndex(resultsView.currentIndex, ListView.Contain);
							event.accepted = true;
						}
					}

					onAccepted:
					{
						if (resultsContainer.visible && resultsView.count > 0)
						{
							var lIndex = resultsView.currentIndex >= 0 ? resultsView.currentIndex : 0;
							var lLoc = geocodeModel.get(lIndex);
							if (lLoc && lLoc.coordinate.isValid)
							{
								map.applySelectedLocation(lLoc.coordinate);
							}
						}
						else
						{
							if (text.trim() !== "")
							{
								geocodeModel.query = text;
								geocodeModel.update();
							}
						}
					}
				}

				CheckBox
				{
					id: keepZoomCheckbox
					checked: false
					text: "Keep Zoom"
					height: parent.height
					anchors.verticalCenter: parent.verticalCenter
				}
			}
		}

		Item
		{
			id: resultsContainer
			anchors.top: searchContainer.bottom
			anchors.topMargin: 2
			anchors.left: searchContainer.left
			anchors.right: searchContainer.right
			height: resultsView.height
			visible: false

			Rectangle
			{
				anchors.fill: parent
				color: Qt.rgba(Material.background.r, Material.background.g, Material.background.b, 0.95)
				border.color: Material.hintTextColor
				radius: 4
			}

			ListView
			{
				id: resultsView
				width: parent.width
				height: Math.min(contentHeight, 150)
				model: geocodeModel
				clip: true

				delegate: Label
				{
					width: resultsView.width
					height: 30
					color: ListView.isCurrentItem ? (Material.theme == Material.Dark ? "#000000" : "#FFFFFF") : Material.foreground
					text: locationData.address.text !== "" ? locationData.address.text : "Unknown location"

					MouseArea
					{
						anchors.fill: parent
						onClicked:
						{
							resultsView.currentIndex = index;
							map.applySelectedLocation(locationData.coordinate);
						}
					}
				}
				
				highlight: Rectangle
				{
					color: Material.accent
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