set(SOURCES VideoLibraryCleaningJob.cpp
            VideoLibraryContentGeometryJob.cpp
            VideoLibraryJob.cpp
            VideoLibraryMarkWatchedJob.cpp
            VideoLibraryProgressJob.cpp
            VideoLibraryRefreshingJob.cpp
            VideoLibraryScanningJob.cpp
            VideoLibraryResetResumePointJob.cpp)

set(HEADERS VideoLibraryCleaningJob.h
            VideoLibraryContentGeometryJob.h
            VideoLibraryJob.h
            VideoLibraryMarkWatchedJob.h
            VideoLibraryProgressJob.h
            VideoLibraryRefreshingJob.h
            VideoLibraryScanningJob.h
            VideoLibraryResetResumePointJob.h)

core_add_library(video_jobs)
