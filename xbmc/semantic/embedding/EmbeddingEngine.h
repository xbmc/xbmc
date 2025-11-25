/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <memory>
#include <string>
#include <vector>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * \brief Embedding dimension for all-MiniLM-L6-v2 model
 *
 * The all-MiniLM-L6-v2 model produces 384-dimensional embeddings.
 * This is a compact sentence transformer optimized for semantic similarity tasks.
 */
constexpr size_t EMBEDDING_DIM = 384;

/*!
 * \brief Type alias for embedding vectors
 *
 * Represents a fixed-size embedding vector produced by the model.
 * Using std::array for better performance and type safety.
 */
using Embedding = std::array<float, EMBEDDING_DIM>;

/*!
 * \brief ONNX Runtime-based embedding engine for semantic search
 *
 * This class provides an interface to load and run ONNX models for generating
 * text embeddings. It is designed to work with sentence transformer models,
 * specifically all-MiniLM-L6-v2.
 *
 * The engine handles:
 * - Loading ONNX model files
 * - Tokenization (via vocabulary files)
 * - Batch inference for efficient processing
 * - Cosine similarity computation
 *
 * Example usage:
 * \code
 * CEmbeddingEngine engine;
 * if (engine.Initialize(modelPath, vocabPath))
 * {
 *   auto embedding = engine.Embed("Sample text for embedding");
 *   // Use embedding for similarity search, clustering, etc.
 * }
 * \endcode
 */
class CEmbeddingEngine
{
public:
  /*!
   * \brief Constructor
   */
  CEmbeddingEngine();

  /*!
   * \brief Destructor
   */
  ~CEmbeddingEngine();

  /*!
   * \brief Initialize the embedding engine with model and vocabulary files
   *
   * Loads the ONNX model and tokenizer vocabulary. This must be called
   * before any embedding operations.
   *
   * \param modelPath Path to the ONNX model file (.onnx)
   * \param vocabPath Path to the tokenizer vocabulary file
   * \param lazyLoad If true, model is loaded on first use (default: true)
   * \param idleTimeoutSec Seconds of idle time before unloading model (0 = never, default: 300)
   * \return true if initialization succeeded, false otherwise
   */
  bool Initialize(const std::string& modelPath,
                  const std::string& vocabPath,
                  bool lazyLoad = true,
                  int idleTimeoutSec = 300);

  /*!
   * \brief Check if the engine is properly initialized
   *
   * \return true if the engine is ready to generate embeddings
   */
  bool IsInitialized() const;

  /*!
   * \brief Check if the model is currently loaded in memory
   *
   * \return true if model is loaded, false if waiting for lazy load
   */
  bool IsModelLoaded() const;

  /*!
   * \brief Manually load the model (if lazy loading is enabled)
   *
   * \return true if load succeeded, false otherwise
   */
  bool LoadModel();

  /*!
   * \brief Manually unload the model to free memory
   */
  void UnloadModel();

  /*!
   * \brief Generate an embedding for a single text string
   *
   * \param text Input text to embed
   * \return 384-dimensional embedding vector
   * \throws std::runtime_error if engine is not initialized
   */
  Embedding Embed(const std::string& text);

  /*!
   * \brief Generate embeddings for multiple text strings in a batch
   *
   * Batch processing is more efficient than individual calls when
   * processing multiple texts.
   *
   * \param texts Vector of input texts to embed
   * \return Vector of embedding vectors, one per input text
   * \throws std::runtime_error if engine is not initialized
   */
  std::vector<Embedding> EmbedBatch(const std::vector<std::string>& texts);

  /*!
   * \brief Compute cosine similarity between two embeddings
   *
   * Cosine similarity ranges from -1 (opposite) to 1 (identical).
   * For normalized embeddings, this is equivalent to dot product.
   *
   * \param a First embedding vector
   * \param b Second embedding vector
   * \return Cosine similarity score in range [-1, 1]
   */
  static float Similarity(const Embedding& a, const Embedding& b);

private:
  /*!
   * \brief Private implementation (PIMPL pattern)
   *
   * Hides ONNX Runtime implementation details from the header.
   * This avoids exposing ONNX Runtime headers to all consumers.
   */
  class Impl;
  std::unique_ptr<Impl> m_impl;
};

} // namespace SEMANTIC
} // namespace KODI
