/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Ros2Translator.h"

#include <oasis_msgs/msg/peripheral_constants.hpp>

using namespace KODI;
using namespace SMART_HOME;

AVPixelFormat CRos2Translator::TranslateEncoding(const std::string& encoding, bool isBigEndian)
{
  // clang-format off

  if (encoding == sensor_msgs::image_encodings::RGB8)
    return AV_PIX_FMT_RGB24;

  else if (encoding == sensor_msgs::image_encodings::RGBA8)
    return AV_PIX_FMT_RGBA;

  else if (encoding == sensor_msgs::image_encodings::RGB16)
    return isBigEndian ?
        AV_PIX_FMT_RGB48BE :
        AV_PIX_FMT_RGB48LE;

  else if (encoding == sensor_msgs::image_encodings::RGBA16)
    return isBigEndian ?
        AV_PIX_FMT_RGBA64BE :
        AV_PIX_FMT_RGBA64LE;

  else if (encoding == sensor_msgs::image_encodings::BGR8)
    return AV_PIX_FMT_BGR24;

  else if (encoding == sensor_msgs::image_encodings::BGRA8)
    return AV_PIX_FMT_BGRA;

  else if (encoding == sensor_msgs::image_encodings::BGR16)
    return isBigEndian ?
      AV_PIX_FMT_BGR48BE :
      AV_PIX_FMT_BGR48LE;

  else if (encoding == sensor_msgs::image_encodings::BGRA16)
    return isBigEndian ?
      AV_PIX_FMT_BGRA64BE :
      AV_PIX_FMT_BGRA64LE;

  else if (encoding == sensor_msgs::image_encodings::MONO8)
    return AV_PIX_FMT_GRAY8;

  else if (encoding == sensor_msgs::image_encodings::MONO16)
    return isBigEndian ?
      AV_PIX_FMT_GRAY16BE :
      AV_PIX_FMT_GRAY16LE;

  // clang-format on

  return AV_PIX_FMT_NONE;
}

uint8_t CRos2Translator::TranslatePeripheralType(PERIPHERALS::PeripheralType peripheralType)
{
  using PeripheralConstants = oasis_msgs::msg::PeripheralConstants;

  switch (peripheralType)
  {
    case PERIPHERALS::PERIPHERAL_JOYSTICK:
      return PeripheralConstants::TYPE_JOYSTICK;
      break;
    case PERIPHERALS::PERIPHERAL_KEYBOARD:
      return PeripheralConstants::TYPE_KEYBOARD;
      break;
    case PERIPHERALS::PERIPHERAL_MOUSE:
      return PeripheralConstants::TYPE_MOUSE;
      break;
    default:
      break;
  }

  return PeripheralConstants::TYPE_UNKNOWN;
}
