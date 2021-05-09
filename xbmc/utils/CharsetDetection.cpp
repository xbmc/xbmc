/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CharsetDetection.h"

#include "LangInfo.h"
#include "utils/CharsetConverter.h"
#include "utils/StringUtils.h"
#include "utils/Utf8Utils.h"
#include "utils/log.h"

#include <algorithm>

/* XML declaration can be virtually any size (with many-many whitespaces)
 * but for in real world we don't need to process megabytes of data
 * so limit search for XML declaration to reasonable value */
const size_t CCharsetDetection::m_XmlDeclarationMaxLength = 250;

/* According to http://www.w3.org/TR/2013/CR-html5-20130806/single-page.html#charset
 * encoding must be placed in first 1024 bytes of document */
const size_t CCharsetDetection::m_HtmlCharsetEndSearchPos = 1024;

/* According to http://www.w3.org/TR/2013/CR-html5-20130806/single-page.html#space-character
 * tab, LF, FF, CR or space can be used as whitespace */
const std::string CCharsetDetection::m_HtmlWhitespaceChars("\x09\x0A\x0C\x0D\x20");    // tab, LF, FF, CR and space

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

bool CCharsetDetection::ConvertHtmlToUtf8(const std::string& htmlContent, std::string& converted, const std::string& serverReportedCharset, std::string& usedHtmlCharset)
{
  converted.clear();
  usedHtmlCharset.clear();
  if (htmlContent.empty())
  {
    usedHtmlCharset = "UTF-8"; // any charset can be used for empty content, use UTF-8 as default
    return false;
  }

  // this is relaxed implementation of http://www.w3.org/TR/2013/CR-html5-20130806/single-page.html#determining-the-character-encoding

  // try to get charset from Byte Order Mark
  std::string bomCharset(GetBomEncoding(htmlContent));
  if (checkConversion(bomCharset, htmlContent, converted))
  {
    usedHtmlCharset = bomCharset;
    return true;
  }

  // try charset from HTTP header (or from other out-of-band source)
  if (checkConversion(serverReportedCharset, htmlContent, converted))
  {
    usedHtmlCharset = serverReportedCharset;
    return true;
  }

  // try to find charset in HTML
  std::string declaredCharset(GetHtmlEncodingFromHead(htmlContent));
  if (!declaredCharset.empty())
  {
    if (declaredCharset.compare(0, 3, "UTF", 3) == 0)
      declaredCharset = "UTF-8"; // charset string was found in singlebyte mode, charset can't be multibyte encoding
    if (checkConversion(declaredCharset, htmlContent, converted))
    {
      usedHtmlCharset = declaredCharset;
      return true;
    }
  }

  // try UTF-8 if not tried before
  if (bomCharset != "UTF-8" && serverReportedCharset != "UTF-8" && declaredCharset != "UTF-8" && checkConversion("UTF-8", htmlContent, converted))
  {
    usedHtmlCharset = "UTF-8";
    return false; // only guessed value
  }

  // try user charset
  std::string userCharset(g_langInfo.GetGuiCharSet());
  if (checkConversion(userCharset, htmlContent, converted))
  {
    usedHtmlCharset = userCharset;
    return false; // only guessed value
  }

  // try WINDOWS-1252
  if (checkConversion("WINDOWS-1252", htmlContent, converted))
  {
    usedHtmlCharset = "WINDOWS-1252";
    return false; // only guessed value
  }

  // can't find exact charset
  // use one of detected as fallback
  if (!bomCharset.empty())
    usedHtmlCharset = bomCharset;
  else if (!serverReportedCharset.empty())
    usedHtmlCharset = serverReportedCharset;
  else if (!declaredCharset.empty())
    usedHtmlCharset = declaredCharset;
  else if (!userCharset.empty())
    usedHtmlCharset = userCharset;
  else
    usedHtmlCharset = "WINDOWS-1252";

  CLog::Log(LOGWARNING, "{}: Can't correctly convert to UTF-8 charset, converting as \"{}\"",
            __FUNCTION__, usedHtmlCharset);
  g_charsetConverter.ToUtf8(usedHtmlCharset, htmlContent, converted, false);

  return false;
}

