set(SOURCES BasicMemoryStream.cpp
            DeltaPairMemoryStream.cpp
            LinearMemoryStream.cpp
)

set(HEADERS BasicMemoryStream.h
            DeltaPairMemoryStream.h
            IMemoryStream.h
            LinearMemoryStream.h
)

core_add_library(retroplayer_memory)
