set(helper_srcs
  XBMCDebugHelpers.h
  XBMCHelper.h
  XBMCHelper.m
  xbmcclientwrapper.h
  xbmcclientwrapper.mm
  xbmchelper_main.mm
  HIDRemote/HIDRemote.m
  HIDRemote/HIDRemote.h
)

add_executable(PlexHTHelper ${helper_srcs})
target_link_libraries(PlexHTHelper
  ${CONFIG_LIBRARY_COCOA}
  ${CONFIG_LIBRARY_IOKIT}
  ${CONFIG_LIBRARY_STDC++}
  ${CONFIG_LIBRARY_CARBON}
)

install(TARGETS PlexHTHelper DESTINATION "Plex Home Theater.app/Contents/Resources/XBMC/tools/darwin/runtime")
