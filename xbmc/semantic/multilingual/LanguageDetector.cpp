/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "LanguageDetector.h"

#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <numeric>

namespace KODI
{
namespace SEMANTIC
{

CLanguageDetector::CLanguageDetector() = default;

CLanguageDetector::~CLanguageDetector() = default;

bool CLanguageDetector::Initialize()
{
  if (m_initialized)
    return true;

  try
  {
    CreateLanguageProfiles();

    // Build language index for quick lookup
    m_languageIndex.clear();
    for (size_t i = 0; i < m_profiles.size(); ++i)
    {
      m_languageIndex[m_profiles[i].language] = i;
    }

    m_initialized = true;
    CLog::Log(LOGINFO, "LanguageDetector: Initialized with {} language profiles",
              m_profiles.size());
    return true;
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "LanguageDetector: Initialization error: {}", e.what());
    return false;
  }
}

LanguageDetection CLanguageDetector::Detect(const std::string& text) const
{
  if (!m_initialized || text.empty())
  {
    return LanguageDetection{"en", 0.0f, "Latin"};
  }

  // Detect script first - helps narrow down candidates
  std::string script = DetectScript(text);

  // Normalize and extract trigrams
  std::string normalized = NormalizeText(text);
  auto trigrams = ExtractTrigrams(normalized);

  if (trigrams.empty())
  {
    return LanguageDetection{"en", 0.0f, script};
  }

  // Score against all language profiles
  std::vector<std::pair<std::string, float>> scores;
  scores.reserve(m_profiles.size());

  for (const auto& profile : m_profiles)
  {
    // Skip profiles with mismatched scripts for efficiency
    if (script != "Mixed" && profile.script != script && profile.script != "Latin")
    {
      continue;
    }

    float score = CalculateScore(trigrams, profile);
    scores.emplace_back(profile.language, score);
  }

  // Sort by score descending
  std::sort(scores.begin(), scores.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  if (scores.empty())
  {
    return LanguageDetection{"en", 0.0f, script};
  }

  // Calculate confidence based on score separation
  float topScore = scores[0].second;
  float confidence = topScore;

  // Reduce confidence if second-best score is close
  if (scores.size() > 1)
  {
    float secondScore = scores[1].second;
    float separation = topScore - secondScore;
    confidence = std::min(confidence, separation + 0.5f);
  }

  // Reduce confidence for very short texts
  if (text.length() < 50)
  {
    confidence *= 0.7f;
  }
  else if (text.length() < 100)
  {
    confidence *= 0.85f;
  }

  confidence = std::clamp(confidence, 0.0f, 1.0f);

  CLog::Log(LOGDEBUG, "LanguageDetector: Detected language '{}' with confidence {:.2f} (script: {})",
            scores[0].first, confidence, script);

  return LanguageDetection{scores[0].first, confidence, script};
}

LanguageDetection CLanguageDetector::DetectWithHint(const std::string& text,
                                                     const std::string& languageHint,
                                                     float hintBoost) const
{
  if (!m_initialized || text.empty())
  {
    return LanguageDetection{languageHint, 0.5f, "Latin"};
  }

  // Perform normal detection
  auto result = Detect(text);

  // If hint language matches detected language, boost confidence
  if (result.language == languageHint)
  {
    result.confidence = std::min(1.0f, result.confidence + hintBoost);
    CLog::Log(LOGDEBUG, "LanguageDetector: Hint '{}' matches detection, boosted confidence to {:.2f}",
              languageHint, result.confidence);
  }
  // If hint language is supported and detected confidence is low, use hint
  else if (IsLanguageSupported(languageHint) && result.confidence < 0.6f)
  {
    result.language = languageHint;
    result.confidence = 0.6f;
    CLog::Log(LOGDEBUG, "LanguageDetector: Using hint '{}' due to low detection confidence",
              languageHint);
  }

  return result;
}

std::vector<std::string> CLanguageDetector::GetSupportedLanguages() const
{
  std::vector<std::string> languages;
  languages.reserve(m_profiles.size());

  for (const auto& profile : m_profiles)
  {
    languages.push_back(profile.language);
  }

  return languages;
}

bool CLanguageDetector::IsLanguageSupported(const std::string& language) const
{
  return m_languageIndex.find(language) != m_languageIndex.end();
}

std::string CLanguageDetector::DetectScript(const std::string& text)
{
  int latinCount = 0;
  int cjkCount = 0;
  int arabicCount = 0;
  int cyrillicCount = 0;
  int otherCount = 0;

  for (unsigned char c : text)
  {
    // Check multi-byte UTF-8 characters
    if (c >= 0x80)
    {
      // This is simplified - in production, use proper UTF-8 decoding
      // CJK Unified Ideographs: U+4E00 to U+9FFF
      // Hiragana: U+3040 to U+309F
      // Katakana: U+30A0 to U+30FF
      // Hangul: U+AC00 to U+D7AF
      if (c >= 0xE4 && c <= 0xE9)
        cjkCount++;
      // Cyrillic: U+0400 to U+04FF
      else if (c == 0xD0 || c == 0xD1)
        cyrillicCount++;
      // Arabic: U+0600 to U+06FF
      else if (c == 0xD8 || c == 0xD9 || c == 0xDA || c == 0xDB)
        arabicCount++;
      else
        otherCount++;
    }
    else if (std::isalpha(c))
    {
      latinCount++;
    }
  }

  int total = latinCount + cjkCount + arabicCount + cyrillicCount + otherCount;
  if (total == 0)
    return "Latin";

  // Determine predominant script
  std::vector<std::pair<std::string, int>> scripts = {
      {"Latin", latinCount},
      {"CJK", cjkCount},
      {"Arabic", arabicCount},
      {"Cyrillic", cyrillicCount}
  };

  std::sort(scripts.begin(), scripts.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  // Check if it's a mixed script
  if (scripts[0].second > 0 && scripts[1].second > total * 0.2)
  {
    return "Mixed";
  }

  return scripts[0].first;
}

std::unordered_map<std::string, int> CLanguageDetector::ExtractTrigrams(const std::string& text) const
{
  std::unordered_map<std::string, int> trigrams;

  if (text.length() < 3)
    return trigrams;

  // Add space padding for better trigram extraction
  std::string padded = " " + text + " ";

  // Extract all trigrams
  for (size_t i = 0; i + 3 <= padded.length(); ++i)
  {
    std::string trigram = padded.substr(i, 3);
    trigrams[trigram]++;
  }

  return trigrams;
}

std::string CLanguageDetector::NormalizeText(const std::string& text) const
{
  std::string normalized = text;

  // Convert to lowercase (ASCII only for simplicity)
  std::transform(normalized.begin(), normalized.end(), normalized.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // Normalize whitespace
  normalized = StringUtils::Trim(normalized);

  return normalized;
}

float CLanguageDetector::CalculateScore(const std::unordered_map<std::string, int>& textTrigrams,
                                        const LanguageProfile& profile) const
{
  if (textTrigrams.empty() || profile.trigrams.empty())
    return 0.0f;

  // Calculate cosine similarity between text trigrams and profile
  float dotProduct = 0.0f;
  float textNorm = 0.0f;
  float profileNorm = 0.0f;

  // Calculate text norm
  for (const auto& [trigram, count] : textTrigrams)
  {
    textNorm += count * count;
  }
  textNorm = std::sqrt(textNorm);

  // Calculate profile norm (pre-normalized, so this is 1.0 for normalized profiles)
  for (const auto& [trigram, freq] : profile.trigrams)
  {
    profileNorm += freq * freq;
  }
  profileNorm = std::sqrt(profileNorm);

  // Calculate dot product
  for (const auto& [trigram, count] : textTrigrams)
  {
    auto it = profile.trigrams.find(trigram);
    if (it != profile.trigrams.end())
    {
      dotProduct += count * it->second;
    }
  }

  // Cosine similarity
  if (textNorm > 0.0f && profileNorm > 0.0f)
  {
    return dotProduct / (textNorm * profileNorm);
  }

  return 0.0f;
}

void CLanguageDetector::CreateLanguageProfiles()
{
  // English (en)
  AddLanguageProfile("en", "English", "Latin", {
    {" th", 3.5f}, {"the", 3.2f}, {"he ", 2.8f}, {" an", 2.5f}, {"and", 2.4f},
    {"nd ", 2.2f}, {"ing", 2.0f}, {" of", 1.8f}, {"of ", 1.7f}, {" to", 1.6f}
  });

  // Spanish (es)
  AddLanguageProfile("es", "Spanish", "Latin", {
    {" de", 3.8f}, {"de ", 3.5f}, {" la", 3.2f}, {"la ", 2.9f}, {"el ", 2.5f},
    {" el", 2.4f}, {"os ", 2.0f}, {"ión", 1.8f}, {"aci", 1.6f}, {"ara", 1.5f}
  });

  // French (fr)
  AddLanguageProfile("fr", "French", "Latin", {
    {" de", 3.6f}, {"de ", 3.3f}, {" le", 3.0f}, {"le ", 2.7f}, {"ent", 2.5f},
    {" la", 2.4f}, {"la ", 2.2f}, {"ion", 2.0f}, {" et", 1.8f}, {"et ", 1.7f}
  });

  // German (de)
  AddLanguageProfile("de", "German", "Latin", {
    {" de", 3.4f}, {"der", 3.2f}, {"en ", 2.9f}, {"er ", 2.6f}, {" un", 2.4f},
    {"und", 2.3f}, {"nd ", 2.1f}, {"die", 2.0f}, {"ie ", 1.8f}, {"ein", 1.6f}
  });

  // Italian (it)
  AddLanguageProfile("it", "Italian", "Latin", {
    {" di", 3.7f}, {"di ", 3.4f}, {" la", 3.0f}, {"la ", 2.7f}, {"to ", 2.5f},
    {"ion", 2.3f}, {"oni", 2.0f}, {" co", 1.9f}, {"con", 1.8f}, {"are", 1.6f}
  });

  // Portuguese (pt)
  AddLanguageProfile("pt", "Portuguese", "Latin", {
    {" de", 3.9f}, {"de ", 3.6f}, {" da", 3.2f}, {"da ", 2.8f}, {" do", 2.5f},
    {"do ", 2.3f}, {"os ", 2.1f}, {"ção", 2.0f}, {"ent", 1.8f}, {"ara", 1.6f}
  });

  // Russian (ru)
  AddLanguageProfile("ru", "Russian", "Cyrillic", {
    {" пр", 3.5f}, {"ост", 3.2f}, {"то ", 2.9f}, {" и ", 2.7f}, {"на ", 2.5f},
    {" на", 2.4f}, {"ени", 2.2f}, {"ств", 2.0f}, {"про", 1.9f}, {"что", 1.7f}
  });

  // Chinese (zh)
  AddLanguageProfile("zh", "Chinese", "CJK", {
    {"的", 5.0f}, {"了", 3.5f}, {"和", 3.2f}, {"是", 3.0f}, {"在", 2.8f},
    {"有", 2.5f}, {"个", 2.3f}, {"人", 2.1f}, {"这", 2.0f}, {"中", 1.9f}
  });

  // Japanese (ja)
  AddLanguageProfile("ja", "Japanese", "CJK", {
    {"の", 4.5f}, {"に", 3.8f}, {"は", 3.5f}, {"を", 3.2f}, {"た", 3.0f},
    {"が", 2.8f}, {"で", 2.5f}, {"と", 2.3f}, {"し", 2.1f}, {"て", 2.0f}
  });

  // Korean (ko)
  AddLanguageProfile("ko", "Korean", "CJK", {
    {"이", 4.0f}, {"의", 3.7f}, {"가", 3.4f}, {"을", 3.1f}, {"는", 2.9f},
    {"에", 2.7f}, {"로", 2.5f}, {"와", 2.3f}, {"한", 2.1f}, {"고", 2.0f}
  });

  // Arabic (ar)
  AddLanguageProfile("ar", "Arabic", "Arabic", {
    {"ال", 5.0f}, {"في", 3.5f}, {"من", 3.2f}, {"على", 3.0f}, {"أن", 2.8f},
    {"هذا", 2.5f}, {"إلى", 2.3f}, {"ما", 2.1f}, {"كان", 2.0f}, {"قد", 1.8f}
  });

  // Polish (pl)
  AddLanguageProfile("pl", "Polish", "Latin", {
    {" w ", 3.6f}, {" na", 3.2f}, {"na ", 2.9f}, {" po", 2.6f}, {"ie ", 2.4f},
    {"ów ", 2.2f}, {"prz", 2.0f}, {"owa", 1.9f}, {" si", 1.7f}, {"się", 1.6f}
  });

  // Dutch (nl)
  AddLanguageProfile("nl", "Dutch", "Latin", {
    {" de", 3.7f}, {"de ", 3.4f}, {"en ", 3.1f}, {" he", 2.8f}, {"het", 2.6f},
    {"et ", 2.4f}, {" va", 2.2f}, {"van", 2.1f}, {"an ", 1.9f}, {"een", 1.8f}
  });

  // Swedish (sv)
  AddLanguageProfile("sv", "Swedish", "Latin", {
    {" oc", 3.5f}, {"och", 3.3f}, {"ch ", 3.0f}, {" de", 2.7f}, {"det", 2.5f},
    {"et ", 2.3f}, {"att", 2.1f}, {" at", 2.0f}, {" så", 1.8f}, {"på ", 1.7f}
  });

  // Turkish (tr)
  AddLanguageProfile("tr", "Turkish", "Latin", {
    {"lar", 3.8f}, {"ar ", 3.4f}, {" bi", 3.0f}, {"bir", 2.8f}, {"ir ", 2.5f},
    {"ın ", 2.3f}, {" ve", 2.1f}, {"ve ", 2.0f}, {"ler", 1.9f}, {"er ", 1.7f}
  });

  // Czech (cs)
  AddLanguageProfile("cs", "Czech", "Latin", {
    {" pr", 3.4f}, {"pro", 3.2f}, {" na", 3.0f}, {"na ", 2.7f}, {" se", 2.5f},
    {"se ", 2.3f}, {" po", 2.1f}, {"ost", 2.0f}, {" je", 1.8f}, {"je ", 1.7f}
  });

  // Hebrew (he)
  AddLanguageProfile("he", "Hebrew", "Arabic", {
    {"של", 4.5f}, {"את", 4.0f}, {"על", 3.5f}, {"לא", 3.2f}, {"זה", 3.0f},
    {"אם", 2.7f}, {"מה", 2.5f}, {"כי", 2.3f}, {"יש", 2.1f}, {"או", 2.0f}
  });
}

void CLanguageDetector::AddLanguageProfile(const std::string& language,
                                           const std::string& name,
                                           const std::string& script,
                                           const std::map<std::string, float>& commonTrigrams)
{
  LanguageProfile profile;
  profile.language = language;
  profile.name = name;
  profile.script = script;

  // Copy trigrams to unordered_map for fast lookup
  for (const auto& [trigram, freq] : commonTrigrams)
  {
    profile.trigrams[trigram] = freq;
  }

  m_profiles.push_back(std::move(profile));
}

} // namespace SEMANTIC
} // namespace KODI
