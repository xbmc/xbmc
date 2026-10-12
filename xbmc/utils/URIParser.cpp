/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URIParser.h"

#include <algorithm>
#include <ranges>
#include <string_view>

namespace URIParser
{
namespace
{

constexpr bool IsASCIIAlpha(const char value)
{
  return (value >= 'A' && value <= 'Z') || (value >= 'a' && value <= 'z');
}

constexpr bool IsASCIIDigit(const char value)
{
  return value >= '0' && value <= '9';
}

constexpr bool IsASCIIHexDigit(const char value)
{
  return IsASCIIDigit(value) || (value >= 'A' && value <= 'F') || (value >= 'a' && value <= 'f');
}

constexpr bool IsURIUnreserved(const char value)
{
  return IsASCIIAlpha(value) || IsASCIIDigit(value) || value == '-' || value == '.' ||
         value == '_' || value == '~';
}

constexpr bool IsURISubDelimiter(const char value)
{
  constexpr std::string_view SUB_DELIMITERS{"!$&'()*+,;="};
  return SUB_DELIMITERS.find(value) != std::string_view::npos;
}

bool AreValidURIChars(std::string_view value, std::string_view extraChars)
{
  for (size_t index = 0; index < value.size(); ++index)
  {
    const char current = value[index];
    if (IsURIUnreserved(current) || IsURISubDelimiter(current) ||
        extraChars.find(current) != std::string_view::npos)
    {
      continue;
    }

    if (current != '%' || index + 2 >= value.size() || !IsASCIIHexDigit(value[index + 1]) ||
        !IsASCIIHexDigit(value[index + 2]))
    {
      return false;
    }

    index += 2;
  }

  return true;
}

bool IsIPv4Address(std::string_view address)
{
  size_t octetStart = 0;
  for (unsigned int octetIndex = 0; octetIndex < 4; ++octetIndex)
  {
    size_t octetEnd = address.find('.', octetStart);
    if ((octetIndex < 3 && octetEnd == std::string_view::npos) ||
        (octetIndex == 3 && octetEnd != std::string_view::npos))
    {
      return false;
    }

    if (octetEnd == std::string_view::npos)
      octetEnd = address.size();

    const std::string_view octet = address.substr(octetStart, octetEnd - octetStart);
    if (octet.empty() || octet.size() > 3 || (octet.size() > 1 && octet.front() == '0'))
      return false;

    unsigned int number = 0;
    for (const char digit : octet)
    {
      if (!IsASCIIDigit(digit))
        return false;
      number = number * 10 + digit - '0';
    }

    if (number > 255)
      return false;

    octetStart = octetEnd + 1;
  }

  return true;
}

constexpr bool IsH16(std::string_view value)
{
  return !value.empty() && value.size() <= 4 && std::ranges::all_of(value, IsASCIIHexDigit);
}

bool ParseIPv6Pieces(std::string_view value, bool allowFinalIPv4, size_t& pieceCount)
{
  if (value.empty())
    return true;

  size_t pieceStart = 0;
  while (pieceStart <= value.size())
  {
    const size_t pieceEnd = value.find(':', pieceStart);
    const bool isFinalPiece = pieceEnd == std::string_view::npos;
    const std::string_view piece =
        value.substr(pieceStart, isFinalPiece ? std::string_view::npos : pieceEnd - pieceStart);

    if (piece.find('.') != std::string_view::npos)
    {
      if (!allowFinalIPv4 || !isFinalPiece || !IsIPv4Address(piece))
        return false;
      pieceCount += 2;
    }
    else
    {
      if (!IsH16(piece))
        return false;
      ++pieceCount;
    }

    if (isFinalPiece)
      return true;

    pieceStart = pieceEnd + 1;
  }

  return false;
}

bool IsIPv6Address(std::string_view address)
{
  const size_t compression = address.find("::");
  size_t pieceCount = 0;

  if (compression == std::string_view::npos)
    return ParseIPv6Pieces(address, true, pieceCount) && pieceCount == 8;

  if (address.find("::", compression + 2) != std::string_view::npos)
    return false;

  const std::string_view left = address.substr(0, compression);
  const std::string_view right = address.substr(compression + 2);

  // An embedded IPv4address is permitted only as the final 32 bits, never on
  // the left side of a compressed IPv6 address.
  return ParseIPv6Pieces(left, false, pieceCount) && ParseIPv6Pieces(right, true, pieceCount) &&
         pieceCount < 8;
}

bool IsIPLiteral(std::string_view literal)
{
  if (literal.empty())
    return false;

  // RFC 3986 section 3.2.2: IPvFuture starts with a case-insensitive "v",
  // one or more hexadecimal version digits, a dot, and a non-empty address.
  if (literal.front() == 'v' || literal.front() == 'V')
  {
    const size_t separator = literal.find('.', 1);
    if (separator == std::string_view::npos || separator == 1 || separator + 1 == literal.size() ||
        !std::ranges::all_of(literal.substr(1, separator - 1), IsASCIIHexDigit))
    {
      return false;
    }

    return std::ranges::all_of(
        literal.substr(separator + 1), [](const char value)
        { return IsURIUnreserved(value) || IsURISubDelimiter(value) || value == ':'; });
  }

  return IsIPv6Address(literal);
}

bool IsURIAuthority(std::string_view authority)
{
  const size_t userInfoEnd = authority.find('@');
  std::string_view hostAndPort = authority;
  if (userInfoEnd != std::string_view::npos)
  {
    if (!AreValidURIChars(authority.substr(0, userInfoEnd), ":"))
      return false;
    hostAndPort = authority.substr(userInfoEnd + 1);
  }

  if (!hostAndPort.empty() && hostAndPort.front() == '[')
  {
    const size_t literalEnd = hostAndPort.find(']');
    if (literalEnd == std::string_view::npos || !IsIPLiteral(hostAndPort.substr(1, literalEnd - 1)))
    {
      return false;
    }

    const std::string_view remainder = hostAndPort.substr(literalEnd + 1);
    return remainder.empty() ||
           (remainder.front() == ':' && std::ranges::all_of(remainder.substr(1), IsASCIIDigit));
  }

  const size_t portSeparator = hostAndPort.find(':');
  const std::string_view host = hostAndPort.substr(0, portSeparator);
  if (!AreValidURIChars(host, {}))
    return false;

  if (portSeparator == std::string_view::npos)
    return true;

  return std::ranges::all_of(hostAndPort.substr(portSeparator + 1), IsASCIIDigit);
}

} // unnamed namespace

std::optional<std::string_view> GetScheme(std::string_view input)
{
  if (input.empty() || !IsASCIIAlpha(input.front()))
    return std::nullopt;

  for (size_t index = 1; index < input.size(); ++index)
  {
    const char value = input[index];
    if (value == ':')
      return input.substr(0, index);
    if (!IsASCIIAlpha(value) && !IsASCIIDigit(value) && value != '+' && value != '-' &&
        value != '.')
      return std::nullopt;
  }
  return std::nullopt;
}

Components ScanReference(std::string_view input)
{
  Components parts;
  size_t pathStart = 0;
  if (const auto scheme = GetScheme(input))
  {
    parts.scheme = Range{0, scheme->size()};
    pathStart = scheme->size() + 1;
  }

  size_t pathEnd = input.size();
  const size_t fragmentStart = input.find('#', pathStart);
  if (fragmentStart != std::string_view::npos)
  {
    parts.fragment = Range{fragmentStart + 1, input.size() - fragmentStart - 1};
    pathEnd = fragmentStart;
  }
  const size_t queryStart = input.find('?', pathStart);
  if (queryStart < pathEnd)
  {
    parts.query = Range{queryStart + 1, pathEnd - queryStart - 1};
    pathEnd = queryStart;
  }

  if (input.substr(pathStart, 2) == "//")
  {
    const size_t authorityStart = pathStart + 2;
    pathStart = std::min(input.find('/', authorityStart), pathEnd);
    parts.authority = Range{authorityStart, pathStart - authorityStart};
  }
  parts.path = Range{pathStart, pathEnd - pathStart};
  return parts;
}

std::optional<Components> ParseReference(std::string_view input)
{
  const Components parts = ScanReference(input);
  if (parts.authority && !IsURIAuthority(parts.authority->Get(input)))
    return std::nullopt;

  const std::string_view path = parts.path.Get(input);
  if (!AreValidURIChars(path, ":@/"))
    return std::nullopt;

  // A relative path's first segment cannot contain a colon (path-noscheme).
  // https://www.rfc-editor.org/rfc/rfc3986.html#section-4.2
  if (!parts.scheme && !parts.authority && !path.starts_with('/') &&
      path.substr(0, path.find('/')).find(':') != std::string_view::npos)
  {
    return std::nullopt;
  }

  if ((parts.query && !AreValidURIChars(parts.query->Get(input), ":@/?")) ||
      (parts.fragment && !AreValidURIChars(parts.fragment->Get(input), ":@/?")))
  {
    return std::nullopt;
  }
  return parts;
}

bool IsURI(std::string_view input)
{
  const auto parts = ParseReference(input);
  return parts && parts->scheme.has_value();
}

} // namespace URIParser
