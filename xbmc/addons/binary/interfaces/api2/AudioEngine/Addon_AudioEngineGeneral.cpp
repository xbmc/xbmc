/*
 *      Copyright (C) 2014-2016 Team KODI
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

#include "Addon_AudioEngineGeneral.h"

#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/interfaces/api2/AddonInterfaceBase.h"
#include "addons/kodi-addon-dev-kit/include/kodi/api2/.internal/AddonLib_internal.hpp"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/DSPAddons/ActiveAEDSP.h"
#include "cores/AudioEngine/DSPAddons/ActiveAEDSPMode.h"
#include "cores/AudioEngine/Interfaces/AEStream.h"
#include "cores/AudioEngine/Utils/AEChannelData.h"
#include "utils/log.h"

using namespace ADDON;
using namespace ActiveAE;

namespace V2
{
namespace KodiAPI
{

namespace AudioEngine
{
extern "C"
{

void CAddOnAEGeneral::Init(struct CB_AddOnLib *interfaces)
{
  interfaces->AudioEngine.add_dsp_menu_hook        = V2::KodiAPI::AudioEngine::CAddOnAEGeneral::add_dsp_menu_hook;
  interfaces->AudioEngine.remove_dsp_menu_hook     = V2::KodiAPI::AudioEngine::CAddOnAEGeneral::remove_dsp_menu_hook;

  interfaces->AudioEngine.register_dsp_mode        = V2::KodiAPI::AudioEngine::CAddOnAEGeneral::register_dsp_mode;
  interfaces->AudioEngine.unregister_dsp_Mode      = V2::KodiAPI::AudioEngine::CAddOnAEGeneral::unregister_dsp_mode;

  interfaces->AudioEngine.get_current_sink_format  = V2::KodiAPI::AudioEngine::CAddOnAEGeneral::get_current_sink_format;

  interfaces->AudioEngine.make_stream              = V2::KodiAPI::AudioEngine::CAddOnAEGeneral::make_stream;
  interfaces->AudioEngine.free_stream              = V2::KodiAPI::AudioEngine::CAddOnAEGeneral::free_stream;
}

/*\_____________________________________________________________________________
\*/

