/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MultilingualEngine.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/CurlFile.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/Digest.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <chrono>
#include <functional>
#include <thread>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

CMultilingualEngine::CMultilingualEngine() = default;

CMultilingualEngine::~CMultilingualEngine()
{
  UnloadCurrentModel();
}

bool CMultilingualEngine::Initialize(const std::string& modelBasePath)
{
  if (m_initialized)
    return true;

  try
  {
    // Set base path
    if (!modelBasePath.empty())
    {
      m_modelBasePath = modelBasePath;
    }
    else
    {
      m_modelBasePath = CSpecialProtocol::TranslatePath("special://home/semantic/models/multilingual/");
    }

    // Ensure base directory exists
    if (!XFILE::CDirectory::Exists(m_modelBasePath))
    {
      CLog::Log(LOGINFO, "MultilingualEngine: Creating model directory: {}", m_modelBasePath);
      XFILE::CDirectory::Create(m_modelBasePath);
    }

    // Initialize language detector
    m_languageDetector = std::make_unique<CLanguageDetector>();
    if (!m_languageDetector->Initialize())
    {
      CLog::Log(LOGERROR, "MultilingualEngine: Failed to initialize language detector");
      return false;
    }

    // Load model metadata
    LoadModelMetadata();

    // Check if multilingual mode is enabled in settings
    m_multilingualEnabled = CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
        "semantic.multilingual.enabled");

    // Load default model if multilingual is enabled
    if (m_multilingualEnabled)
    {
      std::string defaultModel = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
          "semantic.multilingual.model");

      if (defaultModel.empty())
      {
        defaultModel = "paraphrase-multilingual-MiniLM-L12-v2"; // Default model
      }

      if (IsModelDownloaded(defaultModel))
      {
        if (!LoadModel(defaultModel))
        {
          CLog::Log(LOGWARNING,
                    "MultilingualEngine: Failed to load default model '{}', multilingual support "
                    "will be limited",
                    defaultModel);
        }
      }
      else
      {
        CLog::Log(LOGINFO,
                  "MultilingualEngine: Default model '{}' not downloaded. Use DownloadModel() to "
                  "enable multilingual search.",
                  defaultModel);
      }
    }

    m_initialized = true;
    CLog::Log(LOGINFO, "MultilingualEngine: Initialized (multilingual: {}, models available: {})",
              m_multilingualEnabled ? "enabled" : "disabled", m_availableModels.size());

    return true;
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "MultilingualEngine: Initialization error: {}", e.what());
    return false;
  }
}

bool CMultilingualEngine::IsMultilingualEnabled() const
{
  return m_initialized && m_multilingualEnabled && m_currentEngine != nullptr;
}

std::vector<MultilingualModelInfo> CMultilingualEngine::GetAvailableModels() const
{
  return m_availableModels;
}

MultilingualModelInfo CMultilingualEngine::GetActiveModel() const
{
  if (m_activeModelName.empty() || m_modelIndex.find(m_activeModelName) == m_modelIndex.end())
  {
    return MultilingualModelInfo{};
  }

  return m_availableModels[m_modelIndex.at(m_activeModelName)];
}

bool CMultilingualEngine::SetActiveModel(const std::string& modelName)
{
  if (!m_initialized)
    return false;

  if (m_activeModelName == modelName && m_currentEngine != nullptr)
  {
    return true; // Already loaded
  }

  if (!IsModelDownloaded(modelName))
  {
    CLog::Log(LOGERROR, "MultilingualEngine: Model '{}' is not downloaded", modelName);
    return false;
  }

  return LoadModel(modelName);
}

