/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "Ros2Node.h"
#include "peripherals/PeripheralTypes.h"

#include <sensor_msgs/image_encodings.hpp>

extern "C"
{
#include <libavutil/pixfmt.h>
}

#include <stdint.h>
#include <string>

namespace KODI
{
namespace SMART_HOME
{
class CRos2Translator
{
public:
  /*!
   * \brief Translate from ROS encoding string (and endianness) to libav pixel format
   *
   * See:
   *   https://docs.ros.org/en/melodic/api/sensor_msgs/html/msg/Image.html
   *
   * \param encoding The encoding of pixels as defined in sensor_msgs/image_encodings.h
   * \param isBigEndian The endianness, true for big-endian, false for little-endian
   *
   * \return The corresponding libav pixel format, or AV_PIX_FMT_NONE if unknown
   */
  static AVPixelFormat TranslateEncoding(const std::string& encoding, bool isBigEndian);

  /*!
   * \brief Translate from Kodi peripheral type to ROS peripheral constant, as
   *        defined in PeripheralConstants.msg.
   *
   * \param peripheralType The Kodi peripheral type
   *
   * \return The ROS peripheral type
   */
  static uint8_t TranslatePeripheralType(PERIPHERALS::PeripheralType peripheralType);
};
} // namespace SMART_HOME
} // namespace KODI
