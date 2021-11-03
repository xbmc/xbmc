/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ProcessInfoAndroid.h"

#include "ServiceBroker.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"

using namespace VIDEOPLAYER;

CProcessInfo* CProcessInfoAndroid::Create()
{
  return new CProcessInfoAndroid();
}

void CProcessInfoAndroid::Register()
{
  CProcessInfo::RegisterProcessControl("android", CProcessInfoAndroid::Create);
}

EINTERLACEMETHOD CProcessInfoAndroid::GetFallbackDeintMethod()
{
  return EINTERLACEMETHOD::VS_INTERLACEMETHOD_DEINTERLACE_HALF;
}

bool CProcessInfoAndroid::WantsRawPassthrough()
{
  const std::string device = CServiceBroker::GetSettingsComponent()->GetSettings()->GetString(
      CSettings::SETTING_AUDIOOUTPUT_PASSTHROUGHDEVICE);

  if (std::string::npos != device.find("(RAW)"))
    return true;

  return false;
}
