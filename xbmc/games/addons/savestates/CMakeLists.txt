set(SOURCES BasicMemoryStream.cpp
            DeltaPairMemoryStream.cpp
            LinearMemoryStream.cpp
            Savestate.cpp
            SavestateDatabase.cpp
            SavestateReader.cpp
            SavestateTranslator.cpp
            SavestateUtils.cpp
            SavestateWriter.cpp)

set(HEADERS BasicMemoryStream.h
            DeltaPairMemoryStream.h
            IMemoryStream.h
            LinearMemoryStream.h
            Savestate.h
            SavestateDatabase.h
            SavestateDefines.h
            SavestateReader.h
            SavestateTranslator.h
            SavestateUtils.h
            SavestateWriter.h)

core_add_library(gamesavestates)
