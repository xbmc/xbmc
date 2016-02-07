set(SOURCES libKODI_adsp.cpp)

core_add_library(KODI_adsp NO_MAIN_DEPENDS)
set_target_properties(KODI_adsp PROPERTIES POSITION_INDEPENDENT_CODE True)
target_include_directories(KODI_adsp PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CORE_SOURCE_DIR}/xbmc/addons/include
                           ${CORE_SOURCE_DIR}/xbmc)
