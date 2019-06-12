/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "xbmc/powermanagement/DPMSSupport.h"

class CWin32DPMSSupport : public CDPMSSupport
{
public:
  CWin32DPMSSupport();
  ~CWin32DPMSSupport() = default;

protected:
  bool EnablePowerSaving(PowerSavingMode mode) override;
  bool DisablePowerSaving() override;
};
