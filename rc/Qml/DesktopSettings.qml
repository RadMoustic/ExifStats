import QtQuick
import QtQuick.Controls
import QtQuick.Controls.Material
import QtQuick.Layouts
import QtQuick.Dialogs
import QtLocation
import QtCore

import ExifStats

Popup
{
	id: rootItem
	anchors.centerIn: Overlay.overlay
	width: 300
	height: mainSettingsLayout.implicitHeight + 50
	modal: true
	focus: true
	
	FileDialog 
	{
		id: searchModelDialog
		title: "Install the ExifStats Search Models"
		nameFilters: ["ExifStats Search Models (*.essm)"]
		
		onAccepted:
		{
			MainQmlBinder.installSearchModels(selectedFile);
		}
	}
	
	FileDialog 
	{
		id: taggingModelDialog
		title: "Install the ExifStats Tagging Models"
		nameFilters: ["ExifStats Tagging Models (*.estm)"]
		
		onAccepted:
		{
			MainQmlBinder.installTaggingModels(selectedFile);
		}
	}

	ColumnLayout
	{
		id: mainSettingsLayout
		anchors.centerIn: parent
		spacing: 15
		width: parent.width - 40

		RegularButton
		{
			text:"Install Search Models"
			enabled: !MainQmlBinder.mSearchModelsExtracting
			Layout.alignment: Qt.AlignHCenter
			
			onReleased:
			{
				searchModelDialog.open();
			}
		}
		ProgressBar
		{
			id: searchModelsInstallationProgressBar
			width: parent.width
			Layout.alignment: Qt.AlignHCenter
			value: MainQmlBinder.mSearchModelsExtractingProgress
			opacity: MainQmlBinder.mSearchModelsExtracting ? 1.0 : 0.0
		}
		RegularButton
		{
			text:"Install Tagging Models"
			enabled: !MainQmlBinder.mTaggingModelsExtracting
			Layout.alignment: Qt.AlignHCenter
			
			onReleased:
			{
				taggingModelDialog.open();
			}
		}
		ProgressBar
		{
			id: taggingModelsInstallationProgressBar
			width: parent.width
			Layout.alignment: Qt.AlignHCenter
			value: MainQmlBinder.mTaggingModelsExtractingProgress
			opacity: MainQmlBinder.mTaggingModelsExtracting ? 1.0 : 0.0
		}
		
		RegularButton
		{
			text:"Export Archive"
			enabled: !MainQmlBinder.mExporting && !MainQmlBinder.Processing && !MainQmlBinder.mTagging && !MainQmlBinder.UpdatingHNSWIndex && !imageGrid.mLoading
			Layout.alignment: Qt.AlignHCenter
			
			onReleased:
			{
				exportDialog.open();
			}
		}
		RegularButton
		{
			text:"Full Refresh"
			Layout.alignment: Qt.AlignHCenter
			
			onReleased:
			{
				MainQmlBinder.refresh(true);
			}
		}
		RegularButton
		{
			text:"ReTag All Images"
			enabled: MainQmlBinder.mImageTaggerEnabled
			Layout.alignment: Qt.AlignHCenter
			
			onReleased:
			{
				MainQmlBinder.retag();
			}
		}
		RegularButton
		{
			id: clearButton
			text:"Clear Database"
			Layout.alignment: Qt.AlignHCenter
			
			onReleased:
			{
				MainQmlBinder.clear();
			}
		}
	}
	
	Popup
	{
		id: exportProgressPopup
		anchors.centerIn: Overlay.overlay
		width: 300
		height: 100
		modal: true
		focus: true
		closePolicy: Popup.NoAutoClose
		visible: MainQmlBinder.mExporting

		Column
		{
			anchors.centerIn: parent
			spacing: 15
			width: parent.width - 40

			RowLayout
			{
				width: parent.width
				Text
				{
					text: "Exporting... "
					font.pixelSize: 14
					Layout.fillWidth: true
				}
				
				Text
				{
					Layout.fillWidth: false
					text: Math.floor(MainQmlBinder.mExportingProgress*100)+"%"
					font.pixelSize: 14
				}
			}
			
			ProgressBar
			{
				id: progressBar
				width: parent.width
				from: 0
				to: 100
				value: MainQmlBinder.mExportingProgress*100
			}
		}
	}
}