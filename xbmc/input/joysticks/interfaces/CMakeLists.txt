set(HEADERS IButtonMap.h
            IButtonMapCallback.h
            IButtonMapper.h
            IButtonSequence.h
            IDriverHandler.h
            IDriverReceiver.h
            IInputHandler.h
            IInputProvider.h
            IInputReceiver.h
)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(input_joysticks_interfaces)
endif()
