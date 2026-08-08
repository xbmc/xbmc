/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef>
#include <string>

class CURL;

namespace XFILE::DataURI
{

/*!
 * \brief Validate an RFC 2397 data URI and calculate its decoded size
 *
 * \param url The data URI to validate
 * \param decodedSize The decoded payload size
 * \return true if the data URI is valid, false otherwise
 *
 * \sa https://www.rfc-editor.org/rfc/rfc2397.html
 */
bool Validate(const CURL& url, size_t& decodedSize);

/*!
 * \brief Validate and materialize an RFC 2397 data URI
 *
 * \param url The data URI to materialize
 * \param decoded The decoded payload
 * \return true if the data URI is valid and was materialized, false otherwise
 *
 * \sa https://www.rfc-editor.org/rfc/rfc2397.html
 */
bool Materialize(const CURL& url, std::string& decoded);

} // namespace XFILE::DataURI
