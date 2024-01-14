set(HEADERS IKeyMapper.h
            IKeyHandler.h
            IKeyboardActionMap.h
            IKeymapHandler.h
            IKeymapEnvironment.h
            IKeymap.h
)

if(NOT ENABLE_STATIC_LIBS)
  core_add_library(input_keymaps_interfaces)
endif()
