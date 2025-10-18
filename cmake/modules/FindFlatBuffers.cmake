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

  SETUP_BUILD_VARS()

  find_package(flatbuffers CONFIG ${SEARCH_QUIET}
                           HINTS ${DEPENDS_PATH}/lib/cmake
                           ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG})

  # Check for existing Flatbuffers. If version >= FLATBUFFERS-VERSION file version, dont build
  # A corner case, but if a linux/freebsd user WANTS to build internal flatbuffers, build anyway
  if((flatbuffers_VERSION VERSION_LESS ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_VER} AND ENABLE_INTERNAL_FLATBUFFERS) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_FLATBUFFERS))
    cmake_language(EVAL CODE "
      buildmacro${CMAKE_FIND_PACKAGE_NAME}()
    ")
  else()
    find_path(FLATBUFFERS_INCLUDE_DIR NAMES flatbuffers/flatbuffers.h
                                      HINTS ${DEPENDS_PATH}/include
                                      ${${CORE_SYSTEM_NAME}_SEARCH_CONFIG}
                                      NO_CACHE)
  endif()

  if(NOT VERBOSE_FIND)
     set(${CMAKE_FIND_PACKAGE_NAME}_FIND_QUIETLY TRUE)
   endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(FlatBuffers
                                    REQUIRED_VARS FLATBUFFERS_INCLUDE_DIR
                                    VERSION_VAR FLATBUFFERS_VER)

  if(FlatBuffers_FOUND)

    add_library(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} INTERFACE IMPORTED)
    set_target_properties(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} PROPERTIES
                                                                     INTERFACE_INCLUDE_DIRECTORIES "${FLATBUFFERS_INCLUDE_DIR}")

    add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} flatbuffers::flatc)

    if(TARGET ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
      add_dependencies(${APP_NAME_LC}::${CMAKE_FIND_PACKAGE_NAME} ${${${CMAKE_FIND_PACKAGE_NAME}_MODULE}_BUILD_NAME})
    endif()

    ADD_MULTICONFIG_BUILDMACRO()
  else()
    if(FlatBuffers_FIND_REQUIRED)
      message(FATAL_ERROR "Flatbuffer schema headers were not found. You may want to try -DENABLE_INTERNAL_FLATBUFFERS=ON to build the internal headers package")
    endif()
  endif()
endif()