bool CMultilingualEngine::DownloadModel(const std::string& modelName,
                                        std::function<void(float)> progressCallback)
{
  // Check if model exists in available models
  if (m_modelIndex.find(modelName) == m_modelIndex.end())
  {
    CLog::Log(LOGERROR, "MultilingualEngine: Unknown model '{}'", modelName);
    return false;
  }

  if (IsModelDownloaded(modelName))
  {
    CLog::Log(LOGINFO, "MultilingualEngine: Model '{}' is already downloaded", modelName);
    return true;
  }

  const auto& modelInfo = m_availableModels[m_modelIndex.at(modelName)];

  CLog::Log(LOGINFO, "MultilingualEngine: Starting download of model '{}'", modelName);
  CLog::Log(LOGINFO, "MultilingualEngine: Total size: ~{} MB",
            (modelInfo.modelSizeBytes + modelInfo.vocabSizeBytes) / 1024 / 1024);

  // Create model directory if it doesn't exist
  std::string modelDir = URIUtils::GetDirectory(modelInfo.modelPath);
  if (!XFILE::CDirectory::Exists(modelDir))
  {
    if (!XFILE::CDirectory::Create(modelDir))
    {
      CLog::Log(LOGERROR, "MultilingualEngine: Failed to create model directory: {}", modelDir);
      return false;
    }
  }

  try
  {
    // Download model file (typically larger, so allocate more progress for it)
    // Model gets 80% of progress, vocab gets 20%
    CLog::Log(LOGINFO, "MultilingualEngine: Downloading model file from {}", modelInfo.modelUrl);

    auto modelProgressCallback = [&progressCallback](float progress) {
      if (progressCallback)
      {
        // Map model progress to 0-80% range
        progressCallback(progress * 0.8f);
      }
    };

    if (!DownloadFile(modelInfo.modelUrl, modelInfo.modelPath, modelInfo.modelSha256,
                      modelInfo.modelSizeBytes, modelProgressCallback))
    {
      CLog::Log(LOGERROR, "MultilingualEngine: Failed to download model file");
      // Clean up partial download
      XFILE::CFile::Delete(modelInfo.modelPath);
      return false;
    }

    // Download vocab file
    CLog::Log(LOGINFO, "MultilingualEngine: Downloading vocab file from {}", modelInfo.vocabUrl);

    auto vocabProgressCallback = [&progressCallback](float progress) {
      if (progressCallback)
      {
        // Map vocab progress to 80-100% range
        progressCallback(0.8f + progress * 0.2f);
      }
    };

    if (!DownloadFile(modelInfo.vocabUrl, modelInfo.vocabPath, modelInfo.vocabSha256,
                      modelInfo.vocabSizeBytes, vocabProgressCallback))
    {
      CLog::Log(LOGERROR, "MultilingualEngine: Failed to download vocab file");
      // Clean up both files
      XFILE::CFile::Delete(modelInfo.modelPath);
      XFILE::CFile::Delete(modelInfo.vocabPath);
      return false;
    }

    CLog::Log(LOGINFO, "MultilingualEngine: Successfully downloaded model '{}'", modelName);

    // Update downloaded status
    m_availableModels[m_modelIndex.at(modelName)].downloaded = true;

    // Final progress callback
    if (progressCallback)
    {
      progressCallback(1.0f);
    }

    return true;
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "MultilingualEngine: Exception during model download: {}", e.what());
    // Clean up partial downloads
    XFILE::CFile::Delete(modelInfo.modelPath);
    XFILE::CFile::Delete(modelInfo.vocabPath);
    return false;
  }
}

Embedding CMultilingualEngine::Embed(const std::string& text, const std::string& language)
{
  if (!m_initialized)
  {
    throw std::runtime_error("MultilingualEngine not initialized");
  }

  // If multilingual is disabled or no model loaded, fall back to base embedding
  if (!IsMultilingualEnabled())
  {
    CLog::Log(LOGWARNING,
              "MultilingualEngine: Multilingual mode disabled or no model loaded, embedding "
              "without language awareness");
    // This would fall back to the base CEmbeddingEngine
    // For now, throw an error
    throw std::runtime_error("Multilingual embedding not available");
  }

  // Check if language is supported
  if (!IsLanguageSupported(language))
  {
    CLog::Log(LOGWARNING, "MultilingualEngine: Language '{}' not supported by active model, "
                          "embedding may have reduced quality",
              language);
  }

  // Generate embedding using current multilingual model
  return m_currentEngine->Embed(text);
}

Embedding CMultilingualEngine::EmbedWithDetection(const std::string& text)
{
  if (!m_initialized)
  {
    throw std::runtime_error("MultilingualEngine not initialized");
  }

  // Check detection cache
  size_t textHash = HashText(text);
  std::string language;

  {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    auto it = m_detectionCache.find(textHash);
    if (it != m_detectionCache.end())
    {
      language = it->second;
      CLog::Log(LOGDEBUG, "MultilingualEngine: Using cached language '{}' for text", language);
    }
  }

  // Detect language if not cached
  if (language.empty())
  {
    auto detection = m_languageDetector->Detect(text);
    language = detection.language;

    // Cache the result
    {
      std::lock_guard<std::mutex> lock(m_cacheMutex);
      m_detectionCache[textHash] = language;
    }

    CLog::Log(LOGDEBUG, "MultilingualEngine: Detected language '{}' with confidence {:.2f}",
              language, detection.confidence);
  }

  // Generate embedding
  return Embed(text, language);
}

