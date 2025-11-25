/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GroqProvider.h"

#include "ServiceBroker.h"
#include "filesystem/CurlFile.h"
#include "filesystem/File.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/JSONVariantParser.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"

#include <random>
#include <sstream>
#include <vector>

using namespace XFILE;

namespace KODI::SEMANTIC
{

namespace
{
// Generate a random boundary for multipart/form-data
std::string GenerateBoundary()
{
  static const char alphanum[] =
      "0123456789"
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz";

  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, sizeof(alphanum) - 2);

  std::string boundary = "----KodiBoundary";
  for (int i = 0; i < 16; ++i)
    boundary += alphanum[dis(gen)];

  return boundary;
}

// Read entire file into memory
bool ReadFileContents(const std::string& filePath, std::vector<uint8_t>& data)
{
  CFile file;
  if (!file.Open(filePath, READ_TRUNCATED))
  {
    CLog::LogF(LOGERROR, "Failed to open file: {}", filePath);
    return false;
  }

  int64_t fileSize = file.GetLength();
  if (fileSize <= 0)
  {
    CLog::LogF(LOGERROR, "Invalid file size: {}", fileSize);
    return false;
  }

  data.resize(fileSize);
  ssize_t bytesRead = file.Read(data.data(), fileSize);
  file.Close();

  if (bytesRead != fileSize)
  {
    CLog::LogF(LOGERROR, "Failed to read complete file. Expected {} bytes, got {}", fileSize,
               bytesRead);
    return false;
  }

  return true;
}

} // anonymous namespace

CGroqProvider::CGroqProvider()
{
  m_apiKey = GetApiKey();
}

bool CGroqProvider::IsConfigured() const
{
  return !m_apiKey.empty();
}

bool CGroqProvider::IsAvailable() const
{
  // TODO: Could add network connectivity check here
  return IsConfigured();
}

std::string CGroqProvider::GetApiKey() const
{
  auto settingsComponent = CServiceBroker::GetSettingsComponent();
  if (!settingsComponent)
    return "";

  auto settings = settingsComponent->GetSettings();
  if (!settings)
    return "";

  std::string apiKey = settings->GetString(CSettings::SETTING_SEMANTIC_GROQ_APIKEY);
  return apiKey;
}

int64_t CGroqProvider::GetFileSize(const std::string& filePath) const
{
  struct __stat64 buffer;
  if (CFile::Stat(filePath, &buffer) == 0)
    return buffer.st_size;
  return -1;
}

float CGroqProvider::EstimateCost(int64_t durationMs) const
{
  float minutes = durationMs / 60000.0f;
  return minutes * COST_PER_MINUTE;
}

void CGroqProvider::Cancel()
{
  m_cancelled = true;
}

bool CGroqProvider::Transcribe(const std::string& audioPath,
                                SegmentCallback onSegment,
                                ProgressCallback onProgress,
                                ErrorCallback onError)
{
  m_cancelled = false;

  if (!IsConfigured())
  {
    if (onError)
      onError("Groq API key not configured");
    return false;
  }

  int64_t fileSize = GetFileSize(audioPath);
  if (fileSize < 0)
  {
    if (onError)
      onError("Failed to get file size");
    return false;
  }

  CLog::LogF(LOGINFO, "Transcribing file: {} (size: {} bytes)", audioPath, fileSize);

  // Check if file needs to be chunked
  if (fileSize > MAX_FILE_SIZE)
  {
    CLog::LogF(LOGINFO, "File exceeds {}MB limit, using chunked transcription", MAX_FILE_SIZE / (1024 * 1024));
    return TranscribeLargeFile(audioPath, onSegment, onProgress, onError);
  }

  // Report initial progress
  if (onProgress)
    onProgress(0.1f);

  bool success = TranscribeFile(audioPath, onSegment, onError);

  // Report completion
  if (onProgress)
    onProgress(success ? 1.0f : 0.0f);

  return success;
}

bool CGroqProvider::TranscribeFile(const std::string& audioPath,
                                   SegmentCallback onSegment,
                                   ErrorCallback onError)
{
  if (m_cancelled)
    return false;

  // Generate boundary for multipart request
  std::string boundary = GenerateBoundary();
  std::string postData;

  if (!BuildMultipartRequest(audioPath, boundary, postData))
  {
    if (onError)
      onError("Failed to build multipart request");
    return false;
  }

  // Create HTTP client
  CCurlFile curl;
  curl.SetRequestHeader("Authorization", "Bearer " + m_apiKey);
  curl.SetMimeType("multipart/form-data; boundary=" + boundary);
  curl.SetTimeout(CONNECT_TIMEOUT_SEC);

  // Send request
  std::string response;
  CLog::LogF(LOGDEBUG, "Sending transcription request to Groq API");

  if (!curl.Post(API_ENDPOINT, postData, response))
  {
    CLog::LogF(LOGERROR, "Failed to send transcription request");
    if (onError)
      onError("Failed to communicate with Groq API");
    return false;
  }

  if (m_cancelled)
    return false;

  CLog::LogF(LOGDEBUG, "Received response from Groq API ({} bytes)", response.size());

  // Parse response
  return ParseTranscriptionResponse(response, onSegment);
}

