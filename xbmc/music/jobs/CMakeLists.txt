set(SOURCES MusicLibraryJob.cpp
            MusicLibraryProgressJob.cpp
            MusicLibraryExportJob.cpp)

set(HEADERS MusicLibraryJob.h
            MusicLibraryProgressJob.h
            MusicLibraryExportJob.h)

core_add_library(music_jobs)
