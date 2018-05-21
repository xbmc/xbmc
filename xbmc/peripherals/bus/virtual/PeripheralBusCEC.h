#pragma once
/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "peripherals/bus/PeripheralBus.h"

// undefine macro isset, it collides with function in cectypes.h
#ifdef isset
#undef isset
#endif
#include <libcec/cectypes.h>

namespace CEC
{
  class ICECAdapter;
}

namespace PERIPHERALS
{
  class CPeripherals;

  class CPeripheralBusCEC : public CPeripheralBus
  {
  public:
    explicit CPeripheralBusCEC(CPeripherals& manager);
    ~CPeripheralBusCEC(void) override;

    /*!
     * @see PeripheralBus::PerformDeviceScan()
     */
    bool PerformDeviceScan(PeripheralScanResults &results) override;

  private:
    CEC::ICECAdapter*         m_cecAdapter;
    CEC::libcec_configuration m_configuration;
  };
}
