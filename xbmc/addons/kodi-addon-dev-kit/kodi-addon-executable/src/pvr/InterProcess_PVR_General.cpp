/*
 *      Copyright (C) 2016 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InterProcess_PVR_General.h"
#include "InterProcess.h"
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <p8-platform/util/StringUtils.h>
#include <iostream>       // std::cerr

using namespace P8PLATFORM;

extern "C"
{

  void CKODIAddon_InterProcess_PVR_General::AddMenuHook(PVR_MENUHOOK* hook)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_PVR_AddMenuHook, session);
      for (unsigned int i = 0; i < sizeof(PVR_MENUHOOK); ++i)
        vrp.push(API_UINT8_T, hook+i);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_PVR_General::Recording(const std::string& strRecordingName, const std::string& strFileName, bool bOn)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_PVR_EpgEntry, session);
      vrp.push(API_STRING, strRecordingName.c_str());
      vrp.push(API_STRING, strFileName.c_str());
      vrp.push(API_BOOLEAN, &bOn);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

}; /* extern "C" */
