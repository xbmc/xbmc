/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SubtitleParser.h"

#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstdio>
#include <regex>
#include <sstream>
#include <stdexcept>

using namespace KODI::SEMANTIC;

namespace
{
// Maximum subtitle file size to load (5 MB)
constexpr size_t MAX_SUBTITLE_SIZE = 5 * 1024 * 1024;

// Common language codes to check for subtitle discovery
const std::vector<std::string> LANGUAGE_CODES = {"en", "eng", "es", "spa",  "fr", "fre",
                                                  "de", "ger", "it", "ita",  "pt", "por",
                                                  "ru", "rus", "ja", "jpn",  "zh", "chi",
                                                  "ko", "kor", "ar", "ara",  "hi", "hin",
                                                  "nl", "dut", "sv", "swe",  "no", "nor",
                                                  "da", "dan", "fi", "fin",  "pl", "pol",
                                                  "tr", "tur", "el", "gre",  "he", "heb",
                                                  "cs", "cze", "hu", "hun",  "ro", "rum",
                                                  "th", "tha", "vi", "vie",  "id", "ind"};

} // namespace

std::vector<ParsedEntry> CSubtitleParser::Parse(const std::string& path)
{
  if (!CanParse(path))
  {
    throw std::runtime_error("Unsupported subtitle format: " + path);
  }

  // Load and convert file content to UTF-8
  std::string content = LoadFileContent(path);

  // Detect and convert charset if needed
  if (!DetectCharset(content))
  {
    CLog::LogF(LOGWARNING, "Failed to detect charset for: {}", path);
  }

  // Determine format and parse
  std::string ext = URIUtils::GetExtension(path);
  ext = StringUtils::ToLower(ext);

  std::vector<ParsedEntry> entries;

  if (ext == ".srt")
  {
    entries = ParseSRT(content);
  }
  else if (ext == ".ass" || ext == ".ssa")
  {
    entries = ParseASS(content);
  }
  else if (ext == ".vtt")
  {
    entries = ParseVTT(content);
  }
  else
  {
    throw std::runtime_error("Unknown subtitle extension: " + ext);
  }

  CLog::LogF(LOGDEBUG, "Parsed {} entries from {}", entries.size(), path);
  return entries;
}

bool CSubtitleParser::CanParse(const std::string& path) const
{
  return HasSupportedExtension(path, GetSupportedExtensions());
}

std::vector<std::string> CSubtitleParser::GetSupportedExtensions() const
{
  return {"srt", "ass", "ssa", "vtt"};
}

std::vector<ParsedEntry> CSubtitleParser::ParseSRT(const std::string& content)
{
  std::vector<ParsedEntry> entries;
  std::istringstream stream(content);
  std::string line;

  int lineNum = 0;
  int64_t startMs = -1;
  int64_t endMs = -1;
  std::string text;

  auto addEntry = [&]()
  {
    if (startMs >= 0 && endMs >= 0 && !text.empty())
    {
      // Strip HTML tags and normalize whitespace
      StripHTMLTags(text);
      NormalizeWhitespace(text);

      // Filter out non-dialogue content
      if (!IsNonDialogue(text))
      {
        ParsedEntry entry;
        entry.startMs = startMs;
        entry.endMs = endMs;
        entry.text = text;
        entry.confidence = 1.0f;
        entries.push_back(entry);
      }
    }
    startMs = -1;
    endMs = -1;
    text.clear();
  };

  while (std::getline(stream, line))
  {
    lineNum++;
    StringUtils::Trim(line);

    if (line.empty())
    {
      // Empty line signals end of entry
      addEntry();
      continue;
    }

    // Check for timestamp line: "00:00:01,000 --> 00:00:04,500"
    size_t arrowPos = line.find("-->");
    if (arrowPos != std::string::npos)
    {
      // Save any previous entry
      addEntry();

      std::string startStr = line.substr(0, arrowPos);
      std::string endStr = line.substr(arrowPos + 3);
      StringUtils::Trim(startStr);
      StringUtils::Trim(endStr);

      startMs = ParseSRTTimestamp(startStr);
      endMs = ParseSRTTimestamp(endStr);

      if (startMs < 0 || endMs < 0)
      {
        CLog::LogF(LOGWARNING, "Invalid SRT timestamp at line {}: {}", lineNum, line);
        startMs = -1;
        endMs = -1;
      }
      continue;
    }

    // Check for sequence number (just digits)
    bool isNumber = !line.empty() &&
                    std::all_of(line.begin(), line.end(), [](char c) { return std::isdigit(c); });
    if (isNumber)
    {
      // Skip sequence numbers
      continue;
    }

    // Otherwise it's subtitle text
    if (startMs >= 0)
    {
      if (!text.empty())
        text += "\n";
      text += line;
    }
  }

  // Add final entry if any
  addEntry();

  return entries;
}