bool CGroqProvider::BuildMultipartRequest(const std::string& audioPath,
                                          const std::string& boundary,
                                          std::string& postData)
{
  std::vector<uint8_t> fileData;
  if (!ReadFileContents(audioPath, fileData))
    return false;

  std::ostringstream oss;
  std::string filename = URIUtils::GetFileName(audioPath);

  // Build multipart/form-data body
  // Add file field
  oss << "--" << boundary << "\r\n";
  oss << "Content-Disposition: form-data; name=\"file\"; filename=\"" << filename << "\"\r\n";
  oss << "Content-Type: application/octet-stream\r\n";
  oss << "\r\n";

  // Append file data
  std::string prefix = oss.str();
  postData.reserve(prefix.size() + fileData.size() + 256);
  postData = prefix;
  postData.append(reinterpret_cast<const char*>(fileData.data()), fileData.size());
  postData += "\r\n";

  // Add model field
  postData += "--" + boundary + "\r\n";
  postData += "Content-Disposition: form-data; name=\"model\"\r\n";
  postData += "\r\n";
  postData += MODEL_NAME;
  postData += "\r\n";

  // Add response_format field
  postData += "--" + boundary + "\r\n";
  postData += "Content-Disposition: form-data; name=\"response_format\"\r\n";
  postData += "\r\n";
  postData += RESPONSE_FORMAT;
  postData += "\r\n";

  // Add timestamp_granularities field (for word-level timestamps)
  postData += "--" + boundary + "\r\n";
  postData += "Content-Disposition: form-data; name=\"timestamp_granularities[]\"\r\n";
  postData += "\r\n";
  postData += "segment";
  postData += "\r\n";

  // Final boundary
  postData += "--" + boundary + "--\r\n";

  CLog::LogF(LOGDEBUG, "Built multipart request: {} bytes", postData.size());
  return true;
}

bool CGroqProvider::ParseTranscriptionResponse(const std::string& jsonResponse,
                                                SegmentCallback onSegment)
{
  CVariant data;
  if (!CJSONVariantParser::Parse(jsonResponse, data))
  {
    CLog::LogF(LOGERROR, "Failed to parse JSON response");
    CLog::LogF(LOGDEBUG, "Response: {}", jsonResponse);
    return false;
  }

  // Check for error
  if (data.isMember("error"))
  {
    std::string errorMsg = data["error"]["message"].asString("Unknown error");
    CLog::LogF(LOGERROR, "API error: {}", errorMsg);
    return false;
  }

  // Extract language
  std::string language = data["language"].asString("en");

  // Parse segments
  if (!data.isMember("segments") || !data["segments"].isArray())
  {
    CLog::LogF(LOGERROR, "Response missing segments array");
    return false;
  }

  const CVariant& segments = data["segments"];
  CLog::LogF(LOGINFO, "Parsed {} segments", segments.size());

  for (size_t i = 0; i < segments.size(); ++i)
  {
    if (m_cancelled)
      return false;

    const CVariant& seg = segments[i];

    TranscriptSegment segment;
    segment.startMs = static_cast<int64_t>(seg["start"].asDouble() * 1000);
    segment.endMs = static_cast<int64_t>(seg["end"].asDouble() * 1000);
    segment.text = seg["text"].asString();
    segment.language = language;

    // Confidence may not be available in all responses
    segment.confidence = seg["confidence"].asFloat(0.0f);

    // Trim whitespace from text
    StringUtils::Trim(segment.text);

    if (!segment.text.empty() && onSegment)
      onSegment(segment);
  }

  return true;
}

bool CGroqProvider::TranscribeLargeFile(const std::string& audioPath,
                                        SegmentCallback onSegment,
                                        ProgressCallback onProgress,
                                        ErrorCallback onError)
{
  // For now, reject large files
  // TODO: Implement audio chunking using FFmpeg
  // This would involve:
  // 1. Split audio into ~45 minute chunks (to stay well under 25MB)
  // 2. Transcribe each chunk
  // 3. Adjust timestamps for subsequent chunks
  // 4. Merge segments

  CLog::LogF(LOGERROR, "Large file transcription not yet implemented");
  if (onError)
    onError("File too large (>25MB). Chunked transcription not yet implemented.");

  return false;
}

} // namespace KODI::SEMANTIC
