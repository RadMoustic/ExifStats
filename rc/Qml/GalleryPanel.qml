import QtQuick
import QtQuick.Controls

import ExifStats

ESImageGridQuickItem
{
	id: imageGrid
	
	property real imageScale: MainQmlBinder.isMobile() ? 0.75 : 1.0
	
	mImageSize: 250 * imageScale

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
					imageGrid.imageScale = Math.min(2.0, Math.max(0.5, imageGrid.imageScale + (pWheel.angleDelta.y > 0 ? 0.1 : -0.1)));
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
				imageGrid.imageScale = Math.min(2.0, Math.max(0.5, initialScale * pinch.scale));
			}
			
			MouseArea
			{
				anchors.fill: parent
				preventStealing: false
				
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
							Qt.openUrlExternally("file:///" + selectedFile);
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