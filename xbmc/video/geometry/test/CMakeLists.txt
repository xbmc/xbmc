set(SOURCES TestContentBarDetector.cpp
            TestContentGeometryCombiner.cpp
            TestContentGeometryRecord.cpp
            TestContentGeometrySampler.cpp
            TestEffectiveGeometry.cpp
            TestFrameReduction.cpp
            TestFrameSampling.cpp
            TestGeometryPublication.cpp
            TestGeometryTransforms.cpp
            TestLiveGeometrySelector.cpp
            TestSampledGeometry.cpp)

set(HEADERS GeometryTestHelpers.h
            SyntheticFrame.h)

core_add_test_library(video_geometry_test)
