# IOS packaging

set(PACKAGE_OUTPUT_DIR ${CMAKE_BINARY_DIR}/build/${CORE_BUILD_CONFIG}-iphoneos)

file(MAKE_DIRECTORY ${PACKAGE_OUTPUT_DIR}/${APP_NAME}.app)
set(BUNDLE_RESOURCES xbmc/platform/darwin/ios/Default-568h@2x.png
                     xbmc/platform/darwin/ios/Default-667h@2x.png
                     xbmc/platform/darwin/ios/Default-736h@3x.png
                     xbmc/platform/darwin/ios/Default-Landscape-736h@3x.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon29x29.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon29x29@2x.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon40x40.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon40x40@2x.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon50x50.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon50x50@2x.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon57x57.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon57x57@2x.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon60x60.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon60x60@2x.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon72x72.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon72x72@2x.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon76x76.png
                     tools/darwin/packaging/media/ios/rounded/AppIcon76x76@2x.png)

foreach(resource IN LISTS BUNDLE_RESOURCES)
  configure_file(${CORE_SOURCE_DIR}/${resource} ${PACKAGE_OUTPUT_DIR}/${APP_NAME}.app COPYONLY)
endforeach()
configure_file(${CORE_SOURCE_DIR}/xbmc/platform/darwin/ios/English.lproj/InfoPlist.strings
               ${PACKAGE_OUTPUT_DIR}/${APP_NAME}.app/English.lproj/InfoPlist.strings COPYONLY)

configure_file(${CORE_SOURCE_DIR}/xbmc/platform/darwin/ios/Info.plist.in
               ${PACKAGE_OUTPUT_DIR}/${APP_NAME}.app/Info.plist @ONLY)

add_custom_target(bundle
    COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${APP_NAME_LC}> ${PACKAGE_OUTPUT_DIR}/${APP_NAME}.app/${APP_NAME}.bin
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/DllPaths_generated.h
                                     ${CMAKE_BINARY_DIR}/xbmc/DllPaths_generated.h
    COMMAND "ACTION=build"
            "TARGET_BUILD_DIR=${PACKAGE_OUTPUT_DIR}"
            "TARGET_NAME=${APP_NAME}.app"
            "APP_NAME=${APP_NAME}"
            "PRODUCT_NAME=${APP_NAME}"
            "WRAPPER_EXTENSION=app"
            "SRCROOT=${CMAKE_BINARY_DIR}"
            ${CORE_SOURCE_DIR}/tools/darwin/Support/CopyRootFiles-ios.command
    COMMAND "XBMC_DEPENDS=${DEPENDS_PATH}"
            "TARGET_BUILD_DIR=${PACKAGE_OUTPUT_DIR}"
            "TARGET_NAME=${APP_NAME}.app"
            "APP_NAME=${APP_NAME}"
            "PRODUCT_NAME=${APP_NAME}"
            "FULL_PRODUCT_NAME=${APP_NAME}.app"
            "WRAPPER_EXTENSION=app"
            "SRCROOT=${CMAKE_BINARY_DIR}"
            ${CORE_SOURCE_DIR}/tools/darwin/Support/copyframeworks-ios.command)
add_dependencies(bundle ${APP_NAME_LC})

set(DEPENDS_ROOT_FOR_XCODE ${NATIVEPREFIX}/..)
configure_file(${CORE_SOURCE_DIR}/tools/darwin/packaging/ios/mkdeb-ios.sh.in
               ${CMAKE_BINARY_DIR}/tools/darwin/packaging/ios/mkdeb-ios.sh @ONLY)
configure_file(${CORE_SOURCE_DIR}/tools/darwin/packaging/migrate_to_kodi_ios.sh.in
               ${CMAKE_BINARY_DIR}/tools/darwin/packaging/migrate_to_kodi_ios.sh @ONLY)

add_custom_target(deb
    COMMAND "XBMC_DEPENDS_ROOT=${NATIVEPREFIX}/.."
            "PLATFORM_NAME=${PLATFORM}"
            "CODESIGNING_FOLDER_PATH=${PACKAGE_OUTPUT_DIR}/${APP_NAME}.app"
            "BUILT_PRODUCTS_DIR=${PACKAGE_OUTPUT_DIR}"
            "WRAPPER_NAME=${APP_NAME}.app"
            "APP_NAME=${APP_NAME}"
            "CODE_SIGN_IDENTITY=\"\""
            ${CORE_SOURCE_DIR}/tools/darwin/Support/Codesign.command
    COMMAND sh ./mkdeb-ios.sh ${CORE_BUILD_CONFIG}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/tools/darwin/packaging/ios)
add_dependencies(deb bundle)
