/*
*      Copyright (C) 2013 Team XBMC
*      http://xbmc.org
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

#include <algorithm>
#include "CharsetDetection.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"

/* XML declaration can be virtually any size (with many-many whitespaces) 
 * but for in real world we don't need to process megabytes of data
 * so limit search for XML declaration to reasonable value */
const size_t CCharsetDetection::m_XmlDeclarationMaxLength = 250;


std::string CCharsetDetection::GetBomEncoding(const char* const content, const size_t contentLength)
{
  if (contentLength < 2)
    return "";
  if (content[0] == (char)0xFE && content[1] == (char)0xFF)
    return "UTF-16BE";
  if (contentLength >= 4 && content[0] == (char)0xFF && content[1] == (char)0xFE && content[2] == (char)0x00 && content[3] == (char)0x00)
    return "UTF-32LE";  /* first two bytes are same for UTF-16LE and UTF-32LE, so first check for full UTF-32LE mark */
  if (content[0] == (char)0xFF && content[1] == (char)0xFE)
   return "UTF-16LE";
  if (contentLength < 3)
    return "";
  if (content[0] == (char)0xEF && content[1] == (char)0xBB && content[2] == (char)0xBF)
    return "UTF-8";
  if (contentLength < 4)
    return "";
  if (content[0] == (char)0x00 && content[1] == (char)0x00 && content[2] == (char)0xFE && content[3] == (char)0xFF)
    return "UTF-32BE";
  if (contentLength >= 5 && content[0] == (char)0x2B && content[1] == (char)0x2F && content[2] == (char)0x76 &&
            (content[4] == (char)0x32 || content[4] == (char)0x39 || content[4] == (char)0x2B || content[4] == (char)0x2F))
    return "UTF-7";
  if (content[0] == (char)0x84 && content[1] == (char)0x31 && content[2] == (char)0x95 && content[3] == (char)0x33)
    return "GB18030";

  return "";
}

