import QtQuick
import Qt.labs.platform
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts 
import QtQuick.Dialogs
import QtLocation
import QtCore
import QtQuick.Effects

import ExifStats

Pane
{
	id: mainWindow
	visible: true
	enabled: !MainQmlBinder.Processing
	
	padding: 0
	
	Material.theme: Material.Dark
	Material.accent: Material.BlueGrey
	
	Settings
	{
		id: settings
		
		property var theme
		property var imageGridYOffset
		property var imageGridCol
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
	
	Timer
	{
		id: loadDefaultFiltersTimer
		interval: 0
		repeat: false
		onTriggered:
		{
			MainQmlBinder.loadDefaultFilters();
			imageGrid.gridCol = settings.imageGridCol;
			imageGrid.flickableChild.contentY = settings.imageGridYOffset;
		}
	}
	
	Component.onCompleted:
	{
		imageGrid.mFilteredFilesList = MainQmlBinder.getFilteredFilesList();
		displayData();
		MainQmlBinder.dataHasChanged.connect(displayData);
		
		mainWindow.Material.theme = settings.theme ? Material.Dark : Material.Light;
		sidePanel.Material.theme = settings.theme ? Material.Dark : Material.Light;
		
		loadDefaultFiltersTimer.start();
	}
	
	Component.onDestruction:
	{
		settings.theme = mainWindow.Material.theme === Material.Dark ? true : false;
		settings.imageGridYOffset = imageGrid.flickableChild.contentY;
		settings.imageGridCol = imageGrid.gridCol;
		MainQmlBinder.saveDefaultFilters();
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
		
		Material.theme: mainWindow.Material.Dark
		Material.accent: mainWindow.Material.BlueGrey
		
		width: parent.width * 0.90
		height: parent.height
		
		edge: Qt.LeftEdge
		interactive: true
		
		onPositionChanged: mapRoot.panEnabled = (position === 0.0)
		
		Item
		{
			anchors.fill: parent

			ColumnLayout
			{
				id: sidePanelMainLayout
				anchors.fill: parent
				
				RowLayout
				{
					RegularButton
					{
						text:"Database"
						
						onReleased:
						{
							databaseDialog.open();
						}
					}
					RegularButton
					{
						text:"Models"
						
						onReleased:
						{
							tokenizerFolderDialog.open();
						}
					}
					Item
					{
						Layout.fillWidth: true
					}
					RegularButton
					{
						text:"Dark"
						highlighted: mainWindow.Material.theme == Material.Dark
						
						onReleased:
						{
							if(mainWindow.Material.theme == Material.Dark)
							{
								mainWindow.Material.theme = Material.Light;
								sidePanel.Material.theme = Material.Light;
							}
							else
							{
								mainWindow.Material.theme = Material.Dark;
								sidePanel.Material.theme = Material.Dark;
							}
							MainQmlBinder.themeHasChanged();
						}
					}
				}

				Pane
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
		
		RoundButton
		{
			id: closeButton
			text: "X"
			anchors.top: parent.top
			anchors.right: parent.right
			anchors.margins: 15
			radius: 4
			
			implicitWidth: 40
			implicitHeight: 40

			contentItem: Text
			{
				text: closeButton.text
				font.pixelSize: 15
				font.bold: true
				color: "#666666"
				horizontalAlignment: Text.AlignHCenter
				verticalAlignment: Text.AlignVCenter
			}

			background: Item
			{
				Rectangle
				{
					id: bgRect
					anchors.fill: parent
					color: closeButton.down ? "#222222" : "#111111"
					border.color: "#666666"
					border.width: 2
					radius: closeButton.radius
					visible: false
				}

				MultiEffect
				{
					source: bgRect
					anchors.fill: parent
					shadowEnabled: true
					shadowColor: "#CC000000"
					shadowBlur: 1.0
				}
			}
			
			onReleased:
			{
				imageViewer.visible = false;
				MainQmlBinder.mFullScreen = false;
			}
		}
	}
	
	PreviousCrashView
	{
		id: previousCrashView
		anchors.fill: parent
	}
}