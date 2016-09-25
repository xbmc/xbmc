/*
 *      Copyright (C) 2016 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "PlatformAndroid.h"
#include <stdlib.h>
#include "filesystem/SpecialProtocol.h"
#include "platform/android/jni/Context.h"
#include "platform/android/jni/SettingsSecure.h"
#include "utils/log.h"
#include "utils/md5.h"

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformAndroid();
}

void CPlatformAndroid::Init()
{
  // call base init
  CPlatform::Init();
  setenv("SSL_CERT_FILE", CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem").c_str(), 1);
}

void CPlatformAndroid::InitUniqueHardwareIdentifier()
{
  m_uuid = jni::CJNISettingsSecure::getString(CJNIContext::getContentResolver(), jni::CJNISettingsSecure::ANDROID_ID);
#if defined(_DEBUG)
    CLog::Log(LOGDEBUG, "HardwareUUID (nomd5): %s", m_uuid.c_str());
#endif

  m_uuid = XBMC::XBMC_MD5::GetMD5(m_uuid);
  CLog::Log(LOGNOTICE, "HardwareUUID: %s", m_uuid.c_str());
}
