#.rst:
# FindDav1d
# --------
# Finds the dav1d library
#
# This will define the following target:
#
#   LIBRARY::Dav1d   - The dav1d library

if(NOT TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroDav1d)

    set(enable_asm false)

    if(CPU STREQUAL "x86_64" OR CPU MATCHES "i.86")
      find_package(NASM)
      if(NASM_EXECUTABLE)
        set(enable_asm true)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_BINARIES "nasm" "NASM_EXECUTABLE")
      endif()
    elseif(SDK_TARGET_ARCH STREQUAL "x86" OR SDK_TARGET_ARCH MATCHES "x64")
      # Windows processor checks
      find_package(NASM)
      if(NASM_EXECUTABLE)
        set(enable_asm true)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_BINARIES "nasm" "NASM_EXECUTABLE")
      endif()
    else()
      find_package(GASPP)
      if(TARGET GASPP::GASPP)
        set(enable_asm true)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_BINARIES "gas-preprocessor.pl" "GASPP_PL")
      endif()
    endif()

    find_package(Meson REQUIRED)
    find_package(Ninja REQUIRED)

    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType shared)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB 1)

      set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-win-sharedname.patch")
      generate_patchcommand("${patches}")
      unset(patches)

      if(WINDOWS_STORE)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_EXE_LINKER_FLAGS "/APPCONTAINER windowsapp.lib")
      endif()

      create_module_dev_env()
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType static)
    endif()

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
                          --buildtype=release
                          --default-library=${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType}
                          --prefix=${DEPENDS_PATH}
                          --libdir=lib
                          -Denable_asm=${enable_asm}
                          -Denable_tools=false
                          -Denable_examples=false
                          -Denable_tests=false
                          ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_CROSS_FILE})

    set(BUILD_COMMAND ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_dev_env}
                      ${NINJA_EXECUTABLE} -C ./build)
    set(INSTALL_COMMAND ${NINJA_EXECUTABLE} -C ./build install)
    set(BUILD_IN_SOURCE 1)

    BUILD_DEP_TARGET()

  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC dav1d)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_DAV1D) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_DAV1D))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")

    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")

    # Meson uses pkgconfig, therefore we no longer need the windows cmake config.
    # remove the cmake config, as otherwise our search order will constantly find the old
    # cmake config lib rather than the new pkgconfig
    if(TARGET dav1d::dav1d)
      if(EXISTS "${DEPENDS_PATH}/lib/cmake/dav1d")
        file(REMOVE_RECURSE "${DEPENDS_PATH}/lib/cmake/dav1d")
      endif()
    endif()
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    # dav1d::dav1d target is a legacy target from windows kodi-deps build of dav1d
    if(TARGET dav1d::dav1d AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS dav1d::dav1d)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE LIBRARY)

      SETUP_BUILD_TARGET()

      add_dependencies(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      # Property must be set on actual TARGET and not the ALIAS
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  endif()
endif()
