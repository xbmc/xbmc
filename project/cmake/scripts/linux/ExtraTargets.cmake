# xrandr
if(ENABLE_X11 AND XRANDR_FOUND)
  add_executable(${APP_NAME_LC}-xrandr ${CORE_SOURCE_DIR}/xbmc-xrandr.c)
  target_link_libraries(kodi-xrandr ${SYSTEM_LDFLAGS} ${X_LIBRARIES} m ${XRANDR_LIBRARIES})
endif()
