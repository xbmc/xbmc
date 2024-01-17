/*
 *  Copyright (C) 2005-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

/*!
 * \ingroup peripherals
 */
class CPeripheralBusCEC : public CPeripheralBus
{
public:
  explicit CPeripheralBusCEC(CPeripherals& manager);
  ~CPeripheralBusCEC(void) override;

  /*!
   * @see PeripheralBus::PerformDeviceScan()
   */
  bool PerformDeviceScan(PeripheralScanResults& results) override;

private:
  CEC::ICECAdapter* m_cecAdapter;
  CEC::libcec_configuration m_configuration;
};
} // namespace PERIPHERALS
