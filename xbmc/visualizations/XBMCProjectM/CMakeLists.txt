set(src
  Main.cpp
  libprojectM/projectM.cpp
  libprojectM/FBO.cpp
  libprojectM/InitCond.cpp 
  libprojectM/Expr.cpp
  libprojectM/PCM.cpp
  libprojectM/Parser.cpp
  libprojectM/Preset.cpp
  libprojectM/BeatDetect.cpp
  libprojectM/PerPixelEqn.cpp
  libprojectM/Param.cpp
  libprojectM/CustomWave.cpp
  libprojectM/CustomShape.cpp 
  libprojectM/Func.cpp
  libprojectM/Eval.cpp
  libprojectM/PerFrameEqn.cpp
  libprojectM/PerPointEqn.cpp
  libprojectM/fftsg.cpp
  libprojectM/KeyHandler.cpp
  libprojectM/timer.cpp
  libprojectM/wipemalloc.cpp
  libprojectM/BuiltinFuncs.cpp
  libprojectM/BuiltinParams.cpp
  libprojectM/Renderer.cpp 
  libprojectM/PresetLoader.cpp
  libprojectM/PresetChooser.cpp
  libprojectM/PresetFrameIO.cpp
  libprojectM/PresetMerge.cpp
  libprojectM/ConfigFile.cpp
  libprojectM/IdlePreset.cpp
  libprojectM/TextureManager.cpp
  libprojectM/TimeKeeper.cpp
  libprojectM/image_DXT.c
  libprojectM/image_helper.c
  libprojectM/SOIL.c
  libprojectM/stb_image_aug.c
)

if(TARGET_COMMON_DARWIN)
  ADD_DEFINITIONS(-DMACOS -DSTBI_NO_DDS)
elseif(TARGET_WIN32)
  ADD_DEFINITIONS(-DWIN32 -DSTBI_NO_DDS)
else(TARGET_COMMON_LINUX)
  ADD_DEFINITIONS(-DLINUX -DSTBI_NO_DDS)
endif(TARGET_COMMON_DARWIN)

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-format -Wno-parentheses-equality -Wno-enum-conversion -Wno-deprecated-writable-strings -Wno-default-arg-special-member")
endif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")

set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${PLEX_LINK_WRAPPED}")
find_program(ZIP zip)

if(ZIP)
  add_custom_command(
    OUTPUT presets.zip
    COMMAND ${ZIP} -q -j presets.zip ${CMAKE_CURRENT_SOURCE_DIR}/libprojectM/presets/*
  )
  add_custom_target(ProjectMPresets ALL DEPENDS presets.zip)
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/presets.zip DESTINATION ${RESOURCEPATH}/addons/visualization.projectm/resources)
endif()

add_library(projectM MODULE ${src})
target_link_libraries(projectM wrapper ${CONFIG_LIBRARY_OPENGL})
install(TARGETS projectM DESTINATION ${RESOURCEPATH}/addons/visualization.projectm)