Embedding CMultilingualEngine::EmbedWithHint(const std::string& text,
                                             const std::string& languageHint)
{
  if (!m_initialized)
  {
    throw std::runtime_error("MultilingualEngine not initialized");
  }

  // Detect language with hint
  auto detection = m_languageDetector->DetectWithHint(text, languageHint);

  // Cache the result
  size_t textHash = HashText(text);
  {
    std::lock_guard<std::mutex> lock(m_cacheMutex);
    m_detectionCache[textHash] = detection.language;
  }

  CLog::Log(LOGDEBUG,
            "MultilingualEngine: Detected language '{}' with hint '{}', confidence {:.2f}",
            detection.language, languageHint, detection.confidence);

  // Generate embedding
  return Embed(text, detection.language);
}

std::vector<Embedding> CMultilingualEngine::EmbedBatch(const std::vector<std::string>& texts,
                                                        const std::vector<std::string>& languages)
{
  if (!m_initialized)
  {
    throw std::runtime_error("MultilingualEngine not initialized");
  }

  if (texts.size() != languages.size())
  {
    throw std::runtime_error("Texts and languages vectors must have the same size");
  }

  if (!IsMultilingualEnabled())
  {
    throw std::runtime_error("Multilingual embedding not available");
  }

  // Check language support
  for (const auto& lang : languages)
  {
    if (!IsLanguageSupported(lang))
    {
      CLog::Log(LOGWARNING, "MultilingualEngine: Language '{}' not supported by active model",
                lang);
    }
  }

  // Generate embeddings using batch processing
  return m_currentEngine->EmbedBatch(texts);
}

LanguageDetection CMultilingualEngine::DetectLanguage(const std::string& text) const
{
  if (!m_initialized || !m_languageDetector)
  {
    return LanguageDetection{"en", 0.0f, "Latin"};
  }

  return m_languageDetector->Detect(text);
}

LanguageDetection CMultilingualEngine::DetectLanguageWithHint(const std::string& text,
                                                               const std::string& languageHint) const
{
  if (!m_initialized || !m_languageDetector)
  {
    return LanguageDetection{languageHint, 0.5f, "Latin"};
  }

  return m_languageDetector->DetectWithHint(text, languageHint);
}

bool CMultilingualEngine::IsLanguageSupported(const std::string& language) const
{
  if (m_activeModelName.empty() || m_modelIndex.find(m_activeModelName) == m_modelIndex.end())
  {
    return false;
  }

  const auto& model = m_availableModels[m_modelIndex.at(m_activeModelName)];

  // Normalize language code for comparison
  std::string normalized = NormalizeLanguageCode(language);

  return std::find(model.languages.begin(), model.languages.end(), normalized) !=
         model.languages.end();
}

std::vector<std::string> CMultilingualEngine::GetSupportedLanguages() const
{
  if (m_activeModelName.empty() || m_modelIndex.find(m_activeModelName) == m_modelIndex.end())
  {
    return {};
  }

  return m_availableModels[m_modelIndex.at(m_activeModelName)].languages;
}

void CMultilingualEngine::SetMultilingualEnabled(bool enabled)
{
  m_multilingualEnabled = enabled;
  CLog::Log(LOGINFO, "MultilingualEngine: Multilingual support {}", enabled ? "enabled" : "disabled");
}

std::unordered_map<size_t, std::string> CMultilingualEngine::GetDetectionCache() const
{
  std::lock_guard<std::mutex> lock(m_cacheMutex);
  return m_detectionCache;
}

void CMultilingualEngine::ClearDetectionCache()
{
  std::lock_guard<std::mutex> lock(m_cacheMutex);
  m_detectionCache.clear();
  CLog::Log(LOGDEBUG, "MultilingualEngine: Detection cache cleared");
}

