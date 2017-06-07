#.rst:
# FindSparkle
# ----------
# Finds the FindSparkle Framework
#
# This will will define the following variables::
#
# Sparkle_FOUND - system has Sparkle
# Sparkle_INCLUDE_DIRS - the Sparkle include directory
# Sparkle_LIBRARIES - the Sparkle libraries

if(CORE_SYSTEM_NAME STREQUAL osx)
  find_library(Sparkle_LIBRARY NAMES Sparkle
                               PATHS ${DEPENDS_PATH}
                               PATH_SUFFIXES Frameworks
                               NO_DEFAULT_PATH)
  set(Sparkle_INCLUDE_DIR ${Sparkle_LIBRARY}/Headers)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Sparkle
                                  REQUIRED_VARS Sparkle_LIBRARY Sparkle_INCLUDE_DIR)

if(Sparkle_FOUND)
  set(SPARKLE_INCLUDE_DIRS ${Sparkle_INCLUDE_DIR})
  set(SPARKLE_LIBRARIES ${Sparkle_LIBRARY})
endif()

mark_as_advanced(Sparkle_INCLUDE_DIR Sparkle_LIBRARY)
