/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CrossEncoder.h"

#include "semantic/embedding/Tokenizer.h"
#include "semantic/perf/MemoryManager.h"
#include "semantic/perf/PerformanceMonitor.h"
#include "utils/log.h"

#include <algorithm>
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

// Maximum sequence length for cross-encoder input (query + passage)
constexpr int MAX_SEQUENCE_LENGTH = 512;

// PIMPL implementation class
class CCrossEncoder::Impl
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
  Ort::Env m_env{ORT_LOGGING_LEVEL_WARNING, "CrossEncoder"};
  std::unique_ptr<Ort::Session> m_session;
  std::unique_ptr<CTokenizer> m_tokenizer;

  Ort::MemoryInfo m_memoryInfo =
      Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);

  std::vector<const char*> m_inputNames = {"input_ids", "attention_mask", "token_type_ids"};
  std::vector<const char*> m_outputNames = {"logits"};

  bool m_initialized{false};
  bool m_modelLoaded{false};
  CrossEncoderConfig m_config;

  std::string m_modelPath;
  std::string m_vocabPath;

  // Statistics
  Stats m_stats;

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
    if (m_config.idleTimeoutSec <= 0)
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

        if (elapsed >= m_config.idleTimeoutSec)
        {
          CLog::Log(LOGINFO, "CrossEncoder: Idle timeout reached, unloading model");
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

  bool Initialize(const std::string& modelPath, const std::string& vocabPath)
  {
#ifdef HAS_ONNXRUNTIME
    m_modelPath = modelPath;
    m_vocabPath = vocabPath;

    try
    {
      // Always load tokenizer (small footprint)
      m_tokenizer = std::make_unique<CTokenizer>();
      if (!m_tokenizer->Load(vocabPath))
      {
        CLog::Log(LOGERROR, "CrossEncoder: Failed to load tokenizer vocabulary from '{}'",
                  vocabPath);
        return false;
      }

      CLog::Log(LOGINFO, "CrossEncoder: Loaded tokenizer with {} tokens",
                m_tokenizer->GetVocabSize());

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
                          "CrossEncoder: Unloading model due to memory pressure (level={})",
                          static_cast<int>(pressure));
                size_t freed = EstimateModelMemory();
                UnloadModel();
                return freed;
              }
              return 0;
            },
            "CrossEncoder");
      }

      // Load model immediately if not using lazy loading
      if (!m_config.lazyLoad)
      {
        return LoadModel();
      }

      CLog::Log(LOGINFO, "CrossEncoder: Initialized with lazy loading (idle timeout={}s)",
                m_config.idleTimeoutSec);
      return true;
    }
    catch (const std::exception& e)
    {
      CLog::Log(LOGERROR, "CrossEncoder: Initialization error: {}", e.what());
      return false;
    }
#else
    CLog::Log(LOGERROR,
              "CrossEncoder: ONNX Runtime support not available (HAS_ONNXRUNTIME not defined)");
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
      CLog::Log(LOGINFO, "CrossEncoder: Loading ONNX model from '{}'", m_modelPath);

      // Configure ONNX session options with optimizations
      Ort::SessionOptions sessionOptions;
      sessionOptions.SetIntraOpNumThreads(4);
      sessionOptions.SetInterOpNumThreads(2);
      sessionOptions.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);

      // Enable memory pattern optimization
      sessionOptions.EnableMemPattern();
      sessionOptions.EnableCpuMemArena();

      // GPU support (if enabled and available) - future implementation
      if (m_config.useGPU)
      {
        // TODO: Add CUDA/DirectML provider when GPU acceleration is ready
        CLog::Log(LOGWARNING, "CrossEncoder: GPU acceleration requested but not yet implemented");
      }

      // Load ONNX model
      m_session = std::make_unique<Ort::Session>(m_env, m_modelPath.c_str(), sessionOptions);

      m_modelLoaded = true;
      m_lastUsed = std::chrono::steady_clock::now();
      m_stats.modelLoadCount++;

      // Calculate load time
      auto endTime = std::chrono::steady_clock::now();
      double loadTimeMs = std::chrono::duration<double, std::milli>(endTime - startTime).count();

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
        memMgr.UpdateComponentMemory(modelMemory, "cross_encoder_model");
      }

      // Start idle timer
      StartIdleTimer();

      CLog::Log(LOGINFO, "CrossEncoder: Model loaded in {:.2f}ms", loadTimeMs);
      return true;
    }
    catch (const Ort::Exception& e)
    {
      CLog::Log(LOGERROR, "CrossEncoder: ONNX Runtime error: {}", e.what());
      m_modelLoaded = false;
      return false;
    }
    catch (const std::exception& e)
    {
      CLog::Log(LOGERROR, "CrossEncoder: Model load error: {}", e.what());
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

    CLog::Log(LOGINFO, "CrossEncoder: Unloading model");

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
      memMgr.UpdateComponentMemory(0, "cross_encoder_model");
    }
