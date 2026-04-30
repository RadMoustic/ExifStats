import QtQuick
import QtQuick.Controls

import ExifStats

ESImageGridQuickItem
{
	id: imageGrid
	
	property real imageScale: MainQmlBinder.isMobile() ? imageGrid.width : 250
	property var imageViewerItem
	property real maxGridCol: MainQmlBinder.isMobile() ? (width > height ? 8 : 4) : 10
	property alias flickableChild: flickable

	mTargetImageSize: imageGrid.width / Math.floor(imageGrid.width / imageScale)
	mImageSize: mTargetImageSize
	
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

		MenuItem
		{
			id: storeImage
			text: "Reset and Scroll"
			onTriggered:
			{
				scrollToImageTimer.imageToScrollTo = galleryMenu.selectedImage;
				scrollToImageTimer.start();
				MainQmlBinder.resetFilters();
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
		
		flickDeceleration: 1200
		
		onContentYChanged:
		{
			if(!ignoreNextContentYChanges)
				imageGrid.mYOffset = contentY;
			ignoreNextContentYChanges = false;
		}
		
		ScrollBar.vertical: ScrollBar { width: 30 }
		
		WheelHandler
		{
			enabled: !MainQmlBinder.isMobile()
			onWheel: (pWheel) =>
			{
				flickable.ignoreNextContentYChanges = true;
				if(MainQmlBinder.isCtrlPressed())
				{
					imageGrid.mZoomCenter = parent.mapToItem(imageGrid, pWheel);
					imageGrid.imageScale = Math.min(imageGrid.width, Math.max(imageGrid.width / maxGridCol, imageGrid.imageScale * (pWheel.angleDelta.y > 0 ? 1.05 : 0.95)));
				}
				else
				{
					flickable.contentY = Math.max(0, Math.min(imageGrid.mContentHeight-height, flickable.contentY - pWheel.angleDelta.y));
					imageGrid.mYOffset = flickable.contentY;
				}
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
			onPinchUpdated: (pPinch) => 
			{
				flickable.ignoreNextContentYChanges = true;
				imageGrid.mZoomCenter = parent.mapToItem(imageGrid, pPinch.center);
				imageGrid.imageScale = Math.min(imageGrid.width, Math.max(imageGrid.width / maxGridCol, initialScale * pPinch.scale * 0.75));
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