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
	
	FolderDialog 
	{
		id: tokenizerFolderDialog
		title: "Select a folder with tokenizer models"
		
		onAccepted:
		{
			MainQmlBinder.setTokenizerFolder(selectedFolder);
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
	
	Drawer
	{
		id: sidePanel
		
		width: parent.width * 0.90
		height: parent.height
		
		edge: Qt.LeftEdge
		interactive: true
		
		onPositionChanged: mapRoot.panEnabled = (position === 0.0)
		
		Rectangle
		{
			anchors.fill: parent
			
			ColumnLayout
			{
				anchors.fill: parent
				
				RowLayout
				{
					RegularButton
					{
						text:"Select Database"
						
						onReleased:
						{
							databaseDialog.open();
						}
					}
					RegularButton
					{
						text:"Select Tokenizer Folder"
						
						onReleased:
						{
							tokenizerFolderDialog.open();
						}
					}

				}
				
				Text
				{
					text: "Filters Presets"
					font.pointSize: 12
				}
				
				Rectangle
				{
					Layout.fillHeight: true
					Layout.fillWidth: true
					
					FiltersPanel
					{
						id: filtersPanel
						clip: true
						anchors.fill: parent
					}
				}
			}
		}
	}
	
	ColumnLayout
	{
		id: mainLayout
		
		anchors.fill: parent
		
		RowLayout
		{
			RoundButton
			{
				icon.source: "qrc:/Images/Menu.png"
				icon.width: tabBar.height-30
				icon.height: tabBar.height-30
				display: AbstractButton.IconOnly
				radius: 0
				padding: 20

				onReleased: sidePanel.open()
			}
			
			TabBar
			{
				id: tabBar
				Layout.fillWidth: true
				currentIndex: 0
				clip: true
				
				Repeater
				{
				
					model: ["Gallery", "Map", "35mm", "Aperture", "Lens", "Camera", "Timeline", "Orientation", "Folders"]
					TabButton
					{
						text: modelData
						width: implicitWidth
					}
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
			
			GalleryPanel
			{
				id: imageGrid
				imageViewerItem: imageViewer
			}
			
			MapPanel
			{
				id: mapRoot
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
		}
	}
	
	ESImageViewerQuickItem
	{
		id: imageViewer
		visible: false
		anchors.fill: parent

		MouseArea
		{
			anchors.fill: parent
			
			property int swipeThreshold: 50
			property int startX: 0

			onPressed: (pMouse) =>
			{
				startX = pMouse.x
			}

			onReleased: (pMouse) =>
			{
				var diffX = pMouse.x - startX

				if (Math.abs(diffX) > swipeThreshold)
				{
					var newImage = "";
					if (diffX < 0)
					{
						newImage = imageGrid.getNextImage(imageViewer.mImagePath, 5);
					}
					else
					{
						newImage = imageGrid.getPreviousImage(imageViewer.mImagePath, 5);
					}
					if(newImage != "")
					{
						imageViewer.mImagePath = newImage;
					}
				}
			}
		}
		
		RegularButton
		{
			id: sortMode
			text: "X"
			anchors.right: parent.right
			anchors.margins: 10
			
			implicitWidth: 30
			implicitHeight: 30

			onReleased:
			{
				imageViewer.visible = false;
				MainQmlBinder.mFullScreen = false;
			}
		}
	}
}
