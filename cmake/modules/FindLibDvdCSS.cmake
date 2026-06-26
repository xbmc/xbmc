#.rst:
# FindLibDvdCSS
# ----------
# Finds the libdvdcss library
#
# This will define the following target:
#
#   LIBRARY::LibDvdCSS   - The LibDvdCSS library

if(NOT TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroLibDvdCSS)

    find_package(Meson REQUIRED)
    find_package(Ninja REQUIRED)

    set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/01-all-libc_read_improvement.patch")

    if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
      list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/02-darwinembedded-enablebuild.patch")
    endif()

    if("webos" IN_LIST CORE_PLATFORM_NAME_LC)
      # PATH_MAX not defined in limits.h. Just match windows size libdvdcss uses.
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_C_FLAGS "-DPATH_MAX=2048")
    endif()

    if(WIN32 OR WINDOWS_STORE)
      if(WINDOWS_STORE)
        list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/03-win-uwpfixes.patch")
      endif()

      list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/04-win-open_wrapforce.patch")

      create_module_dev_env()
    endif()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType static)

    generate_patchcommand("${patches}")
    unset(patches)

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
                          --buildtype=$<IF:$<CONFIG:Debug>,debug,release>
                          -Ddefault_library=static
                          -Denable_docs=false
                          -Denable_examples=false
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

  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libdvdcss)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_DVDCSS) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_DVDCSS))
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
    if(LibDvdCSS_FIND_REQUIRED)
      message(FATAL_ERROR "Libdvdcss not found. Possibly remove ENABLE_DVDCSS.")
    endif()
  endif()
endif()
