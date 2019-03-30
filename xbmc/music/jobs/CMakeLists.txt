set(SOURCES MusicLibraryJob.cpp
            MusicLibraryProgressJob.cpp
            MusicLibraryCleaningJob.cpp
            MusicLibraryExportJob.cpp
            MusicLibraryImportJob.cpp
            MusicLibraryScanningJob.cpp)

set(HEADERS MusicLibraryJob.h
            MusicLibraryProgressJob.h
            MusicLibraryCleaningJob.h
            MusicLibraryExportJob.h
            MusicLibraryImportJob.h
            MusicLibraryScanningJob.h)

core_add_library(music_jobs)
