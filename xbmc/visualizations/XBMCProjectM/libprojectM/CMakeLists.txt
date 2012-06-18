PROJECT(projectM)

cmake_minimum_required(VERSION 2.6)

OPTION (USE_DEVIL "Use devIL for image loading rather than the builtin SOIL library" OFF)

OPTION (USE_FBO "Use Framebuffer Objects for increased rendering quality.  Disable this for OpenGL ES 1.x or if you are experiencing problems on older or poorly supported hardware." ON)

OPTION (USE_GLES1 "Use OpenGL ES 1.x" OFF)


OPTION (USE_NATIVE_GLEW "Use projectM's native implemention of GLEW." OFF)

#OPTION (FTGL_STATIC "Build the projectM target library in the platform's native static (NOT shared) format." OFF)

OPTION (BUILD_PROJECTM_STATIC "Build the projectM target library in the platform's native static (NOT shared) format." OFF)
SET(SOIL_SOURCES image_DXT.c image_helper.c SOIL.c stb_image_aug.c)

if (USE_NATIVE_GLEW)
ADD_DEFINITIONS(-DUSE_NATIVE_GLEW)
SET(GLEW_SOURCES glew.h glew.c)
SET(GLEW_LINK_TARGETS )
else(USE_NATIVE_GLEW)
SET(GLEW_SOURCES )
SET (GLEW_LINK_TARGETS GLEW)
endif(USE_NATIVE_GLEW)

SET(projectM_SOURCES projectM.cpp FBO.cpp InitCond.cpp 
 Expr.cpp PCM.cpp Parser.cpp Preset.cpp BeatDetect.cpp PerPixelEqn.cpp Param.cpp CustomWave.cpp CustomShape.cpp 
Func.cpp Eval.cpp  PerFrameEqn.cpp PerPointEqn.cpp fftsg.cpp KeyHandler.cpp
 timer.cpp  wipemalloc.cpp BuiltinFuncs.cpp BuiltinParams.cpp Renderer.cpp 
PresetLoader.cpp  PresetChooser.cpp PresetFrameIO.cpp PresetMerge.cpp
ConfigFile.cpp  IdlePreset.cpp TextureManager.cpp TimeKeeper.cpp ${GLEW_SOURCES})

if (USE_DEVIL)
SET (projectM_SOURCES ${projectM_SOURCES})
ADD_DEFINITIONS(-DUSE_DEVIL)
SET (IMAGE_LINK_TARGETS IL ILU ILUT)
else (USE_DEVIL)
SET (projectM_SOURCES ${projectM_SOURCES} ${SOIL_SOURCES})
SET (IMAGE_LINK_TARGETS )
endif (USE_DEVIL)

if(USE_GLES1)
ADD_DEFINITIONS(-DUSE_GLES1)
endif(USE_GLES1)

if(USE_FBO)
ADD_DEFINITIONS(-DUSE_FBO)
endif(USE_FBO)

if (BUILD_PROJECTM_STATIC)
ADD_LIBRARY(projectM STATIC ${projectM_SOURCES})
else(BUILD_PROJECTM_STATIC)
ADD_LIBRARY(projectM SHARED ${projectM_SOURCES})
endif(BUILD_PROJECTM_STATIC)

SET_TARGET_PROPERTIES(projectM PROPERTIES VERSION 2.00 SOVERSION 2)

if (APPLE)
ADD_DEFINITIONS(-DMACOS -DSTBI_NO_DDS)
set(RESOURCE_PREFIX "Resources" )
FIND_LIBRARY(CORE_FOUNDATION_LIBRARY CoreFoundation )
MARK_AS_ADVANCED (CORE_FOUNDATION_LIBRARY)
set(EXTRA_LIBS ${CORE_FOUNDATION_LIBRARY})
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mmacosx-version-min=10.4 -L/opt/local/lib" ) 
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mmacosx-version-min=10.4 -L/opt/local/lib" ) 
else (APPLE)
if (WIN32)
set (RESOURCE_PREFIX "Resources")
ADD_DEFINITIONS(-DWIN32 -DSTBI_NO_DDS)
else(WIN32)
set (RESOURCE_PREFIX "share/projectM")
ADD_DEFINITIONS(-DLINUX -DSTBI_NO_DDS)
endif(WIN32)
endif(APPLE)

ADD_DEFINITIONS(-DCMAKE_INSTALL_PREFIX="\\\"${CMAKE_INSTALL_PREFIX}\\\"") 

FIND_PACKAGE(OpenGL)

INCLUDE(FindPkgConfig.cmake)

pkg_search_module (FTGL ftgl)

if (FTGL_FOUND)
	OPTION(USE_FTGL "Use FTGL for on-screen fonts (found on your system)" ON)
else (FTGL_FOUND)	
	OPTION(USE_FTGL "Use FTGL for on-screen fonts (Warning: NOT found on your system! Turning this on will probably bust the compilation.)" OFF)
endif(FTGL_FOUND)

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
	set (FTGL_LINK_DIRS)
	set (FTGL_LINK_TARGETS )
ENDIF(USE_FTGL)



INCLUDE_DIRECTORIES(${FTGL_INCLUDE_DIRS})
LINK_DIRECTORIES(${FTGL_LINK_DIRS} "./")

if(BUILD_PROJECTM_STATIC)
		TARGET_LINK_LIBRARIES(projectM ${GLEW_LINK_TARGETS} m ${FTGL_LINK_TARGETS} ${OPENGL_LIBRARIES} ${IMAGE_LINK_TARGETS} ${EXTRA_LIBS} )
else(BUILD_PROJECTM_STATIC)
		TARGET_LINK_LIBRARIES(projectM ${GLEW_LINK_TARGETS} m ${FTGL_LINK_TARGETS} ${OPENGL_LIBRARIES} ${IMAGE_LINK_TARGETS} ${EXTRA_LIBS} )
endif(BUILD_PROJECTM_STATIC)


CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/libprojectM.pc.in" "${CMAKE_CURRENT_SOURCE_DIR}/libprojectM.pc" @ONLY)
CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/config.inp.in" "${CMAKE_CURRENT_SOURCE_DIR}/config.inp" @ONLY)

FILE(GLOB presets "presets/*.milk" "presets/*.prjm" "presets/*.tga")
INSTALL(FILES ${presets} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/projectM/presets)
FILE(GLOB fonts "fonts/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/projectM/fonts)
INSTALL(FILES "${CMAKE_CURRENT_SOURCE_DIR}/libprojectM.pc" DESTINATION ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/pkgconfig)
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/config.inp DESTINATION ${CMAKE_INSTALL_PREFIX}/share/projectM)
INSTALL(FILES projectM.hpp event.h dlldefs.h fatal.h PresetFrameIO.hpp PCM.hpp  DESTINATION include/libprojectM)
INSTALL(TARGETS projectM DESTINATION lib${LIB_SUFFIX})
