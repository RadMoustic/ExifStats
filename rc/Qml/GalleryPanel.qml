import QtQuick
import QtQuick.Controls

import ExifStats

ESImageGridQuickItem
{
	id: imageGrid
	
	property var imageViewerItem
	property var mapItem
	property var mapShowAndFocusFunction: function() {}
	
	property real maxGridCol: MainQmlBinder.isMobile() ? (width > height ? 8 : 4) : Math.min(10, Math.floor(width / getMinImageSize()))
	property real gridCol: MainQmlBinder.isMobile() ? 1 : Math.floor(width / 250)
	property alias flickableChild: flickable
	property bool orientation: width > height

	mTargetImageSize: imageGrid.width / Math.min(maxGridCol, Math.floor(gridCol))
	mImageSize: mTargetImageSize
		
	onOrientationChanged:
	{
		if(MainQmlBinder.isMobile())
		{
			var newMaxGridCol = orientation ? 8 : 4;
			var gridColRatio = orientation ? 2 : 0.5;
			imageGrid.mZoomCenter = Qt.point(-1,-1);
			gridCol = Math.min(newMaxGridCol, Math.max(1, Math.round(gridCol * gridColRatio)));
		}
	}
	
	Behavior on mImageSize
	{
		id: imageSizeAnim
		enabled: false
		
		NumberAnimation
		{ 
			duration: 500 
			easing.type: Easing.InOutQuad 
		}
	}

	onMImageFilesChanged:
	{
		flickable.contentY = 0;
	}
	
	onMContentHeightChanged:
	{
		flickable.contentY = mYOffset;
		flickable.contentHeight = mContentHeight;
		imageSizeAnim.enabled = true;
	}
	
	Timer
	{
		id: scrollToImageTimer
		interval: 30
		repeat: false
		property var imageToScrollTo
		onTriggered:
		{
			flickable.contentY = imageGrid.scrollViewTo(imageToScrollTo);
			imageToScrollTo = "";
		}
	}
	
	Menu
	{
		id: galleryMenu
		width: 350
		property var selectedImage
		property var selectedImageGeoCoord
		
		MenuItem
		{
			text: "Reset and Scroll"
			onTriggered:
			{
				imageGrid.mImageFiles = [];
				scrollToImageTimer.imageToScrollTo = galleryMenu.selectedImage;
				scrollToImageTimer.start();
				MainQmlBinder.resetFilters();
			}
		}
		MenuItem
		{
			text: "Search Similar"
			enabled: MainQmlBinder.mTokenizerEnabled
			onTriggered:
			{
				MainQmlBinder.TagsSearchSimilarImage = galleryMenu.selectedImage;
			}
		}
		MenuItem
		{
			id: centerInMapMenuItem
			text: "Center in Map"
			onTriggered:
			{
				mapItem.mapChild.center = galleryMenu.selectedImageGeoCoord;
				mapItem.mapChild.zoomLevel = 14;
				mapItem.mapDotsChild.refresh();
				imageGrid.mapShowAndFocusFunction();
			}
		}
		
		MenuItem
		{
			text: "Center in Map (Keep Zoom)"
			enabled: centerInMapMenuItem.enabled
			onTriggered:
			{
				mapItem.mapChild.center = galleryMenu.selectedImageGeoCoord;
				mapItem.mapDotsChild.refresh();
				imageGrid.mapShowAndFocusFunction();
			}
		}
		
		MenuItem
		{
			text: "Open in Google Maps"
			enabled: centerInMapMenuItem.enabled
			onTriggered:
			{
				var lat = galleryMenu.selectedImageGeoCoord.latitude;
				var lng = galleryMenu.selectedImageGeoCoord.longitude;
				Qt.openUrlExternally("https://www.google.com/maps/@?api=1&map_action=map&center=" + lat + "," + lng + "&zoom=20");
			}
		}
	}

	Flickable
	{
		id: flickable
		anchors.fill: parent
		
		contentWidth: width
		boundsBehavior: Flickable.StopAtBounds
		
		property bool ignoreNextContentYChanges: false
		property bool blockScrollBarInteraction: false
		
		flickDeceleration: 2400
		maximumFlickVelocity: 8000
		
		onContentYChanged:
		{
			if(!ignoreNextContentYChanges)
				imageGrid.mYOffset = contentY;
			ignoreNextContentYChanges = false;
		}
		
		ScrollBar.vertical: ScrollBar
		{
			width: 30
			interactive: (contentItem ? (contentItem.opacity > 0.1) : false) && !flickable.blockScrollBarInteraction
		}
		
		WheelHandler
		{
			enabled: !MainQmlBinder.isMobile()
			onWheel: (pWheel) =>
			{
				flickable.ignoreNextContentYChanges = true;
				if(MainQmlBinder.isCtrlPressed())
				{
					imageGrid.mZoomCenter = parent.mapToItem(imageGrid, pWheel);
					imageGrid.gridCol = Math.min(maxGridCol, Math.max(1, imageGrid.gridCol + (pWheel.angleDelta.y > 0 ? -0.25 : 0.25)));
				}
				else
				{
					flickable.contentY = Math.max(0, Math.min(imageGrid.mContentHeight-height, flickable.contentY - pWheel.angleDelta.y));
					imageGrid.mYOffset = flickable.contentY;
				}
			}
		}
		
		TapHandler
		{
			acceptedButtons: Qt.LeftButton | Qt.RightButton
							
			function popupContextMenu(pEventPoint)
			{
				galleryMenu.selectedImage = imageGrid.getImageFileAtPos(pEventPoint.x, pEventPoint.y);
				if(galleryMenu.selectedImage !== "")
				{
					galleryMenu.selectedImageGeoCoord = imageGrid.getImageGeoCoordinateAtPos(pEventPoint.x, pEventPoint.y);
					centerInMapMenuItem.enabled = galleryMenu.selectedImageGeoCoord.isValid;
					galleryMenu.popup();
				}
			}
			
			onPressedChanged:
			{
				if (pressed)
					flickable.interactive = true;
			}
				
			onTapped: (pEventPoint, pButton) =>
			{
				if(MainQmlBinder.isMobile())
				{
					var selectedFile = imageGrid.getImageFileAtPos(pEventPoint.position.x, pEventPoint.position.y);
					if(selectedFile !== "")
					{
						imageViewerItem.visible = true;
						imageViewerItem.imageViewerChild.mImagePath = selectedFile;
						MainQmlBinder.mFullScreen = true;
					}
				}
				else if (pButton == Qt.RightButton)
				{
					popupContextMenu(pEventPoint.position);
				}
			}
			
			onDoubleTapped: (pEventPoint) =>
			{
				if(MainQmlBinder.isCtrlPressed())
				{
					imageGrid.imageScale = 1.0;
				}
				else
				{
					var selectedFile = imageGrid.getImageFileAtPos(pEventPoint.position.x, pEventPoint.position.y);
					if(selectedFile !== "")
					{
						Qt.openUrlExternally("file:///" + selectedFile);
					}
				}
			}
			
			onLongPressed:
			{
				if(MainQmlBinder.isMobile())
					popupContextMenu(point.position);
			}
		}
		
		PinchHandler
		{
			target: null
			
			property real initialGridCol: 1.0
			property real currentScale: 1.0
			property bool firstPinchFinished: false
			
			onActiveChanged: 
			{
				if (active)
				{
					initialGridCol = imageGrid.gridCol;
					currentScale = 1.0;
					imageGrid.mZoomCenter = parent.mapToItem(imageGrid, centroid.position);
					flickable.ignoreNextContentYChanges = true;
				}
			}
			onScaleChanged: (pDelta) => 
			{
				flickable.interactive = false;
				currentScale *= pDelta;
				flickable.ignoreNextContentYChanges = true;
				imageGrid.gridCol = Math.min(maxGridCol, Math.max(1, initialGridCol - Math.floor(Math.log2(currentScale))));
			}
		}
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
	
	RegularButton
	{
		id: sortMode
		text: imageGrid.mSortingMode == 0 ? "Date Time" : "Similarity Score"
		anchors.right: parent.right
		anchors.margins: 10
		
		implicitHeight: 30

		onReleased:
		{
			imageGrid.mSortingMode = (imageGrid.mSortingMode + 1) % 2;
		}
	}
}