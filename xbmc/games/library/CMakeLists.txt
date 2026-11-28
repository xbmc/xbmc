set(SOURCES GameFileIdentity.cpp
            GameNameInfo.cpp
            GameNameParser.cpp
            GameVersions.cpp
            ReleasePolicy.cpp)

set(HEADERS GameFileIdentity.h
            GameLibraryTypes.h
            GameNameInfo.h
            GameNameParser.h
            GameVersions.h
            ReleasePolicy.h)

core_add_library(games_library)
