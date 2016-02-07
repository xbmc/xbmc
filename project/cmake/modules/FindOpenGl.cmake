# - Try to find OpenGL
# Once done this will define
#
# OPENGL_FOUND - system has OpenGL
# OPENGL_INCLUDE_DIRS - the OpenGL include directory
# OPENGL_LIBRARIES - The OpenGL libraries

if(PKG_CONFIG_FOUND AND NOT APPLE)
  pkg_check_modules (OPENGL gl glu)
  list(APPEND OPENGL_INCLUDE_DIRS "/usr/include")
else()
  set(CMAKE_MODULE_PATH "")
  find_package(OpenGL)
  if(OPENGL_FOUND)
    set(OPENGL_INCLUDE_DIRS ${OPENGL_INCLUDE_DIR})
  endif()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenGl DEFAULT_MSG OPENGL_INCLUDE_DIRS OPENGL_LIBRARIES)

list(APPEND OPENGL_DEFINITIONS -DHAVE_LIBGL=1)

mark_as_advanced(OPENGL_INCLUDE_DIRS OPENGL_LIBRARIES OPENGL_DEFINITIONS)
