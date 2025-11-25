set(SOURCES
    EmbeddingEngine.cpp
    GPUAccelerator.cpp
    Tokenizer.cpp
)

set(HEADERS
    EmbeddingEngine.h
    GPUAccelerator.h
    GPUTypes.h
    Tokenizer.h
)

if(NOT ENABLE_INTERNAL_ONNXRUNTIME)
  find_package(ONNXRuntime REQUIRED)
endif()

core_add_library(semantic_embedding)
