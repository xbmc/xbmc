# FindFlatC
# --------
# Find the FlatBuffers schema compiler
#
# This will define the following variables:
#
# FLATBUFFERS_FOUND - system has FlatBuffers compiler and headers
# FLATBUFFERS_FLATC_EXECUTABLE - the flatc compiler executable
#
# and the following imported targets:
#
#   flatbuffers::flatc - The FlatC compiler

include(cmake/scripts/common/ModuleHelpers.cmake)

set(MODULE_LC flatbuffers)

SETUP_BUILD_VARS()

# Check for existing FLATC.
find_program(FLATBUFFERS_FLATC_EXECUTABLE NAMES flatc
                                          HINTS ${NATIVEPREFIX}/bin)

if(NOT FLATBUFFERS_FLATC_EXECUTABLE)

  # Override build type detection and always build as release
  set(FLATBUFFERS_BUILD_TYPE Release)

  if(NATIVEPREFIX)
    set(INSTALL_DIR "${NATIVEPREFIX}/bin")
    set(FLATBUFFERS_INSTALL_PREFIX ${NATIVEPREFIX})
  else()
    set(INSTALL_DIR "${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/bin")
    set(FLATBUFFERS_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR})
  endif()

  set(CMAKE_ARGS -DFLATBUFFERS_CODE_COVERAGE=OFF
                 -DFLATBUFFERS_BUILD_TESTS=OFF
                 -DFLATBUFFERS_INSTALL=ON
                 -DFLATBUFFERS_BUILD_FLATLIB=OFF
                 -DFLATBUFFERS_BUILD_FLATC=ON
                 -DFLATBUFFERS_BUILD_FLATHASH=OFF
                 -DFLATBUFFERS_BUILD_GRPCTEST=OFF
                 -DFLATBUFFERS_BUILD_SHAREDLIB=OFF)

  # Set host build info for buildtool
  if(EXISTS "${NATIVEPREFIX}/share/Toolchain-Native.cmake")
    set(FLATBUFFERS_TOOLCHAIN_FILE "${NATIVEPREFIX}/share/Toolchain-Native.cmake")
  endif()

  if(WIN32 OR WINDOWS_STORE)
    # Make sure we generate for host arch, not target
    set(FLATBUFFERS_GENERATOR_PLATFORM CMAKE_GENERATOR_PLATFORM ${HOSTTOOLSET})
  endif()

  set(FLATBUFFERS_FLATC_EXECUTABLE ${INSTALL_DIR}/flatc CACHE INTERNAL "FlatBuffer compiler")

  set(BUILD_NAME flatc)
  set(BUILD_BYPRODUCTS ${FLATBUFFERS_FLATC_EXECUTABLE})

  BUILD_DEP_TARGET()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FlatC
                                  REQUIRED_VARS FLATBUFFERS_FLATC_EXECUTABLE
                                  VERSION_VAR FLATBUFFERS_VER)

if(FLATC_FOUND)

  if(NOT TARGET flatbuffers::flatc)
    add_library(flatbuffers::flatc UNKNOWN IMPORTED)
    set_target_properties(flatbuffers::flatc PROPERTIES
                                             FOLDER "External Projects")
  endif()

  if(TARGET flatc)
    add_dependencies(flatbuffers::flatc flatc)
  endif()
else()
  if(FLATC_FIND_REQUIRED)
    message(FATAL_ERROR "Flatc compiler not found.")
  endif()
endif()

mark_as_advanced(FLATBUFFERS_FLATC_EXECUTABLE)
