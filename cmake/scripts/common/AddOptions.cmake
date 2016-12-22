# - Add options without repeating them on the command line
#
# Synopsis:
#
#	add_options (lang build opts)
#
# where:
#
#	lang       Name of the language whose compiler should receive the
#	           options, e.g. CXX. If a comma-separated list is received
#	           then the option is added for all those languages. Use the
#	           special value ALL_LANGUAGES for these languages: CXX, C
#	           and Fortran
#
#	build      Kind of build to which this options should apply,
#              such as DEBUG and RELEASE. This can also be a comma-
#	           separated list. Use the special value ALL_BUILDS to apply
#	           to all builds.
#
#	opts       List of options to add. Each should be quoted.
#
# Example:
#
#	add_options (CXX RELEASE "-O3" "-DNDEBUG" "-Wall")

function(add_options langs builds)
  # special handling of empty language specification
  if("${langs}" STREQUAL "ALL_LANGUAGES")
	set(langs CXX C Fortran)
  endif()
  foreach(lang IN LISTS langs)
	# prepend underscore if necessary
	foreach(build IN LISTS builds)
	  if(NOT ("${build}" STREQUAL "ALL_BUILDS"))
		set(_bld "_${build}")
		string(TOUPPER "${_bld}" _bld)
	  else()
		set(_bld "")
	  endif()
	  foreach(_opt IN LISTS ARGN)
		set(_var "CMAKE_${lang}_FLAGS${_bld}")
		#message(STATUS "Adding \"${_opt}\" to \${${_var}}")
		# remove it first
		string(REPLACE "${_opt}" "" _without "${${_var}}")
		string(STRIP "${_without}" _without)
		# we need to strip this one as well, so they are comparable
		string(STRIP "${${_var}}" _stripped)
		# if it wasn't there, then add it at the end
		if("${_without}" STREQUAL "${_stripped}")
		  # don't add any extra spaces if no options yet are set
		  if(NOT ${_stripped} STREQUAL "")
			set(${_var} "${_stripped} ${_opt}")
		  else()
			set(${_var} "${_opt}")
		  endif()
		  set(${_var} "${${_var}}" PARENT_SCOPE)
		endif()
	  endforeach()
	endforeach()
  endforeach()
endfunction()

# set varname to flag unless user has specified something that matches regex
function(set_default_option varname flag regex)
  if(NOT "$ENV{CXXFLAGS}" MATCHES "${regex}"
	  AND NOT "${CMAKE_CXX_FLAGS}" MATCHES "${regex}"
	  AND NOT "${CMAKE_CXX_FLAGS_${CMAKE_BUILD_TYPE}}" MATCHES "${regex}")
	set(${varname} ${flag} PARENT_SCOPE)
  else()
	set(${varname} PARENT_SCOPE)
  endif()
endfunction()

# note: this must be called before project()
macro(no_default_options)
  # prevent the platform probe to set options
  set(CMAKE_NOT_USING_CONFIG_FLAGS TRUE)
endmacro()
