# Emscripten reads these at link time; without this a change to them does not relink.
set_property(TARGET ${APP_NAME_LC} APPEND PROPERTY LINK_DEPENDS
  ${CMAKE_SOURCE_DIR}/xbmc/platform/wasm/kodi_pre.js
  ${CMAKE_SOURCE_DIR}/xbmc/windowing/wasm/webgl_commit.js)
