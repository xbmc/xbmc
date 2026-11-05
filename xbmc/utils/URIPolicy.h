/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <optional>
#include <string_view>

class CURI;

namespace URIPolicy
{
/*! \brief Operation policy, independent of URI validity and installed handlers. */
struct Policy
{
  bool filesystem{false}; // Path operations for lossless URI storage.
  bool queryIdentity{true};
  bool protocolOptions{false};
  bool redactBody{false};
  bool legacy{false}; // Eligibility for Kodi scheme:// compatibility.
};

/*! \brief Extract a scheme, respecting native drive prefixes.
 * \return A source view valid until input changes or is destroyed.
 * \note On Windows, one-letter prefixes are always drives; elsewhere, // selects VFS.
 *       Use IsURILocation to classify mixed input, or CURL::ParseURI for explicit intent.
 */
std::optional<std::string_view> GetScheme(std::string_view input);

/*! \brief Classify mixed input as a recognized URI scheme or legacy VFS URL.
 *  \note Unknown authority-free identifiers require explicit CURL::ParseURI intent.
 */
bool IsURILocation(std::string_view input);

/*!
 * \brief Compare raw ASCII scheme names case-insensitively, without decoding.
 */
bool SchemeEquals(std::string_view left, std::string_view right);

/*! \brief Return the policy for a case-insensitive scheme name.
 * \note Filesystem policy alone does not establish a navigable magnet path.
 */
Policy GetPolicy(std::string_view scheme);

/*!
 * \brief Recognize Kodi's legacy leading scheme:// spelling, not every URI or URL.
 * \note Native drive prefixes take precedence. Body validity and backend support are unchecked.
 */
bool IsLegacyURL(std::string_view input);
/*! \brief Select Kodi compatibility for a mixed-input location, including legacy backslashes. */
bool UsesLegacyURL(std::string_view input);

bool SupportsPathOperations(const CURI& uri);
bool CanSetPath(const CURI& uri, std::string_view path);
} // namespace URIPolicy
