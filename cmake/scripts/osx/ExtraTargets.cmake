# XBMCHelper
if(ENABLE_XBMCHELPER)
  add_subdirectory(${CMAKE_SOURCE_DIR}/tools/EventClients/Clients/OSXRemote build/XBMCHelper)
  add_dependencies(${APP_NAME_LC} XBMCHelper)
endif()
