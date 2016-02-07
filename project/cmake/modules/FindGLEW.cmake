# - Try to find GLEW
# Once done this will define
#
# GLEW_FOUND - system has libGLEW
# GLEW_INCLUDE_DIRS - the libGLEW include directory
# GLEW_LIBRARIES - The libGLEW libraries

if(PKG_CONFIG_FOUND)
  pkg_check_modules (GLEW glew)
else()
  find_path(GLEW_INCLUDE_DIRS GL/glew.h)
  find_library(GLEW_LIBRARIES GLEW)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLEW DEFAULT_MSG GLEW_INCLUDE_DIRS GLEW_LIBRARIES)

mark_as_advanced(GLEW_INCLUDE_DIRS GLEW_LIBRARIES)
