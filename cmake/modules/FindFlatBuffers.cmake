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
  include(ExternalProject)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  get_archive_name(flatbuffers)

  # Allow user to override the download URL with a local tarball
  # Needed for offline build envs
  if(FLATBUFFERS_URL)
    get_filename_component(FLATBUFFERS_URL "${FLATBUFFERS_URL}" ABSOLUTE)
  else()
    set(FLATBUFFERS_URL http://mirrors.kodi.tv/build-deps/sources/${ARCHIVE})
  endif()
  if(VERBOSE)
    message(STATUS "FLATBUFFERS_URL: ${FLATBUFFERS_URL}")
  endif()

  set(FLATBUFFERS_FLATC_EXECUTABLE ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/bin/flatc CACHE INTERNAL "FlatBuffer compiler")
  set(FLATBUFFERS_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include CACHE INTERNAL "FlatBuffer include dir")

  externalproject_add(flatbuffers
                      URL ${FLATBUFFERS_URL}
                      DOWNLOAD_DIR ${TARBALL_DIR}
                      PREFIX ${CORE_BUILD_DIR}/flatbuffers
                      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                                 -DCMAKE_BUILD_TYPE=Release
                                 -DFLATBUFFERS_CODE_COVERAGE=OFF
                                 -DFLATBUFFERS_BUILD_TESTS=OFF
                                 -DFLATBUFFERS_INSTALL=ON
                                 -DFLATBUFFERS_BUILD_FLATLIB=OFF
                                 -DFLATBUFFERS_BUILD_FLATC=ON
                                 -DFLATBUFFERS_BUILD_FLATHASH=OFF
                                 -DFLATBUFFERS_BUILD_GRPCTEST=OFF
                                 -DFLATBUFFERS_BUILD_SHAREDLIB=OFF
                                 "${EXTRA_ARGS}"
                      BUILD_BYPRODUCTS ${FLATBUFFERS_FLATC_EXECUTABLE})
  set_target_properties(flatbuffers PROPERTIES FOLDER "External Projects"
                                    INTERFACE_INCLUDE_DIRECTORIES ${FLATBUFFERS_INCLUDE_DIR})
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
endif()

mark_as_advanced(FLATBUFFERS_FLATC_EXECUTABLE FLATBUFFERS_INCLUDE_DIR)
