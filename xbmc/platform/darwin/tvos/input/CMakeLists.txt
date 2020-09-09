set(SOURCES LibInputHandler.mm
            LibInputRemote.mm
            LibInputSettings.mm
            LibInputTouch.mm)

set(HEADERS LibInputHandler.h
            LibInputRemote.h
            LibInputSettings.h
            LibInputTouch.h)

set(UNITY_EXCLUDES ${SOURCES})

core_add_library(platform_tvos_input)
