set(subdirs cores cores/AudioRenderers addons cdrip dbwrappers dialogs filesystem guilib input interfaces music network
peripherals pictures playlists powermanagement programs rendering settings storage threads
utils video windowing windows)

if(APPLE)
	list(APPEND subdirs osx)
endif(APPLE)
if(UNIX)
	list(APPEND subdirs linux)
endif(UNIX)

foreach(dir ${subdirs})
	add_subdirectory(${dir})
endforeach()

set(PlexOwnedFiles BackgroundInfoLoader.cpp)

aux_source_directory(. xbmc_SRCS)
foreach(f ${PlexOwnedFiles})
  list(REMOVE_ITEM xbmc_SRCS ./${f})
endforeach()

get_property(ALL_SRCS GLOBAL PROPERTY SRCS_LIST)
add_library(xbmc OBJECT ${xbmc_SRCS} ${ALL_SRCS})

# reset this so we can use add_sources later on.
set_property(GLOBAL PROPERTY SRCS_LIST "")