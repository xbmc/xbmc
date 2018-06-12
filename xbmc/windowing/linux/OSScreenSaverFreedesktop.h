/*
 *      Copyright (C) 2017 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <cstdint>

#include "../OSScreenSaver.h"

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
  static bool IsAvailable();
  void Inhibit() override;
  void Uninhibit() override;

private:
  bool m_inhibited{false};
  std::uint32_t m_cookie;
};

}
}
}
