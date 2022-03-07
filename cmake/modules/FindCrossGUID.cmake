if(ENABLE_INTERNAL_CROSSGUID)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  set(MODULE_LC crossguid)

  SETUP_BUILD_VARS()

  if(APPLE)
    set(EXTRA_ARGS "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
  endif()

  set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                 -DCMAKE_MODULE_PATH=${CMAKE_MODULE_PATH}
                 "${EXTRA_ARGS}")
  set(PATCH_COMMAND ${CMAKE_COMMAND} -E copy
                    ${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/CMakeLists.txt
                    <SOURCE_DIR>)

  BUILD_DEP_TARGET()
else()
  find_path(CROSSGUID_INCLUDE_DIR NAMES guid.hpp guid.h)

  find_library(CROSSGUID_LIBRARY_RELEASE NAMES crossguid)
  find_library(CROSSGUID_LIBRARY_DEBUG NAMES crossguidd)

  include(SelectLibraryConfigurations)
  select_library_configurations(CROSSGUID)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(CrossGUID
                                  REQUIRED_VARS CROSSGUID_LIBRARY CROSSGUID_INCLUDE_DIR
                                  VERSION_VAR CROSSGUID_VER)

if(CROSSGUID_FOUND)
  set(CROSSGUID_LIBRARIES ${CROSSGUID_LIBRARY})
  set(CROSSGUID_INCLUDE_DIRS ${CROSSGUID_INCLUDE_DIR})

  if(EXISTS "${CROSSGUID_INCLUDE_DIR}/guid.hpp")
    set(CROSSGUID_DEFINITIONS -DHAVE_NEW_CROSSGUID)
  endif()

  if(NOT TARGET crossguid)
    add_library(crossguid UNKNOWN IMPORTED)
    set_target_properties(crossguid PROPERTIES
                                    IMPORTED_LOCATION "${CROSSGUID_LIBRARY}"
                                    INTERFACE_INCLUDE_DIRECTORIES "${CROSSGUID_INCLUDE_DIR}")
  endif()

  if(NOT WIN32 AND NOT APPLE)
    find_package(UUID REQUIRED)
    list(APPEND CROSSGUID_INCLUDE_DIRS ${UUID_INCLUDE_DIRS})
    list(APPEND CROSSGUID_LIBRARIES ${UUID_LIBRARIES})
  endif()

  set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP crossguid)
endif()
mark_as_advanced(CROSSGUID_INCLUDE_DIR CROSSGUID_LIBRARY)
