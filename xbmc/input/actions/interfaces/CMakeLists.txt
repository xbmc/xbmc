set(HEADERS IActionListener.h
)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(input_actions_interfaces)
endif()
