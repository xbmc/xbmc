/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EmbeddingEngine.h"

#include "Tokenizer.h"
#include "utils/log.h"

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
  // ONNX Runtime environment and session
  Ort::Env m_env{ORT_LOGGING_LEVEL_WARNING, "SemanticEmbedding"};
  std::unique_ptr<Ort::Session> m_session;
  std::unique_ptr<CTokenizer> m_tokenizer;

  Ort::MemoryInfo m_memoryInfo =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  std::vector<const char*> m_inputNames = {"input_ids", "attention_mask", "token_type_ids"};
  std::vector<const char*> m_outputNames = {"last_hidden_state"};

  bool m_initialized{false};
#endif

  bool Initialize(const std::string& modelPath, const std::string& vocabPath)
  {
#ifdef HAS_ONNXRUNTIME
    try
    {
      // Load tokenizer
      m_tokenizer = std::make_unique<CTokenizer>();
      if (!m_tokenizer->Load(vocabPath))
      {
        CLog::Log(LOGERROR, "EmbeddingEngine: Failed to load tokenizer vocabulary from '{}'",
                  vocabPath);
        return false;
      }

      CLog::Log(LOGINFO, "EmbeddingEngine: Loaded tokenizer with {} tokens",
                m_tokenizer->GetVocabSize());

      // Configure ONNX session options
      Ort::SessionOptions sessionOptions;
      sessionOptions.SetIntraOpNumThreads(2);
      sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

      // Load ONNX model
      m_session = std::make_unique<Ort::Session>(m_env, modelPath.c_str(), sessionOptions);

      CLog::Log(LOGINFO, "EmbeddingEngine: Successfully loaded ONNX model from '{}'", modelPath);

      m_initialized = true;
      return true;
    }
    catch (const Ort::Exception& e)
    {
      CLog::Log(LOGERROR, "EmbeddingEngine: ONNX Runtime error: {}", e.what());
      return false;
    }
    catch (const std::exception& e)
    {
      CLog::Log(LOGERROR, "EmbeddingEngine: Initialization error: {}", e.what());
      return false;
    }
#else
    CLog::Log(LOGERROR,
              "EmbeddingEngine: ONNX Runtime support not available (HAS_ONNXRUNTIME not defined)");
    return false;
#endif
  }

  bool IsInitialized() const
  {
#ifdef HAS_ONNXRUNTIME
    return m_initialized;
#else
    return false;
#endif
  }

  Embedding Embed(const std::string& text)
  {
#ifdef HAS_ONNXRUNTIME
    auto batch = EmbedBatch({text});
    return batch.empty() ? Embedding{} : batch[0];
#else
    Embedding result{};
    return result;
#endif
  }

  std::vector<Embedding> EmbedBatch(const std::vector<std::string>& texts)
  {
#ifdef HAS_ONNXRUNTIME
    if (!m_initialized || texts.empty())
      return {};

    try
    {
      const size_t batchSize = texts.size();
      const size_t maxLength = 256;

      // 1. Tokenize all texts
      std::vector<std::vector<int64_t>> allInputIds;
      std::vector<std::vector<int64_t>> allAttentionMask;
      std::vector<std::vector<int64_t>> allTokenTypeIds;

      for (const auto& text : texts)
      {
        // Encode text with special tokens ([CLS] and [SEP])
        auto tokens = m_tokenizer->Encode(text, maxLength);

        // Convert int32_t tokens to int64_t for ONNX
        std::vector<int64_t> inputIds(tokens.begin(), tokens.end());
        std::vector<int64_t> attentionMask(tokens.size(), 1);
        std::vector<int64_t> tokenTypeIds(tokens.size(), 0); // All zeros for single sentence

        // Pad to maxLength
        while (inputIds.size() < maxLength)
        {
          inputIds.push_back(0);       // PAD token ID
          attentionMask.push_back(0);  // Padding tokens are masked
          tokenTypeIds.push_back(0);
        }

        allInputIds.push_back(std::move(inputIds));
        allAttentionMask.push_back(std::move(attentionMask));
        allTokenTypeIds.push_back(std::move(tokenTypeIds));
      }

      // 2. Flatten batch into single vectors for ONNX tensors
      std::vector<int64_t> flatInputIds;
      std::vector<int64_t> flatAttentionMask;
      std::vector<int64_t> flatTokenTypeIds;

      flatInputIds.reserve(batchSize * maxLength);
      flatAttentionMask.reserve(batchSize * maxLength);
      flatTokenTypeIds.reserve(batchSize * maxLength);

      for (size_t i = 0; i < batchSize; ++i)
      {
        flatInputIds.insert(flatInputIds.end(), allInputIds[i].begin(), allInputIds[i].end());
        flatAttentionMask.insert(flatAttentionMask.end(), allAttentionMask[i].begin(),
                                 allAttentionMask[i].end());
        flatTokenTypeIds.insert(flatTokenTypeIds.end(), allTokenTypeIds[i].begin(),
                                allTokenTypeIds[i].end());
      }

      // 3. Create ONNX input tensors
      std::vector<int64_t> inputShape = {static_cast<int64_t>(batchSize),
                                         static_cast<int64_t>(maxLength)};

      std::vector<Ort::Value> inputTensors;
      inputTensors.reserve(3);

      inputTensors.push_back(Ort::Value::CreateTensor<int64_t>(
          m_memoryInfo, flatInputIds.data(), flatInputIds.size(), inputShape.data(),
          inputShape.size()));

      inputTensors.push_back(Ort::Value::CreateTensor<int64_t>(
          m_memoryInfo, flatAttentionMask.data(), flatAttentionMask.size(), inputShape.data(),
          inputShape.size()));

      inputTensors.push_back(Ort::Value::CreateTensor<int64_t>(
          m_memoryInfo, flatTokenTypeIds.data(), flatTokenTypeIds.size(), inputShape.data(),
          inputShape.size()));

      // 4. Run ONNX inference
      auto outputTensors =
          m_session->Run(Ort::RunOptions{nullptr}, m_inputNames.data(), inputTensors.data(),
                         inputTensors.size(), m_outputNames.data(), m_outputNames.size());

      // 5. Extract output tensor
      // Output shape: [batch_size, sequence_length, 384]
      float* outputData = outputTensors[0].GetTensorMutableData<float>();

      // 6. Perform mean pooling over sequence dimension with attention mask
      std::vector<Embedding> embeddings(batchSize);

      for (size_t b = 0; b < batchSize; ++b)
      {
        Embedding& embedding = embeddings[b];
        embedding.fill(0.0f);

        // Count valid (non-padded) tokens
        size_t validTokens = 0;
        for (size_t t = 0; t < maxLength; ++t)
        {
          if (allAttentionMask[b][t] == 1)
          {
            validTokens++;
            // Sum embeddings for valid tokens
            for (size_t h = 0; h < EMBEDDING_DIM; ++h)
            {
              size_t idx = (b * maxLength * EMBEDDING_DIM) + (t * EMBEDDING_DIM) + h;
              embedding[h] += outputData[idx];
            }
          }
        }

        // Compute mean by dividing by number of valid tokens
        if (validTokens > 0)
        {
          for (size_t h = 0; h < EMBEDDING_DIM; ++h)
          {
            embedding[h] /= static_cast<float>(validTokens);
          }
        }

        // 7. L2 normalization
        float norm = 0.0f;
        for (size_t h = 0; h < EMBEDDING_DIM; ++h)
        {
          norm += embedding[h] * embedding[h];
        }
        norm = std::sqrt(norm);

        if (norm > 0.0f)
        {
          for (size_t h = 0; h < EMBEDDING_DIM; ++h)
          {
            embedding[h] /= norm;
          }
        }
      }

      return embeddings;
    }
    catch (const Ort::Exception& e)
    {
      CLog::Log(LOGERROR, "EmbeddingEngine: ONNX inference error: {}", e.what());
      return {};
    }
    catch (const std::exception& e)
    {
      CLog::Log(LOGERROR, "EmbeddingEngine: Batch embedding error: {}", e.what());
      return {};
    }
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
