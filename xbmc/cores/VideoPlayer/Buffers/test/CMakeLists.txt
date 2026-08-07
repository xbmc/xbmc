if("gbm" IN_LIST CORE_PLATFORM_NAME_LC OR "wayland" IN_LIST CORE_PLATFORM_NAME_LC)
  set(SOURCES TestDmaBufIdentity.cpp
              TestVideoBufferDMA.cpp)

  core_add_test_library(videoplayer_buffers_test)
endif()
