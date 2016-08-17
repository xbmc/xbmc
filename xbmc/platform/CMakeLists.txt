file(GLOB OVERRIDES_CPP overrides/${CORE_SYSTEM_NAME}/*.cpp)
file(GLOB OVERRIDES_H overrides/${CORE_SYSTEM_NAME}/*.h)

set(SOURCES Platform.cpp
            XbmcContext.cpp
            xbmc.cpp
            ${OVERRIDES_CPP})

set(HEADERS MessagePrinter.h
            Platform.h
            xbmc.h
            XbmcContext.h
            ${OVERRIDES_H})

core_add_library(platform_common)

if(OVERRIDES_CPP)
  target_compile_definitions(${CORE_LIBRARY} PRIVATE -DPLATFORM_OVERRIDE)
endif()
