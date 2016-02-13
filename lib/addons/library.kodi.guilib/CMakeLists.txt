set(SOURCES libKODI_guilib.cpp)

core_add_library(KODI_guilib NO_MAIN_DEPENDS)
set_target_properties(KODI_guilib PROPERTIES POSITION_INDEPENDENT_CODE True)
target_include_directories(KODI_guilib PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CORE_SOURCE_DIR}/xbmc/addons/include
                           ${CORE_SOURCE_DIR}/xbmc)
