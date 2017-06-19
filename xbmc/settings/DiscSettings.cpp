/*
 *      Copyright (C) 2017 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"
#ifdef HAVE_LIBBLURAY

#include <string>

#include "DiscSettings.h"
#include "Settings.h"
#include "lib/Setting.h"
#include "DllLibbluray.h"
#include "ServiceBroker.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "dialogs/GUIDialogOK.h"
#include "utils/Variant.h"


CDiscSettings::CDiscSettings(void)
{

  m_dll = new DllLibbluray;
  if (!m_dll->Load())
  {
    delete m_dll;
    m_dll = NULL;
  }
}


CDiscSettings::~CDiscSettings()
{
  delete m_dll;
}

CDiscSettings& CDiscSettings::GetInstance()
{
  static CDiscSettings sDiscSettings;
  return sDiscSettings;
}


void CDiscSettings::OnSettingChanged(std::shared_ptr<const CSetting> setting)
{
#ifdef TARGET_WINDOWS
  if (setting == NULL)
    return;

  const std::string &settingId = setting->GetId();

  if (settingId == CSettings::SETTING_DISC_PLAYBACK)
  {
    int mode = std::static_pointer_cast<const CSettingInt>(setting)->GetValue();
    if (mode == BD_PLAYBACK_DISC_MENU)
    {
      BLURAY* bd = m_dll->bd_init();
      const BLURAY_DISC_INFO* info = m_dll->bd_get_disc_info(bd);
      if (!info->bdj_handled)
      {
        if (!info->libjvm_detected)
        {
          CLog::Log(LOGDEBUG, "DiscSettings - Could not load the java vm.");
          CGUIDialogOK::ShowAndGetInput(CVariant{ 29803 }, CVariant{ 29804 });
        }
        CLog::Log(LOGDEBUG, "DiscSettings - Could not load the libbluray.jar.");
      }
      m_dll->bd_close(bd);
    }
  }
#endif
}
#endif
