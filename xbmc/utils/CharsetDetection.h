/*
*      Copyright (C) 2013 Team XBMC
*      http://kodi.tv
*
*  This Program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This Program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with XBMC; see the file COPYING.  If not, see
*  <http://www.gnu.org/licenses/>.
*
*/

#pragma once

#include <string>


class CCharsetDetection
{
public:
  /**
   * Detect text encoding by Byte Order Mark
   * Multibyte encodings (UTF-16/32) always ends with explicit endianness (LE/BE)
   * @param content     pointer to text to analyze
   * @param contentLength       length of text
   * @return detected encoding or empty string if BOM not detected
   */
  static std::string GetBomEncoding(const char* const content, const size_t contentLength);
  /**
   * Detect text encoding by Byte Order Mark
   * Multibyte encodings (UTF-16/32) always ends with explicit endianness (LE/BE)
   * @param content     the text to analyze
   * @return detected encoding or empty string if BOM not detected
   */
  static inline std::string GetBomEncoding(const std::string& content)
  { return GetBomEncoding(content.c_str(), content.length()); }

  static inline bool DetectXmlEncoding(const std::string& xmlContent, std::string& detectedEncoding)
  { return DetectXmlEncoding(xmlContent.c_str(), xmlContent.length(), detectedEncoding); }

  static bool DetectXmlEncoding(const char* const xmlContent, const size_t contentLength, std::string& detectedEncoding);

  /**
   * Detect HTML charset and HTML convert to UTF-8
   * @param htmlContent content of HTML file
   * @param converted   receive result of conversion
   * @param serverReportedCharset charset from HTTP header or from other out-of-band source, empty if unknown or unset
   * @return true if charset is properly detected and HTML is correctly converted, false if charset is only guessed
   */
  static inline bool ConvertHtmlToUtf8(const std::string& htmlContent, std::string& converted, const std::string& serverReportedCharset = "")
  {
    std::string usedHtmlCharset;
    return ConvertHtmlToUtf8(htmlContent, converted, serverReportedCharset, usedHtmlCharset);
  }
  /**
   * Detect HTML charset and HTML convert to UTF-8
   * @param htmlContent content of HTML file
   * @param converted   receive result of conversion
   * @param serverReportedCharset charset from HTTP header or from other out-of-band source, empty if unknown or unset
   * @param usedHtmlCharset       receive charset used for conversion
   * @return true if charset is properly detected and HTML is correctly converted, false if charset is only guessed
   */
  static bool ConvertHtmlToUtf8(const std::string& htmlContent, std::string& converted, const std::string& serverReportedCharset, std::string& usedHtmlCharset);

  /**
  * Try to convert plain text to UTF-8 using best suitable charset
  * @param textContent text to convert
  * @param converted   receive result of conversion
  * @param serverReportedCharset charset from HTTP header or from other out-of-band source, empty if unknown or unset
  * @param usedCharset       receive charset used for conversion
  * @return true if converted without errors, false otherwise
  */
  static bool ConvertPlainTextToUtf8(const std::string& textContent, std::string& converted, const std::string& serverReportedCharset, std::string& usedCharset);

private:
  static bool GetXmlEncodingFromDeclaration(const char* const xmlContent, const size_t contentLength, std::string& declaredEncoding);
  /**
   * Try to guess text encoding by searching for '<?xml' mark in different encodings
   * Multibyte encodings (UTF/UCS) always ends with explicit endianness (LE/BE)
   * @param content     pointer to text to analyze
   * @param contentLength       length of text
   * @param detectedEncoding    reference to variable that receive supposed encoding
   * @return true if any encoding supposed, false otherwise
   */
  static bool GuessXmlEncoding(const char* const xmlContent, const size_t contentLength, std::string& supposedEncoding);

  static std::string GetHtmlEncodingFromHead(const std::string& htmlContent);
  static size_t GetHtmlAttribute(const std::string& htmlContent, size_t pos, std::string& atrName, std::string& strValue);
  static std::string ExtractEncodingFromHtmlMeta(std::string metaContent, size_t pos = 0);

  static bool checkConversion(const std::string& srcCharset, const std::string& src, std::string& dst);
  static void appendCharAsAsciiUpperCase(std::string& str, const char chr);

  static const size_t m_XmlDeclarationMaxLength;
  static const size_t m_HtmlCharsetEndSearchPos;

  static const std::string m_HtmlWhitespaceChars;
};
