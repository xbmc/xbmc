/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CURL;

namespace PVR::UTILS
{
/*!
 * @brief Check whether the given path contains a client id and a provider id.
 * @param path The path.
 * @return True if both client id and provider id are present, false otherwise.
 */
bool HasClientAndProvider(const std::string& path);

/*!
 * @brief Get client id and provider id from the given URL.
 * @param url The URL.
 * @param clientId Filled with the client id on success, PVR_CLIENT_INVALID_UID otherwise
 * @param providerId Filled with the provider id on success, PVR_PROVIDER_INVALID_UID otherwise
 * @return True on success, false otherwise.
 */
bool GetClientAndProviderFromPath(const CURL& url, int& clientId, int& providerId);

/*!
 * @brief Get the name of a provider from the given path.
 * @param path The path.
 * @return the name on success, an empty string otherwise.
 */
std::string GetProviderNameFromPath(const std::string& path);

} // namespace PVR::UTILS
