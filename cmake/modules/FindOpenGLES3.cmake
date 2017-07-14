#.rst:
# FindOpenGLES3
# ------------
# Finds the OpenGLES3 library
#
# This will will define the following variables::
#
# OPENGLES3_FOUND - system has OpenGLES3
# OPENGLES3_INCLUDE_DIRS - the OpenGLES3 include directory
# OPENGLES3_DEFINITIONS - the OpenGLES3 definitions


find_path(OPENGLES3_INCLUDE_DIR GLES3/gl3.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(OpenGLES3
                                  REQUIRED_VARS OPENGLES3_INCLUDE_DIR)

if(OPENGLES3_FOUND)
  set(OPENGLES3_INCLUDE_DIRS ${OPENGLES3_INCLUDE_DIR})
  set(OPENGLES3_DEFINITIONS -DHAVE_LIBGLESV3)
endif()

mark_as_advanced(OPENGLES3_INCLUDE_DIR)
