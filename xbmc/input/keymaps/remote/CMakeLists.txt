set(SOURCES CustomControllerTranslator.cpp
            IRTranslator.cpp
)

set(HEADERS CustomControllerTranslator.h
            IRRemoteIDs.h
            IRTranslator.h
)

core_add_library(input_keymaps_remote)