std::vector<ParsedEntry> CSubtitleParser::ParseASS(const std::string& content)
{
  std::vector<ParsedEntry> entries;
  std::istringstream stream(content);
  std::string line;

  bool inEvents = false;
  int textFieldIndex = -1;
  int startFieldIndex = -1;
  int endFieldIndex = -1;
  int styleFieldIndex = -1;

  while (std::getline(stream, line))
  {
    StringUtils::Trim(line);

    if (line.empty() || line[0] == ';')
    {
      // Skip empty lines and comments
      continue;
    }

    // Check for [Events] section
    if (StringUtils::StartsWith(line, "[Events]"))
    {
      inEvents = true;
      continue;
    }

    // Check for other sections
    if (line[0] == '[')
    {
      inEvents = false;
      continue;
    }

    if (!inEvents)
      continue;

    // Parse Format line to find field indices
    if (StringUtils::StartsWithNoCase(line, "Format:"))
    {
      std::string formatLine = line.substr(7); // Skip "Format:"
      StringUtils::Trim(formatLine);

      std::vector<std::string> fields = StringUtils::Split(formatLine, ",");
      for (size_t i = 0; i < fields.size(); i++)
      {
        std::string field = StringUtils::Trim(fields[i]);
        if (StringUtils::EqualsNoCase(field, "Start"))
          startFieldIndex = i;
        else if (StringUtils::EqualsNoCase(field, "End"))
          endFieldIndex = i;
        else if (StringUtils::EqualsNoCase(field, "Text"))
          textFieldIndex = i;
        else if (StringUtils::EqualsNoCase(field, "Style"))
          styleFieldIndex = i;
      }
      continue;
    }

    // Parse Dialogue line
    if (StringUtils::StartsWithNoCase(line, "Dialogue:"))
    {
      if (textFieldIndex < 0 || startFieldIndex < 0 || endFieldIndex < 0)
      {
        CLog::LogF(LOGWARNING, "ASS Dialogue line before Format line");
        continue;
      }

      std::string dialogueLine = line.substr(9); // Skip "Dialogue:"
      StringUtils::Trim(dialogueLine);

      // Split by comma, but be careful - Text field may contain commas
      std::vector<std::string> fields;
      size_t pos = 0;
      int fieldCount = 0;
      while (pos < dialogueLine.length() && fieldCount < textFieldIndex)
      {
        size_t commaPos = dialogueLine.find(',', pos);
        if (commaPos == std::string::npos)
          break;
        fields.push_back(dialogueLine.substr(pos, commaPos - pos));
        pos = commaPos + 1;
        fieldCount++;
      }

      // Rest of the line is the text field
      if (pos < dialogueLine.length())
      {
        fields.push_back(dialogueLine.substr(pos));
      }

      if (static_cast<int>(fields.size()) <= textFieldIndex)
      {
        CLog::LogF(LOGWARNING, "Invalid ASS Dialogue line: {}", line);
        continue;
      }

      // Extract fields
      int64_t startMs = ParseASSTimestamp(StringUtils::Trim(fields[startFieldIndex]));
      int64_t endMs = ParseASSTimestamp(StringUtils::Trim(fields[endFieldIndex]));
      std::string text = StringUtils::Trim(fields[textFieldIndex]);
      std::string style = (styleFieldIndex >= 0 && styleFieldIndex < static_cast<int>(fields.size()))
                              ? StringUtils::Trim(fields[styleFieldIndex])
                              : "";

      if (startMs < 0 || endMs < 0)
      {
        CLog::LogF(LOGWARNING, "Invalid ASS timestamp: {}", line);
        continue;
      }

      // Strip ASS formatting codes
      StripASSCodes(text);

      // Strip HTML tags that may be in the text
      StripHTMLTags(text);
      NormalizeWhitespace(text);

      // Filter non-dialogue
      if (text.empty() || IsNonDialogue(text))
        continue;

      ParsedEntry entry;
      entry.startMs = startMs;
      entry.endMs = endMs;
      entry.text = text;
      entry.confidence = 1.0f;

      // Use style as speaker if it's not "Default" or "*Default"
      if (!style.empty() && !StringUtils::EqualsNoCase(style, "Default") &&
          !StringUtils::EqualsNoCase(style, "*Default"))
      {
        entry.speaker = style;
      }

      entries.push_back(entry);
    }
  }

  return entries;
}

