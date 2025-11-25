/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * \brief BERT-style WordPiece tokenizer for sentence transformers
 *
 * This tokenizer implements WordPiece subword tokenization compatible with
 * BERT and sentence-transformers models like all-MiniLM-L6-v2.
 *
 * It loads vocabulary from standard HuggingFace vocab.txt files and provides
 * encoding/decoding functionality with proper special token handling.
 *
 * Tokenization process:
 * 1. Text normalization (lowercase, whitespace handling)
 * 2. Split into words by whitespace and punctuation
 * 3. Apply WordPiece algorithm to split unknown words
 * 4. Add special tokens ([CLS], [SEP])
 * 5. Convert tokens to IDs using vocabulary
 *
 * Example usage:
 * \code
 * CTokenizer tokenizer;
 * if (tokenizer.Load("vocab.txt"))
 * {
 *   auto tokenIds = tokenizer.Encode("Hello world", 256);
 *   // tokenIds = [101, 7592, 2088, 102] ([CLS] hello world [SEP])
 * }
 * \endcode
 */
class CTokenizer
{
public:
  /*!
   * \brief Constructor
   */
  CTokenizer();

  /*!
   * \brief Destructor
   */
  ~CTokenizer();

  /*!
   * \brief Load tokenizer vocabulary from file
   *
   * Loads a HuggingFace-style vocab.txt file. The file should contain
   * one token per line, with the line number being the token ID.
   *
   * Example vocab.txt:
   * [PAD]
   * [UNK]
   * [CLS]
   * [SEP]
   * [MASK]
   * ...
   * hello
   * world
   * ##ing
   *
   * \param vocabPath Path to vocab.txt file
   * \return true if vocabulary loaded successfully, false otherwise
   */
  bool Load(const std::string& vocabPath);

  /*!
   * \brief Check if tokenizer is loaded and ready
   *
   * \return true if vocabulary is loaded
   */
  bool IsLoaded() const;

  /*!
   * \brief Encode text to token IDs with special tokens
   *
   * Tokenizes input text and converts to token IDs. The output includes
   * special tokens: [CLS] at start, [SEP] at end.
   *
   * The sequence is truncated if it exceeds maxLength (including special tokens).
   * The sequence is NOT padded - padding should be done by the caller if needed.
   *
   * \param text Input text to tokenize
   * \param maxLength Maximum sequence length (including special tokens)
   * \return Vector of token IDs
   */
  std::vector<int32_t> Encode(const std::string& text, size_t maxLength = 512);

  /*!
   * \brief Encode text without special tokens
   *
   * Same as Encode() but without adding [CLS] and [SEP] tokens.
   * Useful for intermediate processing or when caller manages special tokens.
   *
   * \param text Input text to tokenize
   * \param maxLength Maximum sequence length (no special tokens added)
   * \return Vector of token IDs
   */
  std::vector<int32_t> EncodeWithoutSpecialTokens(const std::string& text, size_t maxLength = 512);

  /*!
   * \brief Decode token IDs back to text
   *
   * Converts token IDs back to readable text. Special tokens are included
   * in the output (e.g., "[CLS]", "[SEP]").
   *
   * \param tokenIds Vector of token IDs to decode
   * \return Decoded text string
   */
  std::string Decode(const std::vector<int32_t>& tokenIds) const;

  /*!
   * \brief Get the ID of the [PAD] token
   *
   * \return Token ID for padding, typically 0
   */
  int32_t GetPadTokenId() const;

  /*!
   * \brief Get the ID of the [CLS] token
   *
   * [CLS] is added at the start of sequences for classification tasks.
   *
   * \return Token ID for [CLS], typically 101
   */
  int32_t GetClsTokenId() const;

  /*!
   * \brief Get the ID of the [SEP] token
   *
   * [SEP] is added at the end of sequences or between sentence pairs.
   *
   * \return Token ID for [SEP], typically 102
   */
  int32_t GetSepTokenId() const;

  /*!
   * \brief Get the ID of the [UNK] token
   *
   * [UNK] is used for unknown tokens not in the vocabulary.
   *
   * \return Token ID for [UNK], typically 100
   */
  int32_t GetUnkTokenId() const;

  /*!
   * \brief Get the ID of the [MASK] token
   *
   * [MASK] is used for masked language modeling tasks.
   *
   * \return Token ID for [MASK], typically 103
   */
  int32_t GetMaskTokenId() const;

  /*!
   * \brief Get vocabulary size
   *
   * \return Number of tokens in the vocabulary
   */
  size_t GetVocabSize() const;

private:
  /*!
   * \brief Private implementation (PIMPL pattern)
   *
   * Hides implementation details and vocabulary data structures.
   */
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace SEMANTIC
} // namespace KODI
