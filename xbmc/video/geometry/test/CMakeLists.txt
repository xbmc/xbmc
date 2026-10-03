set(SOURCES TestContentGeometryCombiner.cpp
            TestContentGeometryRecord.cpp
            TestEffectiveGeometry.cpp
            TestFrameReduction.cpp
            TestFrameSampling.cpp
            TestGeometryPublication.cpp
            TestGeometryTransforms.cpp)

set(HEADERS GeometryTestHelpers.h)

core_add_test_library(video_geometry_test)
