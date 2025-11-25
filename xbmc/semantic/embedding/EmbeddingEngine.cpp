/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EmbeddingEngine.h"

#include "GPUAccelerator.h"
#include "Tokenizer.h"
#include "semantic/perf/MemoryManager.h"
#include "semantic/perf/PerformanceMonitor.h"
#include "utils/log.h"

#include <cmath>
#include <numeric>
#include <stdexcept>
#include <thread>

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
  ~Impl()
  {
    StopIdleTimer();
    UnloadModel();
  }

#ifdef HAS_ONNXRUNTIME
  // ONNX Runtime environment and session
  Ort::Env m_env{ORT_LOGGING_LEVEL_WARNING, "SemanticEmbedding"};
  std::unique_ptr<Ort::Session> m_session;
  std::unique_ptr<CTokenizer> m_tokenizer;
  std::unique_ptr<CGPUAccelerator> m_gpuAccelerator;

  Ort::MemoryInfo m_memoryInfo =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  std::vector<const char*> m_inputNames = {"input_ids", "attention_mask", "token_type_ids"};
  std::vector<const char*> m_outputNames = {"last_hidden_state"};

  bool m_initialized{false};
  bool m_modelLoaded{false};
  bool m_lazyLoad{true};
  int m_idleTimeoutSec{300};
  bool m_enableGPU{false};

  std::string m_modelPath;
  std::string m_vocabPath;

  // Idle timer management
  std::atomic<bool> m_stopTimer{false};
  std::thread m_idleTimer;
  std::mutex m_timerMutex;
  std::chrono::steady_clock::time_point m_lastUsed;

  // Memory pressure callback ID
  int m_memoryCallbackId{-1};
#endif

  void StartIdleTimer()
  {
#ifdef HAS_ONNXRUNTIME
    if (m_idleTimeoutSec <= 0)
      return;

    StopIdleTimer();

    m_stopTimer.store(false);
    m_idleTimer = std::thread([this]() {
      while (!m_stopTimer.load())
      {
        std::this_thread::sleep_for(std::chrono::seconds(10));

        if (!m_modelLoaded || m_stopTimer.load())
          continue;

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - m_lastUsed).count();

        if (elapsed >= m_idleTimeoutSec)
        {
          CLog::Log(LOGINFO, "EmbeddingEngine: Idle timeout reached, unloading model");
          UnloadModel();
        }
      }
    });
