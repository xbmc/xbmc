/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <string>

#include "DiscSettings.h"
#include "Settings.h"
#include "lib/Setting.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "utils/log.h"
#include "messaging/helpers/DialogOKHelper.h"
#include "utils/Variant.h"

#include <libbluray/bluray.h>
#include <libbluray/bluray-version.h>

using namespace KODI::MESSAGING;

CDiscSettings& CDiscSettings::GetInstance()
{
  static CDiscSettings sDiscSettings;
  return sDiscSettings;
}

void CDiscSettings::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
#if (BLURAY_VERSION >= BLURAY_VERSION_CODE(1,0,1))
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();

  if (settingId == CSettings::SETTING_DISC_PLAYBACK)
  {
    int mode = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    if (mode == BD_PLAYBACK_DISC_MENU)
    {
      bool bdjWorking = false;
      BLURAY* bd = bd_init();
      const BLURAY_DISC_INFO* info = bd_get_disc_info(bd);

      if (!info->libjvm_detected)
        CLog::Log(LOGDEBUG, "DiscSettings - Could not load the java vm.");
      else if (!info->bdj_handled)
        CLog::Log(LOGDEBUG, "DiscSettings - Could not load the libbluray.jar.");
      else
        bdjWorking = true;

      bd_close(bd);

      if (!bdjWorking)
        HELPERS::ShowOKDialogText(CVariant{ 29803 }, CVariant{ 29804 });
    }
  }
#endif
}
