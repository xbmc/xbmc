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

#include "Application.h"
#include "cores/AudioEngine/AEFactory.h"
#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSP.h"
#include "cores/AudioEngine/Engines/ActiveAE/AudioDSPAddons/ActiveAEDSPMode.h"
#include "cores/AudioEngine/Interfaces/AESound.h"
#include "dialogs/GUIDialogKaiToast.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"

#include "AddonCallbacksAudioDSP.h"

using namespace ADDON;
using namespace ActiveAE;

namespace V1
{
namespace KodiAPI
{

namespace AudioDSP
{

CAddonCallbacksADSP::CAddonCallbacksADSP(ADDON::CAddon* addon)
  : ADDON::IAddonInterface(addon, 1, KODI_AE_DSP_API_VERSION),
    m_callbacks(new CB_ADSPLib)
{
  /* write KODI audio DSP specific add-on function addresses to callback table */
  m_callbacks->AddMenuHook                = ADSPAddMenuHook;
  m_callbacks->RemoveMenuHook             = ADSPRemoveMenuHook;
  m_callbacks->RegisterMode               = ADSPRegisterMode;
  m_callbacks->UnregisterMode             = ADSPUnregisterMode;

  m_callbacks->SoundPlay_GetHandle        = ADSPSoundPlay_GetHandle;
  m_callbacks->SoundPlay_ReleaseHandle    = ADSPSoundPlay_ReleaseHandle;
  m_callbacks->SoundPlay_Play             = ADSPSoundPlay_Play;
  m_callbacks->SoundPlay_Stop             = ADSPSoundPlay_Stop;
  m_callbacks->SoundPlay_IsPlaying        = ADSPSoundPlay_IsPlaying;
  m_callbacks->SoundPlay_SetChannel       = ADSPSoundPlay_SetChannel;
  m_callbacks->SoundPlay_GetChannel       = ADSPSoundPlay_GetChannel;
  m_callbacks->SoundPlay_SetVolume        = ADSPSoundPlay_SetVolume;
  m_callbacks->SoundPlay_GetVolume        = ADSPSoundPlay_GetVolume;
}

CAddonCallbacksADSP::~CAddonCallbacksADSP()
{
  /* delete the callback table */
  delete m_callbacks;
}

CActiveAEDSPAddon *CAddonCallbacksADSP::GetAudioDSPAddon(void *addonData)
{
  CAddonInterfaces *addon = static_cast<CAddonInterfaces *>(addonData);
  if (!addon || !addon->GetHelperADSP())
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - called with a null pointer", __FUNCTION__);
    return NULL;
  }

  return dynamic_cast<CActiveAEDSPAddon*>(static_cast<CAddonCallbacksADSP*>(addon->GetHelperADSP())->m_addon);
}

void CAddonCallbacksADSP::ADSPAddMenuHook(void *addonData, AE_DSP_MENUHOOK *hook)
{
  CActiveAEDSPAddon *client = GetAudioDSPAddon(addonData);
  if (!hook || !client)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid handler data", __FUNCTION__);
    return;
  }

  AE_DSP_MENUHOOKS *hooks = client->GetMenuHooks();
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

void CAddonCallbacksADSP::ADSPRemoveMenuHook(void *addonData, AE_DSP_MENUHOOK *hook)
{
  CActiveAEDSPAddon *client = GetAudioDSPAddon(addonData);
  if (!hook || !client)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid handler data", __FUNCTION__);
    return;
  }

  AE_DSP_MENUHOOKS *hooks = client->GetMenuHooks();
  if (hooks)
  {
    for (unsigned int i = 0; i < hooks->size(); i++)
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

void CAddonCallbacksADSP::ADSPRegisterMode(void* addonData, AE_DSP_MODES::AE_DSP_MODE* mode)
{
  CActiveAEDSPAddon *addon = GetAudioDSPAddon(addonData);
  if (!mode || !addon)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid mode data", __FUNCTION__);
    return;
  }

  CActiveAEDSPMode transferMode(*mode, addon->GetID());
  int idMode = transferMode.AddUpdate();
  mode->iUniqueDBModeId = idMode;

  if (idMode > AE_DSP_INVALID_ADDON_ID)
  {
	  CLog::Log(LOGDEBUG, "Audio DSP - %s - successfull registered mode %s of %s adsp-addon", __FUNCTION__, mode->strModeName, addon->Name().c_str());
  }
  else
  {
	  CLog::Log(LOGERROR, "Audio DSP - %s - failed to register mode %s of %s adsp-addon", __FUNCTION__, mode->strModeName, addon->Name().c_str());
  }
}

void CAddonCallbacksADSP::ADSPUnregisterMode(void* addonData, AE_DSP_MODES::AE_DSP_MODE* mode)
{
  CActiveAEDSPAddon *addon = GetAudioDSPAddon(addonData);
  if (!mode || !addon)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid mode data", __FUNCTION__);
    return;
  }

