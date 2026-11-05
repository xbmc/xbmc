/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "URIParser.h"

#include <optional>
#include <string>
#include <string_view>

/*! \brief Owned, lossless URI bytes and RFC 3986 component boundaries.
 *  Recognizable malformed URIs are retained; IsValid() reports generic syntax only.
 *  Component views refer to this object's storage and are invalidated by mutation.
 */
class CURI final
{
public:
  CURI() = default;
  explicit CURI(std::string uri);
  CURI(const CURI&) = default;
  CURI& operator=(const CURI&) = default;
  CURI(CURI&& other) noexcept;
  CURI& operator=(CURI&& other) noexcept;

  /*! \brief Store a URI candidate and return its validity; no scheme resets to empty. */
  bool Parse(std::string uri);
  bool IsValid() const { return m_valid; }
  const std::string& Get() const { return m_source; }
  const URIParser::Components& GetComponents() const { return m_components; }

  std::string_view GetRawScheme() const;
  std::string_view GetRawAuthority() const;
  std::string_view GetRawPath() const { return m_components.path.Get(m_source); }
  std::string_view GetRawQuery() const;
  std::string_view GetRawFragment() const;
  bool HasAuthority() const { return m_components.authority.has_value(); }
  bool HasQuery() const { return m_components.query.has_value(); }
  bool HasFragment() const { return m_components.fragment.has_value(); }
  std::string_view GetSchemeSpecificPart() const;
  std::string_view GetPathPrefix() const;
  std::string_view GetWithoutQueryAndFragment() const;

  /*! \brief Replace a valid URI's path without changing other components.
   *  Invalid component syntax or authority shape leaves the URI unchanged.
   */
  bool SetPath(std::string path);
  /*! \brief Replace a raw query; nullopt removes ?, an empty string retains it.
   *  Invalid query syntax leaves the URI unchanged. Malformed candidates are supported.
   */
  bool SetRawQuery(std::optional<std::string> query);

private:
  void UpdateComponents();

  std::string m_source;
  URIParser::Components m_components;
  bool m_valid{false};
};
