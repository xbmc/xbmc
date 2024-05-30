set(SOURCES ImageCacheCleaner.cpp
            ImageFileURL.cpp
            SpecialImageLoaderFactory.cpp)

set(HEADERS ImageCacheCleaner.h
            ImageFileURL.h
            SpecialImageFileLoader.h
            SpecialImageLoaderFactory.h)

core_add_library(imagefiles)
