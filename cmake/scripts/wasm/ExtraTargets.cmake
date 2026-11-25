# Emscripten reads these at link time; without this a change to them does not relink.
set_property(TARGET ${APP_NAME_LC} APPEND PROPERTY LINK_DEPENDS
  ${CMAKE_SOURCE_DIR}/xbmc/platform/wasm/kodi_pre.js
  ${CMAKE_SOURCE_DIR}/xbmc/platform/wasm/audio_worklet.js
  ${CMAKE_SOURCE_DIR}/cmake/scripts/wasm/addon-exports.txt
  ${CMAKE_SOURCE_DIR}/xbmc/windowing/wasm/webgl_commit.js
  ${CMAKE_SOURCE_DIR}/xbmc/platform/wasm/network/tizen_sockets.js
  ${CMAKE_SOURCE_DIR}/xbmc/cores/VideoPlayer/DVDCodecs/Video/webcodecs_bridge.js
  ${CMAKE_SOURCE_DIR}/xbmc/platform/wasm/media_layer.js
  ${CMAKE_SOURCE_DIR}/xbmc/platform/wasm/mse_bridge.js
  ${CMAKE_SOURCE_DIR}/xbmc/platform/wasm/tizen_emss.js)

# The AudioWorklet module is fetched at run time from next to ${APP_NAME_LC}.js.
configure_file(${CMAKE_SOURCE_DIR}/xbmc/platform/wasm/audio_worklet_processor.js
               ${CMAKE_BINARY_DIR}/audio_worklet_processor.js COPYONLY)

if(ENABLE_WASM_DEV_PROXY)
  set_property(TARGET ${APP_NAME_LC} APPEND PROPERTY LINK_DEPENDS
    ${CMAKE_SOURCE_DIR}/tools/wasm/dev_proxy_pre.js)
endif()

# CPython is linked statically, so it has no install prefix to find on a real
# filesystem. Ship its standard library inside the Emscripten VFS as the zip
# landmark getpath() looks for at <PYTHONHOME>/lib/pythonXY.zip; XBPython points
# PYTHONHOME at special://xbmc, which is WASM_VFS_PREFIX here.
if(TARGET ${APP_NAME_LC}::Python)
  string(REPLACE "." "" PYTHON_ZIP_VERSION "${PYTHON_VERSION}")
  set(PYTHON_STDLIB_DIR "${DEPENDS_PATH}/lib/python${PYTHON_VERSION}")
  set(PYTHON_STDLIB_VFS_DIR "${CMAKE_BINARY_DIR}/python-stdlib/lib")
  set(PYTHON_STDLIB_ZIP "${PYTHON_STDLIB_VFS_DIR}/python${PYTHON_ZIP_VERSION}.zip")
  set(PYTHON_STDLIB_PACKER "${CMAKE_SOURCE_DIR}/tools/wasm/pack_python_stdlib.py")

  find_package(PythonInterpreter REQUIRED)
  file(GLOB_RECURSE PYTHON_STDLIB_SOURCES CONFIGURE_DEPENDS "${PYTHON_STDLIB_DIR}/*.py")

  add_custom_command(OUTPUT "${PYTHON_STDLIB_ZIP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PYTHON_STDLIB_VFS_DIR}"
    COMMAND ${PYTHON_EXECUTABLE} "${PYTHON_STDLIB_PACKER}"
            "${PYTHON_STDLIB_DIR}" "${PYTHON_STDLIB_ZIP}"
    DEPENDS ${PYTHON_STDLIB_SOURCES} "${PYTHON_STDLIB_PACKER}"
    COMMENT "Packing the Python standard library for the WASM virtual filesystem"
    VERBATIM)

  add_custom_target(python-stdlib-zip DEPENDS "${PYTHON_STDLIB_ZIP}")
  set_target_properties(python-stdlib-zip PROPERTIES FOLDER "Build Utilities")
  add_dependencies(${APP_NAME_LC} python-stdlib-zip)

  target_link_options(${APP_NAME_LC} PRIVATE
    "SHELL:--preload-file ${PYTHON_STDLIB_VFS_DIR}@${WASM_VFS_PREFIX}/lib")
  set_property(TARGET ${APP_NAME_LC} APPEND PROPERTY LINK_DEPENDS "${PYTHON_STDLIB_ZIP}")
endif()

include(${CMAKE_SOURCE_DIR}/cmake/scripts/wasm/TizenTargets.cmake)
