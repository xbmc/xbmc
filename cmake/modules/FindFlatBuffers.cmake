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
  include(${WITH_KODI_DEPENDS}/packages/flatbuffers/package.cmake)
  add_depends_for_targets("HOST")

  add_custom_target(flatbuffers ALL DEPENDS flatbuffers-host)

  set(FLATBUFFERS_FLATC_EXECUTABLE ${INSTALL_PREFIX_HOST}/bin/flatc CACHE INTERNAL "FlatBuffer compiler")
  set(FLATBUFFERS_INCLUDE_DIR ${INSTALL_PREFIX_HOST}/include CACHE INTERNAL "FlatBuffer include dir")

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
