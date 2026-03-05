#.rst:
# FindDavs2
# --------
# Finds the davs2 library
#
# This will define the following target:
#
#   LIBRARY::Davs2   - The davs2 library

if(NOT TARGET LIBRARY::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroDavs2)

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
    endif()

    if(WIN32 OR WINDOWS_STORE)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType shared)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_SHARED_LIB 1)

      if(WINDOWS_STORE)
        set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_EXE_LINKER_FLAGS "/APPCONTAINER windowsapp.lib")
      endif()

      create_module_dev_env()
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_libType static)
    endif()

    set(CONFIGURE_COMMAND cd build/linux && ./configure
                          --prefix=${DEPENDS_PATH}
                          --enable-static
                          --disable-shared
                          --enable-pic
                          --disable-cli)

    if(enable_asm)
      list(APPEND CONFIGURE_COMMAND --enable-asm)
    else()
      list(APPEND CONFIGURE_COMMAND --disable-asm)
    endif()

    set(BUILD_COMMAND $(MAKE) -C build/linux)
    set(INSTALL_COMMAND $(MAKE) -C build/linux install)
    set(BUILD_IN_SOURCE 1)

    BUILD_DEP_TARGET()

  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC davs2)

  SETUP_BUILD_VARS()

  SETUP_FIND_SPECS()

  SEARCH_EXISTING_PACKAGES()

  if((${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_DAVS2) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_DAVS2))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")

    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    # davs2::davs2 target is a legacy target
    if(TARGET davs2::davs2 AND NOT TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_library(LIBRARY::${CMAKE_FIND_PACKAGE_NAME} ALIAS davs2::davs2)
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
