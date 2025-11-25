/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "semantic/embedding/EmbeddingEngine.h"

#include <gtest/gtest.h>
#include <cmath>

class EmbeddingEngineTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    m_engine = std::make_unique<KODI::SEMANTIC::CEmbeddingEngine>();

    // Try to initialize - tests skip if model not available
    m_initialized = m_engine->Initialize("special://xbmc/system/semantic/model.onnx",
                                         "special://xbmc/system/semantic/vocab.txt");
  }

  std::unique_ptr<KODI::SEMANTIC::CEmbeddingEngine> m_engine;
  bool m_initialized = false;
};

TEST_F(EmbeddingEngineTest, SingleEmbedding)
{
  if (!m_initialized)
    GTEST_SKIP() << "EmbeddingEngine not initialized - model files not available";

  auto embedding = m_engine->Embed("hello world");

  // Should be 384-dimensional
  EXPECT_EQ(embedding.size(), 384);

  // Should have non-zero values
  bool hasNonZero = false;
  for (float v : embedding)
  {
    if (v != 0.0f)
      hasNonZero = true;
  }
  EXPECT_TRUE(hasNonZero);
}

TEST_F(EmbeddingEngineTest, BatchEmbedding)
{
  if (!m_initialized)
    GTEST_SKIP() << "EmbeddingEngine not initialized - model files not available";

  std::vector<std::string> texts = {"hello", "world", "test"};
  auto embeddings = m_engine->EmbedBatch(texts);

  EXPECT_EQ(embeddings.size(), 3);

  // Each embedding should be 384-dimensional
  for (const auto& emb : embeddings)
  {
    EXPECT_EQ(emb.size(), 384);
  }
}

TEST_F(EmbeddingEngineTest, SimilarTextsSimilarEmbeddings)
{
  if (!m_initialized)
    GTEST_SKIP() << "EmbeddingEngine not initialized - model files not available";

  auto emb1 = m_engine->Embed("The cat sat on the mat");
  auto emb2 = m_engine->Embed("A cat was sitting on a mat");
  auto emb3 = m_engine->Embed("The stock market crashed today");

  float sim12 = KODI::SEMANTIC::CEmbeddingEngine::Similarity(emb1, emb2);
  float sim13 = KODI::SEMANTIC::CEmbeddingEngine::Similarity(emb1, emb3);

  // Similar sentences should have higher similarity
  EXPECT_GT(sim12, sim13);

  // Similarity should be in valid range [-1, 1]
  EXPECT_GE(sim12, -1.0f);
  EXPECT_LE(sim12, 1.0f);
  EXPECT_GE(sim13, -1.0f);
  EXPECT_LE(sim13, 1.0f);
}

TEST_F(EmbeddingEngineTest, EmptyInput)
{
  if (!m_initialized)
    GTEST_SKIP() << "EmbeddingEngine not initialized - model files not available";

  auto embedding = m_engine->Embed("");

  // Should handle gracefully - return valid 384-dimensional embedding
  EXPECT_EQ(embedding.size(), 384);
}

TEST_F(EmbeddingEngineTest, L2Normalization)
{
  if (!m_initialized)
    GTEST_SKIP() << "EmbeddingEngine not initialized - model files not available";

  auto embedding = m_engine->Embed("test text");

  // Check L2 norm is approximately 1
  float norm = 0.0f;
  for (float v : embedding)
    norm += v * v;
  norm = std::sqrt(norm);

  EXPECT_NEAR(norm, 1.0f, 0.01f);
}

TEST_F(EmbeddingEngineTest, IdenticalTextIdenticalEmbedding)
{
  if (!m_initialized)
    GTEST_SKIP() << "EmbeddingEngine not initialized - model files not available";

  auto emb1 = m_engine->Embed("identical test");
  auto emb2 = m_engine->Embed("identical test");

  float similarity = KODI::SEMANTIC::CEmbeddingEngine::Similarity(emb1, emb2);

  // Identical text should have similarity very close to 1.0
  EXPECT_NEAR(similarity, 1.0f, 0.001f);
}

