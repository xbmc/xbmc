# webOS packaging
set(APP_PACKAGE_DIR ${CMAKE_BINARY_DIR}/tools/webOS/packaging)

# Configure files into packaging environment.
configure_file(${CMAKE_SOURCE_DIR}/tools/webOS/packaging/appinfo.json.in ${APP_PACKAGE_DIR}/appinfo.json @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/webOS/packaging/icon.png ${APP_PACKAGE_DIR}/icon.png COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/webOS/packaging/largeIcon.png ${APP_PACKAGE_DIR}/largeIcon.png COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/webOS/packaging/keyboard.xml ${APP_PACKAGE_DIR}/.kodi/userdata/keymaps/keyboard.xml COPYONLY)

# Copy files to the location expected by the webOS packaging scripts.
add_custom_target(bundle
  COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_PACKAGE_DIR}/lib/python3
  COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_PACKAGE_DIR}/addons
  COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_PACKAGE_DIR}/media
  COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_PACKAGE_DIR}/system
  COMMAND ${CMAKE_COMMAND} -E make_directory ${APP_PACKAGE_DIR}/userdata
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/addons ${APP_PACKAGE_DIR}/addons
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/media ${APP_PACKAGE_DIR}/media
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/system ${APP_PACKAGE_DIR}/system
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_BINARY_DIR}/userdata ${APP_PACKAGE_DIR}/userdata
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${DEPENDS_PATH}/lib/python${PYTHON_VERSION} ${APP_PACKAGE_DIR}/lib/python3
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${APP_BINARY} ${APP_PACKAGE_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/usr/lib/libgssapi_krb5.so.2 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/usr/lib/libkrb5.so.3 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/usr/lib/libkrb5support.so.0 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/usr/lib/libk5crypto.so.3 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/usr/lib/libcrypto.so.1.1 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/usr/lib/libcom_err.so.3 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${DEPENDS_PATH}/lib/libwayland-client++.so.0 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${DEPENDS_PATH}/lib/libwayland-cursor++.so.0 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${DEPENDS_PATH}/lib/libwayland-egl++.so.0 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${DEPENDS_PATH}/lib/libshairplay.so.0 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${DEPENDS_PATH}/lib/libdrm.so* ${APP_PACKAGE_DIR}/lib
)
add_dependencies(bundle ${APP_NAME_LC})



add_custom_target(ipk
  COMMAND ares-package ${APP_PACKAGE_DIR}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)
add_dependencies(ipk bundle)

add_custom_target(ipk-clean
  COMMAND ${CMAKE_COMMAND} -E rm -r ${APP_PACKAGE_DIR}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)