#endif
  }

  size_t EstimateModelMemory() const
  {
    // Cross-encoder models are typically larger than bi-encoders
    // ms-marco-MiniLM-L-6-v2: ~23MB on disk, ~90MB in memory
    return 90 * 1024 * 1024;
  }

  std::vector<float> ScoreBatch(const std::vector<std::pair<std::string, std::string>>& pairs)
  {
#ifdef HAS_ONNXRUNTIME
    if (!m_initialized)
      throw std::runtime_error("CrossEncoder not initialized");

    if (pairs.empty())
      return {};

    // Ensure model is loaded
    if (!m_modelLoaded && !LoadModel())
    {
      CLog::Log(LOGERROR, "CrossEncoder: Failed to load model for scoring");
      return std::vector<float>(pairs.size(), 0.5f); // Fallback to neutral scores
    }

    UpdateLastUsed();

    auto startTime = std::chrono::steady_clock::now();

    try
    {
      std::vector<float> allScores;
      allScores.reserve(pairs.size());

      // Process in batches
      for (size_t i = 0; i < pairs.size(); i += m_config.batchSize)
      {
        size_t batchEnd = std::min(i + m_config.batchSize, pairs.size());
        size_t batchSize = batchEnd - i;

        // Prepare batch inputs
        std::vector<int64_t> inputIds;
        std::vector<int64_t> attentionMask;
        std::vector<int64_t> tokenTypeIds;

        inputIds.reserve(batchSize * MAX_SEQUENCE_LENGTH);
        attentionMask.reserve(batchSize * MAX_SEQUENCE_LENGTH);
        tokenTypeIds.reserve(batchSize * MAX_SEQUENCE_LENGTH);

        // Tokenize each pair in batch
        for (size_t j = i; j < batchEnd; ++j)
        {
          const auto& [query, passage] = pairs[j];

          // Tokenize query and passage separately
          auto queryTokens = m_tokenizer->Tokenize(query);
          auto passageTokens = m_tokenizer->Tokenize(passage);

          // Build sequence: [CLS] query [SEP] passage [SEP]
          std::vector<int64_t> sequence;
          std::vector<int64_t> typeIds;

          // Add [CLS] token
          sequence.push_back(m_tokenizer->GetCLSToken());
          typeIds.push_back(0);

          // Add query tokens (type 0)
          for (int64_t token : queryTokens)
          {
            sequence.push_back(token);
            typeIds.push_back(0);
          }

          // Add [SEP] token
          sequence.push_back(m_tokenizer->GetSEPToken());
          typeIds.push_back(0);

          // Add passage tokens (type 1)
          for (int64_t token : passageTokens)
          {
            sequence.push_back(token);
            typeIds.push_back(1);
          }

          // Add final [SEP] token
          sequence.push_back(m_tokenizer->GetSEPToken());
          typeIds.push_back(1);

          // Truncate to MAX_SEQUENCE_LENGTH if needed
          if (sequence.size() > MAX_SEQUENCE_LENGTH)
          {
            sequence.resize(MAX_SEQUENCE_LENGTH);
            typeIds.resize(MAX_SEQUENCE_LENGTH);
            sequence[MAX_SEQUENCE_LENGTH - 1] = m_tokenizer->GetSEPToken();
          }

          // Pad to MAX_SEQUENCE_LENGTH
          size_t seqLen = sequence.size();
          while (sequence.size() < MAX_SEQUENCE_LENGTH)
          {
            sequence.push_back(m_tokenizer->GetPADToken());
            typeIds.push_back(0);
          }

          // Create attention mask (1 for real tokens, 0 for padding)
          std::vector<int64_t> mask(MAX_SEQUENCE_LENGTH, 0);
          for (size_t k = 0; k < seqLen; ++k)
            mask[k] = 1;

          // Add to batch
          inputIds.insert(inputIds.end(), sequence.begin(), sequence.end());
          attentionMask.insert(attentionMask.end(), mask.begin(), mask.end());
          tokenTypeIds.insert(tokenTypeIds.end(), typeIds.begin(), typeIds.end());
        }

        // Create ONNX tensors
        std::vector<int64_t> shape{static_cast<int64_t>(batchSize),
                                    static_cast<int64_t>(MAX_SEQUENCE_LENGTH)};

        auto inputIdsTensor =
            Ort::Value::CreateTensor<int64_t>(m_memoryInfo, inputIds.data(), inputIds.size(),
                                               shape.data(), shape.size());

        auto attentionMaskTensor =
            Ort::Value::CreateTensor<int64_t>(m_memoryInfo, attentionMask.data(),
                                               attentionMask.size(), shape.data(), shape.size());

        auto tokenTypeIdsTensor =
            Ort::Value::CreateTensor<int64_t>(m_memoryInfo, tokenTypeIds.data(),
                                               tokenTypeIds.size(), shape.data(), shape.size());

        // Run inference
        std::vector<Ort::Value> inputTensors;
        inputTensors.push_back(std::move(inputIdsTensor));
        inputTensors.push_back(std::move(attentionMaskTensor));
        inputTensors.push_back(std::move(tokenTypeIdsTensor));

        auto outputTensors = m_session->Run(Ort::RunOptions{nullptr}, m_inputNames.data(),
                                            inputTensors.data(), inputTensors.size(),
                                            m_outputNames.data(), m_outputNames.size());

        // Extract scores from output tensor
        float* outputData = outputTensors[0].GetTensorMutableData<float>();
        auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();

        // For cross-encoders, output is typically [batch_size, 1] or [batch_size, 2]
        // For binary classification (relevant/not relevant), we take the positive class score
        for (size_t j = 0; j < batchSize; ++j)
        {
          float logit = outputData[j * outputShape[1]]; // First element or use softmax
          if (outputShape[1] == 2)
          {
            // Binary classification: use positive class (index 1)
            logit = outputData[j * 2 + 1];
          }
          float score = CCrossEncoder::NormalizeScore(logit);
          allScores.push_back(score);
        }
      }

      // Update statistics
      auto endTime = std::chrono::steady_clock::now();
      double inferenceTimeMs =
          std::chrono::duration<double, std::milli>(endTime - startTime).count();

      m_stats.totalInferences++;
      m_stats.totalPairs += pairs.size();
      m_stats.avgInferenceTimeMs = (m_stats.avgInferenceTimeMs * (m_stats.totalInferences - 1) +
                                    inferenceTimeMs) /
                                   m_stats.totalInferences;

      // Update performance monitor
      auto& perfMon = CPerformanceMonitor::GetInstance();
      if (perfMon.IsEnabled())
      {
        perfMon.RecordInference(inferenceTimeMs, pairs.size());
      }

      CLog::Log(LOGDEBUG, "CrossEncoder: Scored {} pairs in {:.2f}ms ({:.2f}ms per pair)",
                pairs.size(), inferenceTimeMs, inferenceTimeMs / pairs.size());

      return allScores;
    }
    catch (const Ort::Exception& e)
    {
      CLog::Log(LOGERROR, "CrossEncoder: ONNX Runtime error during inference: {}", e.what());
      return std::vector<float>(pairs.size(), 0.5f); // Fallback to neutral scores
    }
    catch (const std::exception& e)
    {
      CLog::Log(LOGERROR, "CrossEncoder: Inference error: {}", e.what());
      return std::vector<float>(pairs.size(), 0.5f);
    }
#else
    CLog::Log(LOGERROR, "CrossEncoder: ONNX Runtime support not available");
    return std::vector<float>(pairs.size(), 0.5f);
#endif
  }
};

