if("gbm" IN_LIST CORE_PLATFORM_NAME_LC OR "wayland" IN_LIST CORE_PLATFORM_NAME_LC
   OR "x11" IN_LIST CORE_PLATFORM_NAME_LC)
  set(SOURCES TestDmaBufIdentity.cpp
              TestDmaBufIdentityCache.cpp)

  # needs the buffer-object stack, which only gbm/wayland build
  if("gbm" IN_LIST CORE_PLATFORM_NAME_LC OR "wayland" IN_LIST CORE_PLATFORM_NAME_LC)
    list(APPEND SOURCES TestVideoBufferDMA.cpp)
  endif()

  core_add_test_library(videoplayer_buffers_test)
endif()