std::vector<ParsedEntry> CSubtitleParser::ParseVTT(const std::string& content)
{
  std::vector<ParsedEntry> entries;
  std::istringstream stream(content);
  std::string line;

  bool headerPassed = false;
  int64_t startMs = -1;
  int64_t endMs = -1;
  std::string text;
  bool inNote = false;

  auto addEntry = [&]()
  {
    if (startMs >= 0 && endMs >= 0 && !text.empty())
    {
      // Strip HTML tags and normalize whitespace
      StripHTMLTags(text);
      NormalizeWhitespace(text);

      // Filter out non-dialogue content
      if (!IsNonDialogue(text))
      {
        ParsedEntry entry;
        entry.startMs = startMs;
        entry.endMs = endMs;
        entry.text = text;
        entry.confidence = 1.0f;
        entries.push_back(entry);
      }
    }
    startMs = -1;
    endMs = -1;
    text.clear();
  };

  while (std::getline(stream, line))
  {
    StringUtils::Trim(line);

    // Check for WEBVTT header
    if (!headerPassed)
    {
      if (StringUtils::StartsWithNoCase(line, "WEBVTT"))
      {
        headerPassed = true;
      }
      continue;
    }

    // Check for NOTE blocks
    if (StringUtils::StartsWithNoCase(line, "NOTE"))
    {
      inNote = true;
      continue;
    }

    if (line.empty())
    {
      inNote = false;
      // Empty line signals end of entry
      addEntry();
      continue;
    }

    if (inNote)
    {
      // Skip lines in NOTE block
      continue;
    }

    // Check for timestamp line: "00:00:01.000 --> 00:00:04.500"
    // May have cue settings after the end time
    size_t arrowPos = line.find("-->");
    if (arrowPos != std::string::npos)
    {
      // Save any previous entry
      addEntry();

      std::string startStr = line.substr(0, arrowPos);
      StringUtils::Trim(startStr);

      std::string endPart = line.substr(arrowPos + 3);
      StringUtils::Trim(endPart);

      // End time may have cue settings (e.g., "00:00:04.500 align:start")
      // Extract just the timestamp
      std::string endStr = endPart;
      size_t spacePos = endPart.find(' ');
      if (spacePos != std::string::npos)
      {
        endStr = endPart.substr(0, spacePos);
        StringUtils::Trim(endStr);
      }

      startMs = ParseVTTTimestamp(startStr);
      endMs = ParseVTTTimestamp(endStr);

      if (startMs < 0 || endMs < 0)
      {
        CLog::LogF(LOGWARNING, "Invalid VTT timestamp: {}", line);
        startMs = -1;
        endMs = -1;
      }
      continue;
    }

    // Check if it's a cue identifier (optional text before timestamp)
    // Skip lines that look like identifiers (no --> and we haven't seen a timestamp yet)
    if (startMs < 0)
    {
      // This is likely a cue identifier, skip it
      continue;
    }

    // Otherwise it's subtitle text
    if (startMs >= 0)
    {
      if (!text.empty())
        text += "\n";
      text += line;
    }
  }

  // Add final entry if any
  addEntry();

  return entries;
}

int64_t CSubtitleParser::ParseSRTTimestamp(const std::string& timestamp)
{
  // Format: HH:MM:SS,mmm
  int hours, minutes, seconds, milliseconds;
  char sep1, sep2, sep3;

  int result = std::sscanf(timestamp.c_str(), "%d%c%d%c%d%c%d", &hours, &sep1, &minutes, &sep2,
                           &seconds, &sep3, &milliseconds);

  if (result == 7 && sep1 == ':' && sep2 == ':' && sep3 == ',')
  {
    return static_cast<int64_t>(hours) * 3600000 + static_cast<int64_t>(minutes) * 60000 +
           static_cast<int64_t>(seconds) * 1000 + static_cast<int64_t>(milliseconds);
  }

  return -1;
}

int64_t CSubtitleParser::ParseASSTimestamp(const std::string& timestamp)
{
  // Format: H:MM:SS.cc (centiseconds)
  int hours, minutes, seconds, centiseconds;
  char sep1, sep2, sep3;

  int result = std::sscanf(timestamp.c_str(), "%d%c%d%c%d%c%d", &hours, &sep1, &minutes, &sep2,
                           &seconds, &sep3, &centiseconds);

  if (result == 7 && sep1 == ':' && sep2 == ':' && (sep3 == '.' || sep3 == ','))
  {
    // Convert centiseconds to milliseconds
    return static_cast<int64_t>(hours) * 3600000 + static_cast<int64_t>(minutes) * 60000 +
           static_cast<int64_t>(seconds) * 1000 + static_cast<int64_t>(centiseconds) * 10;
  }

  return -1;
}

