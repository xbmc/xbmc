set(HEADERS IKeyboardDriverHandler.h
            IKeyboardInputHandler.h
            IKeyboardInputProvider.h
)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(input_keyboard_interfaces)
endif()
