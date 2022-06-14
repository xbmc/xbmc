# FindFlatBuffers
# --------
# Find the FlatBuffers schema compiler and headers
#
# This will define the following variables:
#
# FLATBUFFERS_FOUND - system has FlatBuffers compiler and headers
# FLATBUFFERS_FLATC_EXECUTABLE - the flatc compiler executable
# FLATBUFFERS_INCLUDE_DIRS - the FlatFuffers include directory
# FLATBUFFERS_MESSAGES_INCLUDE_DIR - the directory for generated headers

if(ENABLE_INTERNAL_FLATBUFFERS)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC flatbuffers)

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
  find_path(FLATBUFFERS_INCLUDE_DIR NAMES flatbuffers/flatbuffers.h)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FlatBuffers
                                  REQUIRED_VARS FLATBUFFERS_INCLUDE_DIR
                                  VERSION_VAR FLATBUFFERS_VER)

if(FLATBUFFERS_FOUND)
  set(FLATBUFFERS_MESSAGES_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cores/RetroPlayer/messages CACHE INTERNAL "Generated FlatBuffer headers")
  set(FLATBUFFERS_INCLUDE_DIRS ${FLATBUFFERS_INCLUDE_DIR} ${FLATBUFFERS_MESSAGES_INCLUDE_DIR})

  if(NOT TARGET flatbuffers::flatbuffers)
    add_library(flatbuffers::flatbuffers UNKNOWN IMPORTED)
    set_target_properties(flatbuffers::flatbuffers PROPERTIES
                               FOLDER "External Projects"
                               INTERFACE_INCLUDE_DIRECTORIES ${FLATBUFFERS_INCLUDE_DIR})
  endif()

  add_dependencies(flatbuffers::flatbuffers flatbuffers::flatc)

  if(TARGET flatbuffers)
    add_dependencies(flatbuffers::flatbuffers flatbuffers)
  endif()
  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP flatbuffers::flatbuffers)
endif()

mark_as_advanced(FLATBUFFERS_INCLUDE_DIR)
