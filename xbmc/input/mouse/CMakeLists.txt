set(SOURCES MouseStat.cpp
            MouseTranslator.cpp
)

set(HEADERS MouseEvent.h
            MouseStat.h
            MouseTranslator.h
            MouseTypes.h
)

core_add_library(input_mouse)
