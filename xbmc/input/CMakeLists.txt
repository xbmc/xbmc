set(SOURCES InertialScrollingHandler.cpp
            InputCodingTableBasePY.cpp
            InputCodingTableFactory.cpp
            InputCodingTableKorean.cpp
            InputManager.cpp
            InputTranslator.cpp
            WindowTranslator.cpp
)

set(HEADERS InertialScrollingHandler.h
            InputCodingTable.h
            InputCodingTableBasePY.h
            InputCodingTableFactory.h
            InputCodingTableKorean.h
            InputManager.h
            InputTranslator.h
            InputTypes.h
            WindowTranslator.h
)

core_add_library(input)