// CCrossEncoder implementation

CCrossEncoder::CCrossEncoder() : m_impl(std::make_unique<Impl>())
{
}

CCrossEncoder::CCrossEncoder(const CrossEncoderConfig& config) : m_impl(std::make_unique<Impl>())
{
#ifdef HAS_ONNXRUNTIME
  m_impl->m_config = config;
#endif
  ValidateConfig();
}

CCrossEncoder::~CCrossEncoder() = default;

bool CCrossEncoder::Initialize()
{
#ifdef HAS_ONNXRUNTIME
  if (m_impl->m_config.modelPath.empty())
  {
    CLog::Log(LOGERROR, "CrossEncoder: Model path not set");
    return false;
  }

  // Derive vocab path from model path (same directory, vocab.txt)
  std::string vocabPath = m_impl->m_config.modelPath;
  size_t lastSlash = vocabPath.find_last_of("/\\");
  if (lastSlash != std::string::npos)
  {
    vocabPath = vocabPath.substr(0, lastSlash + 1) + "vocab.txt";
  }
  else
  {
    vocabPath = "vocab.txt";
  }

  return m_impl->Initialize(m_impl->m_config.modelPath, vocabPath);
#else
  return false;
#endif
}

bool CCrossEncoder::Initialize(const std::string& modelPath, const std::string& vocabPath)
{
#ifdef HAS_ONNXRUNTIME
  m_impl->m_config.modelPath = modelPath;
  return m_impl->Initialize(modelPath, vocabPath);
#else
  return false;
#endif
}

