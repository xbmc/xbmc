# OSX packaging

set(PACKAGE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/build/${CORE_BUILD_CONFIG})

configure_file(${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/osx/Info.plist.in
               ${CMAKE_BINARY_DIR}/xbmc/platform/darwin/osx/Info.plist @ONLY)
execute_process(COMMAND perl -p -i -e "s/r####/${APP_SCMID}/" ${CMAKE_BINARY_DIR}/xbmc/platform/darwin/osx/Info.plist)

# Create xcode target that allows to build binary-addons.
if(CMAKE_GENERATOR MATCHES "Xcode")
  if(ADDONS_TO_BUILD)
    set(_addons "ADDONS=${ADDONS_TO_BUILD}")
  endif()
  add_custom_target(binary-addons
    COMMAND make -C ${CMAKE_SOURCE_DIR}/tools/depends/target/binary-addons clean
    COMMAND make -C ${CMAKE_SOURCE_DIR}/tools/depends/target/binary-addons VERBOSE=1 V=99
          INSTALL_PREFIX="${CMAKE_BINARY_DIR}/addons" CROSS_COMPILING=yes ${_addons})
  if(ENABLE_XCODE_ADDONBUILD)
    add_dependencies(${APP_NAME_LC} binary-addons)
  endif()
  unset(_addons)
endif()

# Optional package lib handling
# Copies TARGET INTERFACE_LINK_LIBRARIES or IMPORTED_LOCATION_* files to the
# app bundle's "Libraries" folder.
foreach(_pkg_lib IN LISTS package_libs)
  # Need to check if the target is an alias, or a direct target
  get_target_property(ALIASTARGET ${_pkg_lib} ALIASED_TARGET)
  if(ALIASTARGET)
    set(_pkg_target_name ${ALIASTARGET})
  else()
    set(_pkg_target_name ${_pkg_lib})
  endif()

  # Is the target an INTERFACE target, or an actual imported lib target
  get_target_property(_${dep}_interface_lib ${_pkg_target_name} INTERFACE_LINK_LIBRARIES)
  if(_${dep}_interface_lib)
    # An interface target does not have a CONFIGURATION option, it will only be the
    # single version file. We just always append to package_libs
    set(package_lib_expression "$<TARGET_PROPERTY:${_pkg_target_name},INTERFACE_LINK_LIBRARIES>")
  else()
    # Currently we only support DEBUG or RELEASE specifically.
    set(package_lib_expression "$<TARGET_GENEX_EVAL:${_pkg_target_name},$<IF:$<CONFIG:Debug>,$<TARGET_PROPERTY:${_pkg_target_name},IMPORTED_LOCATION_DEBUG>,$<TARGET_PROPERTY:${_pkg_target_name},IMPORTED_LOCATION_RELEASE>>>")
  endif()
  list(APPEND ADDITIONAL_COMMANDS COMMAND ${CMAKE_COMMAND} -E make_directory $<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME_LC}>/Libraries)
  list(APPEND ADDITIONAL_COMMANDS COMMAND ${CMAKE_COMMAND} -E copy_if_different ${package_lib_expression} $<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME_LC}>/Libraries)
endforeach()

add_custom_command(TARGET ${APP_NAME_LC} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/DllPaths_generated.h
                                     ${CMAKE_BINARY_DIR}/xbmc/DllPaths_generated.h
    COMMAND "ACTION=build"
            "TARGET_BUILD_DIR=$<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME_LC}>"
            "TARGET_NAME=${APP_NAME}.app"
            "APP_NAME=${APP_NAME}"
            "SRCROOT=${CMAKE_BINARY_DIR}"
            ${CMAKE_SOURCE_DIR}/tools/darwin/Support/CopyRootFiles-osx.command
    COMMAND "XBMC_DEPENDS=${DEPENDS_PATH}"
            "TARGET_BUILD_DIR=$<TARGET_BUNDLE_CONTENT_DIR:${APP_NAME_LC}>"
            "TARGET_NAME=${APP_NAME}.app"
            "APP_NAME=${APP_NAME}"
            "FULL_PRODUCT_NAME=${APP_NAME}.app"
            "SRCROOT=${CMAKE_BINARY_DIR}"
            "PYTHON_VERSION=${PYTHON_VERSION}"
            ${CMAKE_SOURCE_DIR}/tools/darwin/Support/copyframeworks-osx.command
    ${ADDITIONAL_COMMANDS})

configure_file(${CMAKE_SOURCE_DIR}/tools/darwin/packaging/osx/mkdmg-osx.sh.in
               ${CMAKE_BINARY_DIR}/tools/darwin/packaging/osx/mkdmg-osx.sh @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/Credits.html.in
               ${CMAKE_BINARY_DIR}/xbmc/platform/darwin/Credits.html @ONLY)

string(TOLOWER ${CORE_BUILD_CONFIG} CORE_BUILD_CONFIG_LOWERCASED)
if(${CORE_BUILD_CONFIG_LOWERCASED} STREQUAL "release")
  set(ALLOW_DEBUGGER "false")
else()
  set(ALLOW_DEBUGGER "true")
endif()
configure_file(${CMAKE_SOURCE_DIR}/tools/darwin/packaging/osx/Kodi.entitlements.in
               ${CMAKE_BINARY_DIR}/tools/darwin/packaging/osx/Kodi.entitlements @ONLY)

add_custom_target(dmg
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/osx/
                                               ${CMAKE_BINARY_DIR}/tools/darwin/packaging/osx/
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/osx/
                                               ${CMAKE_BINARY_DIR}/tools/darwin/packaging/media/osx/
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_SOURCE_DIR}/tools/darwin/Support/Codesign.command
                                     ${CMAKE_BINARY_DIR}/tools/darwin/packaging/osx/Codesign.command
    COMMAND "CODESIGNING_FOLDER_PATH=$<TARGET_BUNDLE_DIR:${APP_NAME_LC}>"
            "APP=$<TARGET_BUNDLE_DIR:${APP_NAME_LC}>"
            "NOTARYTOOL_KEYCHAIN_PROFILE=${NOTARYTOOL_KEYCHAIN_PROFILE}"
            "NOTARYTOOL_KEYCHAIN_PATH=${NOTARYTOOL_KEYCHAIN_PATH}"
            "EXPANDED_CODE_SIGN_IDENTITY_NAME=${CODE_SIGN_IDENTITY}"
            "PLATFORM_NAME=${PLATFORM}"
            "XCODE_BUILDTYPE=${CMAKE_CFG_INTDIR}"
            ./mkdmg-osx.sh ${CORE_BUILD_CONFIG_LOWERCASED}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tools/darwin/packaging/osx)
set_target_properties(dmg PROPERTIES FOLDER "Build Utilities")
add_dependencies(dmg ${APP_NAME_LC})
