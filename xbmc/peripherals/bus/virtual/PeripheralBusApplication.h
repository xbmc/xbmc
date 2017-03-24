/*
 *      Copyright (C) 2015-2016 Team Kodi
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

#include "peripherals/bus/PeripheralBus.h"

namespace PERIPHERALS
{
  /*!
   * @class CPeripheralBusApplication
   *
   * This exposes peripherals that exist logically at the application level,
   * such as emulated joysticks.
   */
  class CPeripheralBusApplication : public CPeripheralBus
  {
  public:
    CPeripheralBusApplication(CPeripherals& manager);
    virtual ~CPeripheralBusApplication(void) { }

    // implementation of CPeripheralBus
    virtual void Initialise(void) override;
    virtual void GetDirectory(const std::string &strPath, CFileItemList &items) const override;

    /*!
     * \brief Get the location for the specified controller index
     */
    std::string MakeLocation(unsigned int controllerIndex) const;

  protected:
    // implementation of CPeripheralBus
    virtual bool PerformDeviceScan(PeripheralScanResults& results) override;
  };
}