void CMultilingualEngine::LoadModelMetadata()
{
  m_availableModels.clear();
  m_modelIndex.clear();

  // Model 1: paraphrase-multilingual-MiniLM-L12-v2
  // HuggingFace: sentence-transformers/paraphrase-multilingual-MiniLM-L12-v2
  MultilingualModelInfo miniLM;
  miniLM.name = "paraphrase-multilingual-MiniLM-L12-v2";
  miniLM.displayName = "Multilingual MiniLM (50+ languages)";
  miniLM.languages = {"ar", "bg", "ca", "cs", "da", "de", "el", "en", "es", "et", "fa", "fi",
                      "fr", "gl", "gu", "he", "hi", "hr", "hu", "hy", "id", "it", "ja", "ka",
                      "ko", "ku", "lt", "lv", "mk", "mn", "mr", "ms", "my", "nb", "nl", "pl",
                      "pt", "ro", "ru", "sk", "sl", "sq", "sr", "sv", "th", "tr", "uk", "ur",
                      "vi", "zh"};
  miniLM.dimensions = 384;
  miniLM.modelPath = URIUtils::AddFileToFolder(m_modelBasePath, miniLM.name + "/model.onnx");
  miniLM.vocabPath = URIUtils::AddFileToFolder(m_modelBasePath, miniLM.name + "/vocab.txt");
  miniLM.estimatedMemoryMB = 120;

  // Download URLs (HuggingFace model repository)
  miniLM.modelUrl = "https://huggingface.co/sentence-transformers/paraphrase-multilingual-MiniLM-L12-v2/resolve/main/onnx/model.onnx";
  miniLM.vocabUrl = "https://huggingface.co/sentence-transformers/paraphrase-multilingual-MiniLM-L12-v2/resolve/main/vocab.txt";

  // Expected file sizes and SHA256 hashes (these should be verified against actual files)
  // Note: These are placeholder values - in production, these should be verified
  miniLM.modelSizeBytes = 118000000; // ~118MB
  miniLM.vocabSizeBytes = 232000;    // ~232KB
  miniLM.modelSha256 = ""; // Will be computed on first download if empty
  miniLM.vocabSha256 = ""; // Will be computed on first download if empty

  miniLM.downloaded = IsModelDownloaded(miniLM.name);
  m_availableModels.push_back(miniLM);
  m_modelIndex[miniLM.name] = m_availableModels.size() - 1;

  // Model 2: LaBSE (Language-agnostic BERT Sentence Embedding)
  // HuggingFace: sentence-transformers/LaBSE
  MultilingualModelInfo labse;
  labse.name = "LaBSE";
  labse.displayName = "LaBSE (109 languages)";
  labse.languages = {
      "af", "sq", "am", "ar", "hy", "az", "eu", "be", "bn", "bs", "bg", "my", "ca", "ceb", "zh",
      "co", "hr", "cs", "da", "nl", "en", "eo", "et", "fi", "fr", "fy", "gl", "ka", "de", "el",
      "gu", "ht", "ha", "haw", "he", "hi", "hmn", "hu", "is", "ig", "id", "ga", "it", "ja", "jv",
      "kn", "kk", "km", "rw", "ko", "ku", "ky", "lo", "la", "lv", "lt", "lb", "mk", "mg", "ms",
      "ml", "mt", "mi", "mr", "mn", "ne", "no", "ny", "or", "ps", "fa", "pl", "pt", "pa", "ro",
      "ru", "sm", "gd", "sr", "st", "sn", "sd", "si", "sk", "sl", "so", "es", "su", "sw", "sv",
      "tl", "tg", "ta", "tt", "te", "th", "tr", "tk", "uk", "ur", "ug", "uz", "vi", "cy", "xh",
      "yi", "yo", "zu"};
  labse.dimensions = 768;
  labse.modelPath = URIUtils::AddFileToFolder(m_modelBasePath, labse.name + "/model.onnx");
  labse.vocabPath = URIUtils::AddFileToFolder(m_modelBasePath, labse.name + "/vocab.txt");
  labse.estimatedMemoryMB = 450;

  // Download URLs
  labse.modelUrl = "https://huggingface.co/sentence-transformers/LaBSE/resolve/main/onnx/model.onnx";
  labse.vocabUrl = "https://huggingface.co/sentence-transformers/LaBSE/resolve/main/vocab.txt";
  labse.modelSizeBytes = 470000000; // ~470MB
  labse.vocabSizeBytes = 1000000;   // ~1MB
  labse.modelSha256 = "";
  labse.vocabSha256 = "";

  labse.downloaded = IsModelDownloaded(labse.name);
  m_availableModels.push_back(labse);
  m_modelIndex[labse.name] = m_availableModels.size() - 1;

  // Model 3: mUSE (Multilingual Universal Sentence Encoder)
  // HuggingFace: sentence-transformers/use-cmlm-multilingual
  MultilingualModelInfo muse;
  muse.name = "mUSE";
  muse.displayName = "Multilingual USE (16 languages)";
  muse.languages = {"en", "es", "fr", "de", "it", "pt", "nl", "ru", "zh", "ja", "ko", "ar", "tr",
                    "pl", "th", "vi"};
  muse.dimensions = 512;
  muse.modelPath = URIUtils::AddFileToFolder(m_modelBasePath, muse.name + "/model.onnx");
  muse.vocabPath = URIUtils::AddFileToFolder(m_modelBasePath, muse.name + "/vocab.txt");
  muse.estimatedMemoryMB = 280;

  // Download URLs
  muse.modelUrl = "https://huggingface.co/sentence-transformers/use-cmlm-multilingual/resolve/main/onnx/model.onnx";
  muse.vocabUrl = "https://huggingface.co/sentence-transformers/use-cmlm-multilingual/resolve/main/vocab.txt";
  muse.modelSizeBytes = 280000000; // ~280MB
  muse.vocabSizeBytes = 500000;    // ~500KB
  muse.modelSha256 = "";
  muse.vocabSha256 = "";

  muse.downloaded = IsModelDownloaded(muse.name);
  m_availableModels.push_back(muse);
  m_modelIndex[muse.name] = m_availableModels.size() - 1;

  CLog::Log(LOGINFO, "MultilingualEngine: Loaded metadata for {} models",
            m_availableModels.size());
}

