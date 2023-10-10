# webOS packaging
find_program(ARES_PACKAGE ares-package PATHS ${TOOLCHAIN}/../CLI ENV WEBOS_CLI_PATH
                                       PATH_SUFFIXES bin
                                       REQUIRED)

set(APP_PACKAGE_DIR ${CMAKE_BINARY_DIR}/tools/webOS/packaging)

# Configure files into packaging environment.
configure_file(${CMAKE_SOURCE_DIR}/tools/webOS/packaging/appinfo.json.in ${APP_PACKAGE_DIR}/appinfo.json @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/webOS/packaging/icon.png ${APP_PACKAGE_DIR}/icon.png COPYONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/webOS/packaging/largeIcon.png ${APP_PACKAGE_DIR}/largeIcon.png COPYONLY)


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


set(APP_INSTALL_DIRS ${CMAKE_BINARY_DIR}/addons
                     ${CMAKE_BINARY_DIR}/media
                     ${CMAKE_BINARY_DIR}/system
                     ${CMAKE_BINARY_DIR}/userdata)
set(APP_TOOLCHAIN_FILES ${TOOLCHAIN}/${HOST}/sysroot/lib/libatomic.so.1
                        ${TOOLCHAIN}/${HOST}/sysroot/lib/libcrypt.so.1
                        ${CMAKE_BINARY_DIR}/libAcbAPI.so.1)
set(BIN_ADDONS_DIR ${DEPENDS_PATH}/addons)

file(WRITE ${CMAKE_BINARY_DIR}/install.cmake "
  file(INSTALL ${APP_BINARY} DESTINATION ${APP_PACKAGE_DIR}
       FILE_PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
  file(INSTALL ${APP_INSTALL_DIRS} DESTINATION ${APP_PACKAGE_DIR})
  file(CREATE_LINK python${PYTHON_VERSION} python3 SYMBOLIC)
  file(INSTALL python3 DESTINATION ${APP_PACKAGE_DIR}/lib)
  file(INSTALL ${DEPENDS_PATH}/lib/python${PYTHON_VERSION} DESTINATION ${APP_PACKAGE_DIR}/lib FOLLOW_SYMLINK_CHAIN)
  file(INSTALL ${APP_TOOLCHAIN_FILES} DESTINATION ${APP_PACKAGE_DIR}/lib FOLLOW_SYMLINK_CHAIN)

  file(STRINGS ${CMAKE_BINARY_DIR}/missing_libs.txt missing_libs)
  foreach(lib IN LISTS missing_libs)
    file(INSTALL ${DEPENDS_PATH}/lib/\$\{lib\} DESTINATION ${APP_PACKAGE_DIR}/lib FOLLOW_SYMLINK_CHAIN)
  endforeach()

  if(EXISTS ${BIN_ADDONS_DIR})
    file(INSTALL ${BIN_ADDONS_DIR} DESTINATION ${APP_PACKAGE_DIR})
  endif()
")

# Copy files to the location expected by the webOS packaging scripts.
add_custom_target(bundle
  DEPENDS ${APP_NAME_LC} ${CMAKE_BINARY_DIR}/missing_libs.txt AcbAPI
  COMMAND ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/install.cmake
)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/missing_libs.txt
  DEPENDS ${CMAKE_BINARY_DIR}/${APP_BINARY}
  COMMAND bash -c "WEBOS_ROOTFS=${WEBOS_ROOTFS} WEBOS_LD_LIBRARY_PATH=${WEBOS_LD_LIBRARY_PATH} \
            ${VERIFY_EXE} ${CMAKE_BINARY_DIR}/${APP_BINARY} \
            | awk '/Missing library:/ { print $3 }'" > missing_libs.txt
  VERBATIM
)

add_custom_target(verify-libs
  DEPENDS bundle ${CMAKE_BINARY_DIR}/missing_libs.txt
  COMMAND echo verifying dynamic library dependencies
  COMMAND env WEBOS_ROOTFS=${WEBOS_ROOTFS} WEBOS_LD_LIBRARY_PATH=${WEBOS_LD_LIBRARY_PATH}
          ${VERIFY_EXE} ${APP_PACKAGE_DIR}/${APP_BINARY}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)

if(CMAKE_BUILD_TYPE STREQUAL Release)
  add_custom_target(strip
    DEPENDS bundle verify-libs
    COMMAND find ${APP_PACKAGE_DIR} -iname *.so* -exec ${CMAKE_STRIP} ${APP_PACKAGE_DIR}/${APP_BINARY} {} \;
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    VERBATIM
  )
  set(IPK_DEPENDS strip)
endif()

add_custom_target(ipk
  DEPENDS bundle ${IPK_DEPENDS}
  COMMAND ${ARES_PACKAGE} ${APP_PACKAGE_DIR}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)

add_custom_target(ipk-clean
  COMMAND ${CMAKE_COMMAND} -E rm -r ${APP_PACKAGE_DIR}
  WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
  VERBATIM
)
