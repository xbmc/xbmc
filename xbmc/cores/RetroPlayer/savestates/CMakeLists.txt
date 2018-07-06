set(SOURCES Savestate.cpp
            SavestateDatabase.cpp
            SavestateReader.cpp
            SavestateTranslator.cpp
            SavestateUtils.cpp
            SavestateWriter.cpp)

set(HEADERS Savestate.h
            SavestateDatabase.h
            SavestateDefines.h
            SavestateReader.h
            SavestateTranslator.h
            SavestateUtils.h
            SavestateWriter.h)

core_add_library(retroplayer_savestates)
