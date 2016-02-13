set(SOURCES libXBMC_pvr.cpp)

core_add_library(XBMC_pvr NO_MAIN_DEPENDS)
set_target_properties(XBMC_pvr PROPERTIES POSITION_INDEPENDENT_CODE True)
target_include_directories(XBMC_pvr PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CORE_SOURCE_DIR}/addons/library.xbmc.addon
                           ${CORE_SOURCE_DIR}/xbmc
                           ${CORE_SOURCE_DIR}/xbmc/addons/include
                           ${CORE_SOURCE_DIR}/xbmc/cores/VideoPlayer/DVDDemuxers)
