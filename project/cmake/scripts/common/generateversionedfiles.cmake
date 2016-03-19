include(${CORE_SOURCE_DIR}/project/cmake/scripts/common/macros.cmake)

core_find_versions()
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/addons/xbmc.addon)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/addons/kodi.guilib)
configure_file(${CORE_SOURCE_DIR}/addons/xbmc.addon/addon.xml.in
               ${CMAKE_BINARY_DIR}/addons/xbmc.addon/addon.xml @ONLY)
configure_file(${CORE_SOURCE_DIR}/addons/kodi.guilib/addon.xml.in
               ${CMAKE_BINARY_DIR}/addons/kodi.guilib/addon.xml @ONLY)
configure_file(${CORE_SOURCE_DIR}/xbmc/CompileInfo.cpp.in
               ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/xbmc/CompileInfo.cpp @ONLY)
set(prefix ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR})
set(APP_LIBDIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/kodi)
