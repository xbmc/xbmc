SET(SRCS
    ActionManager.cpp
    AnimatedGif.cpp
    AudioContext.cpp
    DirectXGraphics.cpp
    FrameBufferObject.cpp
    GUIAudioManager.cpp
    GUIBaseContainer.cpp
    GUIBorderedImage.cpp
    GUIButtonControl.cpp
    GUIButtonScroller.cpp
    GUICheckMarkControl.cpp
    GUIColorManager.cpp
    GUIControl.cpp
    GUIControlGroup.cpp
    GUIControlGroupList.cpp
    GUIDialog.cpp
    GUIEditControl.cpp
    GUIFadeLabelControl.cpp
    GUIFixedListContainer.cpp
    GUIFont.cpp
    GUIFontManager.cpp
    GUIFontTTF.cpp
    GUIIncludes.cpp
    GUIInfoColor.cpp
    GUILabelControl.cpp
    GUILargeImage.cpp
    GUIListContainer.cpp
    GUIListGroup.cpp
    GUIListItem.cpp
    GUIListItemLayout.cpp
    GUIListLabel.cpp
    GUIMessage.cpp
    GUIMoverControl.cpp
    GUIMultiImage.cpp
    GUIMultiSelectText.cpp
    GUIPanelContainer.cpp
    GUIProgressControl.cpp
    GUIRSSControl.cpp
    GUIRadioButtonControl.cpp
    GUIResizeControl.cpp
    GUIScrollBarControl.cpp
    GUISelectButtonControl.cpp
    GUISettingsSliderControl.cpp
    GUISliderControl.cpp
    GUISound.cpp
    GUISpinControl.cpp
    GUISpinControlEx.cpp
    GUIStandardWindow.cpp
    GUITextBox.cpp
    GUITextLayout.cpp
    GUIToggleButtonControl.cpp
    GUIVideoControl.cpp
    GUIVisualisationControl.cpp
    GUIWindow.cpp
    GUIWindowManager.cpp
    GUIWrappingListContainer.cpp
    GraphicContext.cpp
    GuiControlFactory.cpp
    IWindowManagerCallback.cpp
    Key.cpp
    LocalizeStrings.cpp
    Shader.cpp
    SkinInfo.cpp
    Surface.cpp
    TextureBundle.cpp
    TextureManager.cpp
    VisibleEffect.cpp
    XMLUtils.cpp
    guiImage.cpp
    include.cpp)

SET(DIRS common tinyXML)

##
# Do not edit unless you know what you are doing!
##

# Name the library after the directory.
GET_FILENAME_COMPONENT(BIN_NAME ${CMAKE_CURRENT_SOURCE_DIR} NAME)

# Build each directory listed in DIRS.
FOREACH(dir ${DIRS})
  ADD_SUBDIRECTORY(${dir})
ENDFOREACH()

# Create a static library.
ADD_LIBRARY(${BIN_NAME} STATIC ${${BIN_NAME}_SRCS} ${SRCS})

