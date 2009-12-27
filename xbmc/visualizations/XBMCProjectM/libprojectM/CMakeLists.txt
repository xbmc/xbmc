PROJECT(projectM)
cmake_minimum_required(VERSION 2.6.0)
SET(CPACK_PACKAGE_DESCRIPTION_SUMMARY "The projectM core library.")

INCLUDE(cmake/CPack-projectM.cmake)




cmake_policy(SET CMP0005 OLD)


set( CMAKE_MODULE_PATH    ${CMAKE_MODULE_PATH}
                          ${CMAKE_CURRENT_SOURCE_DIR}/cmake )

OPTION (USE_DEVIL "Use devIL for image loading rather than the builtin SOIL library" OFF)

OPTION (USE_FBO "Use Framebuffer Objects for increased rendering quality.  Disable this for OpenGL ES 1.x or if you are experiencing problems on older or poorly supported hardware." ON)

OPTION(USE_FTGL "Use FTGL for on-screen fonts (found on your system)" ON)

OPTION (USE_GLES1 "Use OpenGL ES 1.x" OFF)

OPTION (USE_OPENMP "Use OpenMP and OMPTL for multi-core paralellization" ON)

OPTION (SYNC_PRESET_SWITCHES "Experimental feature to synchronize preset switches internally" OFF)

OPTION (USE_NATIVE_GLEW "Use projectM's native implemention of GLEW." OFF)

OPTION (USE_CG "Use Cg for Pixel Shader support" OFF)

OPTION (BUILD_PROJECTM_STATIC "Build the projectM target library in the platform's native static (NOT shared) format." OFF)

OPTION (DISABLE_NATIVE_PRESETS "Turn off support for native (C++ style) presets" OFF)
OPTION (DISABLE_MILKDROP_PRESETS "Turn off support for Milkdrop (.milk / .prjm) presets"  OFF)

ADD_DEFINITIONS(-DCMAKE_INSTALL_PREFIX="\\\"${CMAKE_INSTALL_PREFIX}\\\"")

if (USE_NATIVE_GLEW)
ADD_DEFINITIONS(-DUSE_NATIVE_GLEW)
SET(GLEW_SOURCES glew.h glew.c)
SET(GLEW_LINK_TARGETS )
else(USE_NATIVE_GLEW)
SET(GLEW_SOURCES )
SET (GLEW_LINK_TARGETS GLEW)
endif(USE_NATIVE_GLEW)

SET(projectM_SOURCES projectM.cpp PCM.cpp Preset.cpp fftsg.cpp KeyHandler.cpp
timer.cpp wipemalloc.cpp PresetLoader.cpp  PresetChooser.cpp PipelineMerger.cpp ConfigFile.cpp  TimeKeeper.cpp PresetFactory.cpp PresetFactoryManager.cpp ${GLEW_SOURCES})

if (NOT DISABLE_NATIVE_PRESETS)
add_subdirectory(NativePresetFactory)
SET(PRESET_FACTORY_SOURCES ${PRESET_FACTORY_SOURCES} ${NativePresetFactory_SOURCE_DIR})
SET(PRESET_FACTORY_BINARY_DIR ${PRESET_FACTORY_BINARY_DIR} ${NativePresetFactory_BINARY_DIR})
SET(PRESET_FACTORY_LINK_TARGETS ${PRESET_FACTORY_LINK_TARGETS} NativePresetFactory)
endif(NOT DISABLE_NATIVE_PRESETS)

if (NOT DISABLE_MILKDROP_PRESETS)
add_subdirectory(MilkdropPresetFactory)
SET(PRESET_FACTORY_SOURCES ${PRESET_FACTORY_SOURCES} ${MilkdropPresetFactory_SOURCE_DIR})
SET(PRESET_FACTORY_BINARY_DIR ${PRESET_FACTORY_BINARY_DIR} ${MilkdropPrmaesetFactory_BINARY_DIR})
SET(PRESET_FACTORY_LINK_TARGETS ${PRESET_FACTORY_LINK_TARGETS} MilkdropPresetFactory)
endif(NOT DISABLE_MILKDROP_PRESETS)

if (USE_DEVIL)
SET (projectM_SOURCES ${projectM_SOURCES})
ADD_DEFINITIONS(-DUSE_DEVIL)
SET (IMAGE_LINK_TARGETS IL ILU ILUT)
else (USE_DEVIL)
SET (projectM_SOURCES ${projectM_SOURCES})
SET (IMAGE_LINK_TARGETS )
endif (USE_DEVIL)

if (SYNC_PRESET_SWITCHES)
ADD_DEFINITIONS(-DSYNC_PRESET_SWITCHES)
endif(SYNC_PRESET_SWITCHES)

if (USE_CG)
ADD_DEFINITIONS(-DUSE_CG)
SET (CG_LINK_TARGETS Cg CgGL)
else (USE_CG)
SET (CG_LINK_TARGETS)
endif(USE_CG)


if(USE_GLES1)
ADD_DEFINITIONS(-DUSE_GLES1)
endif(USE_GLES1)

if(USE_FBO)
ADD_DEFINITIONS(-DUSE_FBO)
endif(USE_FBO)

if(USE_FTGL)
ADD_DEFINITIONS(-DUSE_FTGL)

find_package( Freetype2 REQUIRED)
include_directories( ${FREETYPE2_INCLUDE_DIR} )

endif(USE_FTGL)


if(COMMAND cmake_policy)
       cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)

if (BUILD_PROJECTM_STATIC)
ADD_LIBRARY(projectM STATIC ${projectM_SOURCES})
else(BUILD_PROJECTM_STATIC)
ADD_LIBRARY(projectM SHARED ${projectM_SOURCES})
endif(BUILD_PROJECTM_STATIC)

