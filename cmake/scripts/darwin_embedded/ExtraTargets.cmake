if(CORE_PLATFORM_NAME_LC STREQUAL tvos)
  # top shelf extension
  set(TOPSHELF_EXTENSION_NAME "${APP_NAME_LC}-topshelf")
  set(TOPSHELF_BUNDLE_EXTENSION appex)
  set(TOPSHELF_DIR "${CMAKE_SOURCE_DIR}/xbmc/platform/darwin/tvos/TopShelf")
  # same path as the output Info.plist, taken from cmGlobalXCodeGenerator::ComputeInfoPListLocation()
  set(ENTITLEMENTS_OUT_PATH "${CMAKE_BINARY_DIR}/CMakeFiles/${TOPSHELF_EXTENSION_NAME}.dir/TopShelf.entitlements")

  set(SOURCES
        ${TOPSHELF_DIR}/../../ios-common/DarwinEmbedUtils.mm
        ${TOPSHELF_DIR}/ServiceProvider.mm
        ${TOPSHELF_DIR}/../tvosShared.mm)
  set(HEADERS
        ${TOPSHELF_DIR}/../../ios-common/DarwinEmbedUtils.h
        ${TOPSHELF_DIR}/ServiceProvider.h
        ${TOPSHELF_DIR}/../tvosShared.h)
  add_executable(${TOPSHELF_EXTENSION_NAME} MACOSX_BUNDLE ${SOURCES} ${HEADERS})

  configure_file(${TOPSHELF_DIR}/TopShelf.entitlements.in ${ENTITLEMENTS_OUT_PATH} @ONLY)
  set_target_properties(${TOPSHELF_EXTENSION_NAME} PROPERTIES BUNDLE_EXTENSION ${TOPSHELF_BUNDLE_EXTENSION}
                                                              MACOSX_BUNDLE_INFO_PLIST ${TOPSHELF_DIR}/Info.plist.in
                                                              XCODE_PRODUCT_TYPE com.apple.product-type.tv-app-extension
                                                              XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS ${ENTITLEMENTS_OUT_PATH})
  target_link_libraries(${TOPSHELF_EXTENSION_NAME} "-framework TVServices" "-framework Foundation")

  add_dependencies(${APP_NAME_LC} ${TOPSHELF_EXTENSION_NAME})
endif()
