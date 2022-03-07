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

  set(FLATBUFFERS_FLATC_EXECUTABLE ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/bin/flatc CACHE INTERNAL "FlatBuffer compiler")
  set(FLATBUFFERS_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include CACHE INTERNAL "FlatBuffer include dir")

  set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                 -DCMAKE_BUILD_TYPE=Release
                 -DFLATBUFFERS_CODE_COVERAGE=OFF
                 -DFLATBUFFERS_BUILD_TESTS=OFF
                 -DFLATBUFFERS_INSTALL=ON
                 -DFLATBUFFERS_BUILD_FLATLIB=OFF
                 -DFLATBUFFERS_BUILD_FLATC=ON
                 -DFLATBUFFERS_BUILD_FLATHASH=OFF
                 -DFLATBUFFERS_BUILD_GRPCTEST=OFF
                 -DFLATBUFFERS_BUILD_SHAREDLIB=OFF
                 "${EXTRA_ARGS}")
  set(BUILD_BYPRODUCTS ${FLATBUFFERS_FLATC_EXECUTABLE})

  BUILD_DEP_TARGET()

  set_target_properties(${MODULE_LC} PROPERTIES INTERFACE_INCLUDE_DIRECTORIES ${FLATBUFFERS_INCLUDE_DIR})
else()
  find_program(FLATBUFFERS_FLATC_EXECUTABLE NAMES flatc)
  find_path(FLATBUFFERS_INCLUDE_DIR NAMES flatbuffers/flatbuffers.h)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FlatBuffers
                                  REQUIRED_VARS FLATBUFFERS_FLATC_EXECUTABLE FLATBUFFERS_INCLUDE_DIR
                                  VERSION_VAR FLATBUFFERS_VER)

if(FLATBUFFERS_FOUND)
  set(FLATBUFFERS_MESSAGES_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/cores/RetroPlayer/messages CACHE INTERNAL "Generated FlatBuffer headers")
  set(FLATBUFFERS_INCLUDE_DIRS ${FLATBUFFERS_INCLUDE_DIR} ${FLATBUFFERS_MESSAGES_INCLUDE_DIR})

  if(NOT TARGET flatbuffers)
    add_library(flatbuffers UNKNOWN IMPORTED)
    set_target_properties(flatbuffers PROPERTIES
                               FOLDER "External Projects"
                               INTERFACE_INCLUDE_DIRECTORIES ${FLATBUFFERS_INCLUDE_DIR})
  endif()

  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP flatbuffers)
endif()

mark_as_advanced(FLATBUFFERS_FLATC_EXECUTABLE FLATBUFFERS_INCLUDE_DIR)
