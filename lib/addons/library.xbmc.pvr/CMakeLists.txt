project(XBMC_pvr)
core_add_addon_library(${PROJECT_NAME})
target_include_directories(${PROJECT_NAME} PRIVATE ${CORE_SOURCE_DIR}/xbmc/cores/VideoPlayer/DVDDemuxers)
