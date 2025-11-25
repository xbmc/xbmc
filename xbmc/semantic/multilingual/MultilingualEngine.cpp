/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MultilingualEngine.h"

#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <functional>

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

  // TODO: Implement actual model download
  // This would download from a Kodi model repository or HuggingFace
  // For now, just log a message
  CLog::Log(LOGWARNING,
            "MultilingualEngine: Model download not yet implemented. Please manually download "
            "model '{}' to {}",
            modelName, m_modelBasePath);

  // Placeholder: Simulate download progress
  if (progressCallback)
  {
    for (int i = 0; i <= 10; ++i)
    {
      progressCallback(i / 10.0f);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }

  return false;
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
  miniLM.downloaded = IsModelDownloaded(miniLM.name);
  m_availableModels.push_back(miniLM);
  m_modelIndex[miniLM.name] = m_availableModels.size() - 1;

  // Model 2: LaBSE (Language-agnostic BERT Sentence Embedding)
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
  labse.downloaded = IsModelDownloaded(labse.name);
  m_availableModels.push_back(labse);
  m_modelIndex[labse.name] = m_availableModels.size() - 1;

  // Model 3: mUSE (Multilingual Universal Sentence Encoder)
  MultilingualModelInfo muse;
  muse.name = "mUSE";
  muse.displayName = "Multilingual USE (16 languages)";
  muse.languages = {"en", "es", "fr", "de", "it", "pt", "nl", "ru", "zh", "ja", "ko", "ar", "tr",
                    "pl", "th", "vi"};
  muse.dimensions = 512;
  muse.modelPath = URIUtils::AddFileToFolder(m_modelBasePath, muse.name + "/model.onnx");
  muse.vocabPath = URIUtils::AddFileToFolder(m_modelBasePath, muse.name + "/vocab.txt");
  muse.estimatedMemoryMB = 280;
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

} // namespace SEMANTIC
} // namespace KODI
