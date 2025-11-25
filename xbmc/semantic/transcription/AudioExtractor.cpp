/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AudioExtractor.h"

#include "PlatformDefs.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace KODI::UTILS;

namespace KODI::SEMANTIC
{

CAudioExtractor::CAudioExtractor() = default;

CAudioExtractor::CAudioExtractor(const AudioExtractionConfig& config) : m_config(config)
{
}

CAudioExtractor::~CAudioExtractor() = default;

bool CAudioExtractor::IsFFmpegAvailable()
{
  // Try to run ffmpeg -version
  FILE* pipe = popen("ffmpeg -version 2>&1", "r");
  if (!pipe)
  {
    CLog::Log(LOGWARNING, "AudioExtractor: Failed to execute ffmpeg command");
    return false;
  }

  char buffer[256];
  std::string output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
  {
    output += buffer;
  }

  int result = pclose(pipe);

  if (result == 0 && output.find("ffmpeg version") != std::string::npos)
  {
    CLog::Log(LOGINFO, "AudioExtractor: FFmpeg found - {}", output.substr(0, output.find('\n')));
    return true;
  }

  CLog::Log(LOGWARNING, "AudioExtractor: FFmpeg not available");
  return false;
}

std::string CAudioExtractor::BuildFFmpegCommand(const std::string& input,
                                                const std::string& output,
                                                int64_t startMs,
                                                int64_t durationMs)
{
  std::string cmd = "ffmpeg -y"; // Overwrite output

  // Input seeking (fast seek before input for better performance)
  if (startMs > 0)
  {
    cmd += StringUtils::Format(" -ss {:.3f}", startMs / 1000.0);
  }

  // Input file
  cmd += StringUtils::Format(" -i \"{}\"", input);

  // Duration (if chunking)
  if (durationMs > 0)
  {
    cmd += StringUtils::Format(" -t {:.3f}", durationMs / 1000.0);
  }

  // Audio extraction options
  cmd += " -vn"; // No video
  cmd += StringUtils::Format(" -ar {}", m_config.sampleRate);   // Sample rate
  cmd += StringUtils::Format(" -ac {}", m_config.channels);     // Channels
  cmd += StringUtils::Format(" -b:a {}k", m_config.bitrate);    // Bitrate
  cmd += StringUtils::Format(" -f {}", m_config.format);        // Output format

  // Output file
  cmd += StringUtils::Format(" \"{}\"", output);

  // Redirect stderr to suppress FFmpeg output
  cmd += " 2>/dev/null";

  return cmd;
}

bool CAudioExtractor::ExecuteFFmpeg(const std::string& command)
{
  CLog::Log(LOGDEBUG, "AudioExtractor: Executing: {}",
            command.substr(0, command.find(" 2>/dev/null")));

  int result = system(command.c_str());

  if (result == 0)
  {
    CLog::Log(LOGDEBUG, "AudioExtractor: Command completed successfully");
    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "AudioExtractor: Command failed with exit code {}", result);
    return false;
  }
}

bool CAudioExtractor::ExtractAudio(const std::string& videoPath,
                                   const std::string& outputPath)
{
  if (!IsFFmpegAvailable())
  {
    CLog::Log(LOGERROR, "AudioExtractor: FFmpeg not available");
    return false;
  }

  if (m_cancelled)
  {
    CLog::Log(LOGDEBUG, "AudioExtractor: Operation cancelled before starting");
    return false;
  }

  CLog::Log(LOGINFO, "AudioExtractor: Extracting audio from {} to {}", videoPath, outputPath);

  std::string cmd = BuildFFmpegCommand(videoPath, outputPath);

  bool success = ExecuteFFmpeg(cmd);

  if (success)
  {
    // Verify output file exists
    if (XFILE::CFile::Exists(outputPath))
    {
      CLog::Log(LOGINFO, "AudioExtractor: Successfully extracted audio to {}", outputPath);
      return true;
    }
    else
    {
      CLog::Log(LOGERROR, "AudioExtractor: Output file not created: {}", outputPath);
      return false;
    }
  }

  return false;
}

