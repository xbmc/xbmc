set(SOURCES DirectoryProvider.cpp
            IListProvider.cpp
            MultiProvider.cpp
            StaticProvider.cpp)

set(HEADERS DirectoryProvider.h
            IListProvider.h
            MultiProvider.h
            StaticProvider.h)

core_add_library(guilib_listproviders)
