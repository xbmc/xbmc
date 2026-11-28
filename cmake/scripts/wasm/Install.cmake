# WASM install rules — output is kodi.js / kodi.wasm (and optional .html launcher)
set(APP_BINARY ${APP_NAME_LC})
set(APP_PREFIX ${prefix})
set(APP_LIB_DIR ${libdir}/${APP_NAME_LC})
set(APP_DATA_DIR ${datarootdir}/${APP_NAME_LC})
set(APP_INCLUDE_DIR ${includedir}/${APP_NAME_LC})

configure_file(${CMAKE_SOURCE_DIR}/cmake/KodiConfig.cmake.in
               ${CORE_BUILD_DIR}/scripts/${APP_NAME}Config.cmake @ONLY)

# Stage a servable browser build. kodi.html loads kodi.js relative to itself and
# kodi.js finds kodi.wasm and kodi.data beside the script, so one directory
# holding the launcher and the link outputs is all a browser build needs.
set(WASM_PACKAGE_DIR ${CMAKE_BINARY_DIR}/wasm_packaging)

add_custom_target(package
  DEPENDS ${APP_NAME_LC}
  COMMAND ${CMAKE_COMMAND} -E make_directory ${WASM_PACKAGE_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_SOURCE_DIR}/tools/wasm/kodi.html
          ${WASM_PACKAGE_DIR}/index.html
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
          ${CMAKE_BINARY_DIR}/${APP_NAME_LC}.js
          ${CMAKE_BINARY_DIR}/${APP_NAME_LC}.wasm
          ${CMAKE_BINARY_DIR}/${APP_NAME_LC}.data
          ${CMAKE_BINARY_DIR}/audio_worklet_processor.js
          ${WASM_PACKAGE_DIR}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)

add_custom_target(package-clean
  COMMAND ${CMAKE_COMMAND} -E rm -rf ${WASM_PACKAGE_DIR}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)

set_target_properties(package package-clean PROPERTIES FOLDER "Build Utilities")
