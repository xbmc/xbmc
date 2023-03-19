# webOS packaging
set(APP_PACKAGE_DIR ${CMAKE_BINARY_DIR}/tools/webOS/packaging)

# Configure files into packaging environment.
configure_file(${CMAKE_SOURCE_DIR}/tools/webOS/packaging/appinfo.json.in ${APP_PACKAGE_DIR}/appinfo.json @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/webOS/packaging/icon.png ${APP_PACKAGE_DIR}/icon.png COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/webOS/packaging/largeIcon.png ${APP_PACKAGE_DIR}/largeIcon.png COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/webOS/packaging/keyboard.xml ${APP_PACKAGE_DIR}/.kodi/userdata/keymaps/keyboard.xml COPYONLY)


# Webos-userland are stubs for libs on the target device not contained in the toolchain,
# we don't package them, but still add them to the search path for verify
# Using DEPENDS_PATH/lib could result finding libs that are not on the target device, so
# we specifically use the build dir
get_filename_component(arch_dir ${DEPENDS_PATH} NAME)
set(WEBOS_USERLAND_LIBS ${CMAKE_SOURCE_DIR}/tools/depends/target/webos-userland/${arch_dir}/build)

# toolchain sysroot
set(WEBOS_ROOTFS ${TOOLCHAIN}/${HOST}/sysroot)
set(WEBOS_LD_LIBRARY_PATH ${WEBOS_USERLAND_LIBS}:${APP_PACKAGE_DIR}/lib)
set(VERIFY_EXE ${CMAKE_SOURCE_DIR}/tools/webOS/verify-symbols.sh)

add_custom_command(
  TARGET ${APP_NAME_LC} POST_BUILD
  COMMAND bash -c "WEBOS_ROOTFS=${WEBOS_ROOTFS} WEBOS_LD_LIBRARY_PATH=${WEBOS_LD_LIBRARY_PATH} \
            ${VERIFY_EXE} ${CMAKE_BINARY_DIR}/${APP_BINARY} \
            | awk '/Missing library:/ { print $3 }'" > missing_libs.txt
  BYPRODUCTS missing_libs.txt
  VERBATIM
)

file(WRITE ${CMAKE_BINARY_DIR}/copy_missing_libs.cmake "
  file(STRINGS ${CMAKE_BINARY_DIR}/missing_libs.txt missing_libs)
  foreach(lib IN LISTS missing_libs)
    file(INSTALL ${DEPENDS_PATH}/lib/\$\{lib\} DESTINATION ${APP_PACKAGE_DIR}/lib FOLLOW_SYMLINK_CHAIN)
  endforeach()
")

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
  # webOS 4.9 only
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/lib/libatomic.so.1 ${APP_PACKAGE_DIR}/lib
  # webOS 5?
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/usr/lib/libgssapi_krb5.so.2 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/usr/lib/libkrb5.so.3 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/usr/lib/libkrb5support.so.0 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/usr/lib/libk5crypto.so.3 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/usr/lib/libcrypto.so.1.1 ${APP_PACKAGE_DIR}/lib
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/usr/lib/libcom_err.so.3 ${APP_PACKAGE_DIR}/lib
  # webOS 7+
  COMMAND ${CMAKE_COMMAND} -E copy ${TOOLCHAIN}/${HOST}/sysroot/lib/libcrypt.so.1 ${APP_PACKAGE_DIR}/lib
  # missing depends libs
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/copy_missing_libs.cmake
)
add_dependencies(bundle ${APP_NAME_LC})

add_custom_target(verify-libs
  DEPENDS bundle
  COMMAND echo verifying dynamic library dependencies
  COMMAND env WEBOS_ROOTFS=${WEBOS_ROOTFS} WEBOS_LD_LIBRARY_PATH=${WEBOS_LD_LIBRARY_PATH}
          ${VERIFY_EXE} ${APP_PACKAGE_DIR}/${APP_BINARY}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)

add_custom_target(ipk
  DEPENDS verify-libs bundle
  COMMAND ares-package ${APP_PACKAGE_DIR}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)

add_custom_target(ipk-clean
  COMMAND ${CMAKE_COMMAND} -E rm -r ${APP_PACKAGE_DIR}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)