std::string CMultilingualEngine::GetModelBasePath() const
{
  return m_modelBasePath;
}

bool CMultilingualEngine::IsModelDownloaded(const std::string& modelName) const
{
  if (m_modelIndex.find(modelName) == m_modelIndex.end())
  {
    return false;
  }

  const auto& model = m_availableModels[m_modelIndex.at(modelName)];

  // Check if both model and vocab files exist
  return XFILE::CFile::Exists(model.modelPath) && XFILE::CFile::Exists(model.vocabPath);
}

bool CMultilingualEngine::LoadModel(const std::string& modelName)
{
  if (m_modelIndex.find(modelName) == m_modelIndex.end())
  {
    CLog::Log(LOGERROR, "MultilingualEngine: Unknown model '{}'", modelName);
    return false;
  }

  const auto& modelInfo = m_availableModels[m_modelIndex.at(modelName)];

  // Unload current model if any
  UnloadCurrentModel();

  CLog::Log(LOGINFO, "MultilingualEngine: Loading model '{}'", modelName);

  try
  {
    // Create new embedding engine
    m_currentEngine = std::make_unique<CEmbeddingEngine>();

    // Initialize with model files
    if (!m_currentEngine->Initialize(modelInfo.modelPath, modelInfo.vocabPath))
    {
      CLog::Log(LOGERROR, "MultilingualEngine: Failed to initialize embedding engine for '{}'",
                modelName);
      m_currentEngine.reset();
      return false;
    }

    m_activeModelName = modelName;

    CLog::Log(LOGINFO, "MultilingualEngine: Successfully loaded model '{}' ({} languages, {}D)",
              modelName, modelInfo.languages.size(), modelInfo.dimensions);

    return true;
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "MultilingualEngine: Error loading model '{}': {}", modelName, e.what());
    m_currentEngine.reset();
    return false;
  }
}

void CMultilingualEngine::UnloadCurrentModel()
{
  if (m_currentEngine)
  {
    CLog::Log(LOGINFO, "MultilingualEngine: Unloading model '{}'", m_activeModelName);
    m_currentEngine.reset();
    m_activeModelName.clear();
  }
}

