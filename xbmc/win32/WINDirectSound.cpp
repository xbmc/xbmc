/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "system.h"
#include "WINDirectSound.h"
#include "threads/SingleLock.h"
#include "log.h"
#include "CharsetConverter.h"


BOOL CALLBACK direct_sound_enumerator_callback( LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule, LPVOID lpContext)
{
  CWDSound& enumerator = *static_cast<CWDSound*>(lpContext);
  return enumerator.direct_sound_enumerator_member_callback(lpGuid,lpcstrDescription, lpcstrModule);
}

CWDSound::CWDSound(void)
{
}

CWDSound::~CWDSound(void)
{
  vDSDeviceInfo.clear();
}

std::vector<DSDeviceInfo> CWDSound::GetSoundDevices()
{
  CSingleLock lock (m_critSection);
  vDSDeviceInfo.clear();
  if (FAILED(DirectSoundEnumerate(direct_sound_enumerator_callback, this)))
    CLog::Log(LOGERROR, "%s - failed to enumerate output devices", __FUNCTION__);

  return vDSDeviceInfo;
}

BOOL CWDSound::direct_sound_enumerator_member_callback( LPGUID lpGuid, LPCTSTR lpcstrDescription, LPCTSTR lpcstrModule) 
{
  struct DSDeviceInfo dInfo;
  dInfo.lpGuid = lpGuid;
  dInfo.strDescription = lpcstrDescription;
  dInfo.strModule = lpcstrModule ;
  g_charsetConverter.unknownToUTF8(dInfo.strDescription);
  g_charsetConverter.unknownToUTF8(dInfo.strModule);
  CWDSound::vDSDeviceInfo.push_back(dInfo);
  CLog::Log(LOGDEBUG, "%s - found Device: %s", __FUNCTION__,lpcstrDescription);
  return TRUE;
}