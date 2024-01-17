set(SOURCES AppTranslator.cpp
            ButtonStat.cpp
            ButtonTranslator.cpp
            Keymap.cpp
            KeymapEnvironment.cpp
            WindowKeymap.cpp
)

set(HEADERS AppTranslator.h
            ButtonStat.h
            ButtonTranslator.h
            Keymap.h
            KeymapEnvironment.h
            KeymapTypes.h
            WindowKeymap.h
)

core_add_library(input_keymaps)
