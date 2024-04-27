# FindFlatBuffers
# --------
# Find the FlatBuffers schema headers
#
# This will define the following target:
#
#   flatbuffers::flatheaders - The flatbuffers headers

find_package(FlatC REQUIRED)

if(NOT TARGET flatbuffers::flatheaders)

  include(cmake/scripts/common/ModuleHelpers.cmake)

  macro(buildflatbuffers)
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
  endmacro()

  set(MODULE_LC flatbuffers)

  SETUP_BUILD_VARS()

  find_package(flatbuffers CONFIG
                           HINTS ${DEPENDS_PATH}/lib/cmake
                           ${${CORE_PLATFORM_NAME_LC}_SEARCH_CONFIG})

  # Check for existing Flatbuffers. If version >= FLATBUFFERS-VERSION file version, dont build
  # A corner case, but if a linux/freebsd user WANTS to build internal flatbuffers, build anyway
  if((flatbuffers_VERSION VERSION_LESS ${${MODULE}_VER} AND ENABLE_INTERNAL_FLATBUFFERS) OR
     ((CORE_SYSTEM_NAME STREQUAL linux OR CORE_SYSTEM_NAME STREQUAL freebsd) AND ENABLE_INTERNAL_FLATBUFFERS))

    buildflatbuffers()
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

  if(FlatBuffers_FOUND)

    add_library(flatbuffers::flatheaders INTERFACE IMPORTED)
    set_target_properties(flatbuffers::flatheaders PROPERTIES
                                                   INTERFACE_INCLUDE_DIRECTORIES "${FLATBUFFERS_INCLUDE_DIR}")

    add_dependencies(flatbuffers::flatheaders flatbuffers::flatc)

    if(TARGET flatbuffers)
      add_dependencies(flatbuffers::flatheaders flatbuffers)
    endif()

    # Add internal build target when a Multi Config Generator is used
    # We cant add a dependency based off a generator expression for targeted build types,
    # https://gitlab.kitware.com/cmake/cmake/-/issues/19467
    # therefore if the find heuristics only find the library, we add the internal build
    # target to the project to allow user to manually trigger for any build type they need
    # in case only a specific build type is actually available (eg Release found, Debug Required)
    # This is mainly targeted for windows who required different runtime libs for different
    # types, and they arent compatible
    if(_multiconfig_generator)
      if(NOT TARGET flatbuffers)
        buildflatbuffers()
        set_target_properties(flatbuffers PROPERTIES EXCLUDE_FROM_ALL TRUE)
      endif()
      add_dependencies(build_internal_depends flatbuffers)
    endif()

    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP flatbuffers::flatheaders)
  else()
    if(FlatBuffers_FIND_REQUIRED)
      message(FATAL_ERROR "Flatbuffer schema headers were not found. You may want to try -DENABLE_INTERNAL_FLATBUFFERS=ON to build the internal headers package")
    endif()
  endif()
endif()
