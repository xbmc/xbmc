set(SOURCES "")
set(HEADERS "")

if(LIRC_DEVICE)
  list(APPEND SOURCES LIRC.cpp)
  list(APPEND HEADERS LIRC.h)
endif()

if(CORE_PLATFORM_NAME_LC STREQUAL rbpi OR CORE_PLATFORM_NAME_LC STREQUAL gbm OR CORE_PLATFORM_NAME_LC STREQUAL aml)
  list(APPEND SOURCES LinuxInputDevices.cpp)
  list(APPEND HEADERS LinuxInputDevices.h)
endif()

core_add_library(input_linux)
