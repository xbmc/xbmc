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
#include "platform/android/powermanagement/AndroidPowerSyscall.h"
#if HAS_LIRC
#include "platform/linux/input/LIRC.h"
#endif

CPlatform* CPlatform::CreateInstance()
{
  return new CPlatformAndroid();
}

CPlatformAndroid::CPlatformAndroid()
{
  
}

CPlatformAndroid::~CPlatformAndroid()
{
  
}

void CPlatformAndroid::Init()
{
  setenv("SSL_CERT_FILE", CSpecialProtocol::TranslatePath("special://xbmc/system/certs/cacert.pem").c_str(), 1);
  CAndroidPowerSyscall::Register();
#if HAS_LIRC
  CRemoteControl::Register();
#endif
}
