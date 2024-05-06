set(SOURCES DDSImage.cpp
            DirtyRegionSolvers.cpp
            DirtyRegionTracker.cpp
            FFmpegImage.cpp
            GUIAction.cpp
            GUIAudioManager.cpp
            GUIBaseContainer.cpp
            GUIBorderedImage.cpp
            GUIButtonControl.cpp
            GUIColorButtonControl.cpp
            GUIColorManager.cpp
            GUIComponent.cpp
            GUIControl.cpp
            GUIControlFactory.cpp
            GUIControlGroup.cpp
            GUIControlGroupList.cpp
            GUIControlLookup.cpp
            GUIControlProfiler.cpp
            GUIDialog.cpp
            GUIEditControl.cpp
            GUIFadeLabelControl.cpp
            GUIFixedListContainer.cpp
            GUIFont.cpp
            GUIFontCache.cpp
            GUIFontManager.cpp
            GUIFontTTF.cpp
            GUIImage.cpp
            GUIIncludes.cpp
            GUIKeyboardFactory.cpp
            GUILabelControl.cpp
            GUILabel.cpp
            GUIListContainer.cpp
            GUIListGroup.cpp
            GUIListItem.cpp
            GUIListItemLayout.cpp
            GUIListLabel.cpp
            GUIMessage.cpp
            GUIMoverControl.cpp
            GUIMultiImage.cpp
            GUIPanelContainer.cpp
            GUIProgressControl.cpp
            GUIRadioButtonControl.cpp
            GUIRangesControl.cpp
            GUIRenderingControl.cpp
            GUIResizeControl.cpp
            GUIRSSControl.cpp
            GUIScrollBarControl.cpp
            GUISettingsSliderControl.cpp
            GUISliderControl.cpp
            GUISpinControl.cpp
            GUISpinControlEx.cpp
            GUIStaticItem.cpp
            GUITextBox.cpp
            GUITextLayout.cpp
            GUITexture.cpp
            GUIToggleButtonControl.cpp
            GUIVideoControl.cpp
            GUIVisualisationControl.cpp
            GUIWindow.cpp
            GUIWindowManager.cpp
            GUIWrappingListContainer.cpp
            imagefactory.cpp
            IWindowManagerCallback.cpp
            LocalizeStrings.cpp
            StereoscopicsManager.cpp
            TextureBundle.cpp
            TextureBundleXBT.cpp
            Texture.cpp
            TextureBase.cpp
            TextureManager.cpp
            VisibleEffect.cpp
            XBTF.cpp
            XBTFReader.cpp)

set(HEADERS DDSImage.h
            DirtyRegion.h
            DirtyRegionSolvers.h
            DirtyRegionTracker.h
            DispResource.h
            FFmpegImage.h
            gui3d.h
            GUIAction.h
            GUIAudioManager.h
            GUIBaseContainer.h
            GUIBorderedImage.h
            GUIButtonControl.h
            GUIColorButtonControl.h
            GUIColorManager.h
            GUIComponent.h
            GUIControl.h
            GUIControlFactory.h
            GUIControlGroup.h
            GUIControlGroupList.h
            GUIControlProfiler.h
            GUIControlLookup.h
            GUIDialog.h
            GUIEditControl.h
            GUIFadeLabelControl.h
            GUIFixedListContainer.h
            GUIFont.h
            GUIFontCache.h
            GUIFontManager.h
            GUIFontTTF.h
            GUIImage.h
            GUIIncludes.h
            GUIKeyboard.h
            GUIKeyboardFactory.h
            GUILabel.h
            GUILabelControl.h
            GUIListContainer.h
            GUIListGroup.h
            GUIListItem.h
            GUIListItemLayout.h
            GUIListLabel.h
            GUIMessage.h
            GUIMoverControl.h
            GUIMultiImage.h
            GUIPanelContainer.h
            GUIProgressControl.h
            GUIRadioButtonControl.h
            GUIRangesControl.h
            GUIRenderingControl.h
            GUIResizeControl.h
            GUIRSSControl.h
            GUIScrollBarControl.h
            GUISettingsSliderControl.h
            GUISliderControl.h
            GUISpinControl.h
            GUISpinControlEx.h
            GUIStaticItem.h
            GUITextBox.h
            GUITextLayout.h
            GUITexture.h
            GUIToggleButtonControl.h
            GUIVideoControl.h
            GUIVisualisationControl.h
            GUIWindow.h
            GUIWindowManager.h
            GUIWrappingListContainer.h
            IAudioDeviceChangedCallback.h
            IDirtyRegionSolver.h
            IGUIContainer.h
            iimage.h
            imagefactory.h
            IMsgTargetCallback.h
            IRenderingCallback.h
            ISliderCallback.h
            IWindowManagerCallback.h
            LocalizeStrings.h
            StereoscopicsManager.h
            Texture.h
            TextureBase.h
            TextureBundle.h
            TextureBundleXBT.h
            TextureManager.h
            Tween.h
            VisibleEffect.h
            WindowIDs.h
            XBTF.h
            XBTFReader.h)

