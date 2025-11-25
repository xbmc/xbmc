/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "semantic/embedding/Tokenizer.h"

#include <gtest/gtest.h>

class TokenizerTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_tokenizer = std::make_unique<KODI::SEMANTIC::CTokenizer>();
    // Note: Tests will skip if vocab file not available
    m_initialized = m_tokenizer->Load("special://xbmc/system/semantic/vocab.txt");
  }

  std::unique_ptr<KODI::SEMANTIC::CTokenizer> m_tokenizer;
  bool m_initialized = false;
};

TEST_F(TokenizerTest, BasicTokenization)
{
  if (!m_initialized)
    GTEST_SKIP() << "Tokenizer not initialized - vocab file not available";

  auto tokens = m_tokenizer->Encode("hello world");

  EXPECT_FALSE(tokens.empty());
  EXPECT_EQ(tokens.front(), m_tokenizer->GetClsTokenId()); // [CLS]
  EXPECT_EQ(tokens.back(), m_tokenizer->GetSepTokenId()); // [SEP]
}

TEST_F(TokenizerTest, MaxLengthTruncation)
{
  if (!m_initialized)
    GTEST_SKIP() << "Tokenizer not initialized - vocab file not available";

  std::string longText(1000, 'a'); // Very long text
  auto tokens = m_tokenizer->Encode(longText, 128);

  EXPECT_LE(tokens.size(), 128);
}

TEST_F(TokenizerTest, SpecialCharacters)
{
  if (!m_initialized)
    GTEST_SKIP() << "Tokenizer not initialized - vocab file not available";

  auto tokens = m_tokenizer->Encode("hello, world! how are you?");
  EXPECT_FALSE(tokens.empty());
}

TEST_F(TokenizerTest, SubwordTokenization)
{
  if (!m_initialized)
    GTEST_SKIP() << "Tokenizer not initialized - vocab file not available";

  // Unknown words should be split into subwords
  auto tokens = m_tokenizer->Encode("unbelievable");

  // Should have [CLS], multiple subwords, [SEP]
  EXPECT_GT(tokens.size(), 3);
}

TEST_F(TokenizerTest, EmptyString)
{
  if (!m_initialized)
    GTEST_SKIP() << "Tokenizer not initialized - vocab file not available";

  auto tokens = m_tokenizer->Encode("");

  // Should at least have [CLS] and [SEP]
  EXPECT_GE(tokens.size(), 2);
  EXPECT_EQ(tokens.front(), m_tokenizer->GetClsTokenId());
  EXPECT_EQ(tokens.back(), m_tokenizer->GetSepTokenId());
}

TEST_F(TokenizerTest, EncodeWithoutSpecialTokens)
{
  if (!m_initialized)
    GTEST_SKIP() << "Tokenizer not initialized - vocab file not available";

  auto tokensWithSpecial = m_tokenizer->Encode("hello world");
  auto tokensWithout = m_tokenizer->EncodeWithoutSpecialTokens("hello world");

  // Without special tokens should be 2 tokens shorter
  EXPECT_EQ(tokensWithSpecial.size(), tokensWithout.size() + 2);

  // First/last tokens should not be special tokens
  EXPECT_NE(tokensWithout.front(), m_tokenizer->GetClsTokenId());
  EXPECT_NE(tokensWithout.back(), m_tokenizer->GetSepTokenId());
}

TEST_F(TokenizerTest, DecodeRoundTrip)
{
  if (!m_initialized)
    GTEST_SKIP() << "Tokenizer not initialized - vocab file not available";

  std::string original = "hello world";
  auto tokens = m_tokenizer->Encode(original);
  std::string decoded = m_tokenizer->Decode(tokens);

  // Decoded text should contain original words (case and spacing may differ)
  EXPECT_FALSE(decoded.empty());
  // Should contain special tokens in decoded form
  EXPECT_NE(decoded.find("[CLS]"), std::string::npos);
  EXPECT_NE(decoded.find("[SEP]"), std::string::npos);
}

TEST_F(TokenizerTest, VocabSize)
{
  if (!m_initialized)
    GTEST_SKIP() << "Tokenizer not initialized - vocab file not available";

  size_t vocabSize = m_tokenizer->GetVocabSize();

  // all-MiniLM-L6-v2 has ~30k tokens
  EXPECT_GT(vocabSize, 20000);
  EXPECT_LT(vocabSize, 50000);
}

TEST_F(TokenizerTest, SpecialTokenIds)
{
  if (!m_initialized)
    GTEST_SKIP() << "Tokenizer not initialized - vocab file not available";

  // Standard BERT token IDs
  EXPECT_EQ(m_tokenizer->GetPadTokenId(), 0);
  EXPECT_EQ(m_tokenizer->GetUnkTokenId(), 100);
  EXPECT_EQ(m_tokenizer->GetClsTokenId(), 101);
  EXPECT_EQ(m_tokenizer->GetSepTokenId(), 102);
  EXPECT_EQ(m_tokenizer->GetMaskTokenId(), 103);
}

TEST_F(TokenizerTest, IsLoadedCheck)
{
  KODI::SEMANTIC::CTokenizer unloadedTokenizer;
  EXPECT_FALSE(unloadedTokenizer.IsLoaded());

  if (m_initialized)
  {
    EXPECT_TRUE(m_tokenizer->IsLoaded());
  }
}

TEST_F(TokenizerTest, LongSentence)
{
  if (!m_initialized)
    GTEST_SKIP() << "Tokenizer not initialized - vocab file not available";

  std::string longSentence =
      "This is a very long sentence with many words that will test the tokenizer's ability to "
      "handle longer input sequences with proper truncation and special token handling at the "
      "boundaries of the maximum sequence length parameter that we provide to the encode method.";

  auto tokens = m_tokenizer->Encode(longSentence, 64);

  EXPECT_LE(tokens.size(), 64);
  EXPECT_EQ(tokens.front(), m_tokenizer->GetClsTokenId());
  EXPECT_EQ(tokens.back(), m_tokenizer->GetSepTokenId());
}

TEST_F(TokenizerTest, UnicodCharacters)
{
  if (!m_initialized)
    GTEST_SKIP() << "Tokenizer not initialized - vocab file not available";

  // Test with some unicode characters
  auto tokens = m_tokenizer->Encode("Café résumé naïve");

  EXPECT_FALSE(tokens.empty());
  EXPECT_GT(tokens.size(), 2); // More than just [CLS] and [SEP]
}
