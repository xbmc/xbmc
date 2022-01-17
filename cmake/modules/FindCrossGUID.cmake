if(ENABLE_INTERNAL_CROSSGUID)
  include(ExternalProject)
  include(cmake/scripts/common/ModuleHelpers.cmake)

  get_archive_name(crossguid)

  # allow user to override the download URL with a local tarball
  # needed for offline build envs
  if(CROSSGUID_URL)
    get_filename_component(CROSSGUID_URL "${CROSSGUID_URL}" ABSOLUTE)
  else()
    set(CROSSGUID_URL http://mirrors.kodi.tv/build-deps/sources/${ARCHIVE})
  endif()
  if(VERBOSE)
    message(STATUS "CROSSGUID_URL: ${CROSSGUID_URL}")
  endif()

  if(APPLE)
    set(EXTRA_ARGS "-DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}")
  endif()

  set(CROSSGUID_LIBRARY ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/lib/libcrossguid.a)
  set(CROSSGUID_INCLUDE_DIR ${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}/include)
  externalproject_add(crossguid
                      URL ${CROSSGUID_URL}
                      DOWNLOAD_DIR ${TARBALL_DIR}
                      PREFIX ${CORE_BUILD_DIR}/crossguid
                      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/${CORE_BUILD_DIR}
                                 -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TOOLCHAIN_FILE}
                                 "${EXTRA_ARGS}"
                      PATCH_COMMAND ${CMAKE_COMMAND} -E copy
                                    ${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/CMakeLists.txt
                                    <SOURCE_DIR> &&
                                    ${CMAKE_COMMAND} -E copy
                                    ${CMAKE_SOURCE_DIR}/tools/depends/target/crossguid/FindUUID.cmake
                                    <SOURCE_DIR>
                      BUILD_BYPRODUCTS ${CROSSGUID_LIBRARY})
  set_target_properties(crossguid PROPERTIES FOLDER "External Projects")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CrossGUID
                                    REQUIRED_VARS CROSSGUID_LIBRARY CROSSGUID_INCLUDE_DIR
                                    VERSION_VAR CROSSGUID_VER)

  set(CROSSGUID_LIBRARIES ${CROSSGUID_LIBRARY})
  set(CROSSGUID_INCLUDE_DIRS ${CROSSGUID_INCLUDE_DIR})
else()
  find_path(CROSSGUID_INCLUDE_DIR NAMES guid.hpp guid.h)

  find_library(CROSSGUID_LIBRARY_RELEASE NAMES crossguid)
  find_library(CROSSGUID_LIBRARY_DEBUG NAMES crossguidd)

  include(SelectLibraryConfigurations)
  select_library_configurations(CROSSGUID)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CrossGUID
                                    REQUIRED_VARS CROSSGUID_LIBRARY CROSSGUID_INCLUDE_DIR)

  if(CROSSGUID_FOUND)
    set(CROSSGUID_LIBRARIES ${CROSSGUID_LIBRARY})
    set(CROSSGUID_INCLUDE_DIRS ${CROSSGUID_INCLUDE_DIR})

    if(EXISTS "${CROSSGUID_INCLUDE_DIR}/guid.hpp")
      set(CROSSGUID_DEFINITIONS -DHAVE_NEW_CROSSGUID)
    endif()

    add_custom_target(crossguid)
    set_target_properties(crossguid PROPERTIES FOLDER "External Projects")
  endif()
  mark_as_advanced(CROSSGUID_INCLUDE_DIR CROSSGUID_LIBRARY)
endif()

if(NOT WIN32 AND NOT APPLE)
  find_package(UUID REQUIRED)
  list(APPEND CROSSGUID_INCLUDE_DIRS ${UUID_INCLUDE_DIRS})
  list(APPEND CROSSGUID_LIBRARIES ${UUID_LIBRARIES})
endif()
