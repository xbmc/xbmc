#.rst:
# FindBluray
# ----------
# Finds the libbluray library
#
# This will define the following target:
#
#   ${APP_NAME_LC}::Bluray   - The libbluray library

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroBluray)

    find_package(Meson REQUIRED)
    find_package(Ninja REQUIRED)

    # Required for building libbluray bd jars, however is optional for libbluray
    find_package(ANT)

    find_package(Udfread 1.2.0 REQUIRED ${SEARCH_QUIET})
    find_package(FreeType REQUIRED ${SEARCH_QUIET})
    find_package(LibXml2 REQUIRED ${SEARCH_QUIET})

    if(NOT (WIN32 OR WINDOWS_STORE))
      find_package(Fontconfig REQUIRED ${SEARCH_QUIET})

      list(APPEND additional_env_mod --modify FONTCONFIG_ROOT=set:${DEPENDS_PATH})
    endif()

    if(TARGET ANT::ANT)
      find_package(Java COMPONENTS Development)

      if(Java_Development_FOUND)
        get_filename_component(java_binpath ${Java_JAVAC_EXECUTABLE} DIRECTORY)
        get_filename_component(java_homepath ${java_binpath} DIRECTORY)

        get_target_property(ANT_PATH ANT::ANT ANT_PATH)
        get_target_property(ANT_HOME ANT::ANT ANT_HOME)

        # Todo: --modify requires cmake 3.26. Do we care about supporting older cmake?
        if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.26)
          list(APPEND additional_env_mod --modify PATH=path_list_prepend:${NATIVEPREFIX}/bin
                                         --modify PATH=path_list_prepend:${java_binpath}
                                         --modify PATH=path_list_prepend:${ANT_PATH}
                                         --modify ANT_HOME=set:${ANT_HOME}
                                         --modify JAVA_HOME=set:${java_homepath})

          set(build_env_mod ${CMAKE_COMMAND} -E env ${additional_env_mod})
        endif()
      endif()
    endif()

    if(APPLE)
      set(patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/002-darwin-dlopen_searchpath.patch")

      if(CORE_SYSTEM_NAME STREQUAL darwin_embedded)
        list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/001-darwinembed_DiskArbitration-revert.patch")

        if(${CORE_PLATFORM_NAME} STREQUAL "tvos")
          list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/tvos.patch")
        endif()
      endif()
    endif()

    list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/003-all-libxml_searchname.patch"
                        "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/004-win-msvc_fix-MR54.patch")

    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType shared)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB TRUE)

      list(APPEND patches "${CORE_SOURCE_DIR}/tools/depends/target/${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}/005-win-remove-lib-version.patch")

      if(WINDOWS_STORE)
         set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_EXE_LINKER_FLAGS "/APPCONTAINER windowsapp.lib")
       endif()

      create_module_dev_env()
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType static)
    endif()

    generate_patchcommand("${patches}")
    unset(patches)

    if(${CMAKE_VERSION} VERSION_GREATER_EQUAL 3.26)
      set(configure_env_mod ${CMAKE_COMMAND} -E env --modify FREETYPE_DIR=set:${DEPENDS_PATH}
                                                    --modify LIBXML2_ROOT=set:${DEPENDS_PATH}
                                                    --modify LIBUDFREAD_ROOT=set:${DEPENDS_PATH}
                                                    --modify NINJA=set:${NINJA_EXECUTABLE}
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
                          -Ddefault_library=${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType}
                          -Denable_tools=false
                          -Dembed_udfread=false
                          ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_CROSS_FILE})

    set(BUILD_COMMAND ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_dev_env}
                      ${build_env_mod}
                      ${NINJA_EXECUTABLE} -C ./build)
    set(INSTALL_COMMAND ${NINJA_EXECUTABLE} -C ./build install)
    set(BUILD_IN_SOURCE 1)

    BUILD_DEP_TARGET()

    # Todo: other dependencies
    # fontconfig
    add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} LIBRARY::Udfread
                                                                        Freetype::Freetype
                                                                        LibXml2::LibXml2)

    # Link libraries for target interface
    list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES LIBRARY::Udfread
                                                                    Freetype::Freetype
                                                                    LibXml2::LibXml2)

    if(NOT (WIN32 OR WINDOWS_STORE))
      add_dependencies(${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME} Fontconfig::Fontconfig)
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_LINK_LIBRARIES Fontconfig::Fontconfig)

      # We currently use system FindFontconfig, and it doesnt recognise the need for
      # Freetype link. Adding this corrects the link ordering
      target_link_libraries(Fontconfig::Fontconfig INTERFACE Freetype::Freetype)
    endif()

  endmacro()

  # If there is a potential this library can be built internally
  # Check its dependencies to allow forcing this lib to be built if one of its
  # dependencies requires being rebuilt
  if(ENABLE_INTERNAL_BLURAY)
    # Todo: other dependencies
    # fontconfig freetype2 libxml2 Iconv

    # Dependency list of this find module for an INTERNAL build
    set(${CMAKE_FIND_PACKAGE_NAME}_DEPLIST Udfread)

    check_dependency_build(${CMAKE_FIND_PACKAGE_NAME} "${${CMAKE_FIND_PACKAGE_NAME}_DEPLIST}")
  endif()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC libbluray)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_BLURAY) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_BLURAY) OR
     (DEFINED ${CMAKE_FIND_PACKAGE_NAME}_FORCE_BUILD))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")

    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    if(TARGET libbluray::libbluray AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS libbluray::libbluray)
    elseif(TARGET PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ALIAS PkgConfig::${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME})
    else()
      SETUP_BUILD_TARGET()
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAVE_LIBBLURAY)

    # This is incorrectly applied to all platforms. Requires its own handling in the future
    if(NOT CORE_PLATFORM_NAME_LC STREQUAL windowsstore)
      list(APPEND ${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_COMPILE_DEFINITIONS HAVE_LIBBLURAY_BDJ)
    endif()

    ADD_TARGET_COMPILE_DEFINITION()

    ADD_MULTICONFIG_BUILDMACRO()
  endif()
endif()
