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
  set(BUNDLE_RESOURCES ${CMAKE_SOURCE_DIR}/media/splash.jpg
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon29x29.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon29x29@2x.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon40x40.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon40x40@2x.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon50x50.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon50x50@2x.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon57x57.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon57x57@2x.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon60x60.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon60x60@2x.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon72x72.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon72x72@2x.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon76x76.png
                       ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/ios/rounded/AppIcon76x76@2x.png)

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
if(CORE_PLATFORM_NAME_LC STREQUAL tvos)
  if(ADDONS_TO_BUILD)
    set(_addons "ADDONS=${ADDONS_TO_BUILD}")
  endif()
  add_custom_target(binary-addons
    COMMAND $(MAKE) -C ${CMAKE_SOURCE_DIR}/tools/depends/target/binary-addons clean
    COMMAND $(MAKE) -C ${CMAKE_SOURCE_DIR}/tools/depends/target/binary-addons VERBOSE=1 V=99
          INSTALL_PREFIX="${CMAKE_BINARY_DIR}/addons" CROSS_COMPILING=yes ${_addons})
  if(ENABLE_XCODE_ADDONBUILD)
    add_dependencies(${APP_NAME_LC} binary-addons)
  endif()
  unset(_addons)
endif()

add_custom_command(TARGET ${APP_NAME_LC} POST_BUILD
    # TODO: Remove in sync with CopyRootFiles-darwin_embedded expecting the ".bin" file
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${APP_NAME_LC}>
                                     $<TARGET_FILE_DIR:${APP_NAME_LC}>/${APP_NAME}.bin

    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/DllPaths_generated.h
                                     ${CMAKE_BINARY_DIR}/xbmc/DllPaths_generated.h
    COMMAND "ACTION=build"
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
configure_file(${CMAKE_SOURCE_DIR}/tools/darwin/packaging/darwin_embedded/mkdeb-darwin_embedded.sh.in
               ${CMAKE_BINARY_DIR}/tools/darwin/packaging/darwin_embedded/mkdeb-darwin_embedded.sh @ONLY)
               
if(CORE_PLATFORM_NAME_LC STREQUAL ios)
  configure_file(${CMAKE_SOURCE_DIR}/tools/darwin/packaging/darwin_embedded/migrate_to_kodi.sh.in
                 ${CMAKE_BINARY_DIR}/tools/darwin/packaging/darwin_embedded/migrate_to_kodi.sh @ONLY)
endif()

add_custom_target(deb
    COMMAND sh ./mkdeb-darwin_embedded.sh ${CORE_BUILD_CONFIG}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tools/darwin/packaging/darwin_embedded)
add_dependencies(deb ${APP_NAME_LC})

