set(SOURCES Visualization.cpp)

set(HEADERS Visualization.h)

include_directories(${CORE_SOURCE_DIR}/xbmc/addons/kodi-addon-dev-kit/include)

core_add_library(addonInterface_kodi_visualization)
