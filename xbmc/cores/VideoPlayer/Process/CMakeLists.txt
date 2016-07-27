file(GLOB OVERRIDES_CPP overrides/${CORE_SYSTEM_NAME}/*.cpp)
file(GLOB OVERRIDES_H overrides/${CORE_SYSTEM_NAME}/*.h)

set(SOURCES ProcessInfo.cpp ${OVERRIDES_CPP})
set(HEADERS ProcessInfo.h ${OVERRIDES_H})

core_add_library(process)
if(OVERRIDES_CPP)
  target_compile_definitions(${CORE_LIBRARY} PRIVATE -DPLATFORM_OVERRIDE)
endif()
