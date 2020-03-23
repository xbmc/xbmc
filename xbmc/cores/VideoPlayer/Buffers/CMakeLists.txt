set(SOURCES VideoBuffer.cpp)
set(HEADERS VideoBuffer.h)

if(CORE_PLATFORM_NAME_LC STREQUAL gbm)
  list(APPEND SOURCES VideoBufferDRMPRIME.cpp)
  list(APPEND HEADERS VideoBufferDRMPRIME.h)
endif()

core_add_library(videoplayer-buffers)
