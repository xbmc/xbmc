# Auto generated CMakeLists.txt.
# See xbmc/addons/kodi-dev-kit/tools/code-generator.py.

set(HEADERS
  $<$<STREQUAL:${CORE_SYSTEM_NAME},android>:system.h>
)

if(HEADERS)
  core_add_devkit_header(kodi_c-api_platform_android)
endif()
