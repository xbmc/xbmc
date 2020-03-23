set(SOURCES VideoBuffer.cpp)
set(HEADERS VideoBuffer.h)

if(CORE_PLATFORM_NAME_LC STREQUAL gbm)
  list(APPEND SOURCES VideoBufferDMA.cpp
                      VideoBufferDRMPRIME.cpp
                      VideoBufferPoolDMA.cpp)
  list(APPEND HEADERS VideoBufferDMA.h
                      VideoBufferDRMPRIME.h
                      VideoBufferPoolDMA.h)
endif()

core_add_library(videoplayer-buffers)
