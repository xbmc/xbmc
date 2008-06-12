PROJECT(projectM)
ADD_LIBRARY(projectM SHARED projectM.cpp FBO.cpp InitCond.cpp 
 Expr.cpp PCM.cpp Parser.cpp Preset.cpp BeatDetect.cpp PerPixelEqn.cpp Param.cpp CustomWave.cpp CustomShape.cpp 
Func.cpp Eval.cpp  PerFrameEqn.cpp PerPointEqn.cpp fftsg.cpp KeyHandler.cpp
 timer.cpp  wipemalloc.cpp 
BuiltinFuncs.cpp BuiltinParams.cpp Renderer.cpp 
PresetLoader.cpp  PresetChooser.cpp PresetFrameIO.cpp PresetMerge.cpp
ConfigFile.cpp  IdlePreset.cpp TextureManager.cpp image_DXT.c image_helper.c SOIL.c stb_image_aug.c TimeKeeper.cpp)

SET_TARGET_PROPERTIES(projectM PROPERTIES VERSION 2.00 SOVERSION 2)

ADD_DEFINITIONS(-DLINUX -DUSE_FBO -DSTBI_NO_DDS -DUSE_THREADS)
ADD_DEFINITIONS(-DCMAKE_INSTALL_PREFIX="\\\"${CMAKE_INSTALL_PREFIX}\\\"") 

FIND_PACKAGE(OpenGL)

INCLUDE(FindPkgConfig.cmake)

pkg_search_module (FTGL ftgl)

if (FTGL_FOUND)
	OPTION(USE_FTGL "Use FTGL for on-screen fonts (found on your system)" ON)
else (FTGL_FOUND)	
	OPTION(USE_FTGL "Use FTGL for on-screen fonts (Warning: NOT found on your system! Turning this on will probably bust the compilation.)" OFF)
endif(FTGL_FOUND)

TARGET_LINK_LIBRARIES(projectM GLEW m)

IF(USE_FTGL)
	if (FTGL_FOUND)
		MESSAGE(STATUS "FTGL font support was detected and enabled.")
	else (FTGL_FOUND)
		MESSAGE(
	"Warning! FTGL has been enabled but not detected. Your compilation will probably break. Turn off FTGL support by setting USE_FTGL to false, or visit http://ftgl.wiki.sourceforge.net/ for help on installing FTGL.")
	endif (FTGL_FOUND)
	ADD_DEFINITIONS(-DUSE_FTGL)
	INCLUDE_DIRECTORIES(${FTGL_INCLUDE_DIRS})
	LINK_DIRECTORIES(${FTGL_LIBRARY_DIRS} "./")
        TARGET_LINK_LIBRARIES(projectM GLEW m ftgl freetype)
ELSE (USE_FTGL)
	IF (FTGL_FOUND)
		MESSAGE("Warning: FTGL font support was detected but NOT enabled. You can enable it by setting USE_FTGL=true")
	ELSE (FTGL_FOUND)
		MESSAGE("Warning: FTGL font support was not detected. Visit http://ftgl.wiki.sourceforge.net/ for help on installing FTGL.")
	ENDIF(FTGL_FOUND)
ENDIF(USE_FTGL)

CONFIGURE_FILE("libprojectM.pc.in" "libprojectM.pc" @ONLY)
CONFIGURE_FILE("config.inp.in" "config.inp" @ONLY)

FILE(GLOB presets "presets/*.milk" "presets/*.prjm" "presets/*.tga")
INSTALL(FILES ${presets} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/projectM/presets)
FILE(GLOB fonts "fonts/*.ttf")
INSTALL(FILES ${fonts} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/projectM/fonts)
INSTALL(FILES "${CMAKE_BINARY_DIR}/libprojectM.pc" DESTINATION ${CMAKE_INSTALL_PREFIX}/lib${LIB_SUFFIX}/pkgconfig)
INSTALL(FILES config.inp DESTINATION ${CMAKE_INSTALL_PREFIX}/share/projectM)
INSTALL(FILES projectM.hpp event.h dlldefs.h fatal.h PresetFrameIO.hpp PCM.hpp  DESTINATION include/libprojectM)
INSTALL(TARGETS projectM DESTINATION lib)