bool CCharsetDetection::ConvertPlainTextToUtf8(const std::string& textContent, std::string& converted, const std::string& serverReportedCharset, std::string& usedCharset)
{
  converted.clear();
  usedCharset.clear();
  if (textContent.empty())
  {
    usedCharset = "UTF-8"; // any charset can be used for empty content, use UTF-8 as default
    return true;
  }

  // try to get charset from Byte Order Mark
  std::string bomCharset(GetBomEncoding(textContent));
  if (checkConversion(bomCharset, textContent, converted))
  {
    usedCharset = bomCharset;
    return true;
  }

  // try charset from HTTP header (or from other out-of-band source)
  if (checkConversion(serverReportedCharset, textContent, converted))
  {
    usedCharset = serverReportedCharset;
    return true;
  }

  // try UTF-8 if not tried before
  if (bomCharset != "UTF-8" && serverReportedCharset != "UTF-8" && checkConversion("UTF-8", textContent, converted))
  {
    usedCharset = "UTF-8";
    return true;
  }

  // try user charset
  std::string userCharset(g_langInfo.GetGuiCharSet());
  if (checkConversion(userCharset, textContent, converted))
  {
    usedCharset = userCharset;
    return true;
  }

  // try system default charset
  if (g_charsetConverter.systemToUtf8(textContent, converted, true))
  {
    usedCharset = "char"; // synonym to system charset
    return true;
  }

  // try WINDOWS-1252
  if (checkConversion("WINDOWS-1252", textContent, converted))
  {
    usedCharset = "WINDOWS-1252";
    return true;
  }

  // can't find correct charset
  // use one of detected as fallback
  if (!serverReportedCharset.empty())
    usedCharset = serverReportedCharset;
  else if (!bomCharset.empty())
    usedCharset = bomCharset;
  else if (!userCharset.empty())
    usedCharset = userCharset;
  else
    usedCharset = "WINDOWS-1252";

  CLog::Log(LOGWARNING, "{}: Can't correctly convert to UTF-8 charset, converting as \"{}\"",
            __FUNCTION__, usedCharset);
  g_charsetConverter.ToUtf8(usedCharset, textContent, converted, false);

  return false;
}


bool CCharsetDetection::checkConversion(const std::string& srcCharset, const std::string& src, std::string& dst)
{
  if (srcCharset.empty())
    return false;

  if (srcCharset != "UTF-8")
  {
    if (g_charsetConverter.ToUtf8(srcCharset, src, dst, true))
      return true;
  }
  else if (CUtf8Utils::isValidUtf8(src))
  {
    dst = src;
    return true;
  }

  return false;
}

std::string CCharsetDetection::GetHtmlEncodingFromHead(const std::string& htmlContent)
{
  std::string smallerHtmlContent;
  if (htmlContent.length() > 2 * m_HtmlCharsetEndSearchPos)
    smallerHtmlContent.assign(htmlContent, 0, 2 * m_HtmlCharsetEndSearchPos); // use twice more bytes to search for charset for safety

  const std::string& html = smallerHtmlContent.empty() ? htmlContent : smallerHtmlContent; // limit search
  const char* const htmlC = html.c_str(); // for null-termination
  const size_t len = html.length();

  // this is an implementation of http://www.w3.org/TR/2013/CR-html5-20130806/single-page.html#prescan-a-byte-stream-to-determine-its-encoding
  // labels in comments correspond to the labels in HTML5 standard
  // note: opposite to standard, everything is converted to uppercase instead of lower case
  size_t pos = 0;
  while (pos < len) // "loop" label
  {
    if (html.compare(pos, 4, "<!--", 4) == 0)
    {
      pos = html.find("-->", pos + 2);
      if (pos == std::string::npos)
        return "";
      pos += 2;
    }
    else if (htmlC[pos] == '<' && (htmlC[pos + 1] == 'm' || htmlC[pos + 1] == 'M') && (htmlC[pos + 2] == 'e' || htmlC[pos + 2] == 'E')
             && (htmlC[pos + 3] == 't' || htmlC[pos + 3] == 'T') && (htmlC[pos + 4] == 'a' || htmlC[pos + 4] == 'A')
             && (htmlC[pos + 5] == 0x09 || htmlC[pos + 5] == 0x0A || htmlC[pos + 5] == 0x0C || htmlC[pos + 5] == 0x0D || htmlC[pos + 5] == 0x20 || htmlC[pos + 5] == 0x2F))
    { // this is case insensitive "<meta" and one of tab, LF, FF, CR, space or slash
      pos += 5; // "pos" points to symbol after "<meta"
      std::string attrName, attrValue;
      bool gotPragma = false;
      std::string contentCharset;
      do // "attributes" label
      {
        pos = GetHtmlAttribute(html, pos, attrName, attrValue);
        if (attrName == "HTTP-EQUIV" && attrValue == "CONTENT-TYPE")
          gotPragma = true;
        else if (attrName == "CONTENT")
          contentCharset = ExtractEncodingFromHtmlMeta(attrValue);
        else if (attrName == "CHARSET")
        {
          StringUtils::Trim(attrValue, m_HtmlWhitespaceChars.c_str()); // tab, LF, FF, CR, space
          if (!attrValue.empty())
            return attrValue;
        }
      } while (!attrName.empty() && pos < len);

      // "processing" label
      if (gotPragma && !contentCharset.empty())
        return contentCharset;
    }
    else if (htmlC[pos] == '<' && ((htmlC[pos + 1] >= 'A' && htmlC[pos + 1] <= 'Z') || (htmlC[pos + 1] >= 'a' && htmlC[pos + 1] <= 'z')))
    {
      pos = html.find_first_of("\x09\x0A\x0C\x0D >", pos); // tab, LF, FF, CR, space or '>'
      std::string attrName, attrValue;
      do
      {
        pos = GetHtmlAttribute(html, pos, attrName, attrValue);
      } while (pos < len && !attrName.empty());
    }
    else if (html.compare(pos, 2, "<!", 2) == 0 || html.compare(pos, 2, "</", 2) == 0 || html.compare(pos, 2, "<?", 2) == 0)
      pos = html.find('>', pos);

    if (pos == std::string::npos)
      return "";

    // "next byte" label
    pos++;
  }

  return ""; // no charset was found
}

