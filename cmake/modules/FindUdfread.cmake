#.rst:
# FindUdfread
# --------
# Finds the udfread library
#
# This will define the following targets:
#
#   ${APP_NAME_LC}::Udfread - The libudfread library
#   LIBRARY::Udfread - ALIAS target for the libudfread library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroUdfread)

    find_package(Meson REQUIRED)
    find_package(Ninja REQUIRED)

    if(WIN32 OR WINDOWS_STORE)
      create_module_dev_env()
    endif()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType static)

    set(patches "${CMAKE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/001-win-MR10-merged.patch")
    generate_patchcommand("${patches}")
    unset(patches)

    # generate meson cross file for build target
    generate_mesoncrossfile()

    if(EXISTS ${DEPENDS_PATH}/share/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}-cross-file.meson)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_CROSS_FILE --cross-file=${DEPENDS_PATH}/share/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}-cross-file.meson)
    elseif(EXISTS ${DEPENDS_PATH}/share/cross-file.meson)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_CROSS_FILE --cross-file=${DEPENDS_PATH}/share/cross-file.meson)
    endif()

    set(CONFIGURE_COMMAND ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_dev_env}
                          ${CMAKE_COMMAND} -E env --modify NINJA=set:${NINJA_EXECUTABLE}
                          ${MESON_EXECUTABLE} setup ./build
                          --cmake-prefix-path=['${DEPENDS_PATH}/lib/cmake']
                          --prefix=${DEPENDS_PATH}
                          --libdir=lib
                          --buildtype=release
                          -Ddefault_library=${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType}
                          ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_CROSS_FILE})

    set(BUILD_COMMAND ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_dev_env}
                      ${NINJA_EXECUTABLE} -C ./build)
    set(INSTALL_COMMAND ${NINJA_EXECUTABLE} -C ./build install)
    set(BUILD_IN_SOURCE 1)

    BUILD_DEP_TARGET()

  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC udfread)
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME libudfread)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_UDFREAD) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_UDFREAD))

    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")

    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAS_UDFREAD)

    # windows cmake config populated target
    if(TARGET libudfread::libudfread)
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libudfread::libudfread)
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS libudfread::libudfread)
    # pkgconfig populated target that is sufficient version
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      # Property must be set on actual TARGET and not the ALIAS
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)

      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
    endif()

    ADD_TARGET_COMPILE_DEFINITION()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(Udfread_FIND_REQUIRED)
      message(FATAL_ERROR "Udfread libraries were not found.")
    endif()
  endif()
endif()
