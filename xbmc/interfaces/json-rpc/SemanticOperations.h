/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "JSONRPC.h"

class CVariant;

namespace JSONRPC
{
/*!
 * \brief JSON-RPC handlers for semantic search operations
 *
 * Exposes semantic search functionality to external tools and Kodi UI
 * through the JSON-RPC interface.
 */
class CSemanticOperations
{
public:
  // Search operations

  /*!
   * \brief Search indexed content using full-text search
   * \param method The method name
   * \param transport Transport layer
   * \param client Client information
   * \param parameterObject Parameters including query and options
   * \param result Output result containing search results
   * \return JSONRPC_STATUS indicating success or failure
   */
  static JSONRPC_STATUS Search(const std::string& method,
                               ITransportLayer* transport,
                               IClient* client,
                               const CVariant& parameterObject,
                               CVariant& result);

  /*!
   * \brief Get context around a specific timestamp
   * \param method The method name
   * \param transport Transport layer
   * \param client Client information
   * \param parameterObject Parameters including media_id, media_type, timestamp
   * \param result Output result containing context chunks
   * \return JSONRPC_STATUS indicating success or failure
   */
  static JSONRPC_STATUS GetContext(const std::string& method,
                                   ITransportLayer* transport,
                                   IClient* client,
                                   const CVariant& parameterObject,
                                   CVariant& result);

  /*!
   * \brief Find similar content using vector search
   * \param method The method name
   * \param transport Transport layer
   * \param client Client information
   * \param parameterObject Parameters including chunk_id or media info
   * \param result Output result containing similar results
   * \return JSONRPC_STATUS indicating success or failure
   */
  static JSONRPC_STATUS FindSimilar(const std::string& method,
                                    ITransportLayer* transport,
                                    IClient* client,
                                    const CVariant& parameterObject,
                                    CVariant& result);

  // Index operations

  /*!
   * \brief Get indexing state for a media item
   * \param method The method name
   * \param transport Transport layer
   * \param client Client information
   * \param parameterObject Parameters including media_id and media_type
   * \param result Output result containing index state
   * \return JSONRPC_STATUS indicating success or failure
   */
  static JSONRPC_STATUS GetIndexState(const std::string& method,
                                      ITransportLayer* transport,
                                      IClient* client,
                                      const CVariant& parameterObject,
                                      CVariant& result);

  /*!
   * \brief Queue a media item for indexing
   * \param method The method name
   * \param transport Transport layer
   * \param client Client information
   * \param parameterObject Parameters including media_id and media_type
   * \param result Output result
   * \return JSONRPC_STATUS indicating success or failure
   */
  static JSONRPC_STATUS QueueIndex(const std::string& method,
                                   ITransportLayer* transport,
                                   IClient* client,
                                   const CVariant& parameterObject,
                                   CVariant& result);

  /*!
   * \brief Queue a media item for transcription
   * \param method The method name
   * \param transport Transport layer
   * \param client Client information
   * \param parameterObject Parameters including media_id, media_type, provider_id
   * \param result Output result
   * \return JSONRPC_STATUS indicating success or failure
   */
  static JSONRPC_STATUS QueueTranscription(const std::string& method,
                                           ITransportLayer* transport,
                                           IClient* client,
                                           const CVariant& parameterObject,
                                           CVariant& result);

  // Statistics

  /*!
   * \brief Get overall semantic index statistics
   * \param method The method name
   * \param transport Transport layer
   * \param client Client information
   * \param parameterObject Parameters (none required)
   * \param result Output result containing statistics
   * \return JSONRPC_STATUS indicating success or failure
   */
  static JSONRPC_STATUS GetStats(const std::string& method,
                                 ITransportLayer* transport,
                                 IClient* client,
                                 const CVariant& parameterObject,
                                 CVariant& result);

  // Provider operations

  /*!
   * \brief Get list of available transcription providers
   * \param method The method name
   * \param transport Transport layer
   * \param client Client information
   * \param parameterObject Parameters (none required)
   * \param result Output result containing provider list
   * \return JSONRPC_STATUS indicating success or failure
   */
  static JSONRPC_STATUS GetProviders(const std::string& method,
                                     ITransportLayer* transport,
                                     IClient* client,
                                     const CVariant& parameterObject,
                                     CVariant& result);

  /*!
   * \brief Estimate transcription cost for a media item
   * \param method The method name
   * \param transport Transport layer
   * \param client Client information
   * \param parameterObject Parameters including media_id, media_type, provider_id
   * \param result Output result containing cost estimate
   * \return JSONRPC_STATUS indicating success or failure
   */
  static JSONRPC_STATUS EstimateCost(const std::string& method,
                                     ITransportLayer* transport,
                                     IClient* client,
                                     const CVariant& parameterObject,
                                     CVariant& result);

  // Configuration

  /*!
   * \brief Configure semantic search settings including API keys
   * \param method The method name
   * \param transport Transport layer
   * \param client Client information
   * \param parameterObject Parameters including groq_api_key, auto_transcribe, monthly_budget
   * \param result Output result containing configuration status
   * \return JSONRPC_STATUS indicating success or failure
   */
  static JSONRPC_STATUS Configure(const std::string& method,
                                  ITransportLayer* transport,
                                  IClient* client,
                                  const CVariant& parameterObject,
                                  CVariant& result);

  /*!
   * \brief Get current semantic search configuration status
   * \param method The method name
   * \param transport Transport layer
   * \param client Client information
   * \param parameterObject Parameters (none required)
   * \param result Output result containing current configuration
   * \return JSONRPC_STATUS indicating success or failure
   */
  static JSONRPC_STATUS GetConfig(const std::string& method,
                                  ITransportLayer* transport,
                                  IClient* client,
                                  const CVariant& parameterObject,
                                  CVariant& result);
};

} // namespace JSONRPC
