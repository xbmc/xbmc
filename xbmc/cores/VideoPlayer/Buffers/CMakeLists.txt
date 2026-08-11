set(SOURCES VideoBuffer.cpp)
set(HEADERS VideoBuffer.h)

# dma-buf identity is consumed by the VAAPI EGL path, which builds on x11 too
if("gbm" IN_LIST CORE_PLATFORM_NAME_LC OR "wayland" IN_LIST CORE_PLATFORM_NAME_LC
   OR "x11" IN_LIST CORE_PLATFORM_NAME_LC)
  list(APPEND SOURCES DmaBufIdentity.cpp
                      DmaBufIdentityCache.cpp)
  list(APPEND HEADERS DmaBufIdentity.h
                      DmaBufIdentityCache.h)
endif()

if("gbm" IN_LIST CORE_PLATFORM_NAME_LC OR "wayland" IN_LIST CORE_PLATFORM_NAME_LC)
  list(APPEND SOURCES VideoBufferDMA.cpp
                      VideoBufferDRMPRIME.cpp
                      VideoBufferPoolDMA.cpp)
  list(APPEND HEADERS VideoBufferDMA.h
                      VideoBufferDRMPRIME.h
                      VideoBufferPoolDMA.h)
endif()

if("webos" IN_LIST CORE_PLATFORM_NAME_LC)
    list(APPEND SOURCES VideoBufferStarfish.cpp)
    list(APPEND HEADERS VideoBufferStarfish.h)
endif()


core_add_library(videoplayer-buffers)