CActiveAEDSPAddon *CAddOnAEGeneral::GetAudioDSPAddon(void *hdl)
{
  try
  {
    CAddonInterfaces* addon = static_cast<CAddonInterfaces*>(static_cast<AddonCB*>(hdl)->addonData);
    if (addon && addon->AddOnLib_GetHelper())
      return dynamic_cast<CActiveAEDSPAddon*>(static_cast<CAddonInterfaceAddon*>(addon->AddOnLib_GetHelper())->GetAddon());

    throw ADDON::WrongValueException("CAddOnAEGeneral - %s - invalid handler data", __FUNCTION__);
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

/*\_____________________________________________________________________________
\*/

void CAddOnAEGeneral::add_dsp_menu_hook(void *hdl, AE_DSP_MENUHOOK *hook)
{
  try
  {
    CActiveAEDSPAddon *addon = GetAudioDSPAddon(hdl);
    if (!hook || !addon)
      throw ADDON::WrongValueException("CAddOnAEGeneral - %s - invalid data (addon='%p', hook='%p')", __FUNCTION__, addon, hook);

    AE_DSP_MENUHOOKS *hooks = addon->GetMenuHooks();
    if (hooks)
    {
      AE_DSP_MENUHOOK hookInt;
      hookInt.iHookId            = hook->iHookId;
      hookInt.iLocalizedStringId = hook->iLocalizedStringId;
      hookInt.category           = hook->category;
      hookInt.iRelevantModeId    = hook->iRelevantModeId;
      hookInt.bNeedPlayback      = hook->bNeedPlayback;

      /* add this new hook */
      hooks->push_back(hookInt);
    }
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnAEGeneral::remove_dsp_menu_hook(void *hdl, AE_DSP_MENUHOOK *hook)
{
  try
  {
    CActiveAEDSPAddon *addon = GetAudioDSPAddon(hdl);
    if (!hook || !addon)
      throw ADDON::WrongValueException("CAddOnAEGeneral - %s - invalid data (addon='%p', hook='%p')", __FUNCTION__, addon, hook);

    AE_DSP_MENUHOOKS *hooks = addon->GetMenuHooks();
    if (hooks)
    {
      for (unsigned int i = 0; i < hooks->size(); ++i)
      {
        if (hooks->at(i).iHookId == hook->iHookId)
        {
          /* remove this hook */
          hooks->erase(hooks->begin()+i);
          break;
        }
      }
    }
  }
  HANDLE_ADDON_EXCEPTION
}

/*\_____________________________________________________________________________
\*/

void CAddOnAEGeneral::register_dsp_mode(void* hdl, void* mode)
{
  try
  {
    CActiveAEDSPAddon *addon = GetAudioDSPAddon(hdl);
    if (!mode || !addon)
      throw ADDON::WrongValueException("CAddOnAEGeneral - %s - invalid data (addon='%p', mode='%p')", __FUNCTION__, addon, mode);

    CActiveAEDSPMode transferMode(*(AE_DSP_MODES::AE_DSP_MODE*)mode, addon->GetID());
    int idMode = transferMode.AddUpdate();
    ((AE_DSP_MODES::AE_DSP_MODE*)mode)->iUniqueDBModeId = idMode;

    if (idMode > AE_DSP_INVALID_ADDON_ID)
    {
      CLog::Log(LOGDEBUG, "CAddOnAEGeneral - %s - successfull registered mode %s of %s adsp-addon", __FUNCTION__, ((AE_DSP_MODES::AE_DSP_MODE*)mode)->strModeName, addon->Name().c_str());
    }
    else
    {
      CLog::Log(LOGERROR, "CAddOnAEGeneral - %s - failed to register mode %s of %s adsp-addon", __FUNCTION__, ((AE_DSP_MODES::AE_DSP_MODE*)mode)->strModeName, addon->Name().c_str());
    }
  }
  HANDLE_ADDON_EXCEPTION
}

void CAddOnAEGeneral::unregister_dsp_mode(void* hdl, void* mode)
{
  try
  {
    CActiveAEDSPAddon *addon = GetAudioDSPAddon(hdl);
    if (!mode || !addon)
      throw ADDON::WrongValueException("CAddOnAEGeneral - %s - invalid data (addon='%p', mode='%p')", __FUNCTION__, addon, mode);

    CActiveAEDSPMode transferMode(*(AE_DSP_MODES::AE_DSP_MODE*)mode, addon->GetID());
    transferMode.Delete();
  }
  HANDLE_ADDON_EXCEPTION
}

/*\_____________________________________________________________________________
\*/

void* CAddOnAEGeneral::make_stream(void* hdl, AudioEngineFormat Format, unsigned int Options)
{
  try
  {
    AEAudioFormat format;
    format.m_dataFormat = Format.m_dataFormat;
    format.m_sampleRate = Format.m_sampleRate;
    format.m_channelLayout = Format.m_channels;
    return CAEFactory::MakeStream(format, Options);
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddOnAEGeneral::free_stream(void* hdl, void *StreamHandle)
{
  try
  {
    if (!StreamHandle)
      throw ADDON::WrongValueException("CAddOnAEGeneral - %s - invalid handler data", __FUNCTION__);

    CAEFactory::FreeStream((IAEStream*)StreamHandle);
  }
  HANDLE_ADDON_EXCEPTION
}

bool CAddOnAEGeneral::get_current_sink_format(void *hdl, AudioEngineFormat *sinkFormat)
{
  try
  {
    if (!sinkFormat)
      throw ADDON::WrongValueException("CAddOnAEGeneral - %s - invalid data (handle='%p', sinkFormat='%p')", __FUNCTION__, hdl, sinkFormat);

    AEAudioFormat AESinkFormat;
    if (!CAEFactory::GetEngine() || !CAEFactory::GetEngine()->GetCurrentSinkFormat(AESinkFormat))
    {
      CLog::Log(LOGERROR, "CAddOnAEGeneral - %s - failed to get current sink format from AE!", __FUNCTION__);
      return false;
    }

    sinkFormat->m_channelCount = AESinkFormat.m_channelLayout.Count();
    for (unsigned int ch = 0; ch < AE_CH_MAX; ++ch)
    {
      sinkFormat->m_channels[ch] = AESinkFormat.m_channelLayout[ch];
    }

    sinkFormat->m_dataFormat   = AESinkFormat.m_dataFormat;
    sinkFormat->m_sampleRate   = AESinkFormat.m_sampleRate;
    sinkFormat->m_frames       = AESinkFormat.m_frames;
    sinkFormat->m_frameSize    = AESinkFormat.m_frameSize;

    return true;
  }
  HANDLE_ADDON_EXCEPTION

  return false;
}

} /* extern "C" */
} /* namespace AudioEngine */

} /* namespace KodiAPI */
} /* namespace V2 */