bool CCharsetDetection::DetectXmlEncoding(const char* const xmlContent, const size_t contentLength, std::string& detectedEncoding)
{
  detectedEncoding.clear();

  if (contentLength < 2)
    return false; // too short for any detection

  /* Byte Order Mark has priority over "encoding=" parameter */
  detectedEncoding = GetBomEncoding(xmlContent, contentLength);
  if (!detectedEncoding.empty())
    return true;

  /* try to read encoding from XML declaration */
  if (GetXmlEncodingFromDeclaration(xmlContent, contentLength, detectedEncoding))
  {
    StringUtils::ToUpper(detectedEncoding);

    /* make some safety checks */
    if (detectedEncoding == "UTF-8")
      return true; // fast track for most common case

    if (StringUtils::StartsWith(detectedEncoding, "UCS-") || StringUtils::StartsWith(detectedEncoding, "UTF-"))
    {
      if (detectedEncoding == "UTF-7")
        return true;

      /* XML declaration was detected in UTF-8 mode (by 'GetXmlEncodingFromDeclaration') so we know 
       * that text in single byte encoding, but declaration itself wrongly specify multibyte encoding */
      detectedEncoding.clear();
      return false;
    }
    return true;
  }

  /* try to detect basic encoding */
  std::string guessedEncoding;
  if (!GuessXmlEncoding(xmlContent, contentLength, guessedEncoding))
    return false; /* can't detect any encoding */

  /* have some guessed encoding, try to use it */
  std::string convertedXml;
  /* use 'm_XmlDeclarationMaxLength * 4' below for UTF-32-like encodings */
  if (!g_charsetConverter.ToUtf8(guessedEncoding, std::string(xmlContent, std::min(contentLength, m_XmlDeclarationMaxLength * 4)), convertedXml)
      || convertedXml.empty())
    return false;  /* can't convert, guessed encoding is wrong */

  /* text converted, hopefully at least XML declaration is in UTF-8 now */
  std::string declaredEncoding;
  /* try to read real encoding from converted XML declaration */
  if (!GetXmlEncodingFromDeclaration(convertedXml.c_str(), convertedXml.length(), declaredEncoding))
  { /* did not find real encoding in XML declaration, use guessed encoding */
    detectedEncoding = guessedEncoding;
    return true;
  }

  /* found encoding in converted XML declaration, we know correct endianness and number of bytes per char */
  /* make some safety checks */
  StringUtils::ToUpper(declaredEncoding);
  if (declaredEncoding == guessedEncoding)
    return true;

  if (StringUtils::StartsWith(guessedEncoding, "UCS-4"))
  {
    if (declaredEncoding.length() < 5 ||
        (!StringUtils::StartsWith(declaredEncoding, "UTF-32") && !StringUtils::StartsWith(declaredEncoding, "UCS-4")))
    { /* Guessed encoding was correct because we can convert and read XML declaration, but declaration itself is wrong (not 4-bytes encoding) */
      detectedEncoding = guessedEncoding;
      return true;
    }
  }
  else if (StringUtils::StartsWith(guessedEncoding, "UTF-16"))
  {
    if (declaredEncoding.length() < 5 ||
        (!StringUtils::StartsWith(declaredEncoding, "UTF-16") && !StringUtils::StartsWith(declaredEncoding, "UCS-2")))
    { /* Guessed encoding was correct because we can read XML declaration, but declaration is wrong (not 2-bytes encoding) */
      detectedEncoding = guessedEncoding;
      return true;
    }
  }

  if (StringUtils::StartsWith(guessedEncoding, "UCS-4") || StringUtils::StartsWith(guessedEncoding, "UTF-16"))
  {
    /* Check endianness in declared encoding. We already know correct endianness as XML declaration was detected after conversion. */
    /* Guessed UTF/UCS encoding always ends with endianness */
    std::string guessedEndianness(guessedEncoding, guessedEncoding.length() - 2);

    if (!StringUtils::EndsWith(declaredEncoding, "BE") && !StringUtils::EndsWith(declaredEncoding, "LE")) /* Declared encoding without endianness */
      detectedEncoding = declaredEncoding + guessedEndianness; /* add guessed endianness */
    else if (!StringUtils::EndsWith(declaredEncoding, guessedEndianness)) /* Wrong endianness in declared encoding */
      detectedEncoding = declaredEncoding.substr(0, declaredEncoding.length() - 2) + guessedEndianness; /* replace endianness by guessed endianness */
    else
      detectedEncoding = declaredEncoding; /* declared encoding with correct endianness */

    return true;
  }
  else if (StringUtils::StartsWith(guessedEncoding, "EBCDIC"))
  {
    if (declaredEncoding.find("EBCDIC") != std::string::npos)
      detectedEncoding = declaredEncoding; /* Declared encoding is some specific EBCDIC encoding */
    else
      detectedEncoding = guessedEncoding;

    return true;
  }

  /* should be unreachable */
  return false;
}

