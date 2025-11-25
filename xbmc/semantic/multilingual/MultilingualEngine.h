/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "LanguageDetector.h"
#include "semantic/embedding/EmbeddingEngine.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * \brief Multilingual embedding model information
 */
struct MultilingualModelInfo
{
  std::string name;                     //!< Model identifier
  std::string displayName;              //!< Human-readable name
  std::vector<std::string> languages;   //!< Supported language codes
  int dimensions;                       //!< Embedding dimensions
  std::string modelPath;                //!< ONNX model file path
  std::string vocabPath;                //!< Vocabulary file path
  size_t estimatedMemoryMB;             //!< Estimated memory usage
  bool downloaded;                      //!< Whether model is available locally

  // Download metadata
  std::string modelUrl;                 //!< Download URL for model file
  std::string vocabUrl;                 //!< Download URL for vocab file
  std::string modelSha256;              //!< Expected SHA256 hash of model file
  std::string vocabSha256;              //!< Expected SHA256 hash of vocab file
  size_t modelSizeBytes;                //!< Size of model file in bytes
  size_t vocabSizeBytes;                //!< Size of vocab file in bytes
};

/*!
 * \brief Multilingual embedding engine for cross-language semantic search
 *
 * This engine manages multiple multilingual embedding models that support
 * cross-lingual semantic search. Users can search in their native language
 * and find content in any language supported by the model.
 *
 * Supported Models:
 * - paraphrase-multilingual-MiniLM-L12-v2 (50+ languages, 384D)
 * - LaBSE (Language-agnostic BERT Sentence Embedding, 109 languages, 768D)
 * - mUSE (Multilingual Universal Sentence Encoder, 16 languages, 512D)
 *
 * Features:
 * - Automatic language detection for queries and content
 * - Shared embedding space across all languages
 * - Cross-lingual similarity search (query in English, find Spanish results)
 * - Language-specific tokenization
 * - Model management with on-demand download
 * - Graceful fallback to English-only models
 *
 * Architecture:
 * 1. Language Detection: Identifies query and content languages
 * 2. Model Selection: Chooses appropriate multilingual model
 * 3. Tokenization: Language-aware text processing
 * 4. Embedding: Generates language-agnostic embeddings
 * 5. Search: Cross-lingual similarity matching
 *
 * Example usage:
 * \code
 * CMultilingualEngine engine;
 * if (engine.Initialize())
 * {
 *   // Query in English, find content in any language
 *   auto embedding = engine.Embed("romantic comedy", "en");
 *
 *   // Auto-detect language and embed
 *   auto embedding2 = engine.EmbedWithDetection("película romántica");
 * }
 * \endcode
 */
class CMultilingualEngine
{
public:
  /*!
   * \brief Constructor
   */
  CMultilingualEngine();

  /*!
   * \brief Destructor
   */
  ~CMultilingualEngine();

  /*!
   * \brief Initialize the multilingual engine
   *
   * Loads configuration, initializes language detector, and prepares
   * the default multilingual model.
   *
   * \param modelBasePath Base directory for model files (default: special://home/semantic/models/multilingual/)
   * \return true if initialization succeeded, false otherwise
   */
  bool Initialize(const std::string& modelBasePath = "");

  /*!
   * \brief Check if the engine is initialized
   *
   * \return true if ready to generate embeddings
   */
  bool IsInitialized() const { return m_initialized; }

  /*!
   * \brief Check if multilingual support is enabled
   *
   * \return true if multilingual mode is active
   */
  bool IsMultilingualEnabled() const;

  /*!
   * \brief Get all available multilingual models
   *
   * \return Vector of model information
   */
  std::vector<MultilingualModelInfo> GetAvailableModels() const;

  /*!
   * \brief Get currently active model
   *
   * \return Model info, or empty if none active
   */
  MultilingualModelInfo GetActiveModel() const;

  /*!
   * \brief Set the active multilingual model
   *
   * Switches to a different multilingual model. If the model is not
   * downloaded, returns false.
   *
   * \param modelName Model identifier (e.g., "paraphrase-multilingual-MiniLM-L12-v2")
   * \return true if model was loaded successfully
   */
  bool SetActiveModel(const std::string& modelName);

  /*!
   * \brief Download a multilingual model
   *
   * Downloads model files from the Kodi model repository.
   * This is a blocking operation that may take several minutes.
   *
   * \param modelName Model identifier
   * \param progressCallback Optional callback for download progress (0.0 to 1.0)
   * \return true if download succeeded
   */
  bool DownloadModel(const std::string& modelName,
                     std::function<void(float)> progressCallback = nullptr);

  /*!
   * \brief Generate embedding with explicit language
   *
   * \param text Input text to embed
   * \param language ISO 639-1 language code (e.g., "en", "es", "zh")
   * \return Embedding vector
   * \throws std::runtime_error if engine is not initialized
   */
  Embedding Embed(const std::string& text, const std::string& language);

  /*!
   * \brief Generate embedding with automatic language detection
   *
   * Detects the language of the input text and generates an embedding.
   * Language detection results are cached internally.
   *
   * \param text Input text to embed
   * \return Embedding vector
   * \throws std::runtime_error if engine is not initialized
   */
  Embedding EmbedWithDetection(const std::string& text);

