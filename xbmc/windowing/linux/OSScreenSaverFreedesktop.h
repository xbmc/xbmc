/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../OSScreenSaver.h"

#include <cstdint>
#include <memory>

namespace sdbus
{
class IProxy;
}

namespace KODI
{
namespace WINDOWING
{
namespace LINUX
{

// FIXME This is not really linux-specific, BSD could also have this. Better directory name?

class COSScreenSaverFreedesktop : public IOSScreenSaver
{
public:
  COSScreenSaverFreedesktop();
  static bool IsAvailable();
  void Inhibit() override;
  void Uninhibit() override;

private:
  std::unique_ptr<sdbus::IProxy> m_proxy;
  bool m_inhibited{false};
  std::uint32_t m_cookie;
};

}
}
}