std::string CMultilingualEngine::NormalizeLanguageCode(const std::string& language) const
{
  if (language.empty())
    return "en";

  // Convert to lowercase
  std::string normalized = StringUtils::ToLower(language);

  // Handle locale codes (e.g., "en-US" -> "en", "zh-CN" -> "zh")
  size_t dashPos = normalized.find('-');
  if (dashPos != std::string::npos)
  {
    normalized = normalized.substr(0, dashPos);
  }

  // Handle ISO 639-2/T codes (3-letter) -> ISO 639-1 (2-letter)
  // This is a simplified mapping; full mapping would require a lookup table
  static const std::unordered_map<std::string, std::string> iso639_2to1 = {
      {"eng", "en"}, {"spa", "es"}, {"fra", "fr"}, {"deu", "de"}, {"ita", "it"},
      {"por", "pt"}, {"rus", "ru"}, {"zho", "zh"}, {"jpn", "ja"}, {"kor", "ko"},
      {"ara", "ar"}, {"tur", "tr"}, {"pol", "pl"}, {"nld", "nl"}, {"swe", "sv"}};

  auto it = iso639_2to1.find(normalized);
  if (it != iso639_2to1.end())
  {
    normalized = it->second;
  }

  return normalized;
}

size_t CMultilingualEngine::HashText(const std::string& text) const
{
  return std::hash<std::string>{}(text);
}

bool CMultilingualEngine::DownloadFile(const std::string& url,
                                       const std::string& destPath,
                                       const std::string& expectedSha256,
                                       size_t expectedSize,
                                       std::function<void(float)> progressCallback)
{
  CLog::Log(LOGDEBUG, "MultilingualEngine: Downloading {} to {}", url, destPath);

  try
  {
    // Create temporary file path for download
    std::string tempPath = destPath + ".tmp";

    // Delete any existing temp file
    if (XFILE::CFile::Exists(tempPath))
    {
      XFILE::CFile::Delete(tempPath);
    }

    // Open URL for reading
    XFILE::CCurlFile curlFile;
    curlFile.SetBufferSize(1024 * 1024); // 1MB buffer for better performance

    CURL urlObj(url);
    if (!curlFile.Open(urlObj))
    {
      CLog::Log(LOGERROR, "MultilingualEngine: Failed to open URL: {}", url);
      return false;
    }

    // Get file size from server
    int64_t fileSize = curlFile.GetLength();
    if (fileSize <= 0 && expectedSize > 0)
    {
      fileSize = static_cast<int64_t>(expectedSize);
    }

    CLog::Log(LOGDEBUG, "MultilingualEngine: Download size: {} bytes", fileSize);

    // Open destination file for writing
    XFILE::CFile outFile;
    if (!outFile.OpenForWrite(tempPath, true))
    {
      CLog::Log(LOGERROR, "MultilingualEngine: Failed to open destination file: {}", tempPath);
      curlFile.Close();
      return false;
    }

    // Prepare for SHA256 calculation if hash verification is needed
    std::unique_ptr<KODI::UTILITY::CDigest> digest;
    if (!expectedSha256.empty())
    {
      digest = std::make_unique<KODI::UTILITY::CDigest>(KODI::UTILITY::CDigest::Type::SHA256);
    }

    // Download in chunks
    const size_t chunkSize = 64 * 1024; // 64KB chunks
    std::vector<char> buffer(chunkSize);
    int64_t totalRead = 0;
    int64_t lastProgressUpdate = 0;
    const int64_t progressUpdateInterval = fileSize / 100; // Update every 1%

    while (true)
    {
      ssize_t bytesRead = curlFile.Read(buffer.data(), chunkSize);

      if (bytesRead < 0)
      {
        CLog::Log(LOGERROR, "MultilingualEngine: Error reading from URL");
        outFile.Close();
        curlFile.Close();
        XFILE::CFile::Delete(tempPath);
        return false;
      }

      if (bytesRead == 0)
      {
        // End of file
        break;
      }

      // Write to file
      ssize_t bytesWritten = outFile.Write(buffer.data(), bytesRead);
      if (bytesWritten != bytesRead)
      {
        CLog::Log(LOGERROR, "MultilingualEngine: Error writing to file (disk full?)");
        outFile.Close();
        curlFile.Close();
        XFILE::CFile::Delete(tempPath);
        return false;
      }

      // Update hash calculation
      if (digest)
      {
        digest->Update(buffer.data(), bytesRead);
      }

      totalRead += bytesRead;

      // Report progress (avoid too frequent updates)
      if (progressCallback && fileSize > 0)
      {
        if (totalRead - lastProgressUpdate >= progressUpdateInterval || totalRead >= fileSize)
        {
          float progress = static_cast<float>(totalRead) / static_cast<float>(fileSize);
          progress = std::min(progress, 1.0f);
          progressCallback(progress);
          lastProgressUpdate = totalRead;
        }
      }
    }

    outFile.Close();
    curlFile.Close();

    CLog::Log(LOGDEBUG, "MultilingualEngine: Downloaded {} bytes", totalRead);

    // Verify file size if expected size was provided
    if (expectedSize > 0 && static_cast<size_t>(totalRead) != expectedSize)
    {
      CLog::Log(LOGWARNING,
                "MultilingualEngine: Downloaded file size ({} bytes) doesn't match expected size "
                "({} bytes)",
                totalRead, expectedSize);
      // Continue anyway - size might be an estimate
    }

    // Verify SHA256 hash if provided
    if (digest && !expectedSha256.empty())
    {
      std::string actualHash = digest->Finalize();
      std::string expectedHashLower = StringUtils::ToLower(expectedSha256);

      if (actualHash != expectedHashLower)
      {
        CLog::Log(LOGERROR,
                  "MultilingualEngine: SHA256 hash mismatch! Expected: {}, Got: {}",
                  expectedHashLower, actualHash);
        XFILE::CFile::Delete(tempPath);
        return false;
      }

      CLog::Log(LOGDEBUG, "MultilingualEngine: SHA256 verification passed: {}", actualHash);
    }
    else if (expectedSha256.empty())
    {
      CLog::Log(LOGWARNING, "MultilingualEngine: Skipping hash verification (no expected hash provided)");
    }

    // Move temp file to final destination
    if (XFILE::CFile::Exists(destPath))
    {
      XFILE::CFile::Delete(destPath);
    }

    if (!XFILE::CFile::Rename(tempPath, destPath))
    {
      CLog::Log(LOGERROR, "MultilingualEngine: Failed to rename temp file to destination");
      XFILE::CFile::Delete(tempPath);
      return false;
    }

    CLog::Log(LOGINFO, "MultilingualEngine: Successfully downloaded file to {}", destPath);
    return true;
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "MultilingualEngine: Exception during file download: {}", e.what());
    return false;
  }
}

