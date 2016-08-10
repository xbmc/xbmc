# - Try to find embedded platforms (RPI/IMX6)
# Once done this will define
#
#  EMBEDDED_FOUND        - system is RPI / IMX6 and we most probably want to compile for GLES2 support
#                          (don't configure for OpenGL)

if(NOT CMAKE_SYSTEM_PROCESSOR)
  set(CMAKE_SYSTEM_PROCESSOR ${CPU})
endif()

string(REGEX MATCH "^arm" TARGET_ARCH_ARM "${CMAKE_SYSTEM_PROCESSOR}")
if(NOT CMAKE_CROSSCOMPILING AND NOT TARGET_ARCH_ARM)
  return()
endif()

find_path(EMBEDDED_FOUND NAMES include/linux/imxfb.h include/bcm_host.h PATHS /opt/vc)