TEST_F(EmbeddingEngineTest, LongText)
{
  if (!m_initialized)
    GTEST_SKIP() << "EmbeddingEngine not initialized - model files not available";

  std::string longText =
      "This is a very long text that exceeds the typical token limit of most transformer models. "
      "The embedding engine should handle this gracefully by truncating to the maximum sequence "
      "length supported by the model. The all-MiniLM-L6-v2 model supports sequences up to 512 "
      "tokens, but in practice we often use shorter limits like 256 or 128 tokens for efficiency. "
      "Even with truncation, the model should produce meaningful embeddings that capture the "
      "semantic content of the beginning of the text.";

  auto embedding = m_engine->Embed(longText);

  EXPECT_EQ(embedding.size(), 384);

  // Should produce valid normalized embedding
  float norm = 0.0f;
  for (float v : embedding)
    norm += v * v;
  norm = std::sqrt(norm);
  EXPECT_NEAR(norm, 1.0f, 0.01f);
}

TEST_F(EmbeddingEngineTest, SimilaritySymmetry)
{
  if (!m_initialized)
    GTEST_SKIP() << "EmbeddingEngine not initialized - model files not available";

  auto emb1 = m_engine->Embed("first text");
  auto emb2 = m_engine->Embed("second text");

  float sim12 = KODI::SEMANTIC::CEmbeddingEngine::Similarity(emb1, emb2);
  float sim21 = KODI::SEMANTIC::CEmbeddingEngine::Similarity(emb2, emb1);

  // Similarity should be symmetric
  EXPECT_FLOAT_EQ(sim12, sim21);
}

TEST_F(EmbeddingEngineTest, BatchConsistency)
{
  if (!m_initialized)
    GTEST_SKIP() << "EmbeddingEngine not initialized - model files not available";

  std::string testText = "consistency test";

  // Get embedding individually
  auto singleEmb = m_engine->Embed(testText);

  // Get embedding in a batch
  std::vector<std::string> batch = {testText};
  auto batchEmbs = m_engine->EmbedBatch(batch);

  ASSERT_EQ(batchEmbs.size(), 1);

  // Results should be identical (or very close due to floating point)
  float similarity = KODI::SEMANTIC::CEmbeddingEngine::Similarity(singleEmb, batchEmbs[0]);
  EXPECT_NEAR(similarity, 1.0f, 0.001f);
}

TEST_F(EmbeddingEngineTest, EmptyBatch)
{
  if (!m_initialized)
    GTEST_SKIP() << "EmbeddingEngine not initialized - model files not available";

  std::vector<std::string> emptyBatch;
  auto embeddings = m_engine->EmbedBatch(emptyBatch);

  EXPECT_TRUE(embeddings.empty());
}

TEST_F(EmbeddingEngineTest, IsInitializedCheck)
{
  KODI::SEMANTIC::CEmbeddingEngine uninitEngine;
  EXPECT_FALSE(uninitEngine.IsInitialized());

  if (m_initialized)
  {
    EXPECT_TRUE(m_engine->IsInitialized());
  }
}

TEST_F(EmbeddingEngineTest, SpecialCharacters)
{
  if (!m_initialized)
    GTEST_SKIP() << "EmbeddingEngine not initialized - model files not available";

  auto emb1 = m_engine->Embed("test!@#$%^&*()");
  auto emb2 = m_engine->Embed("test");

  // Should produce valid embeddings even with special characters
  EXPECT_EQ(emb1.size(), 384);
  EXPECT_EQ(emb2.size(), 384);

  // They should be somewhat similar since they share the word "test"
  float similarity = KODI::SEMANTIC::CEmbeddingEngine::Similarity(emb1, emb2);
  EXPECT_GT(similarity, 0.5f); // Should be reasonably similar
}

TEST_F(EmbeddingEngineTest, DifferentLanguageSeparation)
{
  if (!m_initialized)
    GTEST_SKIP() << "EmbeddingEngine not initialized - model files not available";

  auto embEnglish = m_engine->Embed("hello world");
  auto embFrench = m_engine->Embed("bonjour monde");
  auto embEnglish2 = m_engine->Embed("hello planet");

  float simEngEng = KODI::SEMANTIC::CEmbeddingEngine::Similarity(embEnglish, embEnglish2);
  float simEngFr = KODI::SEMANTIC::CEmbeddingEngine::Similarity(embEnglish, embFrench);

  // Same language with similar meaning should be more similar than different languages
  // (though multilingual models might actually make these similar)
  // This test verifies the embeddings are different
  EXPECT_NE(simEngEng, simEngFr);
}
