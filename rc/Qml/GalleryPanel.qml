import QtQuick
import QtQuick.Controls

import ExifStats

ESImageGridQuickItem
{
	id: imageGrid
	
	property real imageScale: MainQmlBinder.isMobile() ? imageGrid.width : 250
	property var imageViewerItem
	property real maxGridCol: MainQmlBinder.isMobile() ? 4 : 20
	
	property var storedImage
	
	mImageSize: imageGrid.width / Math.floor(imageGrid.width / imageScale)

	onMImageFilesChanged:
	{
		flickable.contentY = 0;
	}
	
	Timer
	{
		id: scrollAfterResetTimer
		interval: 1
		repeat: false
		onTriggered:
		{
			flickable.contentY = imageGrid.scrollViewTo(galleryMenu.selectedImage);
		}
	}
	
	Menu
	{
		id: galleryMenu
		width: 350
		property var selectedImage

		MenuItem
		{
			id: storeImage
			text: "Reset and Scroll"
			onTriggered:
			{
				MainQmlBinder.resetFilters();
				scrollAfterResetTimer.start();
			}
		}
	}

	Flickable
	{
		id: flickable
		anchors.fill: parent
		
		contentHeight: imageGrid.mContentHeight
		contentWidth: width
		boundsBehavior: Flickable.StopAtBounds
		
		flickDeceleration: 1200
		contentY: imageGrid.mYOffset
		
		onContentYChanged: imageGrid.mYOffset = contentY
		
		ScrollBar.vertical: ScrollBar { width: 30 }
		
		WheelHandler
		{
			onWheel: (pWheel) =>
			{
				if(MainQmlBinder.isCtrlPressed())
					imageGrid.imageScale = Math.min(imageGrid.width, Math.max(imageGrid.width / maxGridCol, imageGrid.imageScale + (pWheel.angleDelta.y > 0 ? 25 : -25)));
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
				
				acceptedButtons: Qt.LeftButton | Qt.RightButton
				
				property real pressX: 0
				property real pressY: 0
				
				function popupContextMenu(pMouse)
				{
					pMouse.accepted = true;
					galleryMenu.selectedImage = imageGrid.getImageFileAtPos(pMouse.x, pMouse.y);
					print(galleryMenu.selectedImage);
					if(galleryMenu.selectedImage !== "")
						galleryMenu.popup();
				}
				
				onPressed: (pMouse) =>
				{
					pressX = pMouse.x;
					pressY = pMouse.y;
				}
				
				onClicked: (pMouse) =>
				{
					pMouse.accepted = false;
					if (Math.abs(pMouse.x - pressX) > 5 || Math.abs(pMouse.y - pressY) > 5)
						return;
					if(MainQmlBinder.isMobile())
					{
						pMouse.accepted = true;
						var selectedFile = imageGrid.getImageFileAtPos(pMouse.x, pMouse.y);
						if(selectedFile !== "")
						{
							imageViewerItem.visible = true;
							imageViewerItem.mImagePath = selectedFile;
							MainQmlBinder.mFullScreen = true;
						}
					}
					else if (pMouse.button == Qt.RightButton)
					{
						popupContextMenu(pMouse);
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
				
				onPressAndHold: (pMouse)=>
				{
					if(MainQmlBinder.isMobile())
						popupContextMenu(pMouse);
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