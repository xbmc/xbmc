set(SOURCES Screensaver.cpp)

set(HEADERS Screensaver.h)

include_directories(${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include)

core_add_library(addonInterface_kodi_screensaver)
