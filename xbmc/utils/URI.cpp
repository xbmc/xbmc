/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "URI.h"

#include <utility>

CURI::CURI(std::string uri)
{
  Parse(std::move(uri));
}

CURI::CURI(CURI&& other) noexcept
{
  *this = std::move(other);
}

CURI& CURI::operator=(CURI&& other) noexcept
{
  if (this != &other)
  {
    m_source = std::move(other.m_source);
    m_components = std::exchange(other.m_components, {});
    m_valid = std::exchange(other.m_valid, false);
    other.m_source.clear();
  }
  return *this;
}

bool CURI::Parse(std::string uri)
{
  if (!URIParser::GetScheme(uri))
    uri.clear();
  m_source = std::move(uri);
  UpdateComponents();
  return m_valid;
}

void CURI::UpdateComponents()
{
  const auto parsed = URIParser::ParseReference(m_source);
  m_components = parsed ? *parsed : URIParser::ScanReference(m_source);
  m_valid = parsed && parsed->scheme.has_value();
}

std::string_view CURI::GetRawScheme() const
{
  return m_components.scheme ? m_components.scheme->Get(m_source) : std::string_view{};
}

std::string_view CURI::GetRawAuthority() const
{
  return m_components.authority ? m_components.authority->Get(m_source) : std::string_view{};
}

std::string_view CURI::GetRawQuery() const
{
  return m_components.query ? m_components.query->Get(m_source) : std::string_view{};
}

std::string_view CURI::GetRawFragment() const
{
  return m_components.fragment ? m_components.fragment->Get(m_source) : std::string_view{};
}

std::string_view CURI::GetSchemeSpecificPart() const
{
  if (!m_components.scheme)
    return {};
  const auto begin = m_components.scheme->length + 1;
  const auto end = m_components.fragment ? m_components.fragment->offset - 1 : m_source.size();
  return std::string_view(m_source).substr(begin, end - begin);
}

std::string_view CURI::GetPathPrefix() const
{
  return std::string_view(m_source).substr(0, m_components.path.offset);
}

std::string_view CURI::GetWithoutQueryAndFragment() const
{
  return std::string_view(m_source).substr(0, m_components.path.offset + m_components.path.length);
}

bool CURI::SetPath(std::string path)
{
  if (!m_valid || (HasAuthority() && !path.empty() && !path.starts_with('/')) ||
      (!HasAuthority() && path.starts_with("//")))
    return false;

  // A component's colon cannot introduce another outer scheme.
  const std::string reference = (HasAuthority() ? "component://authority" : "component:") + path;
  const auto check = URIParser::ParseReference(reference);
  if (!check || check->path.Get(reference) != path || check->query || check->fragment)
    return false;
  m_source.replace(m_components.path.offset, m_components.path.length, path);
  UpdateComponents();
  return true;
}

bool CURI::SetRawQuery(std::optional<std::string> query)
{
  if (!m_components.scheme)
    return false;
  if (query)
  {
    const auto check = URIParser::ParseReference("?" + *query);
    if (!check || check->fragment)
      return false;
  }
  const auto begin = m_components.path.offset + m_components.path.length;
  const auto end = m_components.fragment ? m_components.fragment->offset - 1 : m_source.size();
  m_source.replace(begin, end - begin, query ? "?" + *query : "");
  UpdateComponents();
  return true;
}