bool CCrossEncoder::IsInitialized() const
{
#ifdef HAS_ONNXRUNTIME
  return m_impl->m_initialized;
#else
  return false;
#endif
}

bool CCrossEncoder::IsModelLoaded() const
{
#ifdef HAS_ONNXRUNTIME
  return m_impl->m_modelLoaded;
#else
  return false;
#endif
}

bool CCrossEncoder::LoadModel()
{
#ifdef HAS_ONNXRUNTIME
  return m_impl->LoadModel();
#else
  return false;
#endif
}

void CCrossEncoder::UnloadModel()
{
#ifdef HAS_ONNXRUNTIME
  m_impl->UnloadModel();
#endif
}

void CCrossEncoder::SetConfig(const CrossEncoderConfig& config)
{
#ifdef HAS_ONNXRUNTIME
  m_impl->m_config = config;
  ValidateConfig();
#endif
}

const CrossEncoderConfig& CCrossEncoder::GetConfig() const
{
#ifdef HAS_ONNXRUNTIME
  return m_impl->m_config;
#else
  static CrossEncoderConfig defaultConfig;
  return defaultConfig;
#endif
}

float CCrossEncoder::Score(const std::string& query, const std::string& passage)
{
  std::vector<std::pair<std::string, std::string>> pairs = {{query, passage}};
  auto scores = ScoreBatch(pairs);
  return scores.empty() ? 0.5f : scores[0];
}

std::vector<float> CCrossEncoder::ScoreBatch(
    const std::vector<std::pair<std::string, std::string>>& pairs)
{
#ifdef HAS_ONNXRUNTIME
  return m_impl->ScoreBatch(pairs);
#else
  return std::vector<float>(pairs.size(), 0.5f);
#endif
}

std::vector<CrossEncoderResult> CCrossEncoder::ReRank(const std::vector<QueryPassagePair>& pairs)
{
#ifdef HAS_ONNXRUNTIME
  return ReRankWithWeight(pairs, m_impl->m_config.scoreWeight);
#else
  // Fallback: return original ordering with original scores
  std::vector<CrossEncoderResult> results;
  results.reserve(pairs.size());
  for (const auto& pair : pairs)
  {
    CrossEncoderResult result;
    result.id = pair.id;
    result.crossEncoderScore = 0.5f;
    result.originalScore = pair.originalScore;
    result.finalScore = pair.originalScore;
    results.push_back(result);
  }
  return results;
#endif
}

