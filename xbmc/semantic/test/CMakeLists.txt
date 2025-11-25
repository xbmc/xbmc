set(SOURCES
    # Phase 1: Content Text Index tests
    TestSemanticDatabase.cpp
    TestSubtitleParser.cpp
    TestMetadataParser.cpp
    TestGroqProvider.cpp
    TestTranscriptionProviderManager.cpp
    TestSemanticSearch.cpp
    TestSemanticIndexService.cpp
    TestAudioExtractor.cpp
)

core_add_test_library(semantic_test)
