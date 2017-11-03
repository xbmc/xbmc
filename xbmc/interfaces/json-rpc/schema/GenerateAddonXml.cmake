include(${CMAKE_SOURCE_DIR}/cmake/scripts/common/Macros.cmake)
core_find_versions()

file(REMOVE ${CORE_BINARY_DIR}/addons/xbmc.json/addon.xml)
configure_file(${CMAKE_SOURCE_DIR}/addons/xbmc.json/addon.xml.in
               ${CORE_BINARY_DIR}/addons/xbmc.json/addon.xml @ONLY)
