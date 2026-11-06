/*
 *  Copyright (C) 2021-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace SMART_HOME
{
class CGUICameraConfig
{
public:
  CGUICameraConfig();
  CGUICameraConfig(const CGUICameraConfig& other);
  ~CGUICameraConfig();

  void Reset();

  // Camera configuration
  const std::string& GetTopic() const { return m_topic; }
  void SetTopic(const std::string& topic) { m_topic = topic; }

  const std::string& GetImageTransport() const { return m_imageTransport; }
  void SetImageTransport(const std::string& imageTransport);

private:
  std::string m_topic;
  std::string m_imageTransport;
};
} // namespace SMART_HOME
} // namespace KODI
