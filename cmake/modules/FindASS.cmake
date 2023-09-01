#.rst:
# FindASS
# -------
# Finds the ASS library
#
# This will define the following target:
#
#   ASS::ASS   - The ASS library

if(NOT TARGET ASS::ASS)
  if(ASS_FIND_REQUIRED)
    set(_find_required "REQUIRED")
  endif()
  if(WIN32 OR WINDOWS_STORE)
    include(FindPackageMessage)

    find_package(libass CONFIG ${_find_required})

    if(libass_FOUND)
      # Specifically tailored to kodi windows cmake config - Prebuilt as RelWithDebInfo always currently
      get_target_property(libass_LIB libass::libass IMPORTED_LOCATION_RELWITHDEBINFO)
      get_target_property(libass_INCLUDE_DIR libass::libass INTERFACE_INCLUDE_DIRECTORIES)
      find_package_message(libass "Found libass: ${libass_LIB}" "[${libass_LIB}][${libass_INCLUDE_DIR}]")
    endif()
  else()
    find_package(PkgConfig)

    if(PKG_CONFIG_FOUND)
      pkg_check_modules(ASS libass ${_find_required} IMPORTED_TARGET GLOBAL)
    else()
      find_path(ASS_INCLUDE_DIR NAMES ass/ass.h
                                PATHS ${PC_ASS_INCLUDEDIR})
      find_library(ASS_LIBRARY NAMES ass libass
                               PATHS ${PC_ASS_LIBDIR})
      include(FindPackageHandleStandardArgs)
      find_package_handle_standard_args(ASS
                                        REQUIRED_VARS ASS_LIBRARY ASS_INCLUDE_DIR
                                        VERSION_VAR ASS_VERSION)
    endif()
  endif()

  if(ASS_FOUND)
    if(TARGET PkgConfig::ASS)
      add_library(ASS::ASS ALIAS PkgConfig::ASS)
    elseif(TARGET libass::libass)
      # Kodi Windows cmake config exports libass::libass
      add_library(ASS::ASS ALIAS libass::libass)
    else()
      add_library(ASS::ASS UNKNOWN IMPORTED)
      set_target_properties(ASS::ASS PROPERTIES
                                     IMPORTED_LOCATION "${ASS_LIBRARY}"
                                     INTERFACE_INCLUDE_DIRECTORIES "${ASS_INCLUDE_DIR}")
    endif()
    set_property(GLOBAL APPEND PROPERTY INTERNAL_DEPS_PROP ASS::ASS)
  endif()

  mark_as_advanced(ASS_INCLUDE_DIR ASS_LIBRARY)
endif()
