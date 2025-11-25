/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Tokenizer.h"

#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <unordered_map>

namespace KODI
{
namespace SEMANTIC
{

namespace
{

/*!
 * \brief Convert string to lowercase
 */
std::string ToLowercase(const std::string& text)
{
  std::string result;
  result.reserve(text.size());
  for (char c : text)
  {
    result.push_back(std::tolower(static_cast<unsigned char>(c)));
  }
  return result;
}

/*!
 * \brief Check if character is whitespace
 */
bool IsWhitespace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

/*!
 * \brief Check if character is control character
 */
bool IsControl(char c)
{
  unsigned char uc = static_cast<unsigned char>(c);
  // Control characters: 0x00-0x1F, 0x7F-0x9F
  return (uc < 0x20 && uc != '\t' && uc != '\n' && uc != '\r') || (uc >= 0x7F && uc < 0xA0);
}

/*!
 * \brief Check if character is punctuation
 */
bool IsPunctuation(char c)
{
  unsigned char uc = static_cast<unsigned char>(c);
  // Punctuation: !"#$%&'()*+,-./:;<=>?@[\]^_`{|}~
  return (uc >= 33 && uc <= 47) ||   // !"#$%&'()*+,-./
         (uc >= 58 && uc <= 64) ||   // :;<=>?@
         (uc >= 91 && uc <= 96) ||   // [\]^_`
         (uc >= 123 && uc <= 126);   // {|}~
}

/*!
 * \brief Clean and normalize text
 */
std::string CleanText(const std::string& text)
{
  std::string result;
  result.reserve(text.size());

  for (char c : text)
  {
    // Remove control characters
    if (IsControl(c))
      continue;

    // Normalize whitespace to single space
    if (IsWhitespace(c))
    {
      result.push_back(' ');
    }
    else
    {
      result.push_back(c);
    }
  }

  return result;
}

/*!
 * \brief Split text into words and punctuation
 */
std::vector<std::string> WhitespaceTokenize(const std::string& text)
{
  std::vector<std::string> tokens;
  std::string current;

  for (size_t i = 0; i < text.size(); ++i)
  {
    char c = text[i];

    if (IsWhitespace(c))
    {
      if (!current.empty())
      {
        tokens.push_back(current);
        current.clear();
      }
    }
    else if (IsPunctuation(c))
    {
      // Split on punctuation
      if (!current.empty())
      {
        tokens.push_back(current);
        current.clear();
      }
      tokens.push_back(std::string(1, c));
    }
    else
    {
      current.push_back(c);
    }
  }

  if (!current.empty())
  {
    tokens.push_back(current);
  }

  return tokens;
}

} // anonymous namespace

class CTokenizer::Impl
{
public:
  // Vocabulary: token -> ID
  std::unordered_map<std::string, int32_t> m_vocab;

  // Reverse vocabulary: ID -> token
  std::vector<std::string> m_idToToken;

  // Special token IDs
  int32_t m_padTokenId = 0;
  int32_t m_unkTokenId = 100;
  int32_t m_clsTokenId = 101;
  int32_t m_sepTokenId = 102;
  int32_t m_maskTokenId = 103;

  bool m_loaded = false;

  /*!
   * \brief Apply WordPiece algorithm to split a word
   *
   * This implements the greedy longest-match-first algorithm used by BERT.
   * It tries to find the longest subword in the vocabulary, starting from
   * the beginning of the word.
   *
   * \param word Word to split (should be lowercase)
   * \param unkToken Token to use for unknown subwords
   * \return Vector of subword tokens
   */
  std::vector<std::string> WordPiece(const std::string& word, const std::string& unkToken)
  {
    std::vector<std::string> outputTokens;

    if (word.empty())
      return outputTokens;

    // If the whole word is in vocab, return it
    if (m_vocab.find(word) != m_vocab.end())
    {
      outputTokens.push_back(word);
      return outputTokens;
    }

    size_t start = 0;
    bool isBad = false;

    while (start < word.size())
    {
      size_t end = word.size();
      std::string curSubstr;
      bool foundSubstr = false;

      // Greedy longest-match-first
      while (start < end)
      {
        std::string substr = word.substr(start, end - start);

        // Add ## prefix for subword tokens (not the first token)
        if (start > 0)
        {
          substr = "##" + substr;
        }

        // Check if this substring is in vocabulary
        if (m_vocab.find(substr) != m_vocab.end())
        {
          curSubstr = substr;
          foundSubstr = true;
          break;
        }

        end--;
      }

      // If no substring found, mark as bad
      if (!foundSubstr)
      {
        isBad = true;
        break;
      }

      outputTokens.push_back(curSubstr);
      start = end;
    }

    // If we couldn't tokenize the word, return unknown token
    if (isBad)
    {
      outputTokens.clear();
      outputTokens.push_back(unkToken);
    }

    return outputTokens;
  }

  /*!
   * \brief Tokenize text into subword tokens
   */
  std::vector<std::string> Tokenize(const std::string& text)
  {
    // Clean and normalize
    std::string cleaned = CleanText(text);
    std::string lowered = ToLowercase(cleaned);

    // Split into words
    std::vector<std::string> words = WhitespaceTokenize(lowered);

    // Apply WordPiece to each word
    std::vector<std::string> tokens;
    for (const auto& word : words)
    {
      if (word.empty())
        continue;

      auto subwords = WordPiece(word, "[UNK]");
      tokens.insert(tokens.end(), subwords.begin(), subwords.end());
    }

    return tokens;
  }

