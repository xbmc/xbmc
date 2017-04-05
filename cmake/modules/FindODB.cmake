#
# This module defines the following variables:
#
#   ODB_USE_FILE - Path to the UseODB.cmake file. Use it to include the ODB use file.
#                  The use file defines the needed functionality to compile and use
#                  odb generated headers.
#
#   ODB_FOUND - All required components and the core library were found
#   ODB_INCLUDR_DIRS - Combined list of all components include dirs
#   ODB_LIBRARIES - Combined list of all componenets libraries
#
#   ODB_LIBODB_FOUND - Libodb core library was found
#   ODB_LIBODB_INCLUDE_DIRS - Include dirs for libodb core library
#   ODB_LIBODB_LIBRARIES - Libraries for libodb core library
#
# For each requested component the following variables are defined:
#
#   ODB_<component>_FOUND - The component was found
#   ODB_<component>_INCLUDE_DIRS - The components include dirs
#   ODB_<component>_LIBRARIES - The components libraries
#
# <component> is the original or uppercase name of the component
#
# The component names relate directly to the odb module names.
# So for the libodb-mysql.so library, the component is named mysql,
# for the libodb-qt.so module it's qt, and so on.
#

set(ODB_USE_FILE "${CMAKE_CURRENT_LIST_DIR}/UseODB.cmake")

find_package(PkgConfig QUIET)

function(find_odb_api component)
	string(TOUPPER "${component}" component_u)
	set(ODB_${component_u}_FOUND FALSE PARENT_SCOPE)

	pkg_check_modules(PC_ODB_${component} QUIET "libodb-${component}")

	find_path(ODB_${component}_INCLUDE_DIR
		NAMES odb/${component}/version.hxx
		HINTS
			${ODB_LIBODB_INCLUDE_DIRS}
			${PC_ODB_${component}_INCLUDE_DIRS})

	find_library(ODB_${component}_LIBRARY
		NAMES odb-${component} libodb-${component}
		HINTS
			${ODB_LIBRARY_PATH}
			${PC_ODB_${component}_LIBRARY_DIRS})

	set(ODB_${component_u}_INCLUDE_DIRS ${ODB_${component}_INCLUDE_DIR} CACHE STRING "ODB ${component} include dirs")
	set(ODB_${component_u}_LIBRARIES ${ODB_${component}_LIBRARY} CACHE STRING "ODB ${component} libraries")

	mark_as_advanced(ODB_${component}_INCLUDE_DIR ODB_${component}_LIBRARY)

	if(ODB_${component_u}_INCLUDE_DIRS AND ODB_${component_u}_LIBRARIES)
		set(ODB_${component_u}_FOUND TRUE PARENT_SCOPE)
		set(ODB_${component}_FOUND TRUE PARENT_SCOPE)

		list(APPEND ODB_INCLUDE_DIRS ${ODB_${component_u}_INCLUDE_DIRS})
		list(REMOVE_DUPLICATES ODB_INCLUDE_DIRS)
		set(ODB_INCLUDE_DIRS ${ODB_INCLUDE_DIRS} PARENT_SCOPE)

		list(APPEND ODB_LIBRARIES ${ODB_${component_u}_LIBRARIES})
		list(REMOVE_DUPLICATES ODB_LIBRARIES)
		set(ODB_LIBRARIES ${ODB_LIBRARIES} PARENT_SCOPE)
	endif()
endfunction()

pkg_check_modules(PC_LIBODB QUIET "libodb")

set(ODB_LIBRARY_PATH "" CACHE STRING "Common library search hint for all ODB libs")

find_path(libodb_INCLUDE_DIR
	NAMES odb/version.hxx
	HINTS
		${PC_LIBODB_INCLUDE_DIRS})

find_library(libodb_LIBRARY
	NAMES odb libodb
	HINTS
		${ODB_LIBRARY_PATH}
		${PC_LIBODB_LIBRARY_DIRS})

find_program(odb_BIN
	NAMES odb
	HINTS
		${libodb_INCLUDE_DIR}/../bin
		${libodb_INCLUDE_DIR}/../bin/odb/bin)

set(ODB_LIBODB_INCLUDE_DIRS ${libodb_INCLUDE_DIR} CACHE STRING "ODB libodb include dirs")
set(ODB_LIBODB_LIBRARIES ${libodb_LIBRARY} CACHE STRING "ODB libodb library")
set(ODB_EXECUTABLE ${odb_BIN} CACHE STRING "ODB executable")

mark_as_advanced(libodb_INCLUDE_DIR libodb_LIBRARY odb_BIN)

if(ODB_LIBODB_INCLUDE_DIRS AND ODB_LIBODB_LIBRARIES)
	set(ODB_LIBODB_FOUND TRUE)
endif()

set(ODB_INCLUDE_DIRS ${ODB_LIBODB_INCLUDE_DIRS})
set(ODB_LIBRARIES ${ODB_LIBODB_LIBRARIES})

foreach(component ${ODB_FIND_COMPONENTS})
	find_odb_api(${component})
endforeach()

list(APPEND ODB_LIBRARIES ${libodb_LIBRARY})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ODB
	FOUND_VAR ODB_FOUND
	REQUIRED_VARS ODB_EXECUTABLE ODB_LIBODB_FOUND
	HANDLE_COMPONENTS)