  CActiveAEDSPMode transferMode(*mode, addon->GetID());
  transferMode.Delete();
}

ADSPHANDLE CAddonCallbacksADSP::ADSPSoundPlay_GetHandle(void *addonData, const char *filename)
{
  CActiveAEDSPAddon *addon = GetAudioDSPAddon(addonData);
  if (!filename || !addon)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid sound play data", __FUNCTION__);
    return NULL;
  }

  IAESound *sound = CAEFactory::MakeSound(filename);
  if (!sound)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - failed to make sound play data", __FUNCTION__);
    return NULL;
  }

  return sound;
}

void CAddonCallbacksADSP::ADSPSoundPlay_ReleaseHandle(void *addonData, ADSPHANDLE handle)
{
  CActiveAEDSPAddon *addon = GetAudioDSPAddon(addonData);
  if (!handle || !addon)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid sound play data", __FUNCTION__);
    return;
  }

  CAEFactory::FreeSound((IAESound*)handle);
}

void CAddonCallbacksADSP::ADSPSoundPlay_Play(void *addonData, ADSPHANDLE handle)
{
  CActiveAEDSPAddon *addon = GetAudioDSPAddon(addonData);
  if (!handle || !addon)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid sound play data", __FUNCTION__);
    return;
  }
  ((IAESound*)handle)->Play();
}

void CAddonCallbacksADSP::ADSPSoundPlay_Stop(void *addonData, ADSPHANDLE handle)
{
  CActiveAEDSPAddon *addon = GetAudioDSPAddon(addonData);
  if (!handle || !addon)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid sound play data", __FUNCTION__);
    return;
  }
  ((IAESound*)handle)->Stop();
}

bool CAddonCallbacksADSP::ADSPSoundPlay_IsPlaying(void *addonData, ADSPHANDLE handle)
{
  CActiveAEDSPAddon *addon = GetAudioDSPAddon(addonData);
  if (!handle || !addon)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid sound play data", __FUNCTION__);
    return false;
  }
  return ((IAESound*)handle)->IsPlaying();
}

void CAddonCallbacksADSP::ADSPSoundPlay_SetChannel(void *addonData, ADSPHANDLE handle, AE_DSP_CHANNEL channel)
{
  CActiveAEDSPAddon *addon = GetAudioDSPAddon(addonData);
  if (!handle || !addon)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid sound play data", __FUNCTION__);
    return;
  }

  ((IAESound*)handle)->SetChannel(CActiveAEDSP::GetKODIChannel(channel));
}

AE_DSP_CHANNEL CAddonCallbacksADSP::ADSPSoundPlay_GetChannel(void *addonData, ADSPHANDLE handle)
{
  CActiveAEDSPAddon *addon = GetAudioDSPAddon(addonData);
  if (!handle || !addon)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid sound play data", __FUNCTION__);
    return AE_DSP_CH_INVALID;
  }

  return CActiveAEDSP::GetDSPChannel(((IAESound*)handle)->GetChannel());
}

void CAddonCallbacksADSP::ADSPSoundPlay_SetVolume(void *addonData, ADSPHANDLE handle, float volume)
{
  CActiveAEDSPAddon *addon = GetAudioDSPAddon(addonData);
  if (!handle || !addon)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid sound play data", __FUNCTION__);
    return;
  }

  ((IAESound*)handle)->SetVolume(volume);
}

float CAddonCallbacksADSP::ADSPSoundPlay_GetVolume(void *addonData, ADSPHANDLE handle)
{
  CActiveAEDSPAddon *addon = GetAudioDSPAddon(addonData);
  if (!handle || !addon)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid sound play data", __FUNCTION__);
    return 0.0f;
  }

  return ((IAESound*)handle)->GetVolume();
}

} /* namespace AudioDSP */

} /* namespace KodiAPI */
} /* namespace V1 */