int64_t CSubtitleParser::ParseVTTTimestamp(const std::string& timestamp)
{
  // Format: HH:MM:SS.mmm or MM:SS.mmm
  int hours = 0, minutes = 0, seconds = 0, milliseconds = 0;

  // Try HH:MM:SS.mmm first
  int result = std::sscanf(timestamp.c_str(), "%d:%d:%d.%d", &hours, &minutes, &seconds, &milliseconds);

  if (result == 4)
  {
    return static_cast<int64_t>(hours) * 3600000 + static_cast<int64_t>(minutes) * 60000 +
           static_cast<int64_t>(seconds) * 1000 + static_cast<int64_t>(milliseconds);
  }

  // Try MM:SS.mmm
  result = std::sscanf(timestamp.c_str(), "%d:%d.%d", &minutes, &seconds, &milliseconds);

  if (result == 3)
  {
    return static_cast<int64_t>(minutes) * 60000 + static_cast<int64_t>(seconds) * 1000 +
           static_cast<int64_t>(milliseconds);
  }

  return -1;
}

void CSubtitleParser::StripASSCodes(std::string& text)
{
  if (text.empty())
    return;

  // Remove ASS/SSA formatting codes: {\ ... }
  // This handles codes like {\an8}, {\pos(x,y)}, {\fad(in,out)}, etc.
  std::regex assCodeRegex(R"(\{\\[^}]*\})");
  text = std::regex_replace(text, assCodeRegex, "");

  // Also handle drawing commands: {\p1}...{\p0}
  std::regex drawingRegex(R"(\{\\p\d+\})");
  text = std::regex_replace(text, drawingRegex, "");

  // Remove any remaining curly braces that are just formatting
  std::regex emptyBraces(R"(\{\})");
  text = std::regex_replace(text, emptyBraces, "");
}

std::string CSubtitleParser::LoadFileContent(const std::string& path)
{
  XFILE::CFile file;

  if (!file.Open(path))
  {
    throw std::runtime_error("Failed to open subtitle file: " + path);
  }

  // Check file size
  int64_t fileSize = file.GetLength();
  if (fileSize < 0)
  {
    throw std::runtime_error("Failed to get file size: " + path);
  }

  if (fileSize > static_cast<int64_t>(MAX_SUBTITLE_SIZE))
  {
    throw std::runtime_error("Subtitle file too large: " + path);
  }

  // Load file content
  std::vector<uint8_t> buffer(fileSize);
  ssize_t bytesRead = file.Read(buffer.data(), fileSize);

  if (bytesRead != fileSize)
  {
    throw std::runtime_error("Failed to read complete subtitle file: " + path);
  }

  file.Close();

  // Convert to string
  return std::string(reinterpret_cast<const char*>(buffer.data()), bytesRead);
}

std::string CSubtitleParser::FindSubtitleForMedia(const std::string& mediaPath)
{
  if (mediaPath.empty())
    return "";

  // Get directory and filename without extension
  std::string directory = URIUtils::GetDirectory(mediaPath);
  std::string baseName = URIUtils::GetFileName(mediaPath);

  // Remove extension
  size_t dotPos = baseName.rfind('.');
  if (dotPos != std::string::npos)
  {
    baseName = baseName.substr(0, dotPos);
  }

  // Supported subtitle extensions
  std::vector<std::string> subtitleExts = {"srt", "ass", "ssa", "vtt"};

  // Check various subtitle file patterns in order of preference
  std::vector<std::string> patterns;

  // 1. Exact match: movie.srt
  for (const auto& ext : subtitleExts)
  {
    patterns.push_back(baseName + "." + ext);
  }

  // 2. With language codes: movie.en.srt, movie.eng.srt, etc.
  for (const auto& langCode : LANGUAGE_CODES)
  {
    for (const auto& ext : subtitleExts)
    {
      patterns.push_back(baseName + "." + langCode + "." + ext);
    }
  }

  // Check in same directory
  for (const auto& pattern : patterns)
  {
    std::string subtitlePath = URIUtils::AddFileToFolder(directory, pattern);
    if (XFILE::CFile::Exists(subtitlePath))
    {
      CLog::LogF(LOGDEBUG, "Found subtitle for {}: {}", mediaPath, subtitlePath);
      return subtitlePath;
    }
  }

  // Check in Subs/ subdirectory
  std::string subsDir = URIUtils::AddFileToFolder(directory, "Subs");
  if (XFILE::CDirectory::Exists(subsDir))
  {
    for (const auto& pattern : patterns)
    {
      std::string subtitlePath = URIUtils::AddFileToFolder(subsDir, pattern);
      if (XFILE::CFile::Exists(subtitlePath))
      {
        CLog::LogF(LOGDEBUG, "Found subtitle for {}: {}", mediaPath, subtitlePath);
        return subtitlePath;
      }
    }
  }

  CLog::LogF(LOGDEBUG, "No subtitle found for {}", mediaPath);
  return "";
}
