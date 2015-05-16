/*
 *      Copyright (C) 2014 Team KODI
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
#include "AddonCallbacksAudioDSP.h"
#include "settings/AdvancedSettings.h"
#include "utils/log.h"
#include "cores/AudioEngine/DSPAddons/ActiveAEDSP.h"
#include "cores/AudioEngine/DSPAddons/ActiveAEDSPMode.h"
#include "cores/AudioEngine/Interfaces/AESound.h"
#include "cores/AudioEngine/AEFactory.h"
#include "dialogs/GUIDialogKaiToast.h"

using namespace ActiveAE;

namespace ADDON
{

CAddonCallbacksADSP::CAddonCallbacksADSP(CAddon* addon)
{
  m_addon     = addon;
  m_callbacks = new CB_ADSPLib;

  /* write KODI audio DSP specific add-on function addresses to callback table */
  m_callbacks->AddMenuHook                = ADSPAddMenuHook;
  m_callbacks->RemoveMenuHook             = ADSPRemoveMenuHook;
  m_callbacks->RegisterMode               = ADSPRegisterMode;
  m_callbacks->UnregisterMode             = ADSPUnregisterMode;
}

CAddonCallbacksADSP::~CAddonCallbacksADSP()
{
  /* delete the callback table */
  delete m_callbacks;
}

CActiveAEDSPAddon *CAddonCallbacksADSP::GetAudioDSPAddon(void *addonData)
{
  CAddonCallbacks *addon = static_cast<CAddonCallbacks *>(addonData);
  if (!addon || !addon->GetHelperADSP())
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - called with a null pointer", __FUNCTION__);
    return NULL;
  }

  return dynamic_cast<CActiveAEDSPAddon *>(addon->GetHelperADSP()->m_addon);
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

}; /* namespace ADDON */
