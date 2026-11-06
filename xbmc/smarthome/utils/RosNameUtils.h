/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace KODI
{
namespace SMART_HOME
{
/*!
 * \brief Normalize a ROS namespace
 *
 * Leading and trailing slashes are normalized and repeated slashes are
 * collapsed. An empty namespace or a namespace containing only slashes is
 * represented by the root namespace, "/".
 *
 * \param rosNamespace Namespace to normalize.
 *
 * \return The root namespace or a fully qualified namespace without a trailing
 *         slash
 */
std::string NormalizeRosNamespace(std::string_view rosNamespace);

/*!
 * \brief Construct a fully qualified ROS name
 *
 * The namespace is normalized before the path components are appended. Leading
 * and trailing slashes on each component are ignored, empty components are
 * skipped, and the result never begins with two slashes.
 *
 * \param rosNamespace Namespace in which to construct the name
 * \param pathComponents Ordered topic, service, or node path components
 *
 * \return A fully qualified ROS name
 */
std::string BuildRosName(std::string_view rosNamespace,
                         const std::vector<std::string_view>& pathComponents);
} // namespace SMART_HOME
} // namespace KODI