SET_TARGET_PROPERTIES(projectM PROPERTIES VERSION 2.00 SOVERSION 2)

ADD_DEFINITIONS(-DUSE_THREADS)

if (APPLE)
ADD_DEFINITIONS(-DMACOS -DSTBI_NO_DDS -DUSE_THREADS)
set(RESOURCE_PREFIX "Resources")
else (APPLE)
if (WIN32)
set (RESOURCE_PREFIX "Resources")
ADD_DEFINITIONS(-DWIN32 -DSTBI_NO_DDS -DUSE_THREADS)
else(WIN32)
set (RESOURCE_PREFIX "share/projectM")
ADD_DEFINITIONS(-DLINUX -DSTBI_NO_DDS -DUSE_THREADS)
endif(WIN32)
endif(APPLE)

add_subdirectory(Renderer)

#ADD_DEFINITIONS(-DCMAKE_INSTALL_PREFIX="\\\"${CMAKE_INSTALL_PREFIX}\\\"") 

FIND_PACKAGE(OpenGL)

INCLUDE(FindPkgConfig.cmake)

pkg_search_module (FTGL ftgl)

#set(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH};/opt/local/include;/Developer/SDKs/MACOSX10.5.sdk/usr/X11/include)
#set(CMAKE_LIBRARY_PATH /opt/local/lib;/Developer/SDKs/MACOSX10.5.sdk;/Developer/SDKs/MacOSX10.5.sdk/usr/X11/lib/)

IF(USE_FTGL)
	if (FTGL_FOUND)
		MESSAGE(STATUS "[projectM] FTGL font support was detected and enabled.")
	else (FTGL_FOUND)
		MESSAGE(STATUS 
	"[projectM] Warning! FTGL has been enabled but not detected. Your compilation will probably break. Turn off FTGL support by setting USE_FTGL to false, or visit http://ftgl.wiki.sourceforge.net/ for help on installing FTGL.")
	endif (FTGL_FOUND)
	
	ADD_DEFINITIONS(-DUSE_FTGL)
	
	set (FTGL_INCUDE_DIRS ${FTGL_INCLUDE_DIRS} ${CMAKE_INCLUDE_PATH} ${OPENGL_INCLUDE_DIR})
	set (FTGL_LINK_DIRS ${FTGL_LIBRARY_DIRS} )
	set (FTGL_LINK_TARGETS ftgl freetype)
	
ELSE (USE_FTGL)
	IF (FTGL_FOUND)
		MESSAGE(STATUS "[projectM] Warning: FTGL font support was detected but NOT enabled. You can enable it by setting USE_FTGL=true")
	ELSE (FTGL_FOUND)
		MESSAGE(STATUS "[projectM] Warning: FTGL font support was not detected. Visit http://ftgl.wiki.sourceforge.net/ for help on installing FTGL.")
	ENDIF(FTGL_FOUND)
	
	# todo: change this because its obscure and probably not being used
	set (FTGL_INCLUDE_DIRS "/opt/local/include")
	set (FTGL_LINK_DIRS )
	set (FTGL_LINK_TARGETS )
ENDIF(USE_FTGL)


IF(USE_OPENMP)
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fopenmp ")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp ")
ENDIF(USE_OPENMP)

INCLUDE_DIRECTORIES(${projectM_SOURCE_DIR} ${FTGL_INCLUDE_DIRS} ${Renderer_SOURCE_DIR} ${PRESET_FACTORY_SOURCES})
LINK_DIRECTORIES(${FTGL_LINK_DIRS} ${Renderer_BINARY_DIR} ${PRESET_FACTORY_BINARY_DIR})

if(BUILD_PROJECTM_STATIC)
		TARGET_LINK_LIBRARIES(projectM ${GLEW_LINK_TARGETS} m ${FTGL_LINK_TARGETS} ${OPENGL_LIBRARIES} ${IMAGE_LINK_TARGETS} ${CG_LINK_TARGETS} ${PRESET_FACTORY_LINK_TARGETS})
else(BUILD_PROJECTM_STATIC)

TARGET_LINK_LIBRARIES(projectM ${GLEW_LINK_TARGETS} m ${FTGL_LINK_TARGETS} ${OPENGL_LIBRARIES}  ${IMAGE_LINK_TARGETS} ${CG_LINK_TARGETS} ${PRESET_FACTORY_LINK_TARGETS})

endif(BUILD_PROJECTM_STATIC)


CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/libprojectM.pc.in" "${CMAKE_CURRENT_SOURCE_DIR}/libprojectM.pc" @ONLY)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.inp.in" "${CMAKE_CURRENT_SOURCE_DIR}/config.inp" @ONLY)

FILE(GLOB presets "presets/*.milk" "presets/*.prjm" "presets/*.tga")
INSTALL(FILES ${presets} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/projectM/presets)
FILE(GLOB fonts "fonts/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/projectM/fonts)
INSTALL(FILES ${Renderer_SOURCE_DIR}/projectM.cg ${Renderer_SOURCE_DIR}/blur.cg DESTINATION ${CMAKE_INSTALL_PREFIX}/share/projectM/shaders)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/libprojectM.pc DESTINATION ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/pkgconfig)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config.inp DESTINATION ${CMAKE_INSTALL_PREFIX}/share/projectM)
INSTALL(FILES projectM.hpp event.h dlldefs.h fatal.h PCM.hpp Common.hpp DESTINATION include/libprojectM)
INSTALL(TARGETS projectM DESTINATION lib${LIB_SUFFIX})
