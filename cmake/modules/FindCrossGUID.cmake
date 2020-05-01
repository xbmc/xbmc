if(NOT KODI_DEPENDSBUILD AND ENABLE_INTERNAL_CROSSGUID)
  include(${WITH_KODI_DEPENDS}/packages/crossguid/package.cmake)
  add_depends_for_targets("HOST")

  add_custom_target(crossguid ALL DEPENDS crossguid-host)

  set(CROSSGUID_LIBRARY ${INSTALL_PREFIX_HOST}/lib/libcrossguid.a)
  set(CROSSGUID_INCLUDE_DIR ${INSTALL_PREFIX_HOST}/include)

  set_target_properties(crossguid PROPERTIES FOLDER "External Projects")

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(CrossGUID
                                    REQUIRED_VARS CROSSGUID_LIBRARY CROSSGUID_INCLUDE_DIR
                                    VERSION_VAR CGUID_VER)

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
