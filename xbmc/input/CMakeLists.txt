set(SOURCES Action.cpp
            ActionTranslator.cpp
            ButtonTranslator.cpp
            CustomControllerTranslator.cpp
            InertialScrollingHandler.cpp
            InputCodingTableBaiduPY.cpp
            InputCodingTableBasePY.cpp
            InputCodingTableFactory.cpp
            InputCodingTableKorean.cpp
            InputManager.cpp
            IRTranslator.cpp
            Key.cpp
            KeyboardLayout.cpp
            KeyboardLayoutManager.cpp
            KeyboardStat.cpp
            MouseStat.cpp
            TouchTranslator.cpp
            XBMC_keytable.cpp)

set(HEADERS Action.h
            ActionIDs.h
            ActionTranslator.h
            ButtonTranslator.h
            CustomControllerTranslator.h
            InertialScrollingHandler.h
            InputCodingTable.h
            InputCodingTableBaiduPY.h
            InputCodingTableBasePY.h
            InputCodingTableFactory.h
            InputCodingTableKorean.h
            InputManager.h
            IRTranslator.h
            Key.h
            KeyboardLayout.h
            KeyboardLayoutManager.h
            KeyboardStat.h
            MouseStat.h
            TouchTranslator.h
            XBIRRemote.h
            XBMC_keyboard.h
            XBMC_keysym.h
            XBMC_keytable.h
            XBMC_vkeys.h)

core_add_library(input)