std::vector<CrossEncoderResult> CCrossEncoder::ReRankWithWeight(
    const std::vector<QueryPassagePair>& pairs,
    float scoreWeight)
{
  if (pairs.empty())
    return {};

#ifdef HAS_ONNXRUNTIME
  // Clamp scoreWeight to [0, 1]
  scoreWeight = std::max(0.0f, std::min(1.0f, scoreWeight));

  // Extract query-passage pairs for scoring
  std::vector<std::pair<std::string, std::string>> scoringPairs;
  scoringPairs.reserve(pairs.size());
  for (const auto& pair : pairs)
  {
    scoringPairs.emplace_back(pair.query, pair.passage);
  }

  // Get cross-encoder scores
  auto crossScores = ScoreBatch(scoringPairs);

  // Combine scores and create results
  std::vector<CrossEncoderResult> results;
  results.reserve(pairs.size());

  double totalScoreChange = 0.0;

  for (size_t i = 0; i < pairs.size(); ++i)
  {
    CrossEncoderResult result;
    result.id = pairs[i].id;
    result.crossEncoderScore = crossScores[i];
    result.originalScore = pairs[i].originalScore;

    // Blend scores: finalScore = (1 - weight) * original + weight * crossEncoder
    result.finalScore = (1.0f - scoreWeight) * result.originalScore +
                        scoreWeight * result.crossEncoderScore;

    // Track score impact for statistics
    totalScoreChange += std::abs(result.finalScore - result.originalScore);

    results.push_back(result);
  }

  // Update statistics
  if (!results.empty())
  {
    double avgChange = totalScoreChange / results.size();
    m_impl->m_stats.avgScoreImpact =
        (m_impl->m_stats.avgScoreImpact * (m_impl->m_stats.totalInferences - 1) + avgChange) /
        m_impl->m_stats.totalInferences;
  }

  // Sort by final score (descending)
  std::sort(results.begin(), results.end(),
            [](const CrossEncoderResult& a, const CrossEncoderResult& b) {
              return a.finalScore > b.finalScore;
            });

  return results;
#else
  // Fallback: return original ordering
  std::vector<CrossEncoderResult> results;
  results.reserve(pairs.size());
  for (const auto& pair : pairs)
  {
    CrossEncoderResult result;
    result.id = pair.id;
    result.crossEncoderScore = 0.5f;
    result.originalScore = pair.originalScore;
    result.finalScore = pair.originalScore;
    results.push_back(result);
  }
  return results;
#endif
}

CCrossEncoder::Stats CCrossEncoder::GetStats() const
{
#ifdef HAS_ONNXRUNTIME
  return m_impl->m_stats;
#else
  return Stats{};
#endif
}

void CCrossEncoder::ResetStats()
{
#ifdef HAS_ONNXRUNTIME
  m_impl->m_stats = Stats{};
#endif
}

float CCrossEncoder::NormalizeScore(float logit)
{
  // Apply sigmoid function to convert logit to probability [0, 1]
  // sigmoid(x) = 1 / (1 + exp(-x))
  return 1.0f / (1.0f + std::exp(-logit));
}

void CCrossEncoder::ValidateConfig()
{
#ifdef HAS_ONNXRUNTIME
  // Clamp topN to reasonable range
  if (m_impl->m_config.topN < 1)
    m_impl->m_config.topN = 1;
  if (m_impl->m_config.topN > 100)
    m_impl->m_config.topN = 100;

  // Clamp scoreWeight to [0, 1]
  if (m_impl->m_config.scoreWeight < 0.0f)
    m_impl->m_config.scoreWeight = 0.0f;
  if (m_impl->m_config.scoreWeight > 1.0f)
    m_impl->m_config.scoreWeight = 1.0f;

  // Clamp batchSize to reasonable range
  if (m_impl->m_config.batchSize < 1)
    m_impl->m_config.batchSize = 1;
  if (m_impl->m_config.batchSize > 32)
    m_impl->m_config.batchSize = 32;

  // Validate idleTimeoutSec
  if (m_impl->m_config.idleTimeoutSec < 0)
    m_impl->m_config.idleTimeoutSec = 0;
#endif
}

} // namespace SEMANTIC
} // namespace KODI
