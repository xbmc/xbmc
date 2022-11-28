set(SOURCES RPProcessInfo.cpp
)

set(HEADERS RPProcessInfo.h
)

if(TARGET ${APP_NAME_LC}::EGL)
  list(APPEND SOURCES egl/RPProcessInfoEGL.cpp)
  list(APPEND HEADERS egl/RPProcessInfoEGL.h)
endif()

core_add_library(rp-process)
