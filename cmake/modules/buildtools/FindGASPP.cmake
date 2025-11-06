#.rst:
# FindGASPP
# ----------
# Finds Gas Preprocessor perl script
#
# This will define the following variables::
#
# GASPP_PL - Gas preprocessor script
#
# The following target will be created::
#
# GASPP::GASPP - Gas preprocessor target
#

if(NOT TARGET GASPP::GASPP)
  find_package(Perl)
  
  if(Perl_FOUND)
    # We only intend to use this for depends platforms, or windows.
    if(KODI_DEPENDSBUILD OR (WIN32 OR WINDOWS_STORE))
      set(GASPP_PL "${CMAKE_SOURCE_DIR}/tools/depends/native/gas-preprocessor/gas-preprocessor.pl" CACHE FILEPATH "Gas Pre-processor script")
    endif()
  endif()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(GASPP REQUIRED_VARS GASPP_PL)

  if(GASPP_FOUND)
    add_executable(GASPP::GASPP IMPORTED GLOBAL)
    set_target_properties(GASPP::GASPP PROPERTIES
                                       IMPORTED_LOCATION "${GASPP_PL}")
  else()
    if(GASPP_FIND_REQUIRED)
      message(FATAL_ERROR "Gas preprocessor not available for this platform.")
    endif()
  endif()
endif()
