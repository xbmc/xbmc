/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ITranscriptionProvider.h"

#include <atomic>
#include <string>

namespace KODI::SEMANTIC
{

/*!
 * \brief Groq Whisper API transcription provider
 *
 * Provides speech-to-text transcription using Groq's Whisper API.
 * Uses the whisper-large-v3-turbo model with verbose_json format
 * for timestamp-accurate transcriptions.
 *
 * API Details:
 * - Endpoint: https://api.groq.com/openai/v1/audio/transcriptions
 * - Model: whisper-large-v3-turbo
 * - Max file size: 25MB (larger files are chunked)
 * - Cost: ~$0.20/hour (~$0.0033/minute)
 */
class CGroqProvider : public ITranscriptionProvider
{
public:
  CGroqProvider();
  ~CGroqProvider() override = default;

  // ITranscriptionProvider implementation
  std::string GetName() const override { return "Groq Whisper"; }
  std::string GetId() const override { return "groq"; }
  bool IsConfigured() const override;
  bool IsAvailable() const override;

  bool Transcribe(const std::string& audioPath,
                  SegmentCallback onSegment,
                  ProgressCallback onProgress,
                  ErrorCallback onError) override;

  void Cancel() override;
  float EstimateCost(int64_t durationMs) const override;

  // Public constants for API configuration
  static constexpr const char* API_ENDPOINT =
      "https://api.groq.com/openai/v1/audio/transcriptions";
  static constexpr const char* MODEL_NAME = "whisper-large-v3-turbo";
  static constexpr const char* RESPONSE_FORMAT = "verbose_json";
  static constexpr int64_t MAX_FILE_SIZE = 25 * 1024 * 1024; // 25MB
  static constexpr float COST_PER_MINUTE = 0.0033f;          // $0.0033/min

private:
  /*!
   * \brief Transcribe a single audio file (must be < 25MB)
   * \return true on success
   */
  bool TranscribeFile(const std::string& audioPath,
                      SegmentCallback onSegment,
                      ErrorCallback onError);

  /*!
   * \brief Transcribe large files by chunking into segments
   * \return true on success
   */
  bool TranscribeLargeFile(const std::string& audioPath,
                           SegmentCallback onSegment,
                           ProgressCallback onProgress,
                           ErrorCallback onError);

  /*!
   * \brief Build multipart/form-data request body
   * \param audioPath Path to audio file
   * \param boundary MIME boundary string
   * \param[out] postData The constructed POST data
   * \return true on success
   */
  bool BuildMultipartRequest(const std::string& audioPath,
                             const std::string& boundary,
                             std::string& postData);

  /*!
   * \brief Parse verbose_json response and extract segments
   * \param jsonResponse Response from API
   * \param onSegment Callback for each segment
   * \return true on success
   */
  bool ParseTranscriptionResponse(const std::string& jsonResponse,
                                  SegmentCallback onSegment);

  /*!
   * \brief Get API key from settings
   * \return API key or empty string if not configured
   */
  std::string GetApiKey() const;

  /*!
   * \brief Get file size in bytes
   * \return File size or -1 on error
   */
  int64_t GetFileSize(const std::string& filePath) const;

  static constexpr int CONNECT_TIMEOUT_SEC = 30;
  static constexpr int REQUEST_TIMEOUT_SEC = 300; // 5 minutes for large files

  std::atomic<bool> m_cancelled{false};
  std::string m_apiKey;
};

} // namespace KODI::SEMANTIC
