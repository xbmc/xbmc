set(HEADERS IHardwareInput.h
)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(input_hardware)
endif()
