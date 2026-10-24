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
/*!
 * \brief Kodi operation policy, independent of generic validity and installed handlers.
 * \note Unknown authority-free schemes have no filesystem semantics. Legacy scheme://
 *       compatibility does not depend on whether a backend is currently installed.
 *       Raw query preservation and full-body redaction are Kodi policies.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-1.2.2
 */
struct Policy
{
  bool filesystem{false}; // Path operations for lossless URI storage.
  bool queryIdentity{true};
  bool protocolOptions{false};
  bool redactBody{false};
  bool legacy{false}; // Eligibility for Kodi scheme:// compatibility.
};

/*!
 * \brief Extract a syntactic scheme while respecting native drive prefixes.
 * \param input Raw location; alphabetic drive prefixes retain native interpretation.
 * \return A source view without the colon, valid until input changes or is destroyed.
 * \note On Windows, a letter followed by ':' is a drive prefix, even with //.
 *       Use CURL::ParseURI or legacy setters for explicit one-letter URI/VFS intent.
 * \note A scheme prefix alone does not establish URI intent; use IsURILocation for mixed input.
 *       Explicit URIParser input has no drive-prefix exception.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.1
 * \sa https://www.rfc-editor.org/rfc/rfc8089.html#appendix-E.2
 * \sa https://www.rfc-editor.org/rfc/rfc8089.html#appendix-E.3
 */
std::optional<std::string_view> GetScheme(std::string_view input);

/*! \brief Classify mixed input as a recognized URI scheme or legacy VFS URL.
 *  \note Unknown authority-free identifiers require explicit CURL::ParseURI intent.
 */
bool IsURILocation(std::string_view input);

/*!
 * \brief Compare raw ASCII scheme names case-insensitively, without decoding.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.1
 */
bool SchemeEquals(std::string_view left, std::string_view right);

/*!
 * \brief Return Kodi's small shared policy for a raw scheme name, case-insensitively.
 * \note A filesystem policy permits operations only on the identified path component.
 *       Established VFS compatibility projections are Kodi policy, not URI syntax.
 *       Magnet member paths require additional shape checks below; they are a Kodi
 *       extension to external magnet syntax. URN NSS slashes never imply file hierarchy.
 * \sa https://www.iana.org/assignments/uri-schemes/prov/magnet
 * \sa https://www.bittorrent.org/beps/bep_0009.html#magnet-uri-format
 * \sa https://www.rfc-editor.org/rfc/rfc8141.html#section-4.3
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3
 */
Policy GetPolicy(std::string_view scheme);

/*!
 * \brief Recognize Kodi's legacy leading scheme:// spelling, not every URI or URL.
 * \note Native drive prefixes take precedence. Body validity and backend support are unchecked.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-1.1.3
 */
bool IsLegacyURL(std::string_view input);
/*! \brief Select Kodi compatibility for a mixed-input location, including legacy backslashes. */
bool UsesLegacyURL(std::string_view input);

bool SupportsPathOperations(const CURI& uri);
bool CanSetPath(const CURI& uri, std::string_view path);
} // namespace URIPolicy
