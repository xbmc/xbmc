set(subdirs cores addons cdrip dbwrappers dialogs filesystem guilib input interfaces music network
peripherals pictures playlists powermanagement programs rendering settings storage threads
utils video windowing windows pvr commons epg main)

if(TARGET_COMMON_DARWIN)
	list(APPEND subdirs osx)
endif(TARGET_COMMON_DARWIN)

if(TARGET_POSIX)
	list(APPEND subdirs linux)
endif(TARGET_POSIX)

if(TARGET_WIN32)
  list(APPEND subdirs win32)
endif(TARGET_WIN32)

if(USE_INTERNAL_FFMPEG)
  include_directories(${FFMPEG_INCLUDE_DIRS})
endif(USE_INTERNAL_FFMPEG)

foreach(dir ${subdirs})
  add_subdirectory(${dir})
endforeach()

#set(PlexOwnedFiles BackgroundInfoLoader.cpp)

find_all_sources(. xbmc_SRCS)
foreach(f ${PlexOwnedFiles})
  list(REMOVE_ITEM xbmc_SRCS ./${f})
endforeach()

if(NOT ENABLE_DVD_DRIVE)
	list(REMOVE_ITEM xbmc_SRCS ./Autorun.cpp)
endif()

get_property(ALL_SRCS GLOBAL PROPERTY SRCS_LIST)

set(SRCS ${xbmc_SRCS} ${ALL_SRCS})
ADD_MSVC_PRECOMPILED_HEADER("pch.h" "win32/pch.cpp" SRCS)
#ADD_MSVC_PRECOMPILED_HEADER("${root}/xbmc/win32/pch.h" "${root}/xbmc/win32/pch.cpp" ALL_SRCS)

add_library(xbmc STATIC ${SRCS})
add_dependencies(xbmc cpluff ffmpeg cximage windowing)

# reset this so we can use add_sources later on.
set_property(GLOBAL PROPERTY SRCS_LIST "")
