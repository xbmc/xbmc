/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <utility>

namespace KODI
{
namespace SMART_HOME
{
/*!
 * \brief Default ROS image transport used by camera controls
 *
 * \ingroup smarthome
 *
 * This transport is selected when a camera control omits its image transport
 * or when the configured value evaluates to an empty string.
 */
constexpr const char* DEFAULT_IMAGE_TRANSPORT = "compressed";

/*!
 * \brief Collision-safe identity for a camera image subscription
 *
 * \ingroup smarthome
 *
 * The first component is the base ROS image topic and the second component is
 * the image transport name. The topic must not include a transport suffix such
 * as `/compressed` or `/raw`.
 */
using ImageSubscriptionKey = std::pair<std::string, std::string>;

/*!
 * \brief Create a normalized camera image subscription identity
 *
 * \ingroup smarthome
 *
 * The base topic is preserved unchanged. An empty image transport is replaced
 * with \ref DEFAULT_IMAGE_TRANSPORT.
 *
 * \param topic Base ROS image topic
 * \param imageTransport ROS image transport name, or empty to use the default
 *
 * \return The normalized topic and image transport pair
 */
inline ImageSubscriptionKey MakeImageSubscriptionKey(const std::string& topic,
                                                     const std::string& imageTransport)
{
  return {topic, imageTransport.empty() ? DEFAULT_IMAGE_TRANSPORT : imageTransport};
}
} // namespace SMART_HOME
} // namespace KODI
