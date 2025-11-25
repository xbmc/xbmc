/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EmbeddingEngine.h"

#include <cmath>
#include <numeric>
#include <stdexcept>

#ifdef HAS_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

namespace KODI
{
namespace SEMANTIC
{

// PIMPL implementation class
class CEmbeddingEngine::Impl
{
public:
  Impl() = default;
  ~Impl() = default;

#ifdef HAS_ONNXRUNTIME
  // ONNX Runtime session and environment
  std::unique_ptr<Ort::Env> env;
  std::unique_ptr<Ort::Session> session;
  bool initialized{false};
#endif

  bool Initialize(const std::string& modelPath, const std::string& vocabPath)
  {
#ifdef HAS_ONNXRUNTIME
    // TODO: Implement ONNX Runtime initialization
    // This is a stub for now
    return false;
#else
    return false;
#endif
  }

  bool IsInitialized() const
  {
#ifdef HAS_ONNXRUNTIME
    return initialized;
#else
    return false;
#endif
  }

  Embedding Embed(const std::string& text)
  {
#ifdef HAS_ONNXRUNTIME
    // TODO: Implement embedding generation
    // This is a stub - returns zero vector
    Embedding result{};
    return result;
#else
    Embedding result{};
    return result;
#endif
  }

  std::vector<Embedding> EmbedBatch(const std::vector<std::string>& texts)
  {
#ifdef HAS_ONNXRUNTIME
    // TODO: Implement batch embedding generation
    std::vector<Embedding> results(texts.size());
    return results;
#else
    std::vector<Embedding> results(texts.size());
    return results;
#endif
  }
};

// CEmbeddingEngine implementation

CEmbeddingEngine::CEmbeddingEngine() : m_impl(std::make_unique<Impl>())
{
}

CEmbeddingEngine::~CEmbeddingEngine() = default;

bool CEmbeddingEngine::Initialize(const std::string& modelPath, const std::string& vocabPath)
{
  return m_impl->Initialize(modelPath, vocabPath);
}

bool CEmbeddingEngine::IsInitialized() const
{
  return m_impl->IsInitialized();
}

Embedding CEmbeddingEngine::Embed(const std::string& text)
{
  if (!IsInitialized())
  {
    throw std::runtime_error("EmbeddingEngine not initialized");
  }
  return m_impl->Embed(text);
}

std::vector<Embedding> CEmbeddingEngine::EmbedBatch(const std::vector<std::string>& texts)
{
  if (!IsInitialized())
  {
    throw std::runtime_error("EmbeddingEngine not initialized");
  }
  return m_impl->EmbedBatch(texts);
}

float CEmbeddingEngine::Similarity(const Embedding& a, const Embedding& b)
{
  // Compute cosine similarity: dot(a, b) / (||a|| * ||b||)
  float dotProduct =
      std::inner_product(a.begin(), a.end(), b.begin(), 0.0f);

  float normA = std::sqrt(
      std::inner_product(a.begin(), a.end(), a.begin(), 0.0f));

  float normB = std::sqrt(
      std::inner_product(b.begin(), b.end(), b.begin(), 0.0f));

  if (normA == 0.0f || normB == 0.0f)
  {
    return 0.0f;
  }

  return dotProduct / (normA * normB);
}

} // namespace SEMANTIC
} // namespace KODI
