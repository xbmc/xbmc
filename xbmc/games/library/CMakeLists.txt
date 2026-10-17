set(SOURCES GameAgeRatings.cpp
            GameFileIdentity.cpp
            GameNameParser.cpp
            ReleasePolicy.cpp)

set(HEADERS GameAgeRatings.h
            GameFileIdentity.h
            GameLibraryTypes.h
            GameNameParser.h
            ReleasePolicy.h)

core_add_library(games_library)