bool CCharsetDetection::GetXmlEncodingFromDeclaration(const char* const xmlContent, const size_t contentLength, std::string& declaredEncoding)
{
  // following code is std::string-processing analog of regular expression-processing
  // regular expression: "<\\?xml([ \n\r\t]+[^ \n\t\r>]+)*[ \n\r\t]+encoding[ \n\r\t]*=[ \n\r\t]*('[^ \n\t\r>']+'|\"[^ \n\t\r>\"]+\")"
  // on win32 x86 machine regular expression is slower that std::string 20-40 times and can slowdown XML processing for several times
  // seems that this regular expression is too slow due to many variable length parts, regexp for '&amp;'-fixing is much faster

  declaredEncoding.clear();

  // avoid extra large search
  std::string strXml(xmlContent, std::min(contentLength, m_XmlDeclarationMaxLength));

  size_t pos = strXml.find("<?xml");
  if (pos == std::string::npos || pos + 6 > strXml.length() || pos > strXml.find('<'))
    return false; // no "<?xml" declaration, "<?xml" is not first element or "<?xml" is incomplete

  pos += 5; // 5 is length of "<?xml"

  const size_t declLength = std::min(std::min(m_XmlDeclarationMaxLength, contentLength - pos), strXml.find('>', pos) - pos);
  const std::string xmlDecl(xmlContent + pos, declLength);
  const char* const xmlDeclC = xmlDecl.c_str(); // for faster processing of [] and for null-termination

  static const char* const whiteSpaceChars = " \n\r\t"; // according to W3C Recommendation for XML, any of them can be used as separator
  pos = 0;

  while (pos + 12 <= declLength) // 12 is minimal length of "encoding='x'"
  {
    pos = xmlDecl.find_first_of(whiteSpaceChars, pos);
    if (pos == std::string::npos)
      return false; // no " encoding=" in declaration

    pos = xmlDecl.find_first_not_of(whiteSpaceChars, pos);
    if (pos == std::string::npos)
      return false; // no "encoding=" in declaration

    if (xmlDecl.compare(pos, 8, "encoding", 8) != 0)
      continue; // not "encoding" parameter
    pos += 8; // length of "encoding"

    if (xmlDeclC[pos] == ' ' || xmlDeclC[pos] == '\n' || xmlDeclC[pos] == '\r' || xmlDeclC[pos] == '\t') // no buffer overrun as string is null-terminated
    {
      pos = xmlDecl.find_first_not_of(whiteSpaceChars, pos);
      if (pos == std::string::npos)
        return false; // this " encoding" is incomplete, only whitespace chars remains
    }
    if (xmlDeclC[pos] != '=')
    { // "encoding" without "=", try to find other
      pos--; // step back to whitespace
      continue;
    }

    pos++; // skip '='
    if (xmlDeclC[pos] == ' ' || xmlDeclC[pos] == '\n' || xmlDeclC[pos] == '\r' || xmlDeclC[pos] == '\t') // no buffer overrun as string is null-terminated
    {
      pos = xmlDecl.find_first_not_of(whiteSpaceChars, pos);
      if (pos == std::string::npos)
        return false; // this " encoding" is incomplete, only whitespace chars remains
    }
    size_t encNameEndPos;
    if (xmlDeclC[pos] == '"')
      encNameEndPos = xmlDecl.find('"', ++pos);
    else if (xmlDeclC[pos] == '\'')
      encNameEndPos = xmlDecl.find('\'', ++pos);
    else
      continue; // no quote or double quote after 'encoding=', try to find other

    if (encNameEndPos != std::string::npos)
    {
      declaredEncoding.assign(xmlDecl, pos, encNameEndPos - pos);
      return true;
    }
    // no closing quote or double quote after 'encoding="x', try to find other
  }

  return false;
}

bool CCharsetDetection::GuessXmlEncoding(const char* const xmlContent, const size_t contentLength, std::string& supposedEncoding)
{
  supposedEncoding.clear();
  if (contentLength < 4)
    return false; // too little data to guess

  if (xmlContent[0] == 0 && xmlContent[1] == 0 && xmlContent[2] == 0 && xmlContent[3] == (char)0x3C) // '<' == '00 00 00 3C' in UCS-4 (UTF-32) big-endian
    supposedEncoding = "UCS-4BE"; // use UCS-4 according to W3C recommendation
  else if (xmlContent[0] == (char)0x3C && xmlContent[1] == 0 && xmlContent[2] == 0 && xmlContent[3] == 0) // '<' == '3C 00 00 00' in UCS-4 (UTF-32) little-endian
    supposedEncoding = "UCS-4LE"; // use UCS-4 according to W3C recommendation
  else if (xmlContent[0] == 0 && xmlContent[1] == (char)0x3C && xmlContent[2] == 0 && xmlContent[3] == (char)0x3F) // "<?" == "00 3C 00 3F" in UTF-16 (UCS-2) big-endian
    supposedEncoding = "UTF-16BE";
  else if (xmlContent[0] == (char)0x3C && xmlContent[1] == 0 && xmlContent[2] == (char)0x3F && xmlContent[3] == 0) // "<?" == "3C 00 3F 00" in UTF-16 (UCS-2) little-endian
    supposedEncoding = "UTF-16LE";
  else if (xmlContent[0] == (char)0x4C && xmlContent[1] == (char)0x6F && xmlContent[2] == (char)0xA7 && xmlContent[3] == (char)0x94) // "<?xm" == "4C 6F A7 94" in most EBCDIC encodings
    supposedEncoding = "EBCDIC-CP-US"; // guessed value, real value must be read from declaration
  else
    return false;

  return true;
}

