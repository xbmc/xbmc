/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace KODI::SEMANTIC
{

/*!
 * \brief Represents a single transcribed segment with timing information
 */
struct TranscriptSegment
{
  int64_t startMs;      //! Start time in milliseconds
  int64_t endMs;        //! End time in milliseconds
  std::string text;     //! Transcribed text
  std::string language; //! Detected language code (e.g., "en")
  float confidence;     //! Confidence score (0.0 to 1.0)
};

//! Callback invoked for each transcribed segment
using SegmentCallback = std::function<void(const TranscriptSegment&)>;

//! Callback to report progress (0.0 to 1.0)
using ProgressCallback = std::function<void(float)>;

//! Callback to report errors
using ErrorCallback = std::function<void(const std::string&)>;

/*!
 * \brief Interface for cloud transcription providers
 *
 * Implementations provide speech-to-text transcription services
 * for audio/video files using cloud APIs.
 */
class ITranscriptionProvider
{
public:
  virtual ~ITranscriptionProvider() = default;

  /*!
   * \brief Get the human-readable name of this provider
   * \return Provider name (e.g., "Groq Whisper")
   */
  virtual std::string GetName() const = 0;

  /*!
   * \brief Get the unique identifier for this provider
   * \return Provider ID (e.g., "groq")
   */
  virtual std::string GetId() const = 0;

  /*!
   * \brief Check if the provider is properly configured
   * \return true if API key and settings are configured
   */
  virtual bool IsConfigured() const = 0;

  /*!
   * \brief Check if the provider is available (network, service status)
   * \return true if the provider can be used
   */
  virtual bool IsAvailable() const = 0;

  /*!
   * \brief Transcribe an audio file
   * \param audioPath Path to audio file to transcribe
   * \param onSegment Callback invoked for each transcribed segment
   * \param onProgress Callback to report progress (0.0 to 1.0)
   * \param onError Callback to report errors
   * \return true if transcription completed successfully
   */
  virtual bool Transcribe(const std::string& audioPath,
                          SegmentCallback onSegment,
                          ProgressCallback onProgress,
                          ErrorCallback onError) = 0;

  /*!
   * \brief Cancel an ongoing transcription
   */
  virtual void Cancel() = 0;

  /*!
   * \brief Estimate the cost for transcribing audio of given duration
   * \param durationMs Duration in milliseconds
   * \return Estimated cost in USD
   */
  virtual float EstimateCost(int64_t durationMs) const = 0;
};

} // namespace KODI::SEMANTIC
