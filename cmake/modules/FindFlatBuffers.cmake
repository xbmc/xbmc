# FindFlatBuffers
# --------
# Find the FlatBuffers schema headers
#
# This will define the following target:
#
#   flatbuffers::flatbuffers - The flatbuffers headers

find_package(FlatC REQUIRED)

if(NOT TARGET flatbuffers::flatbuffers)
  if(ENABLE_INTERNAL_FLATBUFFERS)
    include(cmake/scripts/common/ModuleHelpers.cmake)

    set(MODULE_LC flatbuffers)
    # Duplicate URL may exist from FindFlatC.cmake
    # unset otherwise it thinks we are providing a local file location and incorrect concatenation happens
    unset(FLATBUFFERS_URL)
    SETUP_BUILD_VARS()

    # Override build type detection and always build as release
    set(FLATBUFFERS_BUILD_TYPE Release)

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
  else()
    find_path(FLATBUFFERS_INCLUDE_DIR NAMES flatbuffers/flatbuffers.h
                                      HINTS ${DEPENDS_PATH}/include
                                      ${${CORE_PLATFORM_LC}_SEARCH_CONFIG}
                                      NO_CACHE)
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(FlatBuffers
                                    REQUIRED_VARS FLATBUFFERS_INCLUDE_DIR
                                    VERSION_VAR FLATBUFFERS_VER)

  add_library(flatbuffers::flatbuffers INTERFACE IMPORTED)
  set_target_properties(flatbuffers::flatbuffers PROPERTIES
                                                 INTERFACE_INCLUDE_DIRECTORIES "${FLATBUFFERS_INCLUDE_DIR}")

  add_dependencies(flatbuffers::flatbuffers flatbuffers::flatc)

  if(TARGET flatbuffers)
    add_dependencies(flatbuffers::flatbuffers flatbuffers)
  endif()
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP flatbuffers::flatbuffers)
endif()
