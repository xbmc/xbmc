set(SOURCES libKODI_inputstream.cpp)

core_add_library(KODI_inputstream NO_MAIN_DEPENDS)
set_target_properties(KODI_inputstream PROPERTIES POSITION_INDEPENDENT_CODE True)
target_include_directories(KODI_inputstream PRIVATE
                           ${CMAKE_CURRENT_SOURCE_DIR}
                           ${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include/kodi
                           ${CORE_SOURCE_DIR}/xbmc)
