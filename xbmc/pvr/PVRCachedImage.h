/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace PVR
{

class CPVRCachedImage
{
public:
  CPVRCachedImage() = delete;
  virtual ~CPVRCachedImage() = default;

  explicit CPVRCachedImage(const std::string& owner);
  CPVRCachedImage(const std::string& clientImage, const std::string& owner);

  bool operator==(const CPVRCachedImage& right) const;
  bool operator!=(const CPVRCachedImage& right) const;

  const std::string& GetClientImage() const { return m_clientImage; }
  const std::string& GetLocalImage() const { return m_localImage; }

  void SetClientImage(const std::string& image);

  void SetOwner(const std::string& owner);

private:
  void UpdateLocalImage();

  std::string m_clientImage;
  std::string m_localImage;
  std::string m_owner;
};

} // namespace PVR
