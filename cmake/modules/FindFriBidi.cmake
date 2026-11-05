#.rst:
# FindFriBidi
# -----------
# Finds the GNU FriBidi library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::FriBidi   - The FriBidi library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})
  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroFriBidi)
    find_package(Meson REQUIRED)
    find_package(Ninja REQUIRED)

    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType shared)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB TRUE)
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType static)
    endif()

    # FriBidi builds a custom tool to generate dynamic headers from Unicode tables and meson has trouble to build
    # this tool when host compiler is different of build machine compiler (UWP or arm64 cases). The patch provides
    # the dynamic headers pre-generated to current Unicode version instead of generate during library build.
    if(WINDOWS_STORE OR (WIN32 AND ${ARCH} STREQUAL "arm64"))
      generate_patchcommand("${CMAKE_SOURCE_DIR}/tools/depends/target/fribidi/01-win-gen-tab.patch")
    endif()

    if(WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_EXE_LINKER_FLAGS "/APPCONTAINER windowsapp.lib")
    endif()

    create_module_dev_env()

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
                          --buildtype=release
                          --default-library=${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType}
                          --prefix=${DEPENDS_PATH}
                          --libdir=lib
                          -Ddocs=false
                          -Dtests=false
                          -Dbin=false
                          ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_CROSS_FILE})

    set(BUILD_COMMAND ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_dev_env}
                      ${NINJA_EXECUTABLE} -C ./build)
    set(INSTALL_COMMAND ${NINJA_EXECUTABLE} -C ./build install)
    set(BUILD_IN_SOURCE 1)

    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC fribidi)
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME_PC fribidi)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_FRIBIDI) OR
    (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_FRIBIDI))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")

    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET fribidi::fribidi AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS fribidi::fribidi)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()

      # Add ALIAS TARGET as FriBidi can be a dependency of other libs
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      # Property must be set on actual TARGET and not the ALIAS
      set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    endif()

    if(TARGET ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME)
      ADD_MULTICONFIG_BUILDMACRO()
    endif()
  endif()
endif()
