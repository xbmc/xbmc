#.rst:
# FindSWIG
# --------
# Finds the SWIG executable. With ENABLE_INTERNAL_SWIG, the version pinned in
# tools/depends/native/swig is built instead when the one found does not match it.
#
# This will define the following TARGET:
#
# SWIG::SWIG - the SWIG executable

if(NOT TARGET SWIG::SWIG)
  include(${CMAKE_SOURCE_DIR}/cmake/scripts/common/ModuleHelpers.cmake)

  if(NATIVEPREFIX)
    set(_swig_hints HINTS ${NATIVEPREFIX}/bin)
  endif()

  find_program(SWIG_EXECUTABLE NAMES swig swig4.0 swig3.0 swig2.0
                               NO_CACHE
                               NAMES_PER_DIR
                               PATH_SUFFIXES swig
                               ${_swig_hints})
  unset(_swig_hints)

  if(SWIG_EXECUTABLE)
    execute_process(COMMAND ${SWIG_EXECUTABLE} -version
                    OUTPUT_VARIABLE SWIG_version_output
                    ERROR_VARIABLE SWIG_version_output)
    string(REGEX REPLACE ".*SWIG Version[^0-9.]*\([0-9.]+\).*" "\\1"
           SWIG_VERSION "${SWIG_version_output}")
    unset(SWIG_version_output)
  endif()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC swig)
  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}_LIB_TYPE native)

  SETUP_BUILD_VARS()

  if(ENABLE_INTERNAL_SWIG AND NOT "${SWIG_VERSION}" VERSION_EQUAL "${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}")
    # Host tool, so always a release build
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_TYPE Release)

    if(NATIVEPREFIX)
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INSTALL_PREFIX ${NATIVEPREFIX})
    else()
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR})
    endif()

    # A non-empty CMAKE_ARGS is what makes BUILD_DEP_TARGET drive this as a CMake project
    set(CMAKE_ARGS -DWITH_PCRE=ON)

    if(EXISTS "${NATIVEPREFIX}/share/Toolchain-Native.cmake")
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_TOOLCHAIN_FILE "${NATIVEPREFIX}/share/Toolchain-Native.cmake")
    endif()

    if(WIN32 OR WINDOWS_STORE)
      # Generate for the host arch, not the target
      set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_GENERATOR_PLATFORM CMAKE_GENERATOR_PLATFORM ${HOSTTOOLSET})
    endif()

    set(SWIG_EXECUTABLE ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INSTALL_PREFIX}/bin/swig${CMAKE_HOST_EXECUTABLE_SUFFIX})
    set(SWIG_VERSION ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER})
    set(BUILD_BYPRODUCTS ${SWIG_EXECUTABLE})

    BUILD_DEP_TARGET()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(SWIG
                                    REQUIRED_VARS SWIG_EXECUTABLE
                                    VERSION_VAR SWIG_VERSION)

  if(SWIG_FOUND)
    add_executable(SWIG::SWIG IMPORTED GLOBAL)
    set_target_properties(SWIG::SWIG PROPERTIES
                                     IMPORTED_LOCATION "${SWIG_EXECUTABLE}"
                                     VERSION "${SWIG_VERSION}")

    if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_dependencies(SWIG::SWIG ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()
  endif()
endif()
