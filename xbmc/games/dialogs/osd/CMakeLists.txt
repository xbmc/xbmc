set(SOURCES DialogGameOSD.cpp
            DialogGameVideoFilter.cpp
            DialogGameVideoSelect.cpp
            DialogGameViewMode.cpp
)

set(HEADERS DialogGameOSD.h
            DialogGameVideoFilter.h
            DialogGameVideoSelect.h
            DialogGameViewMode.h
            IVideoSelectCallback.h
)

core_add_library(gameosddialogs)
