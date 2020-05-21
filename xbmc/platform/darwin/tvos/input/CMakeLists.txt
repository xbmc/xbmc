set(SOURCES LibInputHandler.mm
            LibInputRemote.mm
            LibInputSettings.mm
            LibInputTouch.mm)

set(HEADERS LibInputHandler.h
            LibInputRemote.h
            LibInputSettings.h
            LibInputTouch.h)

core_add_library(platform_tvos_input)