  /*!
   * \brief Convert tokens to IDs
   */
  std::vector<int32_t> ConvertTokensToIds(const std::vector<std::string>& tokens)
  {
    std::vector<int32_t> ids;
    ids.reserve(tokens.size());

    for (const auto& token : tokens)
    {
      auto it = m_vocab.find(token);
      if (it != m_vocab.end())
      {
        ids.push_back(it->second);
      }
      else
      {
        ids.push_back(m_unkTokenId);
      }
    }

    return ids;
  }
};

CTokenizer::CTokenizer() : m_impl(std::make_unique<Impl>())
{
}

CTokenizer::~CTokenizer() = default;

bool CTokenizer::Load(const std::string& vocabPath)
{
  try
  {
    // Open vocabulary file
    XFILE::CFile file;
    if (!file.Open(vocabPath))
    {
      CLog::Log(LOGERROR, "Tokenizer: Failed to open vocabulary file: {}", vocabPath);
      return false;
    }

    // Read vocabulary line by line
    std::string content;
    content.resize(file.GetLength());
    file.Read(&content[0], content.size());
    file.Close();

    std::istringstream stream(content);
    std::string line;
    int32_t tokenId = 0;

    m_impl->m_vocab.clear();
    m_impl->m_idToToken.clear();

    while (std::getline(stream, line))
    {
      // Remove trailing whitespace
      while (!line.empty() && IsWhitespace(line.back()))
      {
        line.pop_back();
      }

      if (line.empty())
        continue;

      // Add to vocabulary
      m_impl->m_vocab[line] = tokenId;
      m_impl->m_idToToken.push_back(line);

      // Check for special tokens
      if (line == "[PAD]")
        m_impl->m_padTokenId = tokenId;
      else if (line == "[UNK]")
        m_impl->m_unkTokenId = tokenId;
      else if (line == "[CLS]")
        m_impl->m_clsTokenId = tokenId;
      else if (line == "[SEP]")
        m_impl->m_sepTokenId = tokenId;
      else if (line == "[MASK]")
        m_impl->m_maskTokenId = tokenId;

      tokenId++;
    }

    if (m_impl->m_vocab.empty())
    {
      CLog::Log(LOGERROR, "Tokenizer: Vocabulary is empty");
      return false;
    }

    m_impl->m_loaded = true;

    CLog::Log(LOGINFO, "Tokenizer: Loaded {} tokens from vocabulary", m_impl->m_vocab.size());
    CLog::Log(LOGDEBUG, "Tokenizer: Special tokens - PAD={}, UNK={}, CLS={}, SEP={}, MASK={}",
              m_impl->m_padTokenId, m_impl->m_unkTokenId, m_impl->m_clsTokenId,
              m_impl->m_sepTokenId, m_impl->m_maskTokenId);

    return true;
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "Tokenizer: Exception while loading vocabulary: {}", e.what());
    return false;
  }
}

bool CTokenizer::IsLoaded() const
{
  return m_impl->m_loaded;
}

std::vector<int32_t> CTokenizer::Encode(const std::string& text, size_t maxLength)
{
  if (!m_impl->m_loaded)
    return {};

  // Tokenize text
  std::vector<std::string> tokens = m_impl->Tokenize(text);

  // Reserve space for special tokens
  if (maxLength < 2)
    maxLength = 2;

  // Truncate tokens if needed (leave room for [CLS] and [SEP])
  if (tokens.size() > maxLength - 2)
  {
    tokens.resize(maxLength - 2);
  }

  // Convert to IDs and add special tokens
  std::vector<int32_t> ids;
  ids.reserve(tokens.size() + 2);

  // Add [CLS] token
  ids.push_back(m_impl->m_clsTokenId);

  // Add content tokens
  auto contentIds = m_impl->ConvertTokensToIds(tokens);
  ids.insert(ids.end(), contentIds.begin(), contentIds.end());

  // Add [SEP] token
  ids.push_back(m_impl->m_sepTokenId);

  return ids;
}

std::vector<int32_t> CTokenizer::EncodeWithoutSpecialTokens(const std::string& text,
                                                            size_t maxLength)
{
  if (!m_impl->m_loaded)
    return {};

  // Tokenize text
  std::vector<std::string> tokens = m_impl->Tokenize(text);

  // Truncate if needed
  if (tokens.size() > maxLength)
  {
    tokens.resize(maxLength);
  }

  // Convert to IDs
  return m_impl->ConvertTokensToIds(tokens);
}

std::string CTokenizer::Decode(const std::vector<int32_t>& tokenIds) const
{
  if (!m_impl->m_loaded)
    return "";

  std::string result;

  for (int32_t id : tokenIds)
  {
    if (id < 0 || static_cast<size_t>(id) >= m_impl->m_idToToken.size())
      continue;

    const std::string& token = m_impl->m_idToToken[id];

    // Skip padding tokens
    if (token == "[PAD]")
      continue;

    // Handle subword tokens (remove ## prefix and don't add space)
    if (token.size() > 2 && token[0] == '#' && token[1] == '#')
    {
      result += token.substr(2);
    }
    else
    {
      if (!result.empty())
        result += " ";
      result += token;
    }
  }

  return result;
}

int32_t CTokenizer::GetPadTokenId() const
{
  return m_impl->m_padTokenId;
}

int32_t CTokenizer::GetClsTokenId() const
{
  return m_impl->m_clsTokenId;
}

int32_t CTokenizer::GetSepTokenId() const
{
  return m_impl->m_sepTokenId;
}

int32_t CTokenizer::GetUnkTokenId() const
{
  return m_impl->m_unkTokenId;
}

int32_t CTokenizer::GetMaskTokenId() const
{
  return m_impl->m_maskTokenId;
}

size_t CTokenizer::GetVocabSize() const
{
  return m_impl->m_vocab.size();
}

} // namespace SEMANTIC
} // namespace KODI
