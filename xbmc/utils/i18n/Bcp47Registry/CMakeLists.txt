set(SOURCES RegistryRecordProvider.cpp
            SubTagLoader.cpp
            SubTagRegistryFile.cpp
            SubTagRegistryManager.cpp
            SubTagRegistryTypes.cpp)

set(HEADERS RegistryRecordProvider.h
            SubTagLoader.h
            SubTagRegistryFile.h
            SubTagRegistryManager.h
            SubTagRegistryTypes.h
            SubTagsCollection.h)

core_add_library(utils_i18n_Bcp47Registry)
