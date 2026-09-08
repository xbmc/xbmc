# CPython is linked statically, so it has no install prefix to find on a real
# filesystem. Ship its standard library inside the Emscripten VFS as the zip
# landmark getpath() looks for at <PYTHONHOME>/lib/pythonXY.zip; XBPython points
# PYTHONHOME at special://xbmc, which is WASM_VFS_PREFIX here.
if(ENABLE_PYTHON AND TARGET ${APP_NAME_LC}::Python)
  string(REPLACE "." "" PYTHON_ZIP_VERSION "${PYTHON_VERSION}")
  set(PYTHON_STDLIB_DIR "${DEPENDS_PATH}/lib/python${PYTHON_VERSION}")
  set(PYTHON_STDLIB_VFS_DIR "${CMAKE_BINARY_DIR}/python-stdlib/lib")
  set(PYTHON_STDLIB_ZIP "${PYTHON_STDLIB_VFS_DIR}/python${PYTHON_ZIP_VERSION}.zip")

  file(GLOB_RECURSE PYTHON_STDLIB_SOURCES "${PYTHON_STDLIB_DIR}/*.py")

  add_custom_command(OUTPUT "${PYTHON_STDLIB_ZIP}"
    COMMAND ${CMAKE_COMMAND} -E make_directory "${PYTHON_STDLIB_VFS_DIR}"
    COMMAND ${CMAKE_COMMAND} -E rm -f "${PYTHON_STDLIB_ZIP}"
    COMMAND ${CMAKE_COMMAND} -E chdir "${PYTHON_STDLIB_DIR}"
            ${CMAKE_COMMAND} -E tar cf "${PYTHON_STDLIB_ZIP}" --format=zip .
    DEPENDS ${PYTHON_STDLIB_SOURCES}
    COMMENT "Packing the Python standard library for the WASM virtual filesystem"
    VERBATIM)

  add_custom_target(python-stdlib-zip DEPENDS "${PYTHON_STDLIB_ZIP}")
  set_target_properties(python-stdlib-zip PROPERTIES FOLDER "Build Utilities")
  add_dependencies(${APP_NAME_LC} python-stdlib-zip)

  target_link_options(${APP_NAME_LC} PRIVATE
    "SHELL:--preload-file ${PYTHON_STDLIB_VFS_DIR}@${WASM_VFS_PREFIX}/lib")
endif()
