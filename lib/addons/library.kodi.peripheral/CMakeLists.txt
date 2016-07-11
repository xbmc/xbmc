set(SOURCES libKODI_peripheral.cpp)

core_add_library(KODI_peripheral NO_MAIN_DEPENDS)
set_target_properties(KODI_peripheral PROPERTIES POSITION_INDEPENDENT_CODE True)
target_include_directories(KODI_peripheral PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi
                           ${CORE_SOURCE_DIR}/xbmc)
