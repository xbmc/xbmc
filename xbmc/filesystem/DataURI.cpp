/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DataURI.h"

#include "URL.h"
#include "utils/Base64.h"
#include "utils/StringUtils.h"

#include <string_view>

namespace
{
// DataURI implements RFC 2397, "The data URL scheme":
// https://www.rfc-editor.org/rfc/rfc2397.html
//
// Data URLs are materialized in memory, so these are Kodi resource limits, not
// restrictions from RFC 2397. They are intentionally large enough for media
// resources such as high-resolution artwork and animated images; large video
// should use a streaming VFS source.
constexpr size_t MAX_DATA_URI_SIZE = 128 * 1024 * 1024;
constexpr size_t MAX_DECODED_SIZE = 64 * 1024 * 1024;

struct DataUri
{
  std::string_view payload;
  bool isBase64{false};
  size_t base64Size{0};
  size_t decodedSize{0};
};

bool IsUriChar(const char c)
{
  // RFC 2397 imports "uric" from RFC 2396 (verified erratum 2045).
  return StringUtils::isasciialphanum(c) ||
         std::string_view{";/?:@&=+$,-_.!~*'()"}.find(c) != std::string_view::npos;
}

unsigned char DecodeHexDigit(const char c)
{
  if (c >= '0' && c <= '9')
    return static_cast<unsigned char>(c - '0');
  if (c >= 'A' && c <= 'F')
    return static_cast<unsigned char>(c - 'A' + 10);
  return static_cast<unsigned char>(c - 'a' + 10);
}

bool ValidateUriCharacters(std::string_view value)
{
  for (size_t i = 0; i < value.size(); ++i)
  {
    if (value[i] != '%')
    {
      if (!IsUriChar(value[i]))
        return false;
      continue;
    }

    if (i + 2 >= value.size() || !StringUtils::isasciixdigit(value[i + 1]) ||
        !StringUtils::isasciixdigit(value[i + 2]))
      return false;
    i += 2;
  }
  return true;
}

bool IsMimeTokenChar(const unsigned char c)
{
  // RFC 2045 section 5.1: printable US-ASCII except SPACE and "tspecials".
  return c >= 33 && c <= 126 &&
         std::string_view{"()<>@,;:\\\"/[]?="}.find(c) == std::string_view::npos;
}

bool ValidateMimeToken(std::string_view token)
{
  if (token.empty())
    return false;

  for (size_t i = 0; i < token.size(); ++i)
  {
    unsigned char c = static_cast<unsigned char>(token[i]);
    if (token[i] == '%')
    {
      c = static_cast<unsigned char>((DecodeHexDigit(token[i + 1]) << 4) |
                                     DecodeHexDigit(token[i + 2]));
      i += 2;
    }

    if (!IsMimeTokenChar(c))
      return false;
  }
  return true;
}

bool ValidateMimeValue(std::string_view value)
{
  if (value.empty())
    return false;

  for (size_t i = 0; i < value.size(); ++i)
  {
    if (value[i] != '%')
    {
      if (!IsMimeTokenChar(static_cast<unsigned char>(value[i])))
        return false;
      continue;
    }

    const unsigned char decoded = static_cast<unsigned char>((DecodeHexDigit(value[i + 1]) << 4) |
                                                             DecodeHexDigit(value[i + 2]));
    // RFC 2397 recommends URL escaping for parameter values containing MIME
    // tspecials instead of quoted-string notation. Keep the raw delimiters for
    // parsing and accept escaped printable US-ASCII as value content.
    if (decoded < 32 || decoded > 126)
      return false;
    i += 2;
  }
  return true;
}

bool ParseMediaType(std::string_view metadata, bool& isBase64)
{
  isBase64 = false;

  const size_t firstSemicolon = metadata.find(';');
  const std::string_view mediaType = metadata.substr(0, firstSemicolon);
  if (!mediaType.empty())
  {
    const size_t slash = mediaType.find('/');
    if (slash == std::string_view::npos ||
        mediaType.find('/', slash + 1) != std::string_view::npos ||
        !ValidateMimeToken(mediaType.substr(0, slash)) ||
        !ValidateMimeToken(mediaType.substr(slash + 1)))
      return false;
  }

  if (firstSemicolon == std::string_view::npos)
    return true;

  size_t segmentStart = firstSemicolon + 1;
  while (segmentStart <= metadata.size())
  {
    const size_t nextSemicolon = metadata.find(';', segmentStart);
    const bool isLast = nextSemicolon == std::string_view::npos;
    const std::string_view segment = metadata.substr(
        segmentStart, isLast ? std::string_view::npos : nextSemicolon - segmentStart);
    if (segment.empty())
      return false;

    const size_t equals = segment.find('=');
    if (equals == std::string_view::npos)
    {
      // RFC 2397 distinguishes this terminal marker from MIME parameters by
      // the absence of '='. MIME mechanism names are case-insensitive.
      if (!isLast || !StringUtils::EqualsNoCase(segment, "base64"))
        return false;
      isBase64 = true;
    }
    else if (segment.find('=', equals + 1) != std::string_view::npos ||
             !ValidateMimeToken(segment.substr(0, equals)) ||
             !ValidateMimeValue(segment.substr(equals + 1)))
      return false;

    if (isLast)
      break;
    segmentStart = nextSemicolon + 1;
  }
  return true;
}

bool ParseDataUri(std::string_view uriData, DataUri& dataUri)
{
  if (!ValidateUriCharacters(uriData))
    return false;

  const size_t comma = uriData.find(',');
  if (comma == std::string_view::npos)
    return false;

  // When the mediatype is omitted, RFC 2397 defines the effective default as
  // text/plain;charset=US-ASCII. CDataFile currently exposes bytes only, so no
  // content-type state needs to be retained after validating the metadata.
  if (!ParseMediaType(uriData.substr(0, comma), dataUri.isBase64))
    return false;

  dataUri.payload = uriData.substr(comma + 1);
  return true;
}

bool IsMimeBase64Whitespace(const unsigned char c)
{
  // RFC 2045 section 6.8 allows MIME Base64 decoders to ignore line breaks
  // and other whitespace. Restrict its broader non-alphabet tolerance to
  // these ASCII whitespace octets so all other malformed input is rejected.
  return c == '\t' || c == '\n' || c == '\r' || c == ' ';
}

template<typename Visitor>
bool VisitPercentDecoded(std::string_view encoded,
                         size_t maxDecodedSize,
                         Visitor visitor,
                         size_t& decodedSize)
{
  decodedSize = 0;
  for (size_t i = 0; i < encoded.size(); ++i)
  {
    unsigned char c = static_cast<unsigned char>(encoded[i]);
    if (encoded[i] == '%')
    {
      if (i + 2 >= encoded.size() || !StringUtils::isasciixdigit(encoded[i + 1]) ||
          !StringUtils::isasciixdigit(encoded[i + 2]))
        return false;

      c = static_cast<unsigned char>((DecodeHexDigit(encoded[i + 1]) << 4) |
                                     DecodeHexDigit(encoded[i + 2]));
      i += 2;
    }

    if (++decodedSize > maxDecodedSize || !visitor(c))
      return false;
  }
  return true;
}

int DecodeBase64Digit(const unsigned char c)
{
  if (c >= 'A' && c <= 'Z')
    return c - 'A';
  if (c >= 'a' && c <= 'z')
    return c - 'a' + 26;
  if (c >= '0' && c <= '9')
    return c - '0' + 52;
  if (c == '+')
    return 62;
  if (c == '/')
    return 63;
  return -1;
}

bool ValidateBase64(DataUri& dataUri)
{
  dataUri.base64Size = 0;
  dataUri.decodedSize = 0;
  size_t padding = 0;
  unsigned char finalDigit = 0;
  bool foundPadding = false;
  size_t percentDecodedSize = 0;
  if (!VisitPercentDecoded(
          dataUri.payload, MAX_DATA_URI_SIZE,
          [&](const unsigned char c)
          {
            if (IsMimeBase64Whitespace(c))
              return true;

            ++dataUri.base64Size;
            if (c == '=')
            {
              foundPadding = true;
              return ++padding <= 2;
            }

            if (foundPadding)
              return false;

            const int digit = DecodeBase64Digit(c);
            if (digit < 0)
              return false;
            finalDigit = static_cast<unsigned char>(digit);
            return true;
          },
          percentDecodedSize))
    return false;

  if (dataUri.base64Size == 0)
    return true;

  if (dataUri.base64Size % 4 != 0)
    return false;

  const size_t dataLength = dataUri.base64Size - padding;

  // Require canonical RFC 4648 padding and zero unused bits. This is equivalent
  // to decoding and re-encoding, but does not materialize the payload for
  // Exists() or Stat().
  if ((padding == 1 && dataLength % 4 != 3) || (padding == 2 && dataLength % 4 != 2) ||
      (padding == 1 && (finalDigit & 0x03) != 0) || (padding == 2 && (finalDigit & 0x0F) != 0))
    return false;

  dataUri.decodedSize = (dataUri.base64Size / 4) * 3 - padding;
  return dataUri.decodedSize <= MAX_DECODED_SIZE;
}

bool ValidatePercentEncoded(DataUri& dataUri)
{
  return VisitPercentDecoded(
      dataUri.payload, MAX_DECODED_SIZE, [](const unsigned char) { return true; },
      dataUri.decodedSize);
}

bool ParseAndValidateDataUri(const CURL& url, DataUri& dataUri)
{
  if (!url.IsProtocol("data") || url.GetWithoutFilename() != "data:")
    return false;

  const std::string& uriData = url.GetFileName();
  if (uriData.starts_with('/') || uriData.size() > MAX_DATA_URI_SIZE ||
      !ParseDataUri(uriData, dataUri))
    return false;

  return dataUri.isBase64 ? ValidateBase64(dataUri) : ValidatePercentEncoded(dataUri);
}

bool MaterializePercentEncoded(const DataUri& dataUri, std::string& decoded)
{
  decoded.clear();
  decoded.reserve(dataUri.decodedSize);
  size_t decodedSize = 0;
  return VisitPercentDecoded(
             dataUri.payload, dataUri.decodedSize,
             [&](const unsigned char c)
             {
               decoded.push_back(static_cast<char>(c));
               return true;
             },
             decodedSize) &&
         decodedSize == dataUri.decodedSize;
}

bool MaterializeBase64(const DataUri& dataUri, std::string& decoded)
{
  std::string encoded;
  encoded.reserve(dataUri.base64Size);
  size_t percentDecodedSize = 0;
  if (!VisitPercentDecoded(
          dataUri.payload, MAX_DATA_URI_SIZE,
          [&](const unsigned char c)
          {
            if (!IsMimeBase64Whitespace(c))
              encoded.push_back(static_cast<char>(c));
            return true;
          },
          percentDecodedSize) ||
      encoded.size() != dataUri.base64Size)
    return false;

  decoded.clear();
  Base64::Decode(encoded, decoded);
  return decoded.size() == dataUri.decodedSize;
}

bool MaterializeDataUri(const DataUri& dataUri, std::string& decoded)
{
  return dataUri.isBase64 ? MaterializeBase64(dataUri, decoded)
                          : MaterializePercentEncoded(dataUri, decoded);
}
} // namespace

bool XFILE::DataURI::Validate(const CURL& url, size_t& decodedSize)
{
  DataUri dataUri;
  if (!ParseAndValidateDataUri(url, dataUri))
    return false;

  decodedSize = dataUri.decodedSize;
  return true;
}

bool XFILE::DataURI::Materialize(const CURL& url, std::string& decoded)
{
  DataUri dataUri;
  return ParseAndValidateDataUri(url, dataUri) && MaterializeDataUri(dataUri, decoded);
}
