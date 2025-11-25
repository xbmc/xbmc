/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * \brief Language detection result
 */
struct LanguageDetection
{
  std::string language;      //!< ISO 639-1 language code (e.g., "en", "es", "zh")
  float confidence{0.0f};    //!< Confidence score (0.0 to 1.0)
  std::string script;        //!< Writing script (Latin, CJK, Arabic, Cyrillic, etc.)
};

/*!
 * \brief Lightweight language detector using character n-grams
 *
 * This detector uses character trigram frequency analysis to identify the
 * language of text. It's designed to be fast and memory-efficient, suitable
 * for running on every query and content item.
 *
 * Supported languages:
 * - Western European: English, Spanish, French, German, Italian, Portuguese
 * - Eastern European: Russian, Polish, Czech, Ukrainian
 * - Asian: Chinese, Japanese, Korean
 * - Middle Eastern: Arabic, Hebrew, Turkish
 * - Others: Dutch, Swedish, Danish, Norwegian, Finnish
 *
 * Detection approach:
 * 1. Character n-gram extraction (trigrams)
 * 2. Unicode script detection (for CJK, Arabic, etc.)
 * 3. Frequency-based scoring against language profiles
 * 4. Confidence estimation based on score distribution
 *
 * Example usage:
 * \code
 * CLanguageDetector detector;
 * detector.Initialize();
 *
 * auto result = detector.Detect("This is a sample text");
 * if (result.confidence > 0.7f)
 * {
 *   // Use detected language: result.language
 * }
 * \endcode
 */
class CLanguageDetector
{
public:
  /*!
   * \brief Constructor
   */
  CLanguageDetector();

  /*!
   * \brief Destructor
   */
  ~CLanguageDetector();

  /*!
   * \brief Initialize the language detector with built-in profiles
   *
   * Loads character n-gram frequency profiles for all supported languages.
   * This should be called once before any detection operations.
   *
   * \return true if initialization succeeded, false otherwise
   */
  bool Initialize();

  /*!
   * \brief Check if the detector is initialized
   *
   * \return true if ready to detect languages
   */
  bool IsInitialized() const { return m_initialized; }

  /*!
   * \brief Detect the language of a text string
   *
   * Uses character trigram analysis and Unicode script detection to identify
   * the most likely language. For short texts (<50 chars), confidence may be low.
   *
   * \param text Input text to analyze
   * \return Language detection result with language code and confidence
   */
  LanguageDetection Detect(const std::string& text) const;

  /*!
   * \brief Detect language with a hint from metadata
   *
   * Uses a language hint (e.g., from subtitle metadata) to improve detection
   * accuracy. If the hint language scores above the threshold, it's preferred.
   *
   * \param text Input text to analyze
   * \param languageHint ISO 639-1 language code hint (e.g., "en", "es")
   * \param hintBoost Confidence boost for hint language (default: 0.2)
   * \return Language detection result
   */
  LanguageDetection DetectWithHint(const std::string& text,
                                    const std::string& languageHint,
                                    float hintBoost = 0.2f) const;

  /*!
   * \brief Get all supported languages
   *
   * \return Vector of ISO 639-1 language codes
   */
  std::vector<std::string> GetSupportedLanguages() const;

  /*!
   * \brief Check if a language is supported
   *
   * \param language ISO 639-1 language code
   * \return true if language has a detection profile
   */
  bool IsLanguageSupported(const std::string& language) const;

  /*!
   * \brief Get the writing script for a text
   *
   * Identifies the predominant script based on Unicode character ranges:
   * - "Latin" for Western alphabets
   * - "CJK" for Chinese, Japanese, Korean
   * - "Arabic" for Arabic script
   * - "Cyrillic" for Russian, Ukrainian, etc.
   * - "Mixed" for multiple scripts
   *
   * \param text Input text to analyze
   * \return Script name
   */
  static std::string DetectScript(const std::string& text);

private:
  /*!
   * \brief Language profile with trigram frequencies
   */
  struct LanguageProfile
  {
    std::string language;                           //!< ISO 639-1 code
    std::string name;                               //!< Full language name
    std::string script;                             //!< Primary script
    std::unordered_map<std::string, float> trigrams; //!< Trigram frequencies
  };

  /*!
   * \brief Extract character trigrams from text
   *
   * \param text Input text
   * \return Map of trigrams to their frequencies
   */
  std::unordered_map<std::string, int> ExtractTrigrams(const std::string& text) const;

  /*!
   * \brief Normalize text for language detection
   *
   * Converts to lowercase and normalizes whitespace.
   *
   * \param text Input text
   * \return Normalized text
   */
  std::string NormalizeText(const std::string& text) const;

  /*!
   * \brief Calculate similarity score between text trigrams and language profile
   *
   * Uses cosine similarity or rank-order distance.
   *
   * \param textTrigrams Trigrams from input text
   * \param profile Language profile
   * \return Similarity score (0.0 to 1.0)
   */
  float CalculateScore(const std::unordered_map<std::string, int>& textTrigrams,
                       const LanguageProfile& profile) const;

  /*!
   * \brief Create built-in language profiles
   *
   * Generates character trigram frequency profiles for all supported languages.
   * These are simplified profiles based on common character sequences.
   */
  void CreateLanguageProfiles();

  /*!
   * \brief Add a language profile
   *
   * \param language ISO 639-1 code
   * \param name Full language name
   * \param script Primary script
   * \param commonTrigrams Common trigrams with relative frequencies
   */
  void AddLanguageProfile(const std::string& language,
                          const std::string& name,
                          const std::string& script,
                          const std::map<std::string, float>& commonTrigrams);

  bool m_initialized{false};
  std::vector<LanguageProfile> m_profiles;
  std::unordered_map<std::string, size_t> m_languageIndex; //!< Quick lookup by code
};

} // namespace SEMANTIC
} // namespace KODI
