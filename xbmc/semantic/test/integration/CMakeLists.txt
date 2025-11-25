set(SOURCES
    SemanticIntegrationTest.cpp
)

# Copy test data files to build directory
set(TESTDATA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../testdata)
set(TESTDATA_FILES
    sample.srt
    sample.ass
    sample.vtt
    sample.nfo
)

# Create testdata directory in build
file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/xbmc/semantic/test/testdata)

# Copy test data files
foreach(TESTDATA_FILE ${TESTDATA_FILES})
    configure_file(
        ${TESTDATA_DIR}/${TESTDATA_FILE}
        ${CMAKE_BINARY_DIR}/xbmc/semantic/test/testdata/${TESTDATA_FILE}
        COPYONLY
    )
endforeach()

core_add_test_library(semantic_integration_test)
