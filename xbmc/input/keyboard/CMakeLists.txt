set(SOURCES Key.cpp
            KeyboardEasterEgg.cpp
            KeyboardLayout.cpp
            KeyboardLayoutManager.cpp
            KeyboardStat.cpp
            KeyboardTranslator.cpp
            XBMC_keytable.cpp
)

set(HEADERS Key.h
            KeyboardEasterEgg.h
            KeyboardLayout.h
            KeyboardLayoutManager.h
            KeyboardStat.h
            KeyboardSymbols.h
            KeyboardTranslator.h
            KeyboardTypes.h
            KeyIDs.h
            XBMC_keyboard.h
            XBMC_keysym.h
            XBMC_keytable.h
            XBMC_vkeys.h
)

core_add_library(input_keyboard)
