set(SOURCES VideoLibraryCleaningJob.cpp
            VideoLibraryJob.cpp
            VideoLibraryMarkWatchedJob.cpp
            VideoLibraryProgressJob.cpp
            VideoLibraryRefreshingJob.cpp
            VideoLibraryScanningJob.cpp)

set(HEADERS VideoLibraryCleaningJob.h
            VideoLibraryJob.h
            VideoLibraryMarkWatchedJob.h
            VideoLibraryProgressJob.h
            VideoLibraryRefreshingJob.h
            VideoLibraryScanningJob.h)

core_add_library(video_jobs)
