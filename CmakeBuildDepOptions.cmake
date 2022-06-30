# Dependency build options for all platforms
#
# Note cmake_dependent_option only allows an option to be set by user if it fulfils the
# conditions for its first state (eg ON/OFF after the description string). If it does not,
# the last state is used, but is forced and cant be overridden with a set( CACHE FORCE) or
# user define (-DENABLE_XXX=STATE)
#
# Example
# cmake_dependent_option(ENABLE_INTERNAL_CEC "Enable internal cec?" OFF "DEFINED USE_INTERNAL_LIBS;NOT USE_INTERNAL_LIBS" ON)
# For all platforms that set USE_INTERNAL_LIBS=OFF (Linux/freebsd), they can override and set
# -DENABLE_INTERNAL_CEC=ON if they wish to still build the cec lib.
# Apple/Windows/Android/KODI_DEPENDS will be given the FORCED state of ON, and is not possible
# to change unless the user sets -DUSE_INTERNAL_LIBS=OFF AND then also supplies -DENABLE_INTERNAL_CEC=<STATE>
# If a user goes this route, they will need to manually set most ENABLE_INTERNAL_XXX to build
#

# Internal Depends - supported on all platforms
option(ENABLE_INTERNAL_CROSSGUID "Enable internal crossguid?" ON)
option(ENABLE_INTERNAL_RapidJSON "Enable internal rapidjson?" ON)

# use ffmpeg from depends or system
option(ENABLE_INTERNAL_FFMPEG "Enable internal ffmpeg?" OFF)

cmake_dependent_option(ENABLE_INTERNAL_FLATBUFFERS "Enable internal flatbuffers?" OFF "DEFINED USE_INTERNAL_LIBS;NOT USE_INTERNAL_LIBS" ON)
cmake_dependent_option(ENABLE_INTERNAL_NFS "Enable internal libnfs?" OFF "DEFINED USE_INTERNAL_LIBS;NOT USE_INTERNAL_LIBS" ON)
cmake_dependent_option(ENABLE_INTERNAL_PCRE "Enable internal pcre?" OFF "DEFINED USE_INTERNAL_LIBS;NOT USE_INTERNAL_LIBS" ON)
cmake_dependent_option(ENABLE_INTERNAL_TAGLIB "Enable internal taglib?" OFF "DEFINED USE_INTERNAL_LIBS;NOT USE_INTERNAL_LIBS" ON)

# Internal Depends - supported on UNIX platforms
if(UNIX)
  option(FFMPEG_PATH        "Path to external ffmpeg?" "")
  option(ENABLE_INTERNAL_FMT "Enable internal fmt?" OFF)
  option(ENABLE_INTERNAL_FSTRCMP "Enable internal fstrcmp?" OFF)
  option(ENABLE_INTERNAL_DAV1D "Enable internal dav1d?" OFF)
  option(ENABLE_INTERNAL_GTEST "Enable internal gtest?" OFF)
  option(ENABLE_INTERNAL_UDFREAD "Enable internal udfread?" OFF)
  option(ENABLE_INTERNAL_SPDLOG "Enable internal spdlog?" OFF)

  if(ENABLE_INTERNAL_SPDLOG)
    set(ENABLE_INTERNAL_FMT ON CACHE BOOL "" FORCE)
  endif()
endif()

# prefer kissfft from xbmc/contrib but let use system one on unices
cmake_dependent_option(ENABLE_INTERNAL_KISSFFT "Enable internal kissfft?" ON "UNIX" ON)
