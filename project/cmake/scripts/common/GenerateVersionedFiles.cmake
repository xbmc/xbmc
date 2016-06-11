include(${CORE_SOURCE_DIR}/project/cmake/scripts/common/Macros.cmake)

core_find_versions()
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/addons/xbmc.addon)
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/addons/kodi.guilib)

# configure_file without dependency tracking
# configure_file would register additional file dependencies that interfere
# with the ones from add_custom_command (and the generation would happen twice)
function(generate_versioned_file _SRC _DEST)
  file(READ ${CORE_SOURCE_DIR}/${_SRC} file_content)
  string(CONFIGURE "${file_content}" file_content @ONLY)
  file(WRITE ${CMAKE_BINARY_DIR}/${_DEST} "${file_content}")
endfunction()

generate_versioned_file(addons/xbmc.addon/addon.xml.in addons/xbmc.addon/addon.xml)
generate_versioned_file(addons/kodi.guilib/addon.xml.in addons/kodi.guilib/addon.xml)
generate_versioned_file(xbmc/CompileInfo.cpp.in ${CORE_BUILD_DIR}/xbmc/CompileInfo.cpp)
