# - Multiarch support in object code library directories
#
# This module sets the following variable
#	CMAKE_INSTALL_LIBDIR	     to lib, lib64 or lib/x86_64-linux-gnu
#	                             depending on the platform; use this path
#	                             for platform-specific binaries.
#
#	CMAKE_INSTALL_LIBDIR_NOARCH  to lib or lib64 depending on the platform;
#	                             use this path for architecture-independent
#	                             files.
#
# Note that it will override the results of GNUInstallDirs if included after
# that module.

# Fedora uses lib64/ for 64-bit systems, Debian uses lib/x86_64-linux-gnu;
# Fedora put module files in lib64/ too, but Debian uses lib/ for that
if ("${CMAKE_SYSTEM_NAME}" MATCHES "Linux" AND
    "${CMAKE_INSTALL_PREFIX}" STREQUAL "/usr")
  # Debian or Ubuntu?
  if (EXISTS "/etc/debian_version")
	set (_libdir_def "lib/${CMAKE_LIBRARY_ARCHITECTURE}")
	set (_libdir_noarch "lib")
  elseif (EXISTS "/etc/fedora-release" OR
          EXISTS "/etc/redhat-release" OR
          EXISTS "/etc/slackware-version" OR
          EXISTS "/etc/gentoo-release")
	# 64-bit system?
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
	  set (_libdir_noarch "lib64")
	else (CMAKE_SIZEOF_VOID_P EQUAL 8)
	  set (_libdir_noarch "lib")
	endif (CMAKE_SIZEOF_VOID_P EQUAL 8)
	set (_libdir_def "${_libdir_noarch}")
  else ()
    set (_libdir_def "lib")
    set (_libdir_noarch "lib")
  endif ()
else ()
  set (_libdir_def "lib")
  set (_libdir_noarch "lib")
endif ()

# let the user override if somewhere else is desirable
set (CMAKE_INSTALL_LIBDIR "${_libdir_def}" CACHE PATH "Object code libraries")
set (CMAKE_INSTALL_LIBDIR_NOARCH "${_libdir_noarch}" CACHE PATH "Architecture-independent library files")
mark_as_advanced (
  CMAKE_INSTALL_LIBDIR
  CMAKE_INSTALL_LIBDIR_NOARCH
  )
