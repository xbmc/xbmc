/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#if defined(TARGET_ANDROID)
#include "AndroidSettingDirectory.h"
#include "FileItem.h"
#include "File.h"
#include "utils/URIUtils.h"
#include <vector>
#include "utils/log.h"
#include "utils/StringUtils.h"
#include "URL.h"
#include "android/jni/JNIBase.h"
#include "android/jni/Settings.h"

using namespace XFILE;
using namespace jni;
using namespace std;


static const IntentMapping intents[] =
{
  {NULL                         , "", 0 }
};


CAndroidSettingDirectory::CAndroidSettingDirectory(void)
{
  m_intents.push_back({"Accessibility"              , CJNISettings::ACTION_ACCESSIBILITY_SETTINGS, 0});
  m_intents.push_back({"Applications"               , CJNISettings::ACTION_APPLICATION_SETTINGS, 0 });
  m_intents.push_back({"Bluetooth"                  , CJNISettings::ACTION_BLUETOOTH_SETTINGS, 0 });
  m_intents.push_back({"Date/Time"                  , CJNISettings::ACTION_DATE_SETTINGS, 0 });
  m_intents.push_back({"Cast"                       , CJNISettings::ACTION_CAST_SETTINGS, 21 });
  m_intents.push_back({"Device Info"                , CJNISettings::ACTION_DEVICE_INFO_SETTINGS, 0 });
  m_intents.push_back({"Display"                    , CJNISettings::ACTION_DISPLAY_SETTINGS, 0 });
  m_intents.push_back({"Dream"                      , CJNISettings::ACTION_DREAM_SETTINGS, 18 });
  m_intents.push_back({"Home"                       , CJNISettings::ACTION_HOME_SETTINGS, 21 });
  m_intents.push_back({"Input"                      , CJNISettings::ACTION_INPUT_METHOD_SETTINGS, 0 });
  m_intents.push_back({"Internal storage"           , CJNISettings::ACTION_INTERNAL_STORAGE_SETTINGS, 0 });
  m_intents.push_back({"Locale"                     , CJNISettings::ACTION_LOCALE_SETTINGS, 0 });
  m_intents.push_back({"Memory card"                , CJNISettings::ACTION_MEMORY_CARD_SETTINGS, 0 });
  m_intents.push_back({"Privacy"                    , CJNISettings::ACTION_PRIVACY_SETTINGS, 0 });
  m_intents.push_back({"Security"                   , CJNISettings::ACTION_SECURITY_SETTINGS, 0 });
  m_intents.push_back({"Sound"                      , CJNISettings::ACTION_SOUND_SETTINGS, 0 });
  m_intents.push_back({"Wifi"                       , CJNISettings::ACTION_WIFI_SETTINGS, 0 });
}

CAndroidSettingDirectory::~CAndroidSettingDirectory(void)
{
}

bool CAndroidSettingDirectory::GetDirectory(const CURL& url, CFileItemList &items)
{
  std::string dirname = url.GetFileName();
  URIUtils::RemoveSlashAtEnd(dirname);
  int sdk = CJNIBase::GetSDKVersion();
  CLog::Log(LOGDEBUG, "CAndroidSettingDirectory::GetDirectory: %s (sdk:%d;intents:%d)",dirname.c_str(), sdk, m_intents.size());
  if (dirname == "settings")
  {
    for(int i=0; i < m_intents.size(); ++i)
    {
      int sdk = CJNIBase::GetSDKVersion();
      if (m_intents[i].sdk > sdk)
        continue;

      CFileItemPtr pItem(new CFileItem(m_intents[i].intent));
      pItem->m_bIsFolder = false;
      std::string path = StringUtils::Format("androidsetting://%s/%s/%s", url.GetHostName().c_str(), dirname.c_str(), m_intents[i].intent.c_str());
      pItem->SetPath(path);
      pItem->SetLabel(m_intents[i].name);
      pItem->SetArt("thumb", "DefaultProgram.png");
      items.Add(pItem);
    }
    return true;
  }

  CLog::Log(LOGERROR, "CAndroidSettingDirectory::GetDirectory Failed to open %s",url.Get().c_str());
  return false;
}
#endif
