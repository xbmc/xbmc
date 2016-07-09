set(SOURCES ButtonTranslator.cpp
            InertialScrollingHandler.cpp
            InputCodingTableBaiduPY.cpp
            InputCodingTableBasePY.cpp
            InputCodingTableFactory.cpp
            InputCodingTableKorean.cpp
            InputManager.cpp
            Key.cpp
            KeyboardLayout.cpp
            KeyboardLayoutManager.cpp
            KeyboardStat.cpp
            MouseStat.cpp
            XBMC_keytable.cpp)

set(HEADERS ButtonTranslator.h
            InertialScrollingHandler.h
            InputCodingTable.h
            InputCodingTableBaiduPY.h
            InputCodingTableBasePY.h
            InputCodingTableFactory.h
            InputCodingTableKorean.h
            InputManager.h
            Key.h
            KeyboardLayout.h
            KeyboardLayoutManager.h
            KeyboardStat.h
            MouseStat.h
            XBIRRemote.h
            XBMC_keyboard.h
            XBMC_keysym.h
            XBMC_keytable.h
            XBMC_mouse.h
            XBMC_vkeys.h)

core_add_library(input)
