import QtQuick
import QtQuick.Controls
import QtQuick.Layouts 
import QtQuick.Window

import ExifStats

Rectangle
{
	id: imageViewerRoot
	clip: true
	color: "black"

	property alias imageViewerChild: imageViewer
	property real initialScale: 1.0
	property real maxZoom: 15.0
	
	function resetView()
	{
		imageViewer.scale = 1.0
		imageViewer.x = 0
		imageViewer.y = 0
	}
	
	function closeView()
	{
		imageViewerRoot.visible = false;
		MainQmlBinder.mFullScreen = false;
		imageInfo.display = false;
		imageViewer.cancelAllDownloadRequests();
	}
	
	function getMinMax(pSize, pScale, pOffset)
	{
		let scaledSize = pSize * pScale;
		let scaledOrigin = (pSize - scaledSize) / 2.0;
		let scaledImageOffset = pOffset * pScale;

		let min = (scaledOrigin + scaledImageOffset >= 0) 
			? scaledOrigin 
			: (2.0 * scaledOrigin + scaledImageOffset);
			
		let max = (scaledOrigin + scaledImageOffset >= 0) 
			? scaledOrigin 
			: -scaledImageOffset;
			
		return { "min": min, "max": max };
	}
	
	Connections
	{
		target: Screen
		
		function onOrientationChanged()
		{
			resetView();
		}
	}
	
	Timer
	{
		id: dragBlocker
		interval: 150
	}
	
	ESImageViewerQuickItem
	{
		id: imageViewer
		width: imageViewerRoot.width
		height: imageViewerRoot.height
		transformOrigin: Item.TopLeft
		mIsUserInteracting: dragHandler.active || pinchArea.active
		
		property real viewportRatio: width / height
		property real fullScreenWidth: mImageRatio >= viewportRatio ? width : height * mImageRatio
		property real fullScreenHeight: mImageRatio >= viewportRatio ? width / mImageRatio : height
		property real imageOffsetX: (width - fullScreenWidth) / 2.0
		property real imageOffsetY: (height - fullScreenHeight) / 2.0

		onMImagePathChanged:
		{
			imageViewerRoot.resetView();
			imageViewer.downloadOriginalImage(imageGrid.getNextImage(mImagePath, 0), false);
			imageViewer.downloadOriginalImage(imageGrid.getPreviousImage(mImagePath, 0), false);
			highResLoadingState.hide = false;
		}
		
		DragHandler
		{
			id: dragHandler
			enabled: !pinchArea.pinch.active && !dragBlocker.running && imageViewer.scale !== 1.0
			xAxis.minimum: getMinMax(imageViewer.width, imageViewer.scale, imageViewer.imageOffsetX).min
			xAxis.maximum: getMinMax(imageViewer.width, imageViewer.scale, imageViewer.imageOffsetX).max
			yAxis.minimum: getMinMax(imageViewer.height, imageViewer.scale, imageViewer.imageOffsetY).min
			yAxis.maximum: getMinMax(imageViewer.height, imageViewer.scale, imageViewer.imageOffsetY).max
		}
		
		WheelHandler
		{
			enabled: !MainQmlBinder.isMobile()
			property real previousX: 0
			property real previousY: 0
			onWheel: (pWheel) =>
			{	
				var newScale = Math.max(1.0, Math.min(imageViewerRoot.maxZoom, imageViewer.scale * (pWheel.angleDelta.y > 0 ? 1.1 : 0.9)));
				var zoomRatio = newScale / imageViewer.scale;
				
				let minMaxX = getMinMax(imageViewer.width, newScale, imageViewer.imageOffsetX);
				let minMaxY = getMinMax(imageViewer.height, newScale, imageViewer.imageOffsetY);

				imageViewer.x = Math.min(minMaxX.max, Math.max(minMaxX.min, newScale == 1.0 ? 0 : pWheel.x - (pWheel.x - imageViewer.x) * zoomRatio));
				imageViewer.y = Math.min(minMaxY.max, Math.max(minMaxY.min, newScale == 1.0 ? 0 : pWheel.y - (pWheel.y - imageViewer.y) * zoomRatio));
				imageViewer.scale = newScale;
			}
		}
	}

	PinchArea
	{
		id: pinchArea
		anchors.fill: parent
		
		property bool active: false
		
		onPinchStarted:
		{
			imageViewerRoot.initialScale = imageViewer.scale;
			pinchArea.active = true;
		}
		
		onPinchFinished:
		{
			pinchArea.active = false;
		}
		
		onPinchUpdated: (pPinch) =>
		{
			dragBlocker.restart();
					
			var newScale = Math.max(1.0, Math.min(imageViewerRoot.maxZoom, imageViewerRoot.initialScale * pPinch.scale));
			var zoomRatio = newScale / imageViewer.scale;
			var dx = pPinch.center.x - pPinch.previousCenter.x;
			var dy = pPinch.center.y - pPinch.previousCenter.y;
			
			let minMaxX = getMinMax(imageViewer.width, newScale, imageViewer.imageOffsetX);
			let minMaxY = getMinMax(imageViewer.height, newScale, imageViewer.imageOffsetY);

			imageViewer.x = Math.min(minMaxX.max, Math.max(minMaxX.min, newScale == 1.0 ? 0 : pPinch.center.x - (pPinch.center.x - imageViewer.x) * zoomRatio + dx));
			imageViewer.y = Math.min(minMaxY.max, Math.max(minMaxY.min, newScale == 1.0 ? 0 : pPinch.center.y - (pPinch.center.y - imageViewer.y) * zoomRatio + dy));
			imageViewer.scale = newScale;
		}
		
		MouseArea 
		{
			anchors.fill: parent
			
			enabled: imageViewer.scale === 1.0
			property int swipeThreshold: 80
			property int startX: 0
			property int startY: 0

			onPressed: (pMouse) =>
			{
				startX = pMouse.x;
				startY = pMouse.y;
			}

			onReleased: (pMouse) =>
			{
				var diffX = pMouse.x - startX;
				var diffY = pMouse.y - startY;

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
				else if (Math.abs(diffY) > swipeThreshold)
				{
					if(diffY < 0)
					{
						imageInfo.display = true;
					}
				}
				else if (Math.abs(diffX) < 20 && Math.abs(diffY) < 20)
				{
					closeView();
				}
			}
		}
	}
	
	Pane
	{
		id: imageInfo
		anchors.fill: parent
		anchors.margins: 30
		anchors.topMargin: Math.max(0, height-imageInfoMainLayout.implicitHeight-30)
		anchors.bottomMargin: 0
		opacity: 0
		clip: true
		visible: opacity > 0
		
		property bool display: false
		
		onDisplayChanged:
		{
			opacity = display ? 0.8 : 0;
		}
		
		Behavior on opacity
		{
			NumberAnimation
			{ 
				duration: 300 
				easing.type: Easing.InOutQuad 
			}
		}
				
		Label
		{
			width: parent.width
			text: "Path: " + imageViewer.mImagePath
			elide: Text.ElideLeft
			wrapMode: Text.NoWrap
		}
			
		ColumnLayout
		{
			id: imageInfoMainLayout
			clip: true
			Label{text: "" }
			Label {	text: "Width: " + imageViewer.mImageWidth }
			Label {	text: "Height: " + imageViewer.mImageHeight }
			Label {	text: "Ratio: " + imageViewer.mImageRatio.toFixed(2) }
			Label {	text: "Camera: " + imageViewer.mCameraModel }
			Label {	text: "Lens: " + imageViewer.mLensModel }
			Label {	text: "Date/Time: " + imageViewer.mDateTime }
			Label {	text: "Shutter Speed: " + (imageViewer.mShutterSpeedValue < 1.0 ? (1/imageViewer.mShutterSpeedValue).toFixed(0) : "1/" + imageViewer.mShutterSpeedValue) + "s" }
			Label {	text: "Aperture: " + imageViewer.mFNumber.toFixed(2) }
			Label {	text: "GeoLocation: " + imageViewer.mGeoLocation }
			Label {	text: "Focal Length: " + imageViewer.mFocalLength }
			Label {	text: "Focal Length 35mm equiv: " + imageViewer.mFocalLengthIn35mm }
			Label {	text: "Orientation: " + imageViewer.mOrientation }
			Label {	text: "ISO: " + imageViewer.mISOSpeedRatings }
		}
		
		MouseArea 
		{
			anchors.fill: parent
			onClicked: (pMouse) =>
			{
				imageInfo.display = false;
			}
		}
	}

	ContrastedButton
	{
		id: closeButton
		text: "X"
		anchors.top: parent.top
		anchors.right: parent.right
		anchors.margins: 15
		
		onReleased:
		{
			closeView();
		}
	}
	
	ContrastedButton
	{
		id: infoButton
		text: "i"
		anchors.bottom: parent.bottom
		anchors.right: parent.right
		anchors.margins: 15
		
		onReleased:
		{
			imageInfo.display = !imageInfo.display;
		}
	}
	
	function isError(pStep)
	{
		return pStep >= 10 || pStep == -1;
	}
	
	function getHighResImageStepColor(pStep)
	{
		if(pStep == 0) // Started
			return "grey";
		if(pStep == 1) // Opening File
			return "green";
		if(pStep == 2) // Converting File
			return "green";
		if(pStep == 3) // Sending File
			return "green"
		if(isError(pStep))
			return "red";
			
		return "pink";
	}
	
	CircularProgressBar
	{
		id: highResLoadingState
		
		x: 5
		y: 5
		width: 20
		height: 20
		property bool hide: false
		
		visible: !hide && imageViewer.mHighResImageStep >= -1
		statusColor: getHighResImageStepColor(imageViewer.mHighResImageStep)
		loading: imageViewer.mHighResImageStep >= 0 && imageViewer.mHighResImageStep < 3
		progress: imageViewer.mHighResImageDownloadProgress
		
		MouseArea 
		{
			anchors.fill: parent
			onClicked: (pMouse) =>
			{
				highResLoadingState.hide = true;
			}
		}
	}
}