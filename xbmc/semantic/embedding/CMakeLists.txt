set(SOURCES
    EmbeddingEngine.cpp
    Tokenizer.cpp
)

set(HEADERS
    EmbeddingEngine.h
    Tokenizer.h
)

if(NOT ENABLE_INTERNAL_ONNXRUNTIME)
  find_package(ONNXRuntime REQUIRED)
endif()

core_add_library(semantic_embedding)
