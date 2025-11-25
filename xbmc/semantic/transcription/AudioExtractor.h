/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <atomic>
#include <cstdint>
#include <string>
#include <vector>

namespace KODI::SEMANTIC
{

/*!
 * \brief Configuration for audio extraction
 *
 * Defines output parameters optimized for Whisper API transcription.
 * Default values are selected for best compatibility with cloud providers.
 */
struct AudioExtractionConfig
{
  int sampleRate{16000};        //!< Sample rate in Hz (optimal for Whisper)
  int channels{1};              //!< Number of audio channels (mono)
  std::string format{"mp3"};    //!< Output format (mp3, wav, etc.)
  int bitrate{64};              //!< Audio bitrate in kbps
  int maxSegmentMinutes{45};    //!< Max duration per segment (for chunking)
  int maxFileSizeMB{25};        //!< Max file size in MB (Groq/OpenAI limit)
};

/*!
 * \brief Information about an extracted audio segment
 *
 * Used when chunking large files into smaller segments for processing.
 */
struct AudioSegment
{
  std::string path;             //!< Temporary file path
  int64_t startMs;              //!< Start offset from original (milliseconds)
  int64_t durationMs;           //!< Duration of this segment (milliseconds)
  int64_t fileSizeBytes;        //!< File size in bytes
};

/*!
 * \brief Audio extraction utility for video transcription
 *
 * Extracts and converts audio from video files using FFmpeg.
 * Supports chunking large files to meet cloud API size limits.
 *
 * Features:
 * - FFmpeg availability detection
 * - Configurable output format (MP3, WAV, etc.)
 * - Automatic chunking for large files
 * - Duration detection via FFprobe
 * - Cancellable operations
 *
 * Example:
 * \code
 * CAudioExtractor extractor;
 * if (extractor.IsFFmpegAvailable())
 * {
 *   if (extractor.ExtractAudio("/path/to/video.mkv", "/tmp/audio.mp3"))
 *   {
 *     // Process audio file...
 *   }
 * }
 * \endcode
 */
class CAudioExtractor
{
public:
  CAudioExtractor();
  explicit CAudioExtractor(const AudioExtractionConfig& config);
  ~CAudioExtractor();

  /*!
   * \brief Check if FFmpeg is available on the system
   * \return true if FFmpeg is available and usable
   *
   * Runs 'ffmpeg -version' to verify FFmpeg is installed.
   */
  static bool IsFFmpegAvailable();

  /*!
   * \brief Extract audio from a video file
   * \param videoPath Path to source video file
   * \param outputPath Path to output audio file
   * \return true on success, false on failure
   *
   * Converts the entire video to audio using configured settings.
   * Use this for small-to-medium files that don't need chunking.
   */
  bool ExtractAudio(const std::string& videoPath, const std::string& outputPath);

  /*!
   * \brief Extract audio with automatic chunking for large files
   * \param videoPath Path to source video file
   * \param outputDir Directory for output segment files
   * \return Vector of audio segments (empty on failure)
   *
   * Automatically splits long videos into segments based on
   * maxSegmentMinutes and maxFileSizeMB settings. Useful for
   * processing very long videos within cloud API limits.
   */
  std::vector<AudioSegment> ExtractChunked(const std::string& videoPath,
                                           const std::string& outputDir);

  /*!
   * \brief Get duration of a media file
   * \param path Path to media file (video or audio)
   * \return Duration in milliseconds, or -1 on error
   *
   * Uses FFprobe to detect media duration.
   */
  int64_t GetMediaDuration(const std::string& path);

  /*!
   * \brief Clean up temporary segment files
   * \param segments Vector of segments to delete
   *
   * Deletes all files referenced in the segment list.
   * Use after transcription is complete.
   */
  void CleanupSegments(const std::vector<AudioSegment>& segments);

  /*!
   * \brief Set extraction configuration
   * \param config New configuration
   */
  void SetConfig(const AudioExtractionConfig& config);

  /*!
   * \brief Get current extraction configuration
   * \return Current configuration
   */
  const AudioExtractionConfig& GetConfig() const;

  /*!
   * \brief Cancel ongoing extraction operation
   *
   * Sets cancellation flag. The extraction will stop at the
   * next safe point (usually between segments).
   */
  void Cancel();

  /*!
   * \brief Check if extraction has been cancelled
   * \return true if Cancel() has been called
   */
  bool IsCancelled() const;

private:
  AudioExtractionConfig m_config;
  std::atomic<bool> m_cancelled{false};

  /*!
   * \brief Build FFmpeg command line
   * \param input Input file path
   * \param output Output file path
   * \param startMs Start time in milliseconds (0 for no seeking)
   * \param durationMs Duration in milliseconds (-1 for entire file)
   * \return Complete FFmpeg command string
   */
  std::string BuildFFmpegCommand(const std::string& input,
                                 const std::string& output,
                                 int64_t startMs = 0,
                                 int64_t durationMs = -1);

  /*!
   * \brief Execute FFmpeg command
   * \param command Command to execute
   * \return true on success (exit code 0)
   */
  bool ExecuteFFmpeg(const std::string& command);

  /*!
   * \brief Parse duration from FFprobe output
   * \param output Raw output from FFprobe
   * \return Duration in milliseconds, or -1 on parse error
   */
  int64_t ParseDurationFromFFprobe(const std::string& output);
};

} // namespace KODI::SEMANTIC
