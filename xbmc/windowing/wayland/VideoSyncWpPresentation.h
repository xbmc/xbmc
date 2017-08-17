/*
 *      Copyright (C) 2017 Team XBMC
 *      http://xbmc.org
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

#include "Signals.h"
#include "windowing/VideoSync.h"

namespace KODI
{
namespace WINDOWING
{
namespace WAYLAND
{

class CVideoSyncWpPresentation : public CVideoSync
{
public:
  explicit CVideoSyncWpPresentation(void* clock);

  float GetFps() override;
  bool Setup(PUPDATECLOCK func) override;
  void Run(CEvent& stop) override;
  void Cleanup() override;

private:
  void HandlePresentation(timespec tv, std::uint32_t refresh, std::uint32_t syncOutputID, float syncOutputRefreshRate, std::uint64_t msc);

  CEvent m_stopEvent;
  CSignalRegistration m_presentationHandler;
  std::uint64_t m_lastMsc{};
  std::uint32_t m_syncOutputID{};
};

}
}
}