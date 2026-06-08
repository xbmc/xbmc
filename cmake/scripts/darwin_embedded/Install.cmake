# IOS/TVOS packaging
if(CORE_PLATFORM_NAME_LC STREQUAL tvos)
  # asset catalog
  set(ASSET_CATALOG "${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/tvos/Assets.xcassets")
  set(ASSET_CATALOG_ASSETS Assets)
  set(ASSET_CATALOG_LAUNCH_IMAGE LaunchImage)

  message("generating missing asset catalog images...")
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/tools/darwin/Support/GenerateMissingImages-tvos.py "${ASSET_CATALOG}" ${ASSET_CATALOG_ASSETS} ${ASSET_CATALOG_LAUNCH_IMAGE})

  target_sources(${APP_NAME_LC} PRIVATE "${ASSET_CATALOG}")
  set_source_files_properties("${ASSET_CATALOG}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources") # adds to Copy Bundle Resources build phase

  # entitlements
  set(ENTITLEMENTS_OUT_PATH "${CMAKE_BINARY_DIR}/CMakeFiles/${APP_NAME_LC}.dir/Kodi.entitlements")
  configure_file(${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/tvos/Kodi.entitlements.in ${ENTITLEMENTS_OUT_PATH} @ONLY)

  set_target_properties(${APP_NAME_LC} PROPERTIES XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME ${ASSET_CATALOG_ASSETS}
                                                  XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_LAUNCHIMAGE_NAME ${ASSET_CATALOG_LAUNCH_IMAGE}
                                                  XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS ${ENTITLEMENTS_OUT_PATH})

else()
  set(BUNDLE_RESOURCES ${CMAKE_SOURCE_DIR}/media/applaunch_screen.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon29x29.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon29x29@2x.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon40x40.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon40x40@2x.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon50x50.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon50x50@2x.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon57x57.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon57x57@2x.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon60x60.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon60x60@2x.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon72x72.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon72x72@2x.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon76x76.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/squared/AppIcon76x76@2x.png)

  target_sources(${APP_NAME_LC} PRIVATE ${BUNDLE_RESOURCES})
  foreach(file IN LISTS BUNDLE_RESOURCES)
    set_source_files_properties(${file} PROPERTIES MACOSX_PACKAGE_LOCATION .)
  endforeach()

  target_sources(${APP_NAME_LC} PRIVATE ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/ios/LaunchScreen.storyboard)
  set_source_files_properties(${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/ios/LaunchScreen.storyboard PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")

endif()

# setup code signing
# dev team ID / identity (certificate)
set(DEVELOPMENT_TEAM "" CACHE STRING "Development Team")
set(CODE_SIGN_IDENTITY $<IF:$<BOOL:${DEVELOPMENT_TEAM}>,iPhone\ Developer,> CACHE STRING "Code Sign Identity")

# app provisioning profile
set(CODE_SIGN_STYLE_APP Automatic)
set(PROVISIONING_PROFILE_APP "" CACHE STRING "Provisioning profile name for the Kodi app")
if(PROVISIONING_PROFILE_APP)
  set(CODE_SIGN_STYLE_APP Manual)
endif()

# top shelf provisioning profile
if(CORE_PLATFORM_NAME_LC STREQUAL tvos)
  set(CODE_SIGN_STYLE_TOPSHELF Automatic)
  set(PROVISIONING_PROFILE_TOPSHELF "" CACHE STRING "Provisioning profile name for the Top Shelf")
  if(PROVISIONING_PROFILE_TOPSHELF)
    set(CODE_SIGN_STYLE_TOPSHELF Manual)
  endif()
  set_target_properties(${TOPSHELF_EXTENSION_NAME} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${CODE_SIGN_IDENTITY}"
                                                              XCODE_ATTRIBUTE_CODE_SIGN_STYLE ${CODE_SIGN_STYLE_TOPSHELF}
                                                              XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DEVELOPMENT_TEAM}"
                                                              XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${PROVISIONING_PROFILE_TOPSHELF}")
  # copy extension inside PlugIns dir of the app bundle
  add_custom_command(TARGET ${APP_NAME_LC} POST_BUILD
      COMMAND ${CMAKE_COMMAND} ARGS -E copy_directory $<TARGET_BUNDLE_DIR:${TOPSHELF_EXTENSION_NAME}>
                                                      $<TARGET_BUNDLE_DIR:${APP_NAME_LC}>/PlugIns/${TOPSHELF_EXTENSION_NAME}.${TOPSHELF_BUNDLE_EXTENSION}
                                                      MAIN_DEPENDENCY ${TOPSHELF_EXTENSION_NAME})
endif()
set_target_properties(${APP_NAME_LC} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "${CODE_SIGN_IDENTITY}"
                                                XCODE_ATTRIBUTE_CODE_SIGN_STYLE ${CODE_SIGN_STYLE_APP}
                                                XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${DEVELOPMENT_TEAM}"
                                                XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "${PROVISIONING_PROFILE_APP}")

# Create xcode target that allows to build binary-addons.
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

add_custom_command(TARGET ${APP_NAME_LC} POST_BUILD
    # TODO: Remove in sync with CopyRootFiles-darwin_embedded expecting the ".bin" file
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${APP_NAME_LC}>
                                     $<TARGET_FILE_DIR:${APP_NAME_LC}>/${APP_NAME}.bin

    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/DllPaths_generated.h
                                     ${CMAKE_BINARY_DIR}/xbmc/DllPaths_generated.h
    COMMAND "ACTION=build"
            "APP_NAME=${APP_NAME}"
            "XBMC_DEPENDS=${DEPENDS_PATH}"
            "SRCROOT=${CMAKE_BINARY_DIR}"
            ${CMAKE_SOURCE_DIR}/tools/darwin/Support/CopyRootFiles-darwin_embedded.command
    COMMAND "XBMC_DEPENDS=${DEPENDS_PATH}"
            "PYTHON_VERSION=${PYTHON_VERSION}"
            ${CMAKE_SOURCE_DIR}/tools/darwin/Support/copyframeworks-darwin_embedded.command
    COMMAND ${CMAKE_SOURCE_DIR}/tools/darwin/Support/copyframeworks-dylibs2frameworks.command
    COMMAND "XBMC_DEPENDS=${DEPENDS_PATH}"
            "NATIVEPREFIX=${NATIVEPREFIX}"
            ${CMAKE_SOURCE_DIR}/tools/darwin/Support/Codesign.command
)

if(CORE_PLATFORM_NAME_LC STREQUAL tvos)
  add_custom_command(TARGET ${APP_NAME_LC} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${DEPENDS_PATH}/share/${APP_NAME_LC} $<TARGET_FILE_DIR:${APP_NAME_LC}>/AppData/AppHome
  )
endif()

set(DEPENDS_ROOT_FOR_XCODE ${NATIVEPREFIX}/..)

set(DARWIN_EMBEDDED_CPACK_DIR ${CMAKE_BINARY_DIR}/tools/darwin/packaging/darwin_embedded)
set(DARWIN_EMBEDDED_DSYM_TARGET_DIR /Users/Shared/xbmc-depends/dSyms)
set(DARWIN_EMBEDDED_DSYM_FILENAME ${APP_NAME}.app.dSYM)
set(DARWIN_EMBEDDED_PACKAGE ${PLATFORM_BUNDLE_IDENTIFIER})
set(DARWIN_EMBEDDED_PACKAGE_ARM64 ${DARWIN_EMBEDDED_PACKAGE}64)
set(DARWIN_EMBEDDED_VERSION ${APP_VERSION_MAJOR}.${APP_VERSION_MINOR})
set(DARWIN_EMBEDDED_REVISION 0)
set(DARWIN_EMBEDDED_DEVICE iOS)
set(DARWIN_EMBEDDED_DEB_ARCHITECTURE iphoneos-arm)
if(APP_VERSION_TAG_LC)
  set(DARWIN_EMBEDDED_REVISION ${DARWIN_EMBEDDED_REVISION}~${APP_VERSION_TAG_LC})
endif()
if(CORE_PLATFORM_NAME_LC STREQUAL tvos)
  set(DARWIN_EMBEDDED_DEVICE tvOS)
  set(DARWIN_EMBEDDED_DEB_ARCHITECTURE appletvos-arm64)
endif()

configure_file(${CMAKE_SOURCE_DIR}/tools/darwin/packaging/darwin_embedded/cpack_install-darwin_embedded.cmake.in
               ${DARWIN_EMBEDDED_CPACK_DIR}/cpack_install-darwin_embedded.cmake @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/darwin/packaging/darwin_embedded/cpack_postinst-darwin_embedded.in
               ${DARWIN_EMBEDDED_CPACK_DIR}/postinst @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/darwin/packaging/darwin_embedded/cpack_prerm-darwin_embedded.in
               ${DARWIN_EMBEDDED_CPACK_DIR}/prerm @ONLY)

set(DARWIN_EMBEDDED_CPACK_KIND deb)
set(DARWIN_EMBEDDED_CPACK_GENERATOR DEB)
set(DARWIN_EMBEDDED_CPACK_PACKAGE_NAME ${DARWIN_EMBEDDED_PACKAGE_ARM64})
set(DARWIN_EMBEDDED_CPACK_FILE_NAME ${DARWIN_EMBEDDED_PACKAGE_ARM64}_${DARWIN_EMBEDDED_VERSION}-${DARWIN_EMBEDDED_REVISION}_${PLATFORM}-arm)
configure_file(${CMAKE_SOURCE_DIR}/tools/darwin/packaging/darwin_embedded/CPackConfig-darwin_embedded.cmake.in
               ${DARWIN_EMBEDDED_CPACK_DIR}/CPackConfig-deb.cmake @ONLY)

set(DARWIN_EMBEDDED_CPACK_KIND ipa)
set(DARWIN_EMBEDDED_CPACK_GENERATOR ZIP)
set(DARWIN_EMBEDDED_CPACK_PACKAGE_NAME ${DARWIN_EMBEDDED_PACKAGE})
set(DARWIN_EMBEDDED_CPACK_FILE_NAME ${DARWIN_EMBEDDED_PACKAGE}_${DARWIN_EMBEDDED_VERSION}-${DARWIN_EMBEDDED_REVISION}_${PLATFORM}-arm64)
configure_file(${CMAKE_SOURCE_DIR}/tools/darwin/packaging/darwin_embedded/CPackConfig-darwin_embedded.cmake.in
               ${DARWIN_EMBEDDED_CPACK_DIR}/CPackConfig-ipa.cmake @ONLY)

configure_file(${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/Credits.html.in
               ${CMAKE_BINARY_DIR}/xbmc/platform/darwin/Credits.html @ONLY)

add_custom_target(deb
    COMMAND ${CMAKE_COMMAND} -E env COPYFILE_DISABLE=true COPY_EXTENDED_ATTRIBUTES_DISABLE=true
            ${CMAKE_CPACK_COMMAND} -C ${CORE_BUILD_CONFIG} --config CPackConfig-deb.cmake
    WORKING_DIRECTORY ${DARWIN_EMBEDDED_CPACK_DIR})
add_dependencies(deb ${APP_NAME_LC})

add_custom_target(ipa
    COMMAND ${CMAKE_COMMAND} -E env COPYFILE_DISABLE=true COPY_EXTENDED_ATTRIBUTES_DISABLE=true
            ${CMAKE_CPACK_COMMAND} -C ${CORE_BUILD_CONFIG} --config CPackConfig-ipa.cmake
    WORKING_DIRECTORY ${DARWIN_EMBEDDED_CPACK_DIR})
add_dependencies(ipa ${APP_NAME_LC})
