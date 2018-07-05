if(CORE_PLATFORM_NAME_LC STREQUAL rbpi)
  set(SOURCES RPProcessInfoPi.cpp)

  set(HEADERS RPProcessInfoPi.h)

  core_add_library(rp-process-pi)
endif()