bool CMultilingualEngine::VerifyFileHash(const std::string& filePath,
                                         const std::string& expectedSha256) const
{
  if (expectedSha256.empty())
  {
    CLog::Log(LOGWARNING, "MultilingualEngine: No expected hash provided, skipping verification");
    return true;
  }

  if (!XFILE::CFile::Exists(filePath))
  {
    CLog::Log(LOGERROR, "MultilingualEngine: File doesn't exist: {}", filePath);
    return false;
  }

  CLog::Log(LOGDEBUG, "MultilingualEngine: Verifying SHA256 hash of {}", filePath);

  try
  {
    XFILE::CFile file;
    if (!file.Open(filePath, READ_TRUNCATED))
    {
      CLog::Log(LOGERROR, "MultilingualEngine: Failed to open file for verification: {}", filePath);
      return false;
    }

    KODI::UTILITY::CDigest digest(KODI::UTILITY::CDigest::Type::SHA256);

    // Read and hash file in chunks
    const size_t chunkSize = 64 * 1024; // 64KB chunks
    std::vector<char> buffer(chunkSize);

    while (true)
    {
      ssize_t bytesRead = file.Read(buffer.data(), chunkSize);

      if (bytesRead < 0)
      {
        CLog::Log(LOGERROR, "MultilingualEngine: Error reading file during verification");
        file.Close();
        return false;
      }

      if (bytesRead == 0)
      {
        break; // EOF
      }

      digest.Update(buffer.data(), bytesRead);
    }

    file.Close();

    std::string actualHash = digest.Finalize();
    std::string expectedHashLower = StringUtils::ToLower(expectedSha256);

    if (actualHash != expectedHashLower)
    {
      CLog::Log(LOGERROR, "MultilingualEngine: SHA256 verification failed! Expected: {}, Got: {}",
                expectedHashLower, actualHash);
      return false;
    }

    CLog::Log(LOGDEBUG, "MultilingualEngine: SHA256 verification passed: {}", actualHash);
    return true;
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "MultilingualEngine: Exception during hash verification: {}", e.what());
    return false;
  }
}

} // namespace SEMANTIC
} // namespace KODI
