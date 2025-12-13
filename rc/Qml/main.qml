import QtQuick
import QtQuick.Window
import QtQuick.Dialogs
import Qt.labs.platform
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Shapes
import QtQuick.Layouts
import QtLocation
import QtPositioning
import ExifStats
import QtCore

import "."

Item
{
	id: mainWindow
	visible: true
	enabled: !MainQmlBinder.Processing

	width: 700
	height: 500
	
	Settings
	{
		id: settings
        property alias width: mainWindow.width
        property alias height: mainWindow.height
		property alias panel35mmVisible: focalLength35mmCounter.visible
		property alias panelApertureVisible: apertureCounter.visible
		property alias panelLensVisible: lensModelsCounter.visible
		property alias panelCameraVisible: cameraModelsCounter.visible
		property alias panelTimelineVisible: timelineCounter.visible
		property alias panelMapVisible: mapRoot.visible
		property alias panelImagesVisible: imageGrid.visible
		property var mainSplitViewState
		property var leftPanelState
		property var chartsPanelState
    }
			
	function maxList(pList)
	{
		var result = 0;
		for(var i = 0 ; i < pList.length ; ++i)
			result = Math.max(result, pList[i]);
		return result;
	}

	function displayData()
	{
		if (typeof MainQmlBinder == 'undefined')
			return;
		
		focalLength35mmCounter.categories = MainQmlBinder.getFocalLengthIn35mmLabels();
		focalLength35mmCounter.values = MainQmlBinder.getFocalLengthIn35mmCounts();

		focalLength35mmCounter.max = maxList(focalLength35mmCounter.values);
		focalLength35mmCounter.resetView();
		
		apertureCounter.categories = MainQmlBinder.getApertureLabels();
		apertureCounter.values = MainQmlBinder.getApertureCounts();
		
		apertureCounter.max = maxList(apertureCounter.values);
		apertureCounter.resetView();
	
		lensModelsCounter.categories = MainQmlBinder.getLensModels();
		lensModelsCounter.values = MainQmlBinder.getLensModelsCount();
		
		lensModelsCounter.max = maxList(lensModelsCounter.values)
		lensModelsCounter.resetView();
		
		cameraModelsCounter.categories = MainQmlBinder.getCameraModels();
		cameraModelsCounter.values = MainQmlBinder.getCameraModelsCount();
		
		cameraModelsCounter.max = maxList(cameraModelsCounter.values)
		cameraModelsCounter.resetView();
		
		timelineCounter.categories = MainQmlBinder.getTimeLabels();
		timelineCounter.values = MainQmlBinder.getTimeCounts();
		
		timelineCounter.max = maxList(timelineCounter.values)
		timelineCounter.resetView();
		
		cameraModelsList.model = []
		lensModelsList.model = []
		
		cameraModelsList.selectedItems = MainQmlBinder.getCameraModelsFilter();
		lensModelsList.selectedItems = MainQmlBinder.getLensModelsFilter();
		
		//print(JSON.stringify(cameraModelsList.selectedItems));	
		
		cameraModelsList.model = MainQmlBinder.getCameraModels();
		lensModelsList.model = MainQmlBinder.getLensModels();
		
		mapDots.setDots(MainQmlBinder.getAllGeoLocations());
		filtersPresetsList.model = MainQmlBinder.getFiltersPresets();
		filtersPresetsList.currentIndex = -1;
	}
	
	Plugin
	{
		id: mapPlugin
		name: "osm"
	}
	
	Component.onCompleted:
	{
		mapDots.setMap(map);
		imageGrid.mFilteredFilesList = MainQmlBinder.getFilteredFilesList();
		displayData();
		MainQmlBinder.dataHasChanged.connect(displayData);
		
		mainSplitView.restoreState(settings.mainSplitViewState);
		leftPanel.restoreState(settings.leftPanelState);
		chartsPanel.restoreState(settings.chartsPanelState);
	}
	
	Component.onDestruction:
	{
		settings.mainSplitViewState = mainSplitView.saveState();
		settings.leftPanelState = leftPanel.saveState();
		settings.chartsPanelState = chartsPanel.saveState();
	}

	FolderDialog
	{
		id: folderDialog
		folder: StandardPaths.standardLocations(StandardPaths.PicturesLocation)[0]
		property bool clearDB: true

		onAccepted:
		{
			MainQmlBinder.parseFolder(currentFolder, clearDB);
		}
	}
	
	Dialog
	{
		id: to35mmFocalFactorDialog
		property string cameraModelName: ""
		title: "35mm Focal Length Factor for '" + cameraModelName + "':"
		standardButtons: Dialog.Ok
		anchors.centerIn: parent
		modal: true
		
		TextField
		{
			id: to35mmFocalFactorDialogValue
		}
		
		onAccepted:
		{
			MainQmlBinder.setCameraModelTo35mmFocalLengthFactor(cameraModelName, parseFloat(to35mmFocalFactorDialogValue.text));
		}
	}
	
	Dialog
	{
		id: replaceFiltersDialog
		title: "Do you really want to replace this filters preset '" + filtersPresetsList.model[filtersPresetsList.currentIndex] + "'"
		standardButtons: MessageDialog.Yes | MessageDialog.No
		anchors.centerIn: parent
		modal: true
		
		onAccepted:
		{
			MainQmlBinder.saveFilters(filtersPresetsList.model[filtersPresetsList.currentIndex]);
			filtersPresetsList.model = MainQmlBinder.getFiltersPresets();
		}
	}
	
	Dialog
	{
		id: saveFiltersDialog
		title: "Preset Name: "
		standardButtons: MessageDialog.Save | MessageDialog.Cancel
		anchors.centerIn: parent
		modal: true
		
		TextField
		{
			id: filtersPresetName
		}
		
		onAccepted:
		{
			if(MainQmlBinder.getFiltersPresets().indexOf(filtersPresetName.text) >= 0)
			{
				replaceFiltersDialog.open();
			}
			else
			{
				MainQmlBinder.saveFilters(filtersPresetName.text);
				filtersPresetsList.model = MainQmlBinder.getFiltersPresets();
			}
		}
	}
	
	Dialog
	{
		id: deleteFiltersDialog
		title: "Do you really want to delete this filters preset '" + filtersPresetsList.model[filtersPresetsList.currentIndex] + "'"
		standardButtons: MessageDialog.Yes | MessageDialog.No
		anchors.centerIn: parent
		modal: true
		
		onAccepted:
		{
			MainQmlBinder.deleteFilters(filtersPresetsList.model[filtersPresetsList.currentIndex]);
			filtersPresetsList.model = MainQmlBinder.getFiltersPresets();
		}
	}

	SplitView
	{
		id: mainSplitView
		anchors.fill: parent
		
		Component.onCompleted:
		{
			mainSplitView.restoreState(settings.mainSplitViewState);
		}
		
		Component.onDestruction:
		{
			settings.mainSplitViewState = mainSplitView.saveState();
		}
		
		ColumnLayout
		{
			SplitView.preferredWidth: 200
			SplitView.preferredHeight: parent.height
			clip: true
			
			ColumnLayout
			{
				id: filtersPresets
				
				Layout.fillWidth: true
				
				implicitHeight: 200
				
				ListView
				{
					id: filtersPresetsList
					boundsBehavior: Flickable.StopAtBounds
					focus: true
					highlightFollowsCurrentItem: true
					
					Layout.fillWidth: true
					implicitHeight: 200
					
					model: []
					
					delegate: Text
					{
						text: modelData ? modelData : "<empty>"
						width: parent.width
						MouseArea
						{
							anchors.fill: parent
							onClicked: (pMouse)=>
							{
								filtersPresetsList.currentIndex = index;
							}
							onDoubleClicked:
							{
								filtersPresetsList.currentIndex = index;
								MainQmlBinder.loadFilters(filtersPresetsList.model[filtersPresetsList.currentIndex]);
								filtersPresetsList.currentIndex = index;
							}
						}
					}
	
					highlight: Rectangle
					{
						color: "lightsteelblue"
						width: parent.width
					}
					
					ScrollBar.vertical: ScrollBar
					{
						active: true
					}
					
					WheelHandler
					{
						onWheel: (event)=>{filtersPresetsList.flick(0, event.angleDelta.y*event.y*0.1)}
					}
					
					MouseArea
					{
						anchors.fill: parent
						propagateComposedEvents: true
						onClicked: (pMouse)=>
						{
							filtersPresetsList.currentIndex = -1;
							pMouse.accepted = false;
						}
					}
				}
				
				RowLayout
				{
					RegularButton
					{
						text:"Save"
						Layout.fillWidth: true
						
						onReleased:
						{
							filtersPresetName.text = filtersPresetsList.currentIndex >= 0 ? filtersPresetsList.model[filtersPresetsList.currentIndex] : "";
							saveFiltersDialog.open();
							filtersPresetName.focus = true;
							filtersPresetName.selectAll();
						}
					}
					
					RegularButton
					{
						text:"Load"
						Layout.fillWidth: true
						enabled: filtersPresetsList.currentIndex >= 0
						
						onReleased:
						{
							MainQmlBinder.loadFilters(filtersPresetsList.model[filtersPresetsList.currentIndex]);
						}
					}
					
					RegularButton
					{
						text:"Delete"
						Layout.fillWidth: true
						
						onReleased:
						{
							if(filtersPresetsList.currentIndex >= 0)
								deleteFiltersDialog.open();
						}
					}
				}
			}
			
			RegularButton
			{
				text:"Reset All Filters"
				
				implicitWidth: parent.width
				
				onReleased:
				{
					MainQmlBinder.resetFilters();
				}
			}
			
			RowLayout
			{
				id: pathInclusiveFilters
				
				SplitView.fillWidth: true
				SplitView.preferredHeight: parent.height / parent.children.length
				
				RegularButton
				{
					text:"X"
					
					implicitWidth: 30
					implicitHeight: 30

					onReleased:
					{
						MainQmlBinder.PathInclusiveFilters = [];
					}
				}
				
				Text
				{
					text: "Path contains: "
					Layout.fillWidth: false
				}

				TextInput
				{
					x: 0
					Layout.fillWidth: true
					text: MainQmlBinder.PathInclusiveFilters.join(" ")
					onEditingFinished:
					{
						MainQmlBinder.PathInclusiveFilters = text.split(" ");
					}
				}
			}
			
			RowLayout
			{
				id: focalLength35mmFilters
				
				SplitView.fillWidth: true
				SplitView.preferredHeight: parent.height / parent.children.length
				
				RegularButton
				{
					text:"X"
					
					implicitWidth: 30
					implicitHeight: 30

					onReleased:
					{
						MainQmlBinder.FocalLengthFrom = MainQmlBinder.MinFocalLength35mm;
						MainQmlBinder.FocalLengthTo = MainQmlBinder.MaxFocalLength35mm;
					}
				}
				
				Text
				{
					text: "Focal Length 35mm: from "
					Layout.fillWidth: false
				}

				TextInput
				{
					x: 0
					Layout.fillWidth: false
					text: MainQmlBinder.FocalLengthFrom
					validator: RegularExpressionValidator { regularExpression: /\d{1,4}/ }
					font.bold: true
					onEditingFinished:
					{
						MainQmlBinder.FocalLengthFrom = text;
					}

				}
				
				Text
				{
					text: "mm to "
					Layout.fillWidth: false
				}
				
				TextInput
				{
					Layout.fillWidth: false
					text: MainQmlBinder.FocalLengthTo
					validator: RegularExpressionValidator { regularExpression: /\d{1,4}/ }
					font.bold: true
					onEditingFinished:
					{
						MainQmlBinder.FocalLengthTo = text;
					}
				}
				
				Text
				{
					text: "mm "
					Layout.fillWidth: true
				}
			}
			
			RowLayout
			{
				id: apertureFilters
				
				SplitView.fillWidth: true
				SplitView.preferredHeight: parent.height / parent.children.length
				
				RegularButton
				{
					text:"X"
					
					implicitWidth: 30
					implicitHeight: 30

					onReleased:
					{
						MainQmlBinder.ApertureFrom = MainQmlBinder.MinAperture;
						MainQmlBinder.ApertureTo = MainQmlBinder.MaxAperture;
					}
				}
				
				Text
				{
					text: "Aperture: from "
					Layout.fillWidth: false
				}

				TextInput
				{
					x: 0
					Layout.fillWidth: false
					text: MainQmlBinder.ApertureFrom.toPrecision(2)
					validator: RegularExpressionValidator { regularExpression: /[0-9]*\.?[0-9]+/ }
					font.bold: true
					onEditingFinished:
					{
						MainQmlBinder.ApertureFrom = text;
					}

				}
				
				Text
				{
					text: " to "
					Layout.fillWidth: false
				}
				
				TextInput
				{
					Layout.fillWidth: false
					text: MainQmlBinder.ApertureTo.toPrecision(2)
					validator: RegularExpressionValidator { regularExpression: /[0-9]*\.?[0-9]+/ }
					font.bold: true
					onEditingFinished:
					{
						MainQmlBinder.ApertureTo = text;
					}
				}
				
				Text
				{
					text: ""
					Layout.fillWidth: true
				}
			}
			
			RowLayout
			{
				id: dateFilters
				
				SplitView.fillWidth: true
				SplitView.preferredHeight: parent.height / parent.children.length
				
				RegularButton
				{
					text:"X"
					
					implicitWidth: 30
					implicitHeight: 30

					onReleased:
					{
						MainQmlBinder.TimeFrom = MainQmlBinder.MinTime;
						MainQmlBinder.TimeTo = MainQmlBinder.MaxTime;
					}
				}
				
				Text
				{
					text: "Date: from "
					Layout.fillWidth: false
				}

				TextInput
				{
					x: 0
					Layout.fillWidth: false
					text: MainQmlBinder.TimeFrom
					validator: RegularExpressionValidator { regularExpression: /\d{1,4}\/\d{1,2}\/\d{1,2}/ }
					font.bold: true
					onEditingFinished:
					{
						MainQmlBinder.TimeFrom = text;
					}

				}
				
				Text
				{
					text: " to "
					Layout.fillWidth: false
				}
				
				TextInput
				{
					Layout.fillWidth: false
					text: MainQmlBinder.TimeTo
					validator: RegularExpressionValidator { regularExpression: /\d{1,4}\/\d{1,2}\/\d{1,2}/ }
					font.bold: true
					onEditingFinished:
					{
						MainQmlBinder.TimeTo = text;
					}
				}
				
				Text
				{
					text: ""
					Layout.fillWidth: true
				}
			}
				
			SplitView
			{
				id: leftPanel
				orientation: Qt.Vertical
				Layout.fillWidth: true
				Layout.fillHeight: true
				
				Component.onCompleted:
				{
					leftPanel.restoreState(settings.leftPanelState);
				}
				
				Component.onDestruction:
				{
					settings.leftPanelState = leftPanel.saveState();
				}
				
				Item
				{
					id: cameraFilters
					
					SplitView.fillWidth: true
					SplitView.preferredHeight: parent.height / parent.children.length
					
					ColumnLayout
					{
						anchors.fill: parent

						Text
						{
							text: "Camera Filters"
						}
					
						FilteringList
						{
							id: cameraModelsList
							Layout.fillWidth: true
							Layout.fillHeight: true
							
							property int selectedItemIndex: -1
							
							onSelectionHasChanged:
							{
								MainQmlBinder.setCameraModelsFilter(selectedItems);
							}

							MouseArea
							{
								anchors.fill: cameraModelsList.listViewChild
								acceptedButtons: Qt.LeftButton | Qt.RightButton
								propagateComposedEvents: true
								
								onClicked: (pMouse)=>
								{
									pMouse.accepted = false
									if (pMouse.button == Qt.RightButton)
									{
										pMouse.accepted = true
										var localMousePos = mainWindow.mapToItem(cameraModelsList.listViewChild, pMouse.x, pMouse.y);
										cameraModelsList.selectedItemIndex = cameraModelsList.listViewChild.indexAt(pMouse.x, pMouse.y);
										contextMenu.popup();
									}
								}

								Menu
								{
									id: contextMenu
									MenuItem
									{
										text: "Set 35mm equiv focal length factor"
										onTriggered:
										{
											to35mmFocalFactorDialog.cameraModelName = cameraModelsList.model[cameraModelsList.selectedItemIndex];
											to35mmFocalFactorDialogValue.text = MainQmlBinder.getCameraModelTo35mmFocalLengthFactor(to35mmFocalFactorDialog.cameraModelName);
											to35mmFocalFactorDialog.open();
										}
									}
								}
							}
						}
					}
				}
					
				Item
				{
					id: lensFilters

					SplitView.fillWidth: true
					SplitView.preferredHeight: parent.height / parent.children.length
					
					ColumnLayout
					{
						anchors.fill: parent

						Text
						{
							text: "Lens Filters"
						}
						
						FilteringList
						{
							id: lensModelsList
							Layout.fillWidth: true
							Layout.fillHeight: true
							onSelectionHasChanged:
							{
								MainQmlBinder.setLensModelsFilter(selectedItems);
							}
						}
					}
				}
			}
		}
		ColumnLayout
		{
			id: rightPanel
			SplitView.preferredWidth: parent.width - 400
			SplitView.preferredHeight: parent.height

			ColumnLayout
			{
				id: topToolbar
				Layout.preferredHeight: 160
				Layout.preferredWidth: parent.width
				
				RowLayout
				{	
					Layout.preferredHeight: 50
					
					RegularButton
					{
						text:"Open"

						onReleased:
						{
							folderDialog.clearDB = true;
							folderDialog.open();
						}
					}
					RegularButton
					{
						text:"Add"

						onReleased:
						{
							folderDialog.clearDB = false;
							folderDialog.open();
						}
					}
					RegularButton
					{
						text:"Refresh"

						onReleased:
						{
							MainQmlBinder.refresh(false);
						}
					}
					RegularButton
					{
						text:"Full Refresh"

						onReleased:
						{
							MainQmlBinder.refresh(true);
						}
					}
					RegularButton
					{
						text:"Clear"

						onReleased:
						{
							MainQmlBinder.clear();
						}
					}
					Item
					{
						width: 100
					}
					RegularButton
					{
						text:"35 mm"
						highlighted: focalLength35mmCounter.visible
						
						onReleased:
						{
							focalLength35mmCounter.visible = !focalLength35mmCounter.visible;
							chartsPanel.updateVisibleChartsCount();
						}
					}
					RegularButton
					{
						text:"Aperture"
						highlighted: apertureCounter.visible
						
						onReleased:
						{
							apertureCounter.visible = !apertureCounter.visible;
							chartsPanel.updateVisibleChartsCount();
						}
					}
					RegularButton
					{
						text:"Lens"
						highlighted: lensModelsCounter.visible
						
						onReleased:
						{
							lensModelsCounter.visible = !lensModelsCounter.visible;
							chartsPanel.updateVisibleChartsCount();
						}
					}
					RegularButton
					{
						text:"Camera"
						highlighted: cameraModelsCounter.visible
						
						onReleased:
						{
							cameraModelsCounter.visible = !cameraModelsCounter.visible;
							chartsPanel.updateVisibleChartsCount();
						}
					}
					
					RegularButton
					{
						text:"Timeline"
						highlighted: timelineCounter.visible
						
						onReleased:
						{
							timelineCounter.visible = !timelineCounter.visible;
							chartsPanel.updateVisibleChartsCount();
						}
					}
					
					RegularButton
					{
						text:"Map"
						highlighted: mapRoot.visible
						
						onReleased:
						{
							mapRoot.visible = !mapRoot.visible;
							chartsPanel.updateVisibleChartsCount();
						}
					}
					
					RegularButton
					{
						text:"Images"
						highlighted: imageGrid.visible
						
						onReleased:
						{
							imageGrid.visible = !imageGrid.visible;
						}
					}
				}

				ScrollView
				{
					Layout.preferredWidth: parent.width
					Layout.preferredHeight: 80
					clip: true
					Label
					{
						id: pathLbl
						text: MainQmlBinder.ProcessedFolders.join("\n")
					}
				}
				
				ProgressBar
				{
					id: processingProgressBar
					Layout.preferredWidth: parent.width
					Layout.preferredHeight: 20
					value: MainQmlBinder.ProcessingProgress
					opacity: MainQmlBinder.Processing ? 1.0 : 0.0
					height: 20
				}
			}
			
			SplitView
			{
				id: chartsPanel
				orientation: Qt.Vertical
				spacing: 0
				Layout.preferredHeight: parent.height - topToolbar.Layout.preferredHeight
				Layout.preferredWidth: parent.width
				
				property int visibleCharts: 0
				
				function updateVisibleChartsCount()
				{
					chartsPanel.visibleCharts = 0;
					for(var i=0; i < children.length; ++i)
					{
						if(children[i].visible)
							chartsPanel.visibleCharts++;
					}
				}

				Component.onCompleted:
				{
					updateVisibleChartsCount();
					chartsPanel.restoreState(settings.chartsPanelState);
				}
				
				Component.onDestruction:
				{
					settings.chartsPanelState = chartsPanel.saveState();
				}
				
				CounterChartFromTo
				{
					id: focalLength35mmCounter
					title: "35 mm Focal Length Stats"
					SplitView.preferredHeight: chartsPanel.visibleCharts > 0 ? parent.height / chartsPanel.visibleCharts : 0
					SplitView.preferredWidth: parent.width
					
					fromPropertyName: "FocalLengthFrom"
					toPropertyName: "FocalLengthTo"
					minPropertyName: "MinFocalLength35mm"
					maxPropertyName: "MaxFocalLength35mm"
				}
				
				CounterChartFromTo
				{
					id: apertureCounter
					title: "Aperture Stats"
					SplitView.preferredHeight: chartsPanel.visibleCharts > 0 ? parent.height / chartsPanel.visibleCharts : 0
					SplitView.preferredWidth: parent.width
					
					fromPropertyName: "ApertureFrom"
					toPropertyName: "ApertureTo"
					minPropertyName: "MinAperture"
					maxPropertyName: "MaxAperture"
				}

				CounterChart
				{
					id: lensModelsCounter
					title: "Lens Models Stats"
					SplitView.preferredHeight: chartsPanel.visibleCharts > 0 ? parent.height / chartsPanel.visibleCharts : 0
					SplitView.preferredWidth: parent.width
					barChartChild.mAllCategoriesOnly: true
				}
				
				CounterChart
				{
					id: cameraModelsCounter
					title: "Camera Models Stats"
					SplitView.preferredHeight: chartsPanel.visibleCharts > 0 ? parent.height / chartsPanel.visibleCharts : 0
					SplitView.preferredWidth: parent.width
					barChartChild.mAllCategoriesOnly: true
				}
				
				CounterChartFromTo
				{
					id: timelineCounter
					title: "Timeline Stats"
					SplitView.preferredHeight: chartsPanel.visibleCharts > 0 ? parent.height / chartsPanel.visibleCharts : 0
					SplitView.preferredWidth: parent.width
					
					fromPropertyName: "TimeFrom"
					toPropertyName: "TimeTo"
					minPropertyName: "MinTime"
					maxPropertyName: "MaxTime"
					
					selectedValueFrom: function(pValue)
					{
						return pValue;
					}
					
					selectedValueTo: function(pValue)
					{
						var locale = Qt.locale();
						
						var dateTime = Date.fromLocaleString(locale, pValue, "yyyy/MM/dd");
						var toDate = new Date(dateTime)
						toDate.setDate(dateTime.getDate() + Math.ceil(timeLineStep.value));

						return toDate.toLocaleDateString(locale, "yyyy/MM/dd");
					}
					
					SpinBox
					{
						id: timeLineStep
						x: 100
						y: 20
						value: MainQmlBinder.TimelineStep / (24 * 3600)
						from: 1
						to: 365
						stepSize: 1
						editable: true
						background: Rectangle
						{
							color: "white"
							border.color: "gray"
							border.width: 2
							radius: 10
						}
						
						onValueChanged:
						{
							MainQmlBinder.TimelineStep = value*24*3600;
						}
					}
				}

				Item
				{
					id: mapRoot
					
					SplitView.preferredHeight: chartsPanel.visibleCharts > 0 ? parent.height / chartsPanel.visibleCharts : 0
					SplitView.preferredWidth: parent.width
					
					Map
					{
						id: map
						anchors.fill: parent

						plugin: mapPlugin
						center: QtPositioning.coordinate(43.61, 3.87)
						zoomLevel: 14
						property geoCoordinate startCentroid
						
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
			}
		}
		
		ESImageGridQuickItem
		{
			id: imageGrid
			
			visible: false
			
			SplitView.preferredWidth: 200
			SplitView.preferredHeight: parent.height
			
			property real imageScale: 1.0
			
			mImageWidth: 250 * imageScale
			mImageHeight: 250 * imageScale

			onMImageFilesChanged:
			{
				mYOffset = 0;
			}
			
			MouseArea
			{
				anchors.fill: parent
				onDoubleClicked: (pMouse)=>
				{
					if(MainQmlBinder.isCtrlPressed())
					{
						imageGrid.imageScale = 1.0;
					}
					else
					{
						var selectedFile = imageGrid.getImageFileAtPos(pMouse.x, pMouse.y);
						if(selectedFile !== "")
							Qt.openUrlExternally("file:///" + selectedFile);
					}
				}
				
				property real pressX
				property real lastY

				onPressed:
				{
					pressX = mouseX;
					lastY = mouseY;
				}
				onPositionChanged:
				{
					var dy = mouseY - lastY;
					var newYOffset = 0;
					if(pressX > width - scrollBar.width)
					{
						newYOffset = imageGrid.mYOffset + dy / parent.height * imageGrid.mContentHeight;
					}
					else
					{
						newYOffset = imageGrid.mYOffset - dy;
					}
					imageGrid.mYOffset = Math.max(0, Math.min(imageGrid.mContentHeight-height, newYOffset));
					lastY = mouseY;
				}

				onWheel:
				{
					if(MainQmlBinder.isCtrlPressed())
						imageGrid.imageScale = Math.min(2.0, Math.max(0.5, imageGrid.imageScale + (wheel.angleDelta.y > 0 ? 0.1 : -0.1)));
					else
						imageGrid.mYOffset = Math.max(0, Math.min(imageGrid.mContentHeight-height, imageGrid.mYOffset - wheel.angleDelta.y));
				}
			}
			
			Rectangle
			{
				id: scrollBar
				width: 10
				
				radius: 3
				color: "#55000000"
				anchors.right: parent.right
				height: Math.max(32, parent.height * parent.height / imageGrid.mContentHeight)
				y: parent.height * (imageGrid.mYOffset / imageGrid.mContentHeight)
			}
			
			ProgressBar
			{
				id: loadingProgressBar
				anchors.top: parent.top
				width: parent.width
				value: imageGrid.mLoadingProgress
				opacity: imageGrid.mLoading ? 1.0 : 0.0
				height: 15
			}
		}
	}
}
