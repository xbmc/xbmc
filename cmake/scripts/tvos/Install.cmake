# TVOS packaging

set(BUNDLE_RESOURCES ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/tvos/Assets.xcassets/LaunchImage.launchimage/Splash@2x.png
                     ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/tvos/Assets.xcassets/LaunchImage.launchimage/Splash.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon29x29.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon29x29@2x.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon40x40.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon40x40@2x.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon50x50.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon50x50@2x.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon57x57.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon57x57@2x.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon60x60.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon60x60@2x.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon72x72.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon72x72@2x.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon76x76.png
                     ${CMAKE_SOURCE_DIR}/tools/darwin/packaging/media/tvos/rounded/AppIcon76x76@2x.png)

target_sources(${APP_NAME_LC} PRIVATE ${BUNDLE_RESOURCES})
foreach(file IN LISTS BUNDLE_RESOURCES)
  set_source_files_properties(${file} PROPERTIES MACOSX_PACKAGE_LOCATION .)
endforeach()

target_sources(${APP_NAME_LC} PRIVATE ${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/tvos/English.lproj/InfoPlist.strings)
set_source_files_properties(${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/tvos/English.lproj/InfoPlist.strings PROPERTIES MACOSX_PACKAGE_LOCATION "./English.lproj")

# Options for code signing propagated as env vars to Codesign.command via Xcode
set(TVOS_CODE_SIGN_IDENTITY "" CACHE STRING "Code Sign Identity")
if(TVOS_CODE_SIGN_IDENTITY)
  set_target_properties(${APP_NAME_LC} PROPERTIES XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED TRUE
                                                  XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ${TVOS_CODE_SIGN_IDENTITY})
endif()

add_custom_command(TARGET ${APP_NAME_LC} POST_BUILD
    # TODO: Remove in sync with CopyRootFiles-tvos expecting the ".bin" file
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${APP_NAME_LC}>
                                     $<TARGET_FILE_DIR:${APP_NAME_LC}>/${APP_NAME}.bin

    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/DllPaths_generated.h
                                     ${CMAKE_BINARY_DIR}/xbmc/DllPaths_generated.h
    COMMAND "ACTION=build"
            "TARGET_BUILD_DIR=$<TARGET_FILE_DIR:${APP_NAME_LC}>/.."
            "TARGET_NAME=${APP_NAME}.app"
            "APP_NAME=${APP_NAME}"
            "PRODUCT_NAME=${APP_NAME}"
            "WRAPPER_EXTENSION=app"
            "SRCROOT=${CMAKE_BINARY_DIR}"
            ${CMAKE_SOURCE_DIR}/tools/darwin/Support/CopyRootFiles-tvos.command
    COMMAND "XBMC_DEPENDS=${DEPENDS_PATH}"
            "TARGET_BUILD_DIR=$<TARGET_FILE_DIR:${APP_NAME_LC}>/.."
            "TARGET_NAME=${APP_NAME}.app"
            "APP_NAME=${APP_NAME}"
            "PRODUCT_NAME=${APP_NAME}"
            "FULL_PRODUCT_NAME=${APP_NAME}.app"
            "WRAPPER_EXTENSION=app"
            "SRCROOT=${CMAKE_BINARY_DIR}"
            ${CMAKE_SOURCE_DIR}/tools/darwin/Support/copyframeworks-tvos.command
    COMMAND "XBMC_DEPENDS=${DEPENDS_PATH}"
            "NATIVEPREFIX=${NATIVEPREFIX}"
            "PLATFORM_NAME=${PLATFORM}"
            "CODESIGNING_FOLDER_PATH=$<TARGET_FILE_DIR:${APP_NAME_LC}>"
            "BUILT_PRODUCTS_DIR=$<TARGET_FILE_DIR:${APP_NAME_LC}>/.."
            "WRAPPER_NAME=${APP_NAME}.app"
            "APP_NAME=${APP_NAME}"
            "CURRENT_ARCH=${ARCH}"
            ${CMAKE_SOURCE_DIR}/tools/darwin/Support/Codesign.command
)

set(DEPENDS_ROOT_FOR_XCODE ${NATIVEPREFIX}/..)
configure_file(${CMAKE_SOURCE_DIR}/tools/darwin/packaging/tvos/mkdeb-tvos.sh.in
               ${CMAKE_BINARY_DIR}/tools/darwin/packaging/tvos/mkdeb-tvos.sh @ONLY)
configure_file(${CMAKE_SOURCE_DIR}/tools/darwin/packaging/migrate_to_kodi_tvos.sh.in
               ${CMAKE_BINARY_DIR}/tools/darwin/packaging/migrate_to_kodi_tvos.sh @ONLY)

add_custom_target(deb
    COMMAND sh ./mkdeb-tvos.sh ${CORE_BUILD_CONFIG}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tools/darwin/packaging/tvos)
add_dependencies(deb ${APP_NAME_LC})

