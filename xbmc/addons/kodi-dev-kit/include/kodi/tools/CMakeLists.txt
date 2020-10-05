set(HEADERS DllHelper.h
            EndTime.h
            StringUtils.h
            Thread.h
            Timer.h)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(addons_kodi-dev-kit_include_kodi_tools)
endif()
