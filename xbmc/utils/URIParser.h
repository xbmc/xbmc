/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <optional>
#include <string_view>

namespace URIParser
{
/*!
 * \brief A raw component's byte range, excluding its delimiter.
 * \note Offsets hold no borrowed views and survive copying or moving the source storage.
 *       Reparse after changing its contents; ranges describe the original byte positions.
 */
struct Range
{
  size_t offset{0};
  size_t length{0};

  /*!
   * \brief View this component without decoding or normalization.
   * \param source The original input, or an identical copy, containing this entire range.
   * \return A view valid until the supplied storage is changed or destroyed.
   */
  std::string_view Get(std::string_view source) const { return source.substr(offset, length); }
};

/*!
 * \brief Raw URI-reference boundaries in the scanned input.
 * \note An optional range is present exactly when its delimiter is present, even if empty.
 *       The path is always present and can be empty. No source storage is retained.
 *       Recombining these ranges and their delimiters preserves the original spelling.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-5.3
 */
struct Components
{
  std::optional<Range> scheme;
  std::optional<Range> authority;
  Range path;
  std::optional<Range> query;
  std::optional<Range> fragment;
};

/*!
 * \brief Recognize a leading RFC 3986 scheme followed by a colon, regardless of body validity.
 * \param input URI candidate; native-path intent must be handled by the caller.
 * \return The raw scheme without its colon, or no value if the leading token is not a scheme.
 * \note The view is valid until input storage is changed or destroyed. Spelling is preserved;
 *       scheme comparisons are case-insensitive. Recognition implies neither validity nor a handler.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.1
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-1.2.2
 */
std::optional<std::string_view> GetScheme(std::string_view input);

/*!
 * \brief Locate URI-reference components, including those in malformed candidates.
 * \param input Raw candidate bytes, interpreted only by generic delimiters and GetScheme().
 * \return Source ranges with absent and empty components distinguished; scanning never fails.
 * \note This is the lexical layer used by ParseReference and Kodi compatibility handling.
 *       Except for the leading scheme token, syntax is not validated. No decoding,
 *       normalization, native-path interpretation, or scheme-specific policy is applied.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-4.1
 */
Components ScanReference(std::string_view input);

/*!
 * \brief Validate the RFC 3986 URI-reference production (URI or relative-ref).
 * \param input Raw ASCII URI-reference, including empty input and optional query/fragment.
 * \return Source ranges on success, or no value for invalid generic syntax.
 * \note Only generic syntax is validated, without filesystem/network access or scheme rules.
 *       Raw bytes and empty delimiters are preserved. Percent escapes are checked but never
 *       decoded; no normalization, query mapping, or relative-reference resolution occurs.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-4.1
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-4.2
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#appendix-A
 */
std::optional<Components> ParseReference(std::string_view input);

/*!
 * \brief Validate the RFC 3986 URI production using ParseReference and requiring a scheme.
 * \param input Raw URI candidate; no native-path precedence is applied.
 * \return True for valid generic URI syntax, independently of scheme/backend support.
 * \note Fragments are accepted: URI differs from the fragment-free absolute-URI production.
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3
 * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-4.3
 */
bool IsURI(std::string_view input);
} // namespace URIParser
