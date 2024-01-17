/*
 *  Copyright (C) 2012-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PeripheralHID.h"

#include <atomic>

class CSetting;

namespace PERIPHERALS
{
/*!
 * \ingroup peripherals
 */
class CPeripheralImon : public CPeripheralHID
{
public:
  CPeripheralImon(CPeripherals& manager,
                  const PeripheralScanResult& scanResult,
                  CPeripheralBus* bus);
  ~CPeripheralImon(void) override = default;
  bool InitialiseFeature(const PeripheralFeature feature) override;
  void OnSettingChanged(const std::string& strChangedSetting) override;
  void OnDeviceRemoved() override;
  void AddSetting(const std::string& strKey,
                  const std::shared_ptr<const CSetting>& setting,
                  int order) override;
  inline bool IsImonConflictsWithDInput() { return m_bImonConflictsWithDInput; }
  static inline long GetCountOfImonsConflictWithDInput()
  {
    return m_lCountOfImonsConflictWithDInput;
  }
  static void ActionOnImonConflict(bool deviceInserted = true);

private:
  bool m_bImonConflictsWithDInput;
  static std::atomic<long> m_lCountOfImonsConflictWithDInput;
};
} // namespace PERIPHERALS
