import QtQuick
import Qt.labs.platform
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtLocation
import QtCore

import ExifStats

Item
{
	id: mainWindow
	visible: true
	enabled: !MainQmlBinder.Processing
	
	Settings
	{
		id: settings
        property alias width: mainWindow.width
        property alias height: mainWindow.height
		property alias pathListVisible: foldersList.visible
		property alias panel35mmVisible: focalLength35mmCounter.visible
		property alias panelApertureVisible: apertureCounter.visible
		property alias panelLensVisible: lensModelsCounter.visible
		property alias panelCameraVisible: cameraModelsCounter.visible
		property alias panelTimelineVisible: timelineCounter.visible
		property alias panelOrientationsCounterVisible: orientationsCounter.visible
		property alias panelMapVisible: mapRoot.visible
		property alias panelImagesVisible: imageGrid.visible
		property var mainSplitViewState
		property var leftPanelState
		property var centerPanelState
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

		orientationsCounter.categories = MainQmlBinder.getOrientations();
		orientationsCounter.values = MainQmlBinder.getOrientationsCount();
		
		orientationsCounter.max = maxList(orientationsCounter.values)
		orientationsCounter.resetView();
		
		timelineCounter.categories = MainQmlBinder.getTimeLabels();
		timelineCounter.values = MainQmlBinder.getTimeCounts();
		
		timelineCounter.max = maxList(timelineCounter.values)
		timelineCounter.resetView();
				
		mapRoot.mapDotsChild.setDots(MainQmlBinder.getAllGeoLocations());

		filtersPanel.displayData();
	}
	
	Plugin
	{
		id: mapPlugin
		name: "osm"
	}
	
	Component.onCompleted:
	{
		imageGrid.mFilteredFilesList = MainQmlBinder.getFilteredFilesList();
		displayData();
		MainQmlBinder.dataHasChanged.connect(displayData);
	}
	
	Component.onDestruction:
	{
	}
	
	FileDialog
	{
		id: databaseDialog
		title: "Select an ExifStats archive"
		nameFilters: ["ExifStats Archive (*.esar)"]
		
		onAccepted:
		{
			MainQmlBinder.setDatabaseFolder(selectedFile);
		}
	}
	
	Dialog
	{
		id: to35mmFocalFactorDialog
		property string cameraModelName: ""
		title: "35mm Focal Length Factor for '" + cameraModelName + "':"
		standardButtons: Dialog.Ok
		anchors.centerIn: Overlay.overlay
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
	
	ColumnLayout
	{
		id: mainLayout
		
		anchors.fill: parent
		
		TabBar
		{
			id: tabBar
			Layout.fillWidth: true
			
			Repeater
			{
			
				model: ["[]", "Filters", "Folders", "35mm", "Aperture", "Lens", "Camera", "Timeline", "Orientation", "Map", "Gallery"]
				TabButton
				{
					text: modelData
					width: implicitWidth
				}
			}
		}
		
		StackLayout
		{
			id: mainStackLayout
			currentIndex: tabBar.currentIndex
			anchors.top: tabBar.bottom
			anchors.left: parent.left
			anchors.right: parent.right
			anchors.bottom: parent.bottom
			clip: true
			
			Item
			{
				id: settingsPanel
				
				RegularButton
				{
					text:"Select Database"
					
					onReleased:
					{
						databaseDialog.open();
					}
				}
			}
			
			FiltersPanel
			{
				id: filtersPanel
				anchors.fill: parent
				clip: true
			}
			
			ScrollView
			{
				id: foldersList
							
				clip: true
				Label
				{
					id: pathLbl
					text: MainQmlBinder.ProcessedFolders.join("\n")
				}
			}
			
			CounterChartFromTo
			{
				id: focalLength35mmCounter
				title: "35 mm Focal Length Stats"
				
				fromPropertyName: "FocalLengthFrom"
				toPropertyName: "FocalLengthTo"
				minPropertyName: "MinFocalLength35mm"
				maxPropertyName: "MaxFocalLength35mm"
			}
			
			CounterChartFromTo
			{
				id: apertureCounter
				title: "Aperture Stats"
				
				fromPropertyName: "ApertureFrom"
				toPropertyName: "ApertureTo"
				minPropertyName: "MinAperture"
				maxPropertyName: "MaxAperture"
			}

			CounterChart
			{
				id: lensModelsCounter
				title: "Lens Models Stats"
				barChartChild.mAllCategoriesOnly: true
			}
			
			CounterChart
			{
				id: cameraModelsCounter
				title: "Camera Models Stats"
				barChartChild.mAllCategoriesOnly: true
			}
			
			TimelineCounterChart
			{
				id: timelineCounter
			}
			
			CounterChart
			{
				id: orientationsCounter
				title: "Orientation Stats"
				barChartChild.mAllCategoriesOnly: true
			}

			MapPanel
			{
				id: mapRoot
			}
			GalleryPanel
			{
				id: imageGrid
			}
		}
	}
}
