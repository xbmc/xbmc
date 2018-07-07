# FindXkeyboard-config
# ----------
# Finds the xkeyboard-config distribution files needed to compile keyboard
# maps.
#
# This will define the following variables::
#
# XKEYBOARD-CONFIG_FOUND - system has xkeyboard-config
# XKEYBOARD-CONFIG_VERSION - the version of xkeyboard-config
# XKEYBOARD-CONFIG_DIR - the xkeyboard-config files
#
# This will install the keyboard configs to the data root directory. The
# destination should be exposed to Kodi via the XKB_CONFIG_ROOT environment
# variable, e.g.
#
#   export XKB_CONFIG_ROOT="/usr/share/X11/xkb"
#   ./kodi.bin
#

pkg_check_modules(PC_XKEYBOARD_CONFIG xkeyboard-config)

if(PC_XKEYBOARD_CONFIG_FOUND)
  set(XKEYBOARD-CONFIG_PREFIX ${PC_XKEYBOARD_CONFIG_PREFIX})
  set(XKEYBOARD-CONFIG_VERSION ${PC_XKEYBOARD_CONFIG_VERSION})
endif()

include (FindPackageHandleStandardArgs)
find_package_handle_standard_args(Xkeyboard-config
  REQUIRED_VARS XKEYBOARD-CONFIG_PREFIX
  VERSION_VAR XKEYBOARD-CONFIG_VERSION)

find_file(XKEYBOARD-CONFIG_DIR xkb PATHS ${XKEYBOARD-CONFIG_PREFIX}/share PATH_SUFFIXES X11)

if(XKEYBOARD-CONFIG_FOUND)
  install(DIRECTORY ${XKEYBOARD-CONFIG_DIR}/ DESTINATION ${CMAKE_INSTALL_FULL_DATAROOTDIR}/X11/xkb)
endif()

mark_as_advanced(XKEYBOARD-CONFIG_PREFIX)
