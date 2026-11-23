set(SOURCES LibraryQuery.cpp
            LibraryQueryDatabase.cpp
            LibraryQueryFileItemListModifier.cpp
            LibraryQueryReferences.cpp
            LibraryQueryRule.cpp
            LibraryQueryType.cpp
            MediaSection.cpp
            MediaType.cpp)

set(HEADERS LibraryQuery.h
            LibraryQueryDatabase.h
            LibraryQueryFileItemListModifier.h
            LibraryQueryReferences.h
            LibraryQueryRule.h
            LibraryQueryType.h
            MediaLockState.h
            MediaSection.h
            MediaStreams.h
            MediaType.h)

core_add_library(media)
