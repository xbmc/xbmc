set(SOURCES
    AudioExtractor.cpp
    GroqProvider.cpp
    TranscriptionProviderManager.cpp
)

set(HEADERS
    AudioExtractor.h
    GroqProvider.h
    ITranscriptionProvider.h
    TranscriptionProviderManager.h
)

core_add_library(semantic_transcription)
