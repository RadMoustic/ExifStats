import QtQuick
import Qt.labs.platform
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts 
import QtQuick.Dialogs
import QtLocation
import QtCore
import QtQuick.Window

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
		
		isoSpeedCounter.categories = MainQmlBinder.getISOSpeedLabels();
		isoSpeedCounter.values = MainQmlBinder.getISOSpeedCounts();
		
		isoSpeedCounter.max = maxList(isoSpeedCounter.values);
		isoSpeedCounter.resetView();
		
		shutterSpeedCounter.categories = MainQmlBinder.getShutterSpeedLabels();
		shutterSpeedCounter.categoriesRealValues = MainQmlBinder.getShutterSpeedValues();
		shutterSpeedCounter.values = MainQmlBinder.getShutterSpeedCounts();
		
		shutterSpeedCounter.max = maxList(shutterSpeedCounter.values);
		shutterSpeedCounter.resetView();
	
		lensModelsCounter.categories = MainQmlBinder.getLensModels();
		lensModelsCounter.values = MainQmlBinder.getLensModelsCount();
		
		lensModelsCounter.max = maxList(lensModelsCounter.values)
		lensModelsCounter.resetView();
		
		cameraModelsCounter.categories = MainQmlBinder.getCameraModels();
		cameraModelsCounter.values = MainQmlBinder.getCameraModelsCount();
		
		cameraModelsCounter.max = maxList(cameraModelsCounter.values)
		cameraModelsCounter.resetView();
		
		resolutionsCounter.categories = MainQmlBinder.getResolutions();
		resolutionsCounter.values = MainQmlBinder.getResolutionsCount();
		
		resolutionsCounter.max = maxList(resolutionsCounter.values)
		resolutionsCounter.resetView();

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
		id: loadSettingsTimer
		interval: 0
		repeat: false
		onTriggered:
		{
			//MainQmlBinder.loadDefaultFilters();
			imageGrid.gridCol = Math.min(imageGrid.maxGridCol, settings.imageGridCol);
			//imageGrid.flickableChild.contentY = settings.imageGridYOffset;
		}
	}
	
	Component.onCompleted:
	{
		imageGrid.mFilteredFilesList = MainQmlBinder.getFilteredFilesList();
		displayData();
		MainQmlBinder.dataHasChanged.connect(displayData);
		
		mainWindow.Material.theme = settings.theme ? Material.Dark : Material.Light;
		sidePanel.Material.theme = settings.theme ? Material.Dark : Material.Light;
		
		loadSettingsTimer.start();
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
			MainQmlBinder.setDatabaseArchive(selectedFile);
		}
	}
	
	FolderDialog 
	{
		id: searchModelDialog
		title: "Select a folder containing the *.essm models files"
		
		onAccepted:
		{
			MainQmlBinder.installSearchModels(selectedFolder);
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
						text:"Search Models"
						enabled: !MainQmlBinder.mSearchModelsExtracting
						
						onReleased:
						{
							searchModelDialog.open();
						}
					}
					RegularButton
					{
						text:"Logs"
						
						onReleased:
						{
							sidePanel.close();
							logsViewer.visible = true;
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
				
				ProgressBar
				{
					id: searchModelsInstallationProgressBar
					Layout.preferredWidth: parent.width
					Layout.preferredHeight: 10
					value: MainQmlBinder.mSearchModelsExtractingProgress
					opacity: MainQmlBinder.mSearchModelsExtracting ? 1.0 : 0.0
					height: 10
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
						
						onFiltersResetted:
						{
							imageGrid.mImageFiles = [];
						}
					}
				}
			}
		}
	}
	
	ColumnLayout
	{
		id: mainLayout
		
		anchors.fill: parent
		anchors.topMargin: mainWindow.Window.window ? mainWindow.Window.window.SafeArea.margins.top : 0
		
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
				
					model: ["Gallery", "Map", "Timeline", "Camera", "Lens", "35mm", "Aperture", "ISO", "Shutter Speed", "Resolution", "Orientation", "Folders"]
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
				imageViewerItem: imageViewerRoot
				mapItem: mapRoot
				mapShowAndFocusFunction: function() { tabBar.currentIndex = 1; }
			}
			
			MapPanel
			{
				id: mapRoot
			}
			
			TimelineCounterChart
			{
				id: timelineCounter
			}
			
			CounterChart
			{
				id: cameraModelsCounter
				title: "Camera Models Stats"
				barChartChild.mAllCategoriesOnly: true
			}
			
			CounterChart
			{
				id: lensModelsCounter
				title: "Lens Models Stats"
				barChartChild.mAllCategoriesOnly: true
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
					
			CounterChartFromTo
			{
				id: isoSpeedCounter
				title: "ISO Stats"
				
				fromPropertyName: "ISOSpeedFrom"
				toPropertyName: "ISOSpeedTo"
				minPropertyName: "MinISOSpeed"
				maxPropertyName: "MaxISOSpeed"
			}
			
			CounterChartFromTo
			{
				id: shutterSpeedCounter
				title: "Shutter Speed Stats"
				
				fromPropertyName: "ShutterSpeedFrom"
				toPropertyName: "ShutterSpeedTo"
				minPropertyName: "MinShutterSpeed"
				maxPropertyName: "MaxShutterSpeed"
			}
					
			CounterChart
			{
				id: resolutionsCounter
				title: "Resolutions Stats"
				SplitView.preferredHeight: chartsPanel.visibleCharts > 0 ? parent.height / chartsPanel.visibleCharts : 0
				SplitView.preferredWidth: parent.width
				barChartChild.mAllCategoriesOnly: true
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
	
	ImageViewer
	{
		id: imageViewerRoot
		visible: false
		anchors.fill: parent
	}
	
	PreviousCrashView
	{
		id: previousCrashView
		anchors.fill: parent
	}
	
	FallbackQmlErrors
	{
		id: logsViewer
		anchors.fill: parent
		visible: false
		
		RoundButton
		{
			id: closeLogsButton
			text: "X"
			anchors.top: parent.top
			anchors.right: parent.right
			anchors.margins: 15
			radius: 4
			
			implicitWidth: 40
			implicitHeight: 40

			onReleased:
			{
				logsViewer.visible = false;
			}
		}
	}
}