if(CORE_PLATFORM_NAME_LC STREQUAL rbpi)
  set(SOURCES RPProcessInfoPi.cpp)

  set(HEADERS RPProcessInfoPi.h)

  if(MMAL_FOUND)
    list(APPEND SOURCES RenderBufferMMAL.cpp
                        RenderBufferPoolMMAL.cpp)
    list(APPEND HEADERS RenderBufferMMAL.h
                        RenderBufferPoolMMAL.h)
  endif()

  core_add_library(rp-process-pi)
endif()
