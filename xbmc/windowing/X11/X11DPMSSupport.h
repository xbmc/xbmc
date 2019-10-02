/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "xbmc/powermanagement/DPMSSupport.h"

class CX11DPMSSupport : public CDPMSSupport
{
public:
  CX11DPMSSupport();
  ~CX11DPMSSupport() override = default;

protected:
  bool EnablePowerSaving(PowerSavingMode mode) override;
  bool DisablePowerSaving() override;
};
