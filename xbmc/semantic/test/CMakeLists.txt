set(SOURCES
    # Phase 1 tests
    TestSemanticDatabase.cpp
    TestSubtitleParser.cpp
    TestMetadataParser.cpp
    TestGroqProvider.cpp
    TestTranscriptionProviderManager.cpp
    TestSemanticSearch.cpp
    TestSemanticIndexService.cpp
    TestAudioExtractor.cpp

    # Phase 2+ tests (future)
    TestTokenizer.cpp
    TestEmbeddingEngine.cpp
    TestVectorSearcher.cpp
    TestResultRanker.cpp
    TestHybridSearch.cpp
)

core_add_test_library(semantic_test)

# Add integration tests subdirectory
if(ENABLE_INTERNAL_GTEST)
  add_subdirectory(integration)
endif()
