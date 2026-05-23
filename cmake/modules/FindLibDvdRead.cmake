#.rst:
# FindLibDvdRead
# ----------
# Finds the dvdread library
#
# This will define the following target:
#
#   LIBRARY::LibDvdRead   - The LibDvdRead library

if(NOT TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroLibDvdRead)

    find_package(Meson REQUIRED)
    find_package(Ninja REQUIRED)

    if(ENABLE_DVDCSS)
      find_package(LibDvdCSS 1.5.0 REQUIRED ${SEARCH_QUIET})
      set(DVDCSS -Dlibdvdcss=enabled)
    endif()

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-all-disableopendir.patch"
                "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/02-all-disablesymlink.patch")

    if(WIN32 OR WINDOWS_STORE)
      list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/03-win-add_defines.patch"
                          "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/04-win-uwp_compat.patch"
                          "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/05-win-remove_stat.patch"
                          "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/06-win-remove_config_h.patch"
                          "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/08-win-ssize_t.patch")

      create_module_dev_env()
    endif()

    if(CMAKE_SYSTEM_NAME STREQUAL "Emscripten")
      list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/07-emscripten-bswap.patch")
    endif()

    generate_patchcommand("${patches}")
    unset(patches)

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType static)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_C_FLAGS "-D_XBMC")

    if("webos" IN_LIST CORE_PLATFORM_NAME_LC)
      # PATH_MAX not defined in limits.h. Just match windows size libdvdcss uses.
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_C_FLAGS "-DPATH_MAX=2048")
    endif()

    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.26)
      set(configure_env_mod ${CMAKE_COMMAND} -E env --modify NINJA=set:${NINJA_EXECUTABLE}
                                                    ${additional_env_mod})
    endif()

    # generate meson cross file for build target
    generate_mesoncrossfile()

    if(EXISTS ${DEPENDS_PATH}/share/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}-cross-file.meson)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_CROSS_FILE --cross-file=${DEPENDS_PATH}/share/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}-cross-file.meson)
    elseif(EXISTS ${DEPENDS_PATH}/share/cross-file.meson)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_CROSS_FILE --cross-file=${DEPENDS_PATH}/share/cross-file.meson)
    endif()

    # We set a bunch of env vars to assist meson finding cmake packages for dependencies
    # If we do not do this, it may find system libs outside of DEPENDS_PATH, or not find anything at all
    set(CONFIGURE_COMMAND ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_dev_env}
                          ${configure_env_mod}
                          ${MESON_EXECUTABLE} setup ./build
                          --cmake-prefix-path=['${DEPENDS_PATH}/lib/cmake']
                          --prefix=${DEPENDS_PATH}
                          --libdir=lib
                          --buildtype=release
                          -Ddefault_library=static
                          -Denable_docs=false
                          ${DVDCSS}
                          ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_EXTRAS}
                          ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_CROSS_FILE})

    set(BUILD_COMMAND ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_dev_env}
                      ${build_env_mod}
                      ${NINJA_EXECUTABLE} -C ./build)
    set(INSTALL_COMMAND ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_dev_env}
                        ${build_env_mod}
                        ${NINJA_EXECUTABLE} -C ./build install)
    set(BUILD_IN_SOURCE 1)

    BUILD_DEP_TARGET()

    if(ENABLE_DVDCSS)
      add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} LIBRARY::LibDvdCSS)

      # Link libraries for target interface
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES LIBRARY::LibDvdCSS)
    endif()
  endmacro()

  # If there is a potential this library can be built internally
  # Check its dependencies to allow forcing this lib to be built if one of its
  # dependencies requires being rebuilt
  if(ENABLE_DVDCSS)
    # Dependency list of this find module for an INTERNAL build
    set(${CMAKE_FIND_PACKAGE_NAME}_DEPLIST LibDvdCSS)

    check_dependency_build(${CMAKE_FIND_PACKAGE_NAME} "${${CMAKE_FIND_PACKAGE_NAME}_DEPLIST}")
  endif()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libdvdread)
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME_PC dvdread)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd)) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")

    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TYPE LIBRARY)

    if(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()
      add_dependencies(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})

      # We are building as a requirement, so set LIB_BUILD property to allow calling
      # modules to know we will be building, and they will want to rebuild as well.
      # Property must be set on actual TARGET and not the ALIAS
      set_target_properties(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES LIB_BUILD ON)
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(LibDvdRead_FIND_REQUIRED)
      message(FATAL_ERROR "Libdvdread not found")
    endif()
  endif()
endif()
