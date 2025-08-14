set(SOURCES JobManager.cpp
            JobQueue.cpp)

set(HEADERS IJobCallback.h
            Job.h
            JobManager.h
            JobQueue.h
            LambdaJob.h)

core_add_library(jobs)
