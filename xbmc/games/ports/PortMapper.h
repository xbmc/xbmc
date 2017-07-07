/*
 *      Copyright (C) 2015-2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "PortTypes.h"
#include "peripherals/PeripheralTypes.h"
#include "utils/Observer.h"

#include <map>

namespace PERIPHERALS
{
  class CPeripherals;
}

namespace KODI
{
namespace GAME
{
  class CPortManager;

  class CPortMapper : public Observer
  {
  public:
    CPortMapper(PERIPHERALS::CPeripherals& peripheralManager, CPortManager& portManager);

    virtual ~CPortMapper();

    virtual void Notify(const Observable& obs, const ObservableMessage msg) override;

  private:
    void ProcessPeripherals();

    // Construction parameters
    PERIPHERALS::CPeripherals &m_peripheralManager;
    CPortManager &m_portManager;

    // Port paremters
    std::map<PERIPHERALS::PeripheralPtr, PortPtr> m_portMap;
  };
}
}