if(TARGET OpenGL::GL OR TARGET OpenGL::GLES)
  list(APPEND SOURCES Shader.cpp
                      TextureGL.cpp)
  list(APPEND HEADERS Shader.h
                      TextureGL.h)

  if(TARGET OpenGL::GL)
    list(APPEND SOURCES GUIFontTTFGL.cpp
                        GUITextureGL.cpp)
    list(APPEND HEADERS GUIFontTTFGL.h
                        GUITextureGL.h)
  endif()

  if(TARGET OpenGL::GLES)
    list(APPEND SOURCES GUIFontTTFGLES.cpp
                        GUITextureGLES.cpp)
    list(APPEND HEADERS GUIFontTTFGLES.h
                        GUITextureGLES.h)
  endif()

endif()

if(CORE_SYSTEM_NAME STREQUAL windows OR CORE_SYSTEM_NAME STREQUAL windowsstore)
  list(APPEND SOURCES D3DResource.cpp
                      DirectXGraphics.cpp
                      GUIFontTTFDX.cpp
                      GUIShaderDX.cpp
                      GUITextureD3D.cpp
                      TextureDX.cpp)
  list(APPEND HEADERS D3DResource.h
                      DirectXGraphics.h
                      GUIFontTTFDX.h
                      GUIShaderDX.h
                      GUITextureD3D.h
                      TextureDX.h)
endif()

core_add_library(guilib)

if(CORE_SYSTEM_NAME STREQUAL windows OR CORE_SYSTEM_NAME STREQUAL windowsstore)
  set(SHADERS_VERTEX guishader_vert.hlsl)
  set(SHADERS_PIXEL guishader_checkerboard_right.hlsl
                    guishader_checkerboard_left.hlsl
                    guishader_default.hlsl
                    guishader_fonts.hlsl
                    guishader_interlaced_right.hlsl
                    guishader_interlaced_left.hlsl
                    guishader_multi_texture_blend.hlsl
                    guishader_texture.hlsl
                    guishader_texture_noblend.hlsl)
  foreach(shader ${SHADERS_VERTEX})
    get_filename_component(file ${shader} NAME_WE)
    add_custom_command(OUTPUT ${file}.h
                       COMMAND windows::FXC
                       ARGS /Fh ${file}.h /E VS /T vs_4_0_level_9_1 /Vn ${file} /Qstrip_reflect
                            ${CMAKE_SOURCE_DIR}/system/shaders/${shader}
                       DEPENDS ${CMAKE_SOURCE_DIR}/system/shaders/${shader}
                       COMMENT "FX compile vertex shader ${shader}"
                       VERBATIM)
    list(APPEND SHADERS ${file}.h)
  endforeach()
  foreach(shader ${SHADERS_PIXEL})
    get_filename_component(file ${shader} NAME_WE)
    add_custom_command(OUTPUT ${file}.h
                       COMMAND windows::FXC
                       ARGS /Fh ${file}.h /E PS /T ps_4_0_level_9_1 /Vn ${file} /Qstrip_reflect
                            ${CMAKE_SOURCE_DIR}/system/shaders/${shader}
                       DEPENDS ${CMAKE_SOURCE_DIR}/system/shaders/${shader}
                       COMMENT "FX compile pixel shader ${shader}"
                       VERBATIM)
    list(APPEND SHADERS ${file}.h)
  endforeach()

  add_custom_target(generate_shaders ALL DEPENDS ${SHADERS})
  set_target_properties(generate_shaders PROPERTIES FOLDER "Build Utilities")
  target_include_directories(${CORE_LIBRARY} PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
  add_dependencies(${CORE_LIBRARY} generate_shaders)
endif()