std::vector<AudioSegment> CAudioExtractor::ExtractChunked(const std::string& videoPath,
                                                          const std::string& outputDir)
{
  std::vector<AudioSegment> segments;

  if (!IsFFmpegAvailable())
  {
    CLog::Log(LOGERROR, "AudioExtractor: FFmpeg not available");
    return segments;
  }

  // Get total duration
  int64_t totalDurationMs = GetMediaDuration(videoPath);
  if (totalDurationMs <= 0)
  {
    CLog::Log(LOGERROR, "AudioExtractor: Could not get duration for {}", videoPath);
    return segments;
  }

  CLog::Log(LOGINFO, "AudioExtractor: Total duration: {:.2f} minutes", totalDurationMs / 60000.0);

  int64_t segmentDurationMs = static_cast<int64_t>(m_config.maxSegmentMinutes) * 60 * 1000;
  int segmentIndex = 0;
  int64_t currentMs = 0;

  while (currentMs < totalDurationMs && !m_cancelled)
  {
    int64_t remainingMs = totalDurationMs - currentMs;
    int64_t thisDurationMs = std::min(segmentDurationMs, remainingMs);

    // Generate output path
    std::string outputPath =
        StringUtils::Format("{}/segment_{:03d}.{}", outputDir, segmentIndex, m_config.format);

    CLog::Log(LOGINFO, "AudioExtractor: Extracting segment {} ({:.2f} - {:.2f} minutes)",
              segmentIndex, currentMs / 60000.0, (currentMs + thisDurationMs) / 60000.0);

    // Extract segment
    std::string cmd = BuildFFmpegCommand(videoPath, outputPath, currentMs, thisDurationMs);

    if (!ExecuteFFmpeg(cmd))
    {
      CLog::Log(LOGERROR, "AudioExtractor: Failed to extract segment {}", segmentIndex);
      break;
    }

    // Verify file size using __stat64
    struct __stat64 fileStat;
    if (XFILE::CFile::Stat(outputPath, &fileStat) == 0)
    {
      AudioSegment segment;
      segment.path = outputPath;
      segment.startMs = currentMs;
      segment.durationMs = thisDurationMs;
      segment.fileSizeBytes = fileStat.st_size;

      CLog::Log(LOGDEBUG, "AudioExtractor: Segment {} created - {:.2f} MB", segmentIndex,
                segment.fileSizeBytes / (1024.0 * 1024.0));

      segments.push_back(segment);
    }
    else
    {
      CLog::Log(LOGERROR, "AudioExtractor: Failed to stat output file: {}", outputPath);
      break;
    }

    currentMs += thisDurationMs;
    segmentIndex++;
  }

  if (m_cancelled)
  {
    CLog::Log(LOGINFO, "AudioExtractor: Extraction cancelled after {} segments", segments.size());
  }
  else
  {
    CLog::Log(LOGINFO, "AudioExtractor: Extracted {} segments from {}", segments.size(),
              videoPath);
  }

  return segments;
}

int64_t CAudioExtractor::GetMediaDuration(const std::string& path)
{
  std::string cmd = StringUtils::Format(
      "ffprobe -v error -show_entries format=duration "
      "-of default=noprint_wrappers=1:nokey=1 \"{}\" 2>&1",
      path);

  FILE* pipe = popen(cmd.c_str(), "r");
  if (!pipe)
  {
    CLog::Log(LOGERROR, "AudioExtractor: Failed to run ffprobe for {}", path);
    return -1;
  }

  char buffer[256];
  std::string output;
  while (fgets(buffer, sizeof(buffer), pipe) != nullptr)
  {
    output += buffer;
  }

  int result = pclose(pipe);

  if (result != 0)
  {
    CLog::Log(LOGERROR, "AudioExtractor: FFprobe failed for {}", path);
    return -1;
  }

  return ParseDurationFromFFprobe(output);
}

int64_t CAudioExtractor::ParseDurationFromFFprobe(const std::string& output)
{
  try
  {
    // Trim whitespace
    std::string trimmed = StringUtils::Trim(output);

    if (trimmed.empty() || trimmed == "N/A")
    {
      CLog::Log(LOGERROR, "AudioExtractor: No duration information available");
      return -1;
    }

    // Parse duration as double (seconds)
    double seconds = std::stod(trimmed);

    if (seconds <= 0)
    {
      CLog::Log(LOGERROR, "AudioExtractor: Invalid duration: {}", seconds);
      return -1;
    }

    int64_t durationMs = static_cast<int64_t>(seconds * 1000);
    CLog::Log(LOGDEBUG, "AudioExtractor: Parsed duration: {:.2f} seconds ({} ms)", seconds,
              durationMs);

    return durationMs;
  }
  catch (const std::exception& e)
  {
    CLog::Log(LOGERROR, "AudioExtractor: Could not parse duration from: '{}' - {}", output,
              e.what());
    return -1;
  }
}

void CAudioExtractor::CleanupSegments(const std::vector<AudioSegment>& segments)
{
  CLog::Log(LOGDEBUG, "AudioExtractor: Cleaning up {} segments", segments.size());

  for (const auto& segment : segments)
  {
    if (XFILE::CFile::Exists(segment.path))
    {
      if (XFILE::CFile::Delete(segment.path))
      {
        CLog::Log(LOGDEBUG, "AudioExtractor: Deleted {}", segment.path);
      }
      else
      {
        CLog::Log(LOGWARNING, "AudioExtractor: Failed to delete {}", segment.path);
      }
    }
  }
}

void CAudioExtractor::SetConfig(const AudioExtractionConfig& config)
{
  m_config = config;
  CLog::Log(LOGDEBUG,
            "AudioExtractor: Config updated - {}Hz, {} channels, {} format, {}kbps, {}min segments",
            m_config.sampleRate, m_config.channels, m_config.format, m_config.bitrate,
            m_config.maxSegmentMinutes);
}

const AudioExtractionConfig& CAudioExtractor::GetConfig() const
{
  return m_config;
}

void CAudioExtractor::Cancel()
{
  CLog::Log(LOGINFO, "AudioExtractor: Cancellation requested");
  m_cancelled = true;
}

bool CAudioExtractor::IsCancelled() const
{
  return m_cancelled;
}

} // namespace KODI::SEMANTIC
