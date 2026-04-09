# WASM install rules — output is kodi.js / kodi.wasm (and optional .html launcher)
set(APP_BINARY ${APP_NAME_LC})
set(APP_PREFIX ${prefix})
set(APP_LIB_DIR ${libdir}/${APP_NAME_LC})
set(APP_DATA_DIR ${datarootdir}/${APP_NAME_LC})
set(APP_INCLUDE_DIR ${includedir}/${APP_NAME_LC})

configure_file(${CMAKE_SOURCE_DIR}/cmake/KodiConfig.cmake.in
               ${CORE_BUILD_DIR}/scripts/${APP_NAME}Config.cmake @ONLY)

message(STATUS "WASM build: install ${APP_NAME_LC}.js / ${APP_NAME_LC}.wasm from build tree")
