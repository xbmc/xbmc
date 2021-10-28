# Auto generated CMakeLists.txt.
# See xbmc/addons/kodi-dev-kit/tools/code-generator.py.

set(HEADERS
  $<$<STREQUAL:${CORE_SYSTEM_NAME},android>:System.h>
)

if(HEADERS)
  core_add_devkit_header(kodi_platform_android)
endif()
