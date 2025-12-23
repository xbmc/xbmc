# FindFlatBuffers
# --------
# Find the FlatBuffers schema headers
#
# This will define the following target:
#
#   ${APP_NAME_LC}::FlatBuffers - The flatbuffers headers

find_package(FlatC REQUIRED ${SEARCH_QUIET})

if(NOT TARGET ${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME})

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildmacroFlatBuffers)
    # Override build type detection and always build as release
    set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_TYPE Release)

    set(CMAKE_ARGS -DFLATBUFFERS_CODE_COVERAGE=OFF
                   -DFLATBUFFERS_BUILD_TESTS=OFF
                   -DFLATBUFFERS_INSTALL=ON
                   -DFLATBUFFERS_BUILD_FLATLIB=OFF
                   -DFLATBUFFERS_BUILD_FLATC=OFF
                   -DFLATBUFFERS_BUILD_FLATHASH=OFF
                   -DFLATBUFFERS_BUILD_GRPCTEST=OFF
                   -DFLATBUFFERS_BUILD_SHAREDLIB=OFF
                   "${EXTRA_ARGS}")
    set(BUILD_BYPRODUCTS ${DEPENDS_PATH}/include/flatbuffers/flatbuffers.h)

    BUILD_DEP_TARGET()
  endmacro()

  set(${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC flatbuffers)
  set(${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME FlatBuffers)
  SETUP_BUILD_VARS()

  set(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INTERFACE_LIB TRUE)

  find_package(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME} CONFIG ${SEARCH_QUIET}
                                                         HINTS ${DEPENDS_PATH}/lib/cmake
                                                         NAMES FlatBuffers flatbuffers
                                                         ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

  # Check for existing Flatbuffers. If version >= FLATBUFFERS-VERSION file version, dont build
  # A corner case, but if a linux/freebsd user WANTS to build internal flatbuffers, build anyway
  if(("${${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_VERSION}" VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_FLATBUFFERS) OR
     (((CORE_SYSTEM_NAME STREQUAL linux AND NOT "webos" IN_LIST CORE_PLATFORM_NAME_LC) OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_FLATBUFFERS))
    message(STATUS "Building ${${CMAKE_FIND_PACKAGE_NAME}_MODULE_LC}: \(version \"${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER}\"\)")
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    # Flatbuffers cmake targets are odd. We dont use static/shared builds, and only use header only
    # however they do not provide header only targets. Manually search for header to create our own
    # target.
    find_path(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR NAMES flatbuffers/flatbuffers.h
                                                               HINTS ${DEPENDS_PATH}/include
                                                               ${${CORE_PLATFORM_LC}_SEARCH_CONFIG})

    if(${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_INCLUDE_DIR)
      set(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND 1)
    endif()
  endif()

  if(${${CMAKE_FIND_PACKAGE_NAME}_SEARCH_NAME}_FOUND)
    SETUP_BUILD_TARGET()

    if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} flatbuffers::flatc)

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(FlatBuffers_FIND_REQUIRED)
      message(FATAL_ERROR "Flatbuffer schema headers were not found. You may want to try -DENABLE_INTERNAL_FLATBUFFERS=ON to build the internal headers package")
    endif()
  endif()
endif()
