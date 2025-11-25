/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ContentParserBase.h"

#include <string>
#include <vector>

namespace KODI::SEMANTIC
{

/*!
 * \brief Parser for subtitle files (SRT, ASS, VTT)
 *
 * This parser handles common subtitle formats used in media playback:
 * - SRT (SubRip) - Simple text-based format
 * - ASS/SSA (Advanced SubStation Alpha) - Advanced formatting support
 * - VTT (WebVTT) - Web Video Text Tracks
 *
 * The parser extracts dialogue text, timing information, and speaker
 * identification where available.
 */
class CSubtitleParser : public ContentParserBase
{
public:
  /*!
   * \brief Parse subtitle file from path
   * \param path The subtitle file path
   * \return Vector of parsed entries with timing and text
   * \throws std::runtime_error if file cannot be read or format is invalid
   */
  std::vector<ParsedEntry> Parse(const std::string& path) override;

  /*!
   * \brief Check if this parser can handle the file
   * \param path The file path to check
   * \return true if file extension is srt, ass, ssa, or vtt
   */
  bool CanParse(const std::string& path) const override;

  /*!
   * \brief Get supported file extensions
   * \return Vector of extensions: srt, ass, ssa, vtt
   */
  std::vector<std::string> GetSupportedExtensions() const override;

  /*!
   * \brief Find subtitle file for a media file
   * \param mediaPath The path to the media file (video)
   * \return Path to subtitle file if found, empty string otherwise
   *
   * Searches for subtitle files in:
   * 1. Same directory as media file (e.g., movie.srt, movie.en.srt)
   * 2. Subs/ subdirectory
   * Supports multiple language codes in filename (e.g., .en, .eng)
   */
  static std::string FindSubtitleForMedia(const std::string& mediaPath);

private:
  /*!
   * \brief Parse SRT (SubRip) format
   * \param content The file content as UTF-8 string
   * \return Vector of parsed entries
   *
   * Format:
   * 1
   * 00:00:01,000 --> 00:00:04,500
   * Hello, world!
   */
  std::vector<ParsedEntry> ParseSRT(const std::string& content);

  /*!
   * \brief Parse ASS/SSA (Advanced SubStation Alpha) format
   * \param content The file content as UTF-8 string
   * \return Vector of parsed entries
   *
   * Format:
   * [Events]
   * Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
   * Dialogue: 0,0:00:01.00,0:00:04.50,Default,,0,0,0,,Hello, world!
   */
  std::vector<ParsedEntry> ParseASS(const std::string& content);

  /*!
   * \brief Parse VTT (WebVTT) format
   * \param content The file content as UTF-8 string
   * \return Vector of parsed entries
   *
   * Format:
   * WEBVTT
   *
   * 00:00:01.000 --> 00:00:04.500
   * Hello, world!
   */
  std::vector<ParsedEntry> ParseVTT(const std::string& content);

  /*!
   * \brief Parse SRT timestamp (HH:MM:SS,mmm)
   * \param timestamp The timestamp string
   * \return Time in milliseconds, or -1 if invalid
   */
  static int64_t ParseSRTTimestamp(const std::string& timestamp);

  /*!
   * \brief Parse ASS timestamp (H:MM:SS.cc, centiseconds)
   * \param timestamp The timestamp string
   * \return Time in milliseconds, or -1 if invalid
   */
  static int64_t ParseASSTimestamp(const std::string& timestamp);

  /*!
   * \brief Parse VTT timestamp (HH:MM:SS.mmm)
   * \param timestamp The timestamp string
   * \return Time in milliseconds, or -1 if invalid
   */
  static int64_t ParseVTTTimestamp(const std::string& timestamp);

  /*!
   * \brief Strip ASS/SSA formatting codes from text
   * \param text The text to process (modified in-place)
   *
   * Removes codes like:
   * - {\an8} - Alignment
   * - {\pos(x,y)} - Position
   * - {\fad(in,out)} - Fade timing
   * - {\i1}, {\b1} - Italic, bold
   */
  static void StripASSCodes(std::string& text);

  /*!
   * \brief Load file content as UTF-8 string
   * \param path The file path
   * \return File content as UTF-8 string
   * \throws std::runtime_error if file cannot be read
   */
  static std::string LoadFileContent(const std::string& path);
};

} // namespace KODI::SEMANTIC
