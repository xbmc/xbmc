set(SOURCES MusicLibraryJob.cpp
            MusicLibraryProgressJob.cpp
            MusicLibraryCleaningJob.cpp
            MusicLibraryExportJob.cpp
            MusicLibraryScanningJob.cpp)

set(HEADERS MusicLibraryJob.h
            MusicLibraryProgressJob.h
            MusicLibraryCleaningJob.h
            MusicLibraryExportJob.h
            MusicLibraryScanningJob.h)

core_add_library(music_jobs)