  /*!
   * \brief Generate embedding with language hint
   *
   * Uses a language hint (e.g., from metadata) to improve detection accuracy.
   *
   * \param text Input text to embed
   * \param languageHint ISO 639-1 language code hint
   * \return Embedding vector
   * \throws std::runtime_error if engine is not initialized
   */
  Embedding EmbedWithHint(const std::string& text, const std::string& languageHint);

  /*!
   * \brief Generate embeddings for multiple texts in batch
   *
   * More efficient than individual calls for multiple texts.
   *
   * \param texts Vector of input texts
   * \param languages Vector of language codes (same length as texts)
   * \return Vector of embedding vectors
   * \throws std::runtime_error if engine is not initialized or sizes mismatch
   */
  std::vector<Embedding> EmbedBatch(const std::vector<std::string>& texts,
                                     const std::vector<std::string>& languages);

  /*!
   * \brief Detect language of text
   *
   * \param text Input text
   * \return Language detection result
   */
  LanguageDetection DetectLanguage(const std::string& text) const;

  /*!
   * \brief Detect language with metadata hint
   *
   * \param text Input text
   * \param languageHint Language hint from metadata
   * \return Language detection result
   */
  LanguageDetection DetectLanguageWithHint(const std::string& text,
                                           const std::string& languageHint) const;

  /*!
   * \brief Check if a language is supported by the active model
   *
   * \param language ISO 639-1 language code
   * \return true if language is supported
   */
  bool IsLanguageSupported(const std::string& language) const;

  /*!
   * \brief Get all supported languages for active model
   *
   * \return Vector of ISO 639-1 language codes
   */
  std::vector<std::string> GetSupportedLanguages() const;

  /*!
   * \brief Enable or disable multilingual support
   *
   * When disabled, falls back to English-only model.
   *
   * \param enabled true to enable multilingual mode
   */
  void SetMultilingualEnabled(bool enabled);

  /*!
   * \brief Get language detection statistics
   *
   * Returns cached language detection results for debugging.
   *
   * \return Map of text hash to detected language
   */
  std::unordered_map<size_t, std::string> GetDetectionCache() const;

  /*!
   * \brief Clear language detection cache
   */
  void ClearDetectionCache();

private:
  /*!
   * \brief Load model metadata from configuration
   */
  void LoadModelMetadata();

  /*!
   * \brief Get model base path with special:// protocol resolution
   *
   * \return Resolved base path
   */
  std::string GetModelBasePath() const;

  /*!
   * \brief Check if a model is downloaded
   *
   * \param modelName Model identifier
   * \return true if model files exist
   */
  bool IsModelDownloaded(const std::string& modelName) const;

  /*!
   * \brief Load a multilingual model
   *
   * \param modelName Model identifier
   * \return true if model loaded successfully
   */
  bool LoadModel(const std::string& modelName);

  /*!
   * \brief Unload current model
   */
  void UnloadCurrentModel();

  /*!
   * \brief Normalize language code to ISO 639-1
   *
   * Handles various formats: "en-US" -> "en", "eng" -> "en"
   *
   * \param language Input language code
   * \return Normalized ISO 639-1 code
   */
  std::string NormalizeLanguageCode(const std::string& language) const;

  /*!
   * \brief Hash text for detection cache
   *
   * \param text Input text
   * \return Hash value
   */
  size_t HashText(const std::string& text) const;

  /*!
   * \brief Download a single file with progress and verification
   *
   * \param url Download URL
   * \param destPath Destination file path
   * \param expectedSha256 Expected SHA256 hash (empty to skip verification)
   * \param expectedSize Expected file size in bytes (0 to skip size check)
   * \param progressCallback Progress callback (0.0 to 1.0)
   * \return true if download and verification succeeded
   */
  bool DownloadFile(const std::string& url,
                    const std::string& destPath,
                    const std::string& expectedSha256,
                    size_t expectedSize,
                    std::function<void(float)> progressCallback);

  /*!
   * \brief Verify file integrity using SHA256
   *
   * \param filePath Path to file to verify
   * \param expectedSha256 Expected SHA256 hash
   * \return true if hash matches
   */
  bool VerifyFileHash(const std::string& filePath, const std::string& expectedSha256) const;

  bool m_initialized{false};
  bool m_multilingualEnabled{true};
  std::string m_modelBasePath;
  std::string m_activeModelName;

  // Language detector
  std::unique_ptr<CLanguageDetector> m_languageDetector;

  // Embedding engines (can support multiple models)
  std::unique_ptr<CEmbeddingEngine> m_currentEngine;

  // Model metadata
  std::vector<MultilingualModelInfo> m_availableModels;
  std::unordered_map<std::string, size_t> m_modelIndex; //!< Quick lookup

  // Language detection cache (text hash -> language)
  mutable std::unordered_map<size_t, std::string> m_detectionCache;
  mutable std::mutex m_cacheMutex;
};

} // namespace SEMANTIC
} // namespace KODI
