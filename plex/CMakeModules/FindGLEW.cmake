# Try to find GLEW. Once done, this will define:
#
#   GLEW_FOUND - variable which returns the result of the search
#   GLEW_INCLUDE_DIRS - list of include directories
#   GLEW_LIBRARIES - options for the linker

find_package(PkgConfig)
pkg_check_modules(PC_GLEW QUIET glew)

find_path(GLEW_INCLUDE_DIR
	GL/glew.h
	HINTS ${PC_GLEW_INCLUDEDIR} ${PC_GLEW_INCLUDE_DIRS}
)
find_library(GLEW_LIBRARY
	GLEW
	HINTS ${PC_GLEW_LIBDIR} ${PC_GLEW_LIBRARY_DIRS}
)

set(GLEW_INCLUDE_DIRS ${GLEW_INCLUDE_DIR})
set(GLEW_LIBRARIES ${GLEW_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLEW DEFAULT_MSG
	GLEW_INCLUDE_DIR
	GLEW_LIBRARY
)

mark_as_advanced(
	GLEW_INCLUDE_DIR
	GLEW_LIBRARY
)