#endif
  }

  void StopIdleTimer()
  {
#ifdef HAS_ONNXRUNTIME
    m_stopTimer.store(true);
    if (m_idleTimer.joinable())
    {
      m_idleTimer.join();
    }
#endif
  }

  void UpdateLastUsed()
  {
#ifdef HAS_ONNXRUNTIME
    m_lastUsed = std::chrono::steady_clock::now();
#endif
  }

  bool Initialize(const std::string& modelPath,
                  const std::string& vocabPath,
                  bool lazyLoad,
                  int idleTimeoutSec,
                  bool enableGPU)
  {
#ifdef HAS_ONNXRUNTIME
    m_modelPath = modelPath;
    m_vocabPath = vocabPath;
    m_lazyLoad = lazyLoad;
    m_idleTimeoutSec = idleTimeoutSec;
    m_enableGPU = enableGPU;

    try
    {
      // Always load tokenizer (small footprint)
      m_tokenizer = std::make_unique<CTokenizer>();
      if (!m_tokenizer->Load(vocabPath))
      {
        CLog::Log(LOGERROR, "EmbeddingEngine: Failed to load tokenizer vocabulary from '{}'",
                  vocabPath);
        return false;
      }

      CLog::Log(LOGINFO, "EmbeddingEngine: Loaded tokenizer with {} tokens",
                m_tokenizer->GetVocabSize());

      // Initialize GPU acceleration if enabled
      if (m_enableGPU)
      {
        m_gpuAccelerator = std::make_unique<CGPUAccelerator>();

        GPUConfig gpuConfig;
        gpuConfig.enabled = true;
        gpuConfig.preferredBackend = CGPUAccelerator::DetectBestBackend();
        gpuConfig.deviceId = 0; // Use default device
        gpuConfig.memoryLimitMB = 0; // No limit (use all available)
        gpuConfig.batchSize = 32; // Will be adjusted based on device
        gpuConfig.enablePinnedMemory = true;
        gpuConfig.enableAsyncTransfer = true;
        gpuConfig.fallbackToCPU = true; // Always allow CPU fallback

        if (m_gpuAccelerator->Initialize(gpuConfig))
        {
          CLog::Log(LOGINFO, "EmbeddingEngine: GPU acceleration initialized - {}",
                    m_gpuAccelerator->GetStatusMessage());

          if (m_gpuAccelerator->IsAvailable())
          {
            auto device = m_gpuAccelerator->GetActiveDevice();
            CLog::Log(LOGINFO, "EmbeddingEngine: Using GPU: {} ({}, {}MB VRAM)",
                      device.name, GPUBackendToString(device.backend), device.totalMemoryMB);
          }
        }
        else
        {
          CLog::Log(LOGWARNING, "EmbeddingEngine: GPU initialization failed - {}",
                    m_gpuAccelerator->GetStatusMessage());

          // Keep the accelerator for status reporting even if init failed
        }
      }

      m_initialized = true;

      // Register memory pressure callback
      auto& memMgr = CMemoryManager::GetInstance();
      if (memMgr.IsInitialized())
      {
        m_memoryCallbackId = memMgr.RegisterPressureCallback(
            [this](MemoryPressure pressure) -> size_t {
              if (pressure >= MemoryPressure::Medium && m_modelLoaded)
              {
                CLog::Log(LOGINFO,
                          "EmbeddingEngine: Unloading model due to memory pressure (level={})",
                          static_cast<int>(pressure));
                size_t freed = EstimateModelMemory();
                UnloadModel();
                return freed;
              }
              return 0;
            },
            "EmbeddingEngine");
      }

      // Load model immediately if not using lazy loading
      if (!lazyLoad)
      {
        return LoadModel();
      }

      CLog::Log(LOGINFO,
                "EmbeddingEngine: Initialized with lazy loading (idle timeout={}s)",
                idleTimeoutSec);
      return true;
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

  bool LoadModel()
  {
#ifdef HAS_ONNXRUNTIME
    if (m_modelLoaded)
      return true;

    auto startTime = std::chrono::steady_clock::now();

    try
    {
      CLog::Log(LOGINFO, "EmbeddingEngine: Loading ONNX model from '{}'", m_modelPath);

      // Configure ONNX session options with optimizations
      Ort::SessionOptions sessionOptions;
      sessionOptions.SetIntraOpNumThreads(4); // Increased from 2
      sessionOptions.SetInterOpNumThreads(2);
      sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

      // Enable memory pattern optimization
      sessionOptions.EnableMemPattern();
      sessionOptions.EnableCpuMemArena();

      // Configure GPU execution provider if available
      if (m_gpuAccelerator && m_gpuAccelerator->IsAvailable())
      {
        if (m_gpuAccelerator->ConfigureSessionOptions(sessionOptions))
        {
          CLog::Log(LOGINFO, "EmbeddingEngine: GPU execution provider configured");
        }
        else
        {
          CLog::Log(LOGWARNING,
                    "EmbeddingEngine: Failed to configure GPU provider, using CPU fallback");
        }
      }

      // Load ONNX model
      m_session = std::make_unique<Ort::Session>(m_env, m_modelPath.c_str(), sessionOptions);

      m_modelLoaded = true;
      m_lastUsed = std::chrono::steady_clock::now();

      // Calculate load time
      auto endTime = std::chrono::steady_clock::now();
      double loadTimeMs =
          std::chrono::duration<double, std::milli>(endTime - startTime).count();

      // Update performance monitor
      auto& perfMon = CPerformanceMonitor::GetInstance();
      if (perfMon.IsEnabled())
      {
        perfMon.RecordModelLoad(loadTimeMs);
      }

      // Update memory manager
      auto& memMgr = CMemoryManager::GetInstance();
      if (memMgr.IsInitialized())
      {
        size_t modelMemory = EstimateModelMemory();
        memMgr.UpdateComponentMemory(modelMemory, "model");
      }

      // Start idle timer
      StartIdleTimer();

      CLog::Log(LOGINFO, "EmbeddingEngine: Model loaded in {:.2f}ms", loadTimeMs);
      return true;
    }
    catch (const Ort::Exception& e)
    {
      CLog::Log(LOGERROR, "EmbeddingEngine: ONNX Runtime error: {}", e.what());
      m_modelLoaded = false;
      return false;
    }
    catch (const std::exception& e)
    {
      CLog::Log(LOGERROR, "EmbeddingEngine: Model load error: {}", e.what());
      m_modelLoaded = false;
      return false;
    }
#else
    return false;
#endif
  }

  void UnloadModel()
  {
#ifdef HAS_ONNXRUNTIME
    if (!m_modelLoaded)
      return;

    CLog::Log(LOGINFO, "EmbeddingEngine: Unloading model");

    m_session.reset();
    m_modelLoaded = false;

    // Update performance monitor
    auto& perfMon = CPerformanceMonitor::GetInstance();
    if (perfMon.IsEnabled())
    {
      perfMon.RecordModelUnload();
    }

    // Update memory manager
    auto& memMgr = CMemoryManager::GetInstance();
    if (memMgr.IsInitialized())
    {
      memMgr.UpdateComponentMemory(0, "model");
    }
#endif
  }

  size_t EstimateModelMemory() const
  {
    // Rough estimate: all-MiniLM-L6-v2 is ~23MB on disk, ~80MB in memory
    return 80 * 1024 * 1024;
  }

  bool IsInitialized() const
  {
#ifdef HAS_ONNXRUNTIME
    return m_initialized;
#else
    return false;
#endif
  }

  bool IsModelLoaded() const
  {
#ifdef HAS_ONNXRUNTIME
    return m_modelLoaded;
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

    // Lazy load model if needed
    if (!m_modelLoaded)
    {
      if (!LoadModel())
      {
        CLog::Log(LOGERROR, "EmbeddingEngine: Failed to load model for embedding");
        return {};
      }
    }

    // Update last used timestamp
    UpdateLastUsed();

    // Start performance timer
    auto startTime = std::chrono::steady_clock::now();

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

      // Calculate elapsed time and update performance monitor
      auto endTime = std::chrono::steady_clock::now();
      double elapsedMs =
          std::chrono::duration<double, std::milli>(endTime - startTime).count();

      auto& perfMon = CPerformanceMonitor::GetInstance();
      if (perfMon.IsEnabled())
      {
        perfMon.RecordEmbedding(elapsedMs, batchSize);
      }

      CLog::Log(LOGDEBUG, "EmbeddingEngine: Generated {} embeddings in {:.2f}ms ({:.2f}ms/item)",
                batchSize, elapsedMs, elapsedMs / batchSize);

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

bool CEmbeddingEngine::Initialize(const std::string& modelPath,
                                   const std::string& vocabPath,
                                   bool lazyLoad,
                                   int idleTimeoutSec,
                                   bool enableGPU)
{
  return m_impl->Initialize(modelPath, vocabPath, lazyLoad, idleTimeoutSec, enableGPU);
}

bool CEmbeddingEngine::IsInitialized() const
{
  return m_impl->IsInitialized();
}

bool CEmbeddingEngine::IsModelLoaded() const
{
  return m_impl->IsModelLoaded();
}

bool CEmbeddingEngine::LoadModel()
{
  return m_impl->LoadModel();
}

void CEmbeddingEngine::UnloadModel()
{
  m_impl->UnloadModel();
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

bool CEmbeddingEngine::IsGPUEnabled() const
{
#ifdef HAS_ONNXRUNTIME
  return m_impl->m_gpuAccelerator && m_impl->m_gpuAccelerator->IsAvailable();
#else
  return false;
#endif
}

std::string CEmbeddingEngine::GetGPUStatus() const
{
#ifdef HAS_ONNXRUNTIME
  if (!m_impl->m_gpuAccelerator)
  {
    return "GPU acceleration not initialized";
  }
  return m_impl->m_gpuAccelerator->GetStatusMessage();
#else
  return "ONNX Runtime not available";
#endif
}

int CEmbeddingEngine::GetOptimalBatchSize() const
{
#ifdef HAS_ONNXRUNTIME
  if (m_impl->m_gpuAccelerator && m_impl->m_gpuAccelerator->IsAvailable())
  {
    return m_impl->m_gpuAccelerator->GetOptimalBatchSize();
  }
#endif
  // CPU default
  return 4;
}

} // namespace SEMANTIC
} // namespace KODI
