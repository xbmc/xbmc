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

#include "InterProcess_PVR_Transfer.h"
#include "InterProcess.h"
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <p8-platform/util/StringUtils.h>
#include <iostream>       // std::cerr

using namespace P8PLATFORM;

extern "C"
{

  void CKODIAddon_InterProcess_PVR_Transfer::EpgEntry(const ADDON_HANDLE handle, const EPG_TAG* entry)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_PVR_EpgEntry, session);
      vrp.push(API_UINT64_T, &ptr);
      for (unsigned int i = 0; i < sizeof(EPG_TAG); ++i)
        vrp.push(API_UINT8_T, entry+i);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_PVR_Transfer::ChannelEntry(const ADDON_HANDLE handle, const PVR_CHANNEL* entry)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_PVR_ChannelEntry, session);
      vrp.push(API_UINT64_T, &ptr);
      for (unsigned int i = 0; i < sizeof(PVR_CHANNEL); ++i)
        vrp.push(API_UINT8_T, entry+i);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_PVR_Transfer::TimerEntry(const ADDON_HANDLE handle, const PVR_TIMER* entry)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_PVR_TimerEntry, session);
      vrp.push(API_UINT64_T, &ptr);
      for (unsigned int i = 0; i < sizeof(PVR_TIMER); ++i)
        vrp.push(API_UINT8_T, entry+i);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_PVR_Transfer::RecordingEntry(const ADDON_HANDLE handle, const PVR_RECORDING* entry)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_PVR_RecordingEntry, session);
      vrp.push(API_UINT64_T, &ptr);
      for (unsigned int i = 0; i < sizeof(PVR_RECORDING); ++i)
        vrp.push(API_UINT8_T, entry+i);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_PVR_Transfer::ChannelGroup(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP* entry)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_PVR_ChannelGroup, session);
      vrp.push(API_UINT64_T, &ptr);
      for (unsigned int i = 0; i < sizeof(PVR_CHANNEL_GROUP); ++i)
        vrp.push(API_UINT8_T, entry+i);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_PVR_Transfer::ChannelGroupMember(const ADDON_HANDLE handle, const PVR_CHANNEL_GROUP_MEMBER* entry)
  {
    try
    {
      uint64_t ptr = (uint64_t)handle;
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_PVR_ChannelGroupMember, session);
      vrp.push(API_UINT64_T, &ptr);
      for (unsigned int i = 0; i < sizeof(PVR_CHANNEL_GROUP_MEMBER); ++i)
        vrp.push(API_UINT8_T, entry+i);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

}; /* extern "C" */
