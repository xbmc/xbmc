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

#include "InterProcess_AudioEngine_General.h"
#include "InterProcess.h"
#include "RequestPacket.h"
#include "ResponsePacket.h"

#include <p8-platform/util/StringUtils.h>
#include <iostream>       // std::cerr

using namespace P8PLATFORM;

extern "C"
{

  void CKODIAddon_InterProcess_AudioEngine_General::AddDSPMenuHook(AE_DSP_MENUHOOK* hook)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_General_AddDSPMenuHook, session);
      vrp.push(API_INT,     &hook->iHookId);
      vrp.push(API_INT,     &hook->iLocalizedStringId);
      vrp.push(API_INT,     &hook->category);
      vrp.push(API_INT,     &hook->iRelevantModeId);
      vrp.push(API_BOOLEAN, &hook->bNeedPlayback);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_AudioEngine_General::RemoveDSPMenuHook(AE_DSP_MENUHOOK* hook)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_General_RemoveDSPMenuHook, session);
      vrp.push(API_INT,     &hook->iHookId);
      vrp.push(API_INT,     &hook->iLocalizedStringId);
      vrp.push(API_INT,     &hook->category);
      vrp.push(API_INT,     &hook->iRelevantModeId);
      vrp.push(API_BOOLEAN, &hook->bNeedPlayback);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_AudioEngine_General::RegisterDSPMode(AE_DSP_MODES::AE_DSP_MODE* mode)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_General_RegisterDSPMode, session);
      vrp.push(API_INT,          &mode->iUniqueDBModeId);
      vrp.push(API_INT,          &mode->iModeType);
      vrp.push(API_STRING,       &mode->strModeName);
      vrp.push(API_UNSIGNED_INT, &mode->iModeNumber);
      vrp.push(API_UNSIGNED_INT, &mode->iModeSupportTypeFlags);
      vrp.push(API_BOOLEAN,      &mode->bHasSettingsDialog);
      vrp.push(API_BOOLEAN,      &mode->bIsDisabled);
      vrp.push(API_UNSIGNED_INT, &mode->iModeName);
      vrp.push(API_UNSIGNED_INT, &mode->iModeSetupName);
      vrp.push(API_UNSIGNED_INT, &mode->iModeDescription);
      vrp.push(API_UNSIGNED_INT, &mode->iModeHelp);
      vrp.push(API_STRING,       &mode->strOwnModeImage);
      vrp.push(API_STRING,       &mode->strOverrideModeImage);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  void CKODIAddon_InterProcess_AudioEngine_General::UnregisterDSPMode(AE_DSP_MODES::AE_DSP_MODE* mode)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_General_UnregisterDSPMode, session);
      vrp.push(API_INT,          &mode->iUniqueDBModeId);
      vrp.push(API_INT,          &mode->iModeType);
      vrp.push(API_STRING,       &mode->strModeName);
      vrp.push(API_UNSIGNED_INT, &mode->iModeNumber);
      vrp.push(API_UNSIGNED_INT, &mode->iModeSupportTypeFlags);
      vrp.push(API_BOOLEAN,      &mode->bHasSettingsDialog);
      vrp.push(API_BOOLEAN,      &mode->bIsDisabled);
      vrp.push(API_UNSIGNED_INT, &mode->iModeName);
      vrp.push(API_UNSIGNED_INT, &mode->iModeSetupName);
      vrp.push(API_UNSIGNED_INT, &mode->iModeDescription);
      vrp.push(API_UNSIGNED_INT, &mode->iModeHelp);
      vrp.push(API_STRING,       &mode->strOwnModeImage);
      vrp.push(API_STRING,       &mode->strOverrideModeImage);
      uint32_t retCode = PROCESS_ADDON_CALL(session, vrp);
      if (retCode != API_SUCCESS)
        throw retCode;
    }
    PROCESS_HANDLE_EXCEPTION;
  }

  bool CKODIAddon_InterProcess_AudioEngine_General::GetCurrentSinkFormat(AudioEngineFormat &sinkFormat)
  {
    try
    {
      CKODIAddon_InterProcess *session = g_threadMap.at(std::this_thread::get_id());
      CRequestPacket vrp(KODICall_AudioEngine_General_UnregisterDSPMode, session);
      uint32_t retCode;
      CLockObject lock(session->m_callMutex);
      std::unique_ptr<CResponsePacket> vresp(session->ReadResult(&vrp));
      if (!vresp)
        throw API_ERR_BUFFER;
      vresp->pop(API_UINT32_T,     &retCode);
      vresp->pop(API_INT,          &sinkFormat.m_dataFormat);
      vresp->pop(API_UNSIGNED_INT, &sinkFormat.m_sampleRate);
      vresp->pop(API_UNSIGNED_INT, &sinkFormat.m_encodedRate);
      vresp->pop(API_UNSIGNED_INT, &sinkFormat.m_channelCount);
      for (unsigned int i = 0; i < AE_CH_MAX; ++i)
        vresp->pop(API_INT, &sinkFormat.m_channels[i]);
      vresp->pop(API_UNSIGNED_INT, &sinkFormat.m_frames);
      vresp->pop(API_UNSIGNED_INT, &sinkFormat.m_frameSize);
      if (retCode == API_ERR_OTHER)
        return false;
      if (retCode != API_SUCCESS)
        throw retCode;
      return true;
    }
    PROCESS_HANDLE_EXCEPTION;
    return false;
  }

}; /* extern "C" */
