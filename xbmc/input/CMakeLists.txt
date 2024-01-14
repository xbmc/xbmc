set(SOURCES InertialScrollingHandler.cpp
            InputCodingTableBasePY.cpp
            InputCodingTableFactory.cpp
            InputCodingTableKorean.cpp
            InputManager.cpp
            InputTranslator.cpp
            Key.cpp
            KeyboardLayout.cpp
            KeyboardLayoutManager.cpp
            KeyboardStat.cpp
            WindowTranslator.cpp
            XBMC_keytable.cpp)

set(HEADERS InertialScrollingHandler.h
            InputCodingTable.h
            InputCodingTableBasePY.h
            InputCodingTableFactory.h
            InputCodingTableKorean.h
            InputManager.h
            InputTranslator.h
            InputTypes.h
            Key.h
            KeyboardLayout.h
            KeyboardLayoutManager.h
            KeyboardStat.h
            WindowTranslator.h
            XBMC_keyboard.h
            XBMC_keysym.h
            XBMC_keytable.h
            XBMC_vkeys.h)

core_add_library(input)