size_t CCharsetDetection::GetHtmlAttribute(const std::string& htmlContent, size_t pos, std::string& attrName, std::string& attrValue)
{
  attrName.clear();
  attrValue.clear();
  static const char* const htmlWhitespaceSlash = "\x09\x0A\x0C\x0D\x20\x2F"; // tab, LF, FF, CR, space or slash
  const char* const htmlC = htmlContent.c_str();
  const size_t len = htmlContent.length();

  // this is an implementation of http://www.w3.org/TR/2013/CR-html5-20130806/single-page.html#concept-get-attributes-when-sniffing
  // labels in comments correspond to the labels in HTML5 standard
  // note: opposite to standard, everything is converted to uppercase instead of lower case
  pos = htmlContent.find_first_not_of(htmlWhitespaceSlash, pos);
  if (pos == std::string::npos || htmlC[pos] == '>')
    return pos; // only white spaces or slashes up to the end of the htmlContent or no more attributes

  while (pos < len && htmlC[pos] != '=')
  {
    const char chr = htmlC[pos];
    if (chr == '/' || chr == '>')
      return pos; // no attributes or empty attribute value
    else if (m_HtmlWhitespaceChars.find(chr) != std::string::npos) // chr is one of whitespaces
    {
      pos = htmlContent.find_first_not_of(m_HtmlWhitespaceChars, pos); // "spaces" label
      if (pos == std::string::npos || htmlC[pos] != '=')
        return pos; // only white spaces up to the end or no attribute value
      break;
    }
    else
      appendCharAsAsciiUpperCase(attrName, chr);

    pos++;
  }

  if (pos >= len)
    return std::string::npos; // no '=', '/' or '>' were found up to the end of htmlContent

  pos++; // advance pos to character after '='

  pos = htmlContent.find_first_not_of(m_HtmlWhitespaceChars, pos); // "value" label
  if (pos == std::string::npos)
    return pos; // only white spaces remain in htmlContent

  if (htmlC[pos] == '>')
    return pos; // empty attribute value
  else if (htmlC[pos] == '"' || htmlC[pos] == '\'')
  {
    const char qChr = htmlC[pos];
    // "quote loop" label
    while (++pos < len)
    {
      const char chr = htmlC[pos];
      if (chr == qChr)
        return pos + 1;
      else
        appendCharAsAsciiUpperCase(attrValue, chr);
    }
    return std::string::npos; // no closing quote is found
  }

  appendCharAsAsciiUpperCase(attrValue, htmlC[pos]);
  pos++;

  while (pos < len)
  {
    const char chr = htmlC[pos];
    if (m_HtmlWhitespaceChars.find(chr) != std::string::npos || chr == '>')
      return pos;
    else
      appendCharAsAsciiUpperCase(attrValue, chr);

    pos++;
  }

  return std::string::npos; // rest of htmlContent was attribute value
}

std::string CCharsetDetection::ExtractEncodingFromHtmlMeta(const std::string& metaContent,
                                                           size_t pos /*= 0*/)
{
  size_t len = metaContent.length();
  if (pos >= len)
    return "";

  const char* const metaContentC = metaContent.c_str();

  // this is an implementation of http://www.w3.org/TR/2013/CR-html5-20130806/single-page.html#algorithm-for-extracting-a-character-encoding-from-a-meta-element
  // labels in comments correspond to the labels in HTML5 standard
  // note: opposite to standard, case sensitive match is used as argument is always in uppercase
  std::string charset;
  do
  {
    // "loop" label
    pos = metaContent.find("CHARSET", pos);
    if (pos == std::string::npos)
      return "";

    pos = metaContent.find_first_not_of(m_HtmlWhitespaceChars, pos + 7); // '7' is the length of 'CHARSET'
    if (pos != std::string::npos && metaContentC[pos] == '=')
    {
      pos = metaContent.find_first_not_of(m_HtmlWhitespaceChars, pos + 1);
      if (pos != std::string::npos)
      {
        if (metaContentC[pos] == '\'' || metaContentC[pos] == '"')
        {
          const char qChr = metaContentC[pos];
          pos++;
          const size_t closeQpos = metaContent.find(qChr, pos);
          if (closeQpos != std::string::npos)
            charset.assign(metaContent, pos, closeQpos - pos);
        }
        else
          charset.assign(metaContent, pos, metaContent.find("\x09\x0A\x0C\x0D ;", pos) - pos); // assign content up to the next tab, LF, FF, CR, space, semicolon or end of string
      }
      break;
    }
  } while (pos < len);

  static const char* const htmlWhitespaceCharsC = m_HtmlWhitespaceChars.c_str();
  StringUtils::Trim(charset, htmlWhitespaceCharsC);

  return charset;
}

inline void CCharsetDetection::appendCharAsAsciiUpperCase(std::string& str, const char chr)
{
  if (chr >= 'a' && chr <= 'z')
    str.push_back(chr - ('a' - 'A')); // convert to upper case
  else
    str.push_back(chr);
}
