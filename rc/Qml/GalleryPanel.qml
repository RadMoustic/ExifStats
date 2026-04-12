import QtQuick
import QtQuick.Controls

import ExifStats

ESImageGridQuickItem
{
	id: imageGrid
	
	property real imageScale: MainQmlBinder.isMobile() ? imageGrid.width : 250
	property var imageViewerItem
	property real maxGridCol: 4
	
	mImageSize: MainQmlBinder.isMobile() ? imageGrid.width / Math.floor(imageGrid.width / imageScale) : imageScale

	onMImageFilesChanged:
	{
		flickable.contentY = 0;
	}

	Flickable
	{
		id: flickable
		anchors.fill: parent
		
		contentHeight: imageGrid.mContentHeight
		contentWidth: width
		boundsBehavior: Flickable.StopAtBounds
		
		flickDeceleration: 1200
		
		onContentYChanged: imageGrid.mYOffset = contentY
		
		ScrollBar.vertical: ScrollBar { width: 30 }
		
		WheelHandler
		{
			onWheel: (pWheel) =>
			{
				if(MainQmlBinder.isCtrlPressed())
					imageGrid.imageScale = Math.min(imageGrid.width, Math.max(imageGrid.width / maxGridCol, imageGrid.imageScale + (pWheel.angleDelta.y > 0 ? 0.1 : -0.1)));
				else
					flickable.contentY = Math.max(0, Math.min(imageGrid.mContentHeight-height, flickable.contentY - pWheel.angleDelta.y));
			}
		}
		
		PinchArea
		{
			anchors.fill: parent
			
			property real initialScale: 1.0
			
			onPinchStarted: 
			{
				initialScale = imageGrid.imageScale;
			}
			onPinchUpdated: (pinch) => 
			{
				imageGrid.imageScale = Math.min(imageGrid.width, Math.max(imageGrid.width / maxGridCol, initialScale * pinch.scale));
			}
			
			MouseArea
			{
				anchors.fill: parent
				preventStealing: false
				
				onClicked: (pMouse) =>
				{
					if(MainQmlBinder.isMobile())
					{
						var selectedFile = imageGrid.getImageFileAtPos(pMouse.x, pMouse.y);
						if(selectedFile !== "")
						{
							imageViewerItem.visible = true;
							imageViewerItem.mImagePath = selectedFile;
							MainQmlBinder.mFullScreen = true;
						}
					}
				}
				
				onDoubleClicked: (pMouse) =>
				{
					if(MainQmlBinder.isCtrlPressed())
					{
						imageGrid.imageScale = 1.0;
					}
					else
					{
						var selectedFile = imageGrid.getImageFileAtPos(pMouse.x, pMouse.y);
						if(selectedFile !== "")
						{
							Qt.openUrlExternally("file:///" + selectedFile);
						}
					}
				}
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