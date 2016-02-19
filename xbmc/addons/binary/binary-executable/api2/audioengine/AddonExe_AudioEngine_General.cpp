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

#include "AddonExe_AudioEngine_General.h"
#include "addons/Addon.h"
#include "addons/binary/callbacks/api2/AudioEngine/AudioEngineCB_General.h"

namespace V2
{
namespace KodiAPI
{

bool CAddonExeCB_AudioEngine_General::AddDSPMenuHook(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  AE_DSP_MENUHOOK hook;
  req.pop(API_INT,    &hook.iHookId);
  req.pop(API_INT,    &hook.iLocalizedStringId);
  req.pop(API_INT,    &hook.category);
  req.pop(API_INT,    &hook.iRelevantModeId);
  req.pop(API_BOOLEAN,    &hook.bNeedPlayback);
  AudioEngine::CAddOnAEGeneral::add_dsp_menu_hook(addon, &hook);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_AudioEngine_General::RemoveDSPMenuHook(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  AE_DSP_MENUHOOK hook;
  req.pop(API_INT,         &hook.iHookId);
  req.pop(API_INT,         &hook.iLocalizedStringId);
  req.pop(API_INT,         &hook.category);
  req.pop(API_INT,         &hook.iRelevantModeId);
  req.pop(API_BOOLEAN,     &hook.bNeedPlayback);
  AudioEngine::CAddOnAEGeneral::remove_dsp_menu_hook(addon, &hook);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_AudioEngine_General::RegisterDSPMode(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  AE_DSP_MODES::AE_DSP_MODE mode;
  req.pop(API_INT,          &mode.iUniqueDBModeId);
  req.pop(API_INT,          &mode.iModeType);
  req.pop(API_STRING,       &mode.strModeName);
  req.pop(API_UNSIGNED_INT, &mode.iModeNumber);
  req.pop(API_UNSIGNED_INT, &mode.iModeSupportTypeFlags);
  req.pop(API_BOOLEAN,      &mode.bHasSettingsDialog);
  req.pop(API_BOOLEAN,      &mode.bIsDisabled);
  req.pop(API_UNSIGNED_INT, &mode.iModeName);
  req.pop(API_UNSIGNED_INT, &mode.iModeSetupName);
  req.pop(API_UNSIGNED_INT, &mode.iModeDescription);
  req.pop(API_UNSIGNED_INT, &mode.iModeHelp);
  req.pop(API_STRING,       &mode.strOwnModeImage);
  req.pop(API_STRING,       &mode.strOverrideModeImage);
  AudioEngine::CAddOnAEGeneral::register_dsp_mode(addon, &mode);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_AudioEngine_General::UnregisterDSPMode(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  AE_DSP_MODES::AE_DSP_MODE mode;
  req.pop(API_INT,          &mode.iUniqueDBModeId);
  req.pop(API_INT,          &mode.iModeType);
  req.pop(API_STRING,       &mode.strModeName);
  req.pop(API_UNSIGNED_INT, &mode.iModeNumber);
  req.pop(API_UNSIGNED_INT, &mode.iModeSupportTypeFlags);
  req.pop(API_BOOLEAN,      &mode.bHasSettingsDialog);
  req.pop(API_BOOLEAN,      &mode.bIsDisabled);
  req.pop(API_UNSIGNED_INT, &mode.iModeName);
  req.pop(API_UNSIGNED_INT, &mode.iModeSetupName);
  req.pop(API_UNSIGNED_INT, &mode.iModeDescription);
  req.pop(API_UNSIGNED_INT, &mode.iModeHelp);
  req.pop(API_STRING,       &mode.strOwnModeImage);
  req.pop(API_STRING,       &mode.strOverrideModeImage);
  AudioEngine::CAddOnAEGeneral::unregister_dsp_mode(addon, &mode);
  PROCESS_ADDON_RETURN_CALL(API_SUCCESS);
  return true;
}

bool CAddonExeCB_AudioEngine_General::GetCurrentSinkFormat(AddonCB* addon, CRequestPacket& req, CResponsePacket& resp)
{
  AudioEngineFormat sinkFormat;
  bool ret = AudioEngine::CAddOnAEGeneral::get_current_sink_format(addon, &sinkFormat);
  uint32_t retValue = ret ? API_SUCCESS : API_ERR_OTHER;
  resp.init(req.getRequestID());
  resp.push(API_UINT32_T,     &retValue);
  resp.push(API_INT,          &sinkFormat.m_dataFormat);
  resp.push(API_UNSIGNED_INT, &sinkFormat.m_sampleRate);
  resp.push(API_UNSIGNED_INT, &sinkFormat.m_encodedRate);
  resp.push(API_UNSIGNED_INT, &sinkFormat.m_channelCount);
  for (unsigned int i = 0; i < AE_CH_MAX; ++i)
    resp.push(API_INT, &sinkFormat.m_channels[i]);
  resp.push(API_UNSIGNED_INT, &sinkFormat.m_frames);
  resp.push(API_UNSIGNED_INT, &sinkFormat.m_frameSize);
  resp.finalise();
  return true;
}

}; /* namespace KodiAPI */
}; /* namespace V2 */
