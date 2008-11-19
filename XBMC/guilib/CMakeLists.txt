SET(SRCS ActionManager.cpp AnimatedGif.cpp AudioContext.cpp DirectXGraphics.cpp FrameBufferObject.cpp GUIAudioManager.cpp GUIBaseContainer.cpp GUIBorderedImage.cpp GUIButtonControl.cpp GUIButtonScroller.cpp GUICheckMarkControl.cpp GUIColorManager.cpp GUIControl.cpp GUIControlGroup.cpp GUIControlGroupList.cpp GUIDialog.cpp GUIEditControl.cpp GUIFadeLabelControl.cpp GUIFixedListContainer.cpp GUIFont.cpp GUIFontManager.cpp GUIFontTTF.cpp GUIIncludes.cpp GUIInfoColor.cpp GUILabelControl.cpp GUILargeImage.cpp GUIListContainer.cpp GUIListGroup.cpp GUIListItem.cpp GUIListItemLayout.cpp GUIListLabel.cpp GUIMessage.cpp GUIMoverControl.cpp GUIMultiImage.cpp GUIMultiSelectText.cpp GUIPanelContainer.cpp GUIProgressControl.cpp GUIRSSControl.cpp GUIRadioButtonControl.cpp GUIResizeControl.cpp GUIScrollBarControl.cpp GUISelectButtonControl.cpp GUISettingsSliderControl.cpp GUISliderControl.cpp GUISound.cpp GUISpinControl.cpp GUISpinControlEx.cpp GUIStandardWindow.cpp GUITextBox.cpp GUITextLayout.cpp GUIToggleButtonControl.cpp GUIVideoControl.cpp GUIVisualisationControl.cpp GUIWindow.cpp GUIWindowManager.cpp GUIWrappingListContainer.cpp GraphicContext.cpp GuiControlFactory.cpp IWindowManagerCallback.cpp Key.cpp LocalizeStrings.cpp Shader.cpp SkinInfo.cpp Surface.cpp TextureBundle.cpp TextureManager.cpp VisibleEffect.cpp XMLUtils.cpp guiImage.cpp include.cpp)

SET(CMAKE_VERBOSE_MAKEFILE ON)

INCLUDE_DIRECTORIES(../xbmc/ ./)

# Note: We check for WIN32 rather than UNIX because both UNIX and WIN32 are
# TRUE for CYGWIN
IF(WIN32)
 INCLUDE_DIRECTORIES(../xbmc/win32/)
 ADD_DEFINITIONS(-D_WIN32)
ELSE(WIN32)
 INCLUDE_DIRECTORIES(../xbmc/linux/)
 ADD_DEFINITIONS(-D_LINUX)
 IF(APPLE)
  ADD_DEFINITIONS(-D__APPLE__ "-DARCH=\\\"osx\\\"")
  INCLUDE_DIRECTORIES(../xbmc/osx/ /opt/local/include/ /opt/local/include/freetype2)
 ENDIF(APPLE)
ENDIF(WIN32)

ADD_SUBDIRECTORY(tinyXML)
ADD_SUBDIRECTORY(common)

ADD_LIBRARY(guilib STATIC ${SRCS})
TARGET_LINK_LIBRARIES(guilib tinyXML common)

#FIND_PACKAGE(SDL REQUIRED)
#INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR})
