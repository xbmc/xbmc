if(CORE_PLATFORM_NAME_LC STREQUAL aml)
  set(SOURCES RPProcessInfoAmlogic.cpp)
  set(HEADERS RPProcessInfoAmlogic.h)

  core_add_library(rp-process-amlogic)
endif()
