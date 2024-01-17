/*
 *  Copyright (C) 2015-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "peripherals/bus/PeripheralBus.h"

namespace PERIPHERALS
{
/*!
 * \ingroup peripherals
 *
 * @class CPeripheralBusApplication
 *
 * This exposes peripherals that exist logically at the application level,
 * such as emulated joysticks.
 */
class CPeripheralBusApplication : public CPeripheralBus
{
public:
  explicit CPeripheralBusApplication(CPeripherals& manager);
  ~CPeripheralBusApplication(void) override = default;

  // implementation of CPeripheralBus
  void Initialise(void) override;
  void GetDirectory(const std::string& strPath, CFileItemList& items) const override;

  /*!
   * \brief Get the location for the specified controller index
   */
  std::string MakeLocation(unsigned int controllerIndex) const;

protected:
  // implementation of CPeripheralBus
  bool PerformDeviceScan(PeripheralScanResults& results) override;
};
} // namespace PERIPHERALS
