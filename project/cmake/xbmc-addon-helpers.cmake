# Install an add-on in the appropriate way
macro (install_addon target version)
  IF(PACKAGE_ZIP OR PACKAGE_TGZ)
    # Pack files together to create an archive
    INSTALL(DIRECTORY ${target} DESTINATION ./)
    IF(WIN32)
      INSTALL(PROGRAMS ${CMAKE_BINARY_DIR}/${target}.dll DESTINATION ${target})
    ELSE(WIN32)
      INSTALL(TARGETS ${target} DESTINATION ${target})
    ENDIF(WIN32)
    IF(PACKAGE_ZIP)
      SET(CPACK_GENERATOR "ZIP")
    ENDIF(PACKAGE_ZIP)
    IF(PACKAGE_TGZ)
      SET(CPACK_GENERATOR "TGZ")
    ENDIF(PACKAGE_TGZ)
    SET(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF)
    SET(CPACK_PACKAGE_FILE_NAME ${target}-${version})
    IF(CMAKE_BUILD_TYPE STREQUAL "Release")
      SET(CPACK_STRIP_FILES TRUE)
    ENDIF(CMAKE_BUILD_TYPE STREQUAL "Release")
    INCLUDE(CPack)
  ELSE(PACKAGE_ZIP OR PACKAGE_TGZ)
    INSTALL(TARGETS ${target} DESTINATION lib/xbmc/addons/${target})
    INSTALL(DIRECTORY ${target} DESTINATION share/xbmc/addons)
  ENDIF(PACKAGE_ZIP OR PACKAGE_TGZ)
endmacro()

# Grab the version from a given add-on's addon.xml
macro (addon_version dir prefix)
  FILE(READ ${dir}/addon.xml ADDONXML)
  STRING(REGEX MATCH "<addon[^>]*version.?=.?.[0-9\\.]+" VERSION_STRING ${ADDONXML}) 
  STRING(REGEX REPLACE ".*version=.([0-9\\.]+).*" "\\1" ${prefix}_VERSION ${VERSION_STRING})
  message(STATUS ${prefix}_VERSION=${${prefix}_VERSION})
endmacro()

# Prepare the add-on build environment
macro (prepare_addon_environment)
  IF(WIN32)
    SET(BINDING_FILE ${XBMC_BINDINGS}.zip)
    message (STATUS "downloading XBMC bindings: " ${BINDING_FILE})
    file(DOWNLOAD http://mirrors.xbmc.org/build-deps/win32/${BINDING_FILE} ${CMAKE_BINARY_DIR}/downloads/${BINDING_FILE} STATUS STATUSLIST SHOW_PROGRESS)
    LIST(GET STATUSLIST 0 VALUE)
    IF(${VALUE} STRGREATER "0")
      LIST(GET STATUSLIST 1 VALUE)
      message (STATUS "failed to download XBMC bindings: " ${VALUE})
    ENDIF(${VALUE} STRGREATER "0")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar xzf ${CMAKE_BINARY_DIR}/downloads/${BINDING_FILE}
    )
  ENDIF(WIN32)
endmacro()

# Build and link an add-on
macro (build_addon target sources libs version)
  ADD_LIBRARY(${target} ${${sources}})
  TARGET_LINK_LIBRARIES(${target} ${${libs}})
  SET_TARGET_PROPERTIES(${target} PROPERTIES VERSION ${${version}}
                                             SOVERSION 13.0
                                             PREFIX "")
  IF(OS STREQUAL "android")
    SET_TARGET_PROPERTIES(${target} PROPERTIES PREFIX "lib")
  ENDIF(OS STREQUAL "android")
endmacro()
