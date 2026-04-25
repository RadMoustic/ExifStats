import QtQuick
import Qt.labs.platform
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtLocation
import QtCore

import ExifStats

Pane
{
	id: mainWindow
	visible: true
	enabled: !MainQmlBinder.Processing
	
	Material.theme: Material.Dark
	Material.accent: Material.BlueGrey

	width: 700
	height: 500
	
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
		property var theme
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
		
	Component.onCompleted:
	{
		imageGrid.mFilteredFilesList = MainQmlBinder.getFilteredFilesList();
		displayData();
		MainQmlBinder.dataHasChanged.connect(displayData);
		
		mainSplitView.restoreState(settings.mainSplitViewState);
			centerPanel.restoreState(settings.centerPanelState);
		chartsPanel.restoreState(settings.chartsPanelState);
		mainWindow.Material.theme = settings.theme;
	}
	
	Component.onDestruction:
	{
		settings.mainSplitViewState = mainSplitView.saveState();
		settings.centerPanelState = centerPanel.saveState();
		settings.chartsPanelState = chartsPanel.saveState();
		settings.theme = mainWindow.Material.theme;
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
		
		FiltersPanel
		{
			id: filtersPanel
			
			SplitView.preferredWidth: 200
			SplitView.preferredHeight: parent.height
		}
		
		ColumnLayout
		{
			id: rightPanel
			SplitView.preferredHeight: parent.height
			SplitView.minimumWidth: 350
			clip: true

			ColumnLayout
			{
				id: topToolbar
				Layout.preferredHeight: 90
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
						text:"ReTag"
						enabled: MainQmlBinder.isImageTaggerEnabled()

						onReleased:
						{
							MainQmlBinder.retag();
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
					ColumnLayout
					{
						Layout.preferredHeight: 50
						CheckBox
						{
							id: pauseCaching
							Layout.preferredHeight: 25
							padding: 0
							checked: MainQmlBinder.mPauseCaching
							text: "Pause Caching"
							onCheckedChanged:
							{
								MainQmlBinder.mPauseCaching = pauseCaching.checked;
							}
						}
						CheckBox
						{
							id: pauseTagging
							enabled: MainQmlBinder.isImageTaggerEnabled()
							Layout.preferredHeight: 25
							padding: 0
							checked: MainQmlBinder.mPauseTagging
							text: "Pause Tagging"
							onCheckedChanged:
							{
								MainQmlBinder.mPauseTagging = pauseTagging.checked;
							}
						}
					}
					Item
					{
						width: 100
					}
					RegularButton
					{
						text:"Folders"
						highlighted: foldersList.visible
						
						onReleased:
						{
							foldersList.visible = !foldersList.visible;
							chartsPanel.updateVisibleChartsCount();
						}
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
						text:"Orientation"
						highlighted: orientationsCounter.visible
						
						onReleased:
						{
							orientationsCounter.visible = !orientationsCounter.visible;
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
					
					Item
					{
						Layout.fillWidth: true
					}
					
					RegularButton
					{
						text:"Dark Theme"
						highlighted: mainWindow.Material.theme == Material.Dark
						
						onReleased:
						{
							if(mainWindow.Material.theme == Material.Dark)
								mainWindow.Material.theme = Material.Light;
							else
								mainWindow.Material.theme = Material.Dark;
							MainQmlBinder.themeHasChanged();
						}
					}
				}

				ProgressBar
				{
					id: processingProgressBar
					Layout.preferredWidth: parent.width
					Layout.preferredHeight: 10
					value: MainQmlBinder.ProcessingProgress
					opacity: MainQmlBinder.Processing ? 1.0 : 0.0
					height: 10
				}
				ProgressBar
				{
					id: taggingProgressBar
					Layout.preferredWidth: parent.width
					Layout.preferredHeight: 10
					value: MainQmlBinder.mTaggingProgress
					opacity: MainQmlBinder.mTagging ? 1.0 : 0.0
					height: 10
					
					Material.accent: Material.Green
				}
				ProgressBar
				{
					id: updatingHNSWIndexProgressBar
					Layout.preferredWidth: parent.width
					Layout.preferredHeight: 10
					value: MainQmlBinder.UpdatingHNSWIndexProgress
					opacity: MainQmlBinder.UpdatingHNSWIndex ? 1.0 : 0.0
					height: 10
					
					Material.accent: Material.Blue
				}
			}
			
			SplitView
			{
				id: centerPanel
				orientation: Qt.Horizontal
				
				Layout.preferredHeight: parent.height - topToolbar.Layout.preferredHeight
				Layout.preferredWidth: parent.width
				
				SplitView
				{
					id: chartsPanel
					orientation: Qt.Vertical
					spacing: 0
					
					SplitView.minimumWidth: 350
					
					
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
					
					ScrollView
					{
						id: foldersList
						
						SplitView.preferredHeight: chartsPanel.visibleCharts > 0 ? parent.height / chartsPanel.visibleCharts : 0
						SplitView.preferredWidth: parent.width
						
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
					
					TimelineCounterChart
					{
						id: timelineCounter
						SplitView.preferredHeight: chartsPanel.visibleCharts > 0 ? parent.height / chartsPanel.visibleCharts : 0
						SplitView.preferredWidth: parent.width
					}
					
					CounterChart
					{
						id: orientationsCounter
						title: "Orientation Stats"
						SplitView.preferredHeight: chartsPanel.visibleCharts > 0 ? parent.height / chartsPanel.visibleCharts : 0
						SplitView.preferredWidth: parent.width
						barChartChild.mAllCategoriesOnly: true
					}

					MapPanel
					{
						id: mapRoot
						
						SplitView.preferredHeight: chartsPanel.visibleCharts > 0 ? parent.height / chartsPanel.visibleCharts : 0
						SplitView.preferredWidth: parent.width
					}
				}
				GalleryPanel
				{
					id: imageGrid
					
					visible: false
					
					SplitView.preferredWidth: 200
					SplitView.preferredHeight: parent.height
					SplitView.minimumWidth: 270
				}
			}
		}
	}
}
