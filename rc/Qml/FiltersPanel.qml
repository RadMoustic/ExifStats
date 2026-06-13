import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Flickable
{
	id: rootItem
	clip: true
	width: 300
	contentHeight: mainLayout.implicitHeight
	contentWidth: width
	boundsBehavior: Flickable.StopAtBounds
	
	signal filtersResetted
	
	flickDeceleration: 1200
	
	ScrollBar.vertical: ScrollBar
	{
		width: 30
		policy: ScrollBar.AlwaysOff
	}
	
	function displayData()
	{
		if (typeof MainQmlBinder == 'undefined')
			return;
		
		cameraModelsList.model = []
		lensModelsList.model = []
		
		cameraModelsList.selectedItems = MainQmlBinder.getCameraModelsFilter();
		lensModelsList.selectedItems = MainQmlBinder.getLensModelsFilter();

		cameraModelsList.model = MainQmlBinder.getCameraModels();
		lensModelsList.model = MainQmlBinder.getLensModels();

		filtersPresetsList.model = MainQmlBinder.getFiltersPresets();
		filtersPresetsList.currentIndex = -1;
	}
	
	Dialog
	{
		id: replaceFiltersDialog
		title: "Do you really want to replace this filters preset '" + filtersPresetsList.model[filtersPresetsList.currentIndex] + "'"
		standardButtons: MessageDialog.Yes | MessageDialog.No
		anchors.centerIn: Overlay.overlay
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
		anchors.centerIn: Overlay.overlay
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
		anchors.centerIn: Overlay.overlay
		modal: true
		
		onAccepted:
		{
			MainQmlBinder.deleteFilters(filtersPresetsList.model[filtersPresetsList.currentIndex]);
			filtersPresetsList.model = MainQmlBinder.getFiltersPresets();
		}
	}
	
	ColumnLayout
	{
		id: mainLayout
		width: parent.width

		ColumnLayout
		{
			id: filtersPresets
			
			Layout.fillWidth: true
			
			Label
			{
				text: "Filters Presets"
				font.pointSize: 12
				visible: MainQmlBinder.isMobile()
			}
			
			Frame 
			{
				Layout.fillWidth: true
				implicitHeight: MainQmlBinder.isMobile() ? 100 : 200
				
				ListView
				{
					id: filtersPresetsList
					boundsBehavior: Flickable.StopAtBounds
					focus: true
					highlightFollowsCurrentItem: true
					clip: true
					
					anchors.fill: parent
					
					model: []
					
					delegate: Label
					{
						id: labelDelegate
						text: modelData ? modelData : "<empty>"
						color: ListView.isCurrentItem ? (Material.theme == Material.Dark ? "#000000" : "#FFFFFF") : Material.foreground
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
						color: Material.accent
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
				actualSearchedTags.text = ""
				rootItem.filtersResetted();
			}
		}
		
		RowLayout
		{
			id: searchTextFilter
			
			enabled: MainQmlBinder.mTokenizerEnabled
			SplitView.fillWidth: true
			SplitView.preferredHeight: parent.height / parent.children.length
			
			RegularButton
			{
				text:"X"
				
				implicitWidth: 30
				implicitHeight: 30

				onReleased:
				{
					MainQmlBinder.TagsSearchSimilarImage = "";
					MainQmlBinder.TagsSearchString = "";
					actualSearchedTags.text = ""
				}
			}
			
			TextField
			{
				id: searchText
				Layout.fillWidth: true
				implicitHeight: 40
				text: MainQmlBinder.TagsSearchSimilarImage !== "" ? MainQmlBinder.TagsSearchSimilarImage : MainQmlBinder.TagsSearchString
				placeholderText: "Search"

				onEditingFinished:
				{
					MainQmlBinder.TagsSearchSimilarImage = "";
					MainQmlBinder.TagsSearchString = text.trim();
					actualSearchedTags.text = MainQmlBinder.getTagsFound().join(", ");
				}
			}
			
			Slider
			{
				id: tagsMinSimilarityScore
				from: 15
				to: 35
				value: MainQmlBinder.TagsMinSimilarityScore * 100
				stepSize: 1
				live: false
				implicitWidth: 100
				
				onValueChanged:
				{
					MainQmlBinder.TagsMinSimilarityScore = value / 100;
				}
			}
		}
		
		RowLayout
		{
			visible: MainQmlBinder.mTokenizerEnabled && actualSearchedTags.text !== ""
			Label
			{
				id: actualSearchedTags
				Layout.fillWidth: true
				color: "darkgrey"
				horizontalAlignment: Text.AlignHCenter
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
			
			TextField
			{
				id: searchPathText
				Layout.fillWidth: true
				implicitHeight: 40
				placeholderText: "Path"
				text: MainQmlBinder.PathInclusiveFilters.length > 0 ? MainQmlBinder.PathInclusiveFilters.join(" ") : ""
				
				onEditingFinished:
				{
					var trimmedText = text.trim();
					if(trimmedText === "")
						MainQmlBinder.PathInclusiveFilters = [];
					else
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
					MainQmlBinder.FocalLengthFilterOutInvalid = false;
				}
			}
			
			Label
			{
				text: "Focal: "
				Layout.fillWidth: false
			}

			TextField
			{
				x: 0
				Layout.fillWidth: false
				text: MainQmlBinder.FocalLengthFrom
				validator: RegularExpressionValidator { regularExpression: /\d{1,4}/ }
				font.bold: true
				background: null
				onEditingFinished:
				{
					MainQmlBinder.FocalLengthFrom = text;
					MainQmlBinder.FocalLengthFilterOutInvalid = true;
				}

			}
			
			Label
			{
				text: "mm  to "
				Layout.fillWidth: false
			}
			
			TextField
			{
				Layout.fillWidth: false
				text: MainQmlBinder.FocalLengthTo
				validator: RegularExpressionValidator { regularExpression: /\d{1,4}/ }
				font.bold: true
				background: null
				onEditingFinished:
				{
					MainQmlBinder.FocalLengthTo = text;
					MainQmlBinder.FocalLengthFilterOutInvalid = true;
				}
			}
			
			Label
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
					MainQmlBinder.ApertureFilterOutInvalid = false;
				}
			}
			
			Label
			{
				text: "Aperture: "
				Layout.fillWidth: false
			}

			TextField
			{
				x: 0
				Layout.fillWidth: false
				text: MainQmlBinder.ApertureFrom.toPrecision(2)
				validator: RegularExpressionValidator { regularExpression: /[0-9]*\.?[0-9]+/ }
				font.bold: true
				background: null
				onEditingFinished:
				{
					MainQmlBinder.ApertureFrom = text;
					MainQmlBinder.ApertureFilterOutInvalid = true;
				}

			}
			
			Label
			{
				text: " to "
				Layout.fillWidth: false
			}
			
			TextField
			{
				Layout.fillWidth: false
				text: MainQmlBinder.ApertureTo.toPrecision(2)
				validator: RegularExpressionValidator { regularExpression: /[0-9]*\.?[0-9]+/ }
				font.bold: true
				background: null
				onEditingFinished:
				{
					MainQmlBinder.ApertureTo = text;
					MainQmlBinder.ApertureFilterOutInvalid = true;
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
				id: dateResetButton
				text:"X"
				
				implicitWidth: 30
				implicitHeight: 30

				onReleased:
				{
					MainQmlBinder.TimeFrom = MainQmlBinder.MinTime;
					MainQmlBinder.TimeTo = MainQmlBinder.MaxTime;
					MainQmlBinder.TimeFilterOutInvalid = false;
					
				}
			}
			
			Label
			{
				text: ""
				Layout.fillWidth: false
			}

			TextField
			{
				x: 0
				Layout.fillWidth: false
				text: MainQmlBinder.TimeFrom
				validator: RegularExpressionValidator { regularExpression: /\d{1,4}\/\d{1,2}\/\d{1,2}/ }
				font.bold: true
				background: null
				onEditingFinished:
				{
					MainQmlBinder.TimeFrom = text;
				}

			}
			
			Label
			{
				text: " to "
				Layout.fillWidth: false
			}
			
			TextField
			{
				Layout.fillWidth: false
				text: MainQmlBinder.TimeTo
				validator: RegularExpressionValidator { regularExpression: /\d{1,4}\/\d{1,2}\/\d{1,2}/ }
				font.bold: true
				background: null
				onEditingFinished:
				{
					MainQmlBinder.TimeTo = text;
				}
			}
			
			Label
			{
				text: ""
				Layout.fillWidth: true
			}
		}
		
		RowLayout
		{
			id: invalidTimestampFilter
			
			Item
			{
				width: dateResetButton.width
			}
			
			CheckBox
			{
				id: invalidDateTimeFilterCheckbox
				checked: !MainQmlBinder.TimeFilterOutInvalid
				text: "No Date/Time"
				onCheckedChanged:
				{
					MainQmlBinder.TimeFilterOutInvalid = !invalidDateTimeFilterCheckbox.checked;
				}
			}
		}
		
		RowLayout
		{
			id: orientationFilters
			
			function updateOrientationFilterMode()
			{
				if(portraitFilterCheckbox.checked && landcapeFilterCheckbox.checked)
					MainQmlBinder.OrientationFilterMode = 0;
				else if(landcapeFilterCheckbox.checked)
					MainQmlBinder.OrientationFilterMode = 1;
				else if(portraitFilterCheckbox.checked)
					MainQmlBinder.OrientationFilterMode = 2;
				else
					MainQmlBinder.OrientationFilterMode = 3;
			}
			
			RegularButton
			{
				text:"X"
				
				implicitWidth: 30
				implicitHeight: 30

				onReleased:
				{
					MainQmlBinder.OrientationFilterMode = 0;
				}
			}
			
			CheckBox
			{
				id: portraitFilterCheckbox
				checked: MainQmlBinder.OrientationFilterMode == 0 || MainQmlBinder.OrientationFilterMode == 2
				text: "Portrait"
				onCheckedChanged:
				{
					orientationFilters.updateOrientationFilterMode();
				}
			}
			
			CheckBox
			{
				id: landcapeFilterCheckbox
				checked: MainQmlBinder.OrientationFilterMode == 0 || MainQmlBinder.OrientationFilterMode == 1
				text: "Landscape"
				onCheckedChanged:
				{
					orientationFilters.updateOrientationFilterMode();
				}
			}
		}
		Item
		{
			id: cameraFilters
			
			implicitHeight: MainQmlBinder.isMobile() ? 200 : Math.max(200,(Window.height - 700) / 2)
			Layout.fillWidth: true
			
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
			
			ColumnLayout
			{
				anchors.fill: parent

				Label
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
						enabled: !MainQmlBinder.isMobile()
						anchors.fill: MainQmlBinder.isMobile() ? undefined : parent.listViewChild
						anchors.rightMargin: MainQmlBinder.isMobile() ? 0 : cameraModelsList.vertScrollBarChild.width
						acceptedButtons: !MainQmlBinder.isMobile() ? Qt.LeftButton | Qt.RightButton : Qt.NoButton
						propagateComposedEvents: true
						
						onClicked: (pMouse)=>
						{
							pMouse.accepted = false
							if (pMouse.button == Qt.RightButton)
							{
								pMouse.accepted = true
								var localMousePos = mapToItem(cameraModelsList.listViewChild, pMouse.x, pMouse.y);
								cameraModelsList.selectedItemIndex = cameraModelsList.listViewChild.indexAt(localMousePos.x, localMousePos.y + cameraModelsList.listViewChild.contentY);
								contextMenu.popup();
							}
						}
					}
				}
			}
		}
			
		Item
		{
			id: lensFilters
			
			implicitHeight: cameraFilters.implicitHeight
			Layout.fillWidth: true
			
			ColumnLayout
			{
				anchors.fill: parent

				Label
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