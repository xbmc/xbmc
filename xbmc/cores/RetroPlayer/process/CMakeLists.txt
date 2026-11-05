set(SOURCES DisplayPacing.cpp
            RPProcessInfo.cpp
)

set(HEADERS DisplayPacing.h
            RPProcessInfo.h
)

if(TARGET ${APP_NAME_LC}::EGL)
  list(APPEND SOURCES egl/RPProcessInfoEGL.cpp)
  list(APPEND HEADERS egl/RPProcessInfoEGL.h)
endif()

core_add_library(rp-process)
