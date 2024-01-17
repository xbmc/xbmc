set(HEADERS IMouseDriverHandler.h
            IMouseInputHandler.h
            IMouseInputProvider.h
)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(input_mouse_interfaces)
endif()
