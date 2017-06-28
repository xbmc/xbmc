/*
 *      Copyright (C) 2010-2015 Team Kodi
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

#include <vector>
#include "Application.h"
#include "ActiveAEDSPAddon.h"
#include "ActiveAEDSP.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace ADDON;
using namespace ActiveAE;

#define DEFAULT_INFO_STRING_VALUE "unknown"

CActiveAEDSPAddon::CActiveAEDSPAddon(BinaryAddonBasePtr addonInfo)
  : IAddonInstanceHandler(ADDON_INSTANCE_ADSP, addonInfo)
{
  ResetProperties();
}

CActiveAEDSPAddon::~CActiveAEDSPAddon(void)
{
  Destroy();
}

void CActiveAEDSPAddon::ResetProperties(int iClientId /* = AE_DSP_INVALID_ADDON_ID */)
{
  /* initialise members */
  m_menuhooks.clear();
  m_strUserPath           = CSpecialProtocol::TranslatePath(Profile());
  m_strAddonPath          = CSpecialProtocol::TranslatePath(Path());
  m_bReadyToUse           = false;
  m_isInUse               = false;
  m_iClientId             = iClientId;
  m_strAudioDSPVersion    = DEFAULT_INFO_STRING_VALUE;
  m_strFriendlyName       = DEFAULT_INFO_STRING_VALUE;
  m_strAudioDSPName       = DEFAULT_INFO_STRING_VALUE;
  m_addonCapabilities = {0};

  m_struct = {0};
  m_struct.props.strUserPath = m_strUserPath.c_str();
  m_struct.props.strAddonPath = m_strAddonPath.c_str();

  m_struct.toKodi.kodiInstance = this;
  m_struct.toKodi.add_menu_hook = cb_add_menu_hook;
  m_struct.toKodi.remove_menu_hook = cb_remove_menu_hook;
  m_struct.toKodi.register_mode = cb_register_mode;
  m_struct.toKodi.unregister_mode = cb_unregister_mode;
}

bool CActiveAEDSPAddon::Create(int iClientId)
{
  if (iClientId <= AE_DSP_INVALID_ADDON_ID)
    return false;

  /* ensure that a previous instance is destroyed */
  Destroy();

  /* reset all properties to defaults */
  ResetProperties(iClientId);

  /* initialise the add-on */
  CLog::Log(LOGDEBUG, "ActiveAE DSP - %s - creating audio dsp add-on instance '%s'", __FUNCTION__, Name().c_str());
  /* Open the class "kodi::addon::CInstanceAudioDSP" on add-on side */
  m_bReadyToUse = (CreateInstance(&m_struct) == ADDON_STATUS_OK);
  if (!m_bReadyToUse)
  {
    CLog::Log(LOGFATAL, "ActiveAE DSP: failed to create instance for '%s' and not usable!", ID().c_str());
    return false;
  }

  GetAddonProperties();

  return true;
}

void CActiveAEDSPAddon::Destroy(void)
{
  /* reset 'ready to use' to false */
  if (!m_bReadyToUse)
    return;
  m_bReadyToUse = false;

  CLog::Log(LOGDEBUG, "ActiveAE DSP - %s - destroying audio dsp add-on '%s'", __FUNCTION__, GetFriendlyName().c_str());

  /* Destroy the class "kodi::addon::CInstanceAudioDSP" on add-on side */
  DestroyInstance();

  /* reset all properties to defaults */
  ResetProperties();
}

void CActiveAEDSPAddon::ReCreate(void)
{
  int iClientID(m_iClientId);
  Destroy();

  /* recreate the instance */
  Create(iClientID);
}

bool CActiveAEDSPAddon::ReadyToUse(void) const
{
  return m_bReadyToUse;
}

int CActiveAEDSPAddon::GetID(void) const
{
  return m_iClientId;
}

bool CActiveAEDSPAddon::IsInUse() const
{
  return m_isInUse;
}

void CActiveAEDSPAddon::GetAddonProperties(void)
{
  AE_DSP_ADDON_CAPABILITIES addonCapabilities;

  /* get the capabilities */
  addonCapabilities = {0};
  m_struct.toAddon.get_capabilities(&m_struct, &addonCapabilities);

  /* get the name of the dsp addon */
  std::string strDSPName = m_struct.toAddon.get_dsp_name(&m_struct);

  /* display name = backend name string */
  std::string strFriendlyName = StringUtils::Format("%s", strDSPName.c_str());

  /* backend version number */
  std::string strAudioDSPVersion = m_struct.toAddon.get_dsp_version(&m_struct);

  /* update the members */
  m_strAudioDSPName     = strDSPName;
  m_strFriendlyName     = strFriendlyName;
  m_strAudioDSPVersion  = strAudioDSPVersion;
  m_addonCapabilities   = addonCapabilities;
}

AE_DSP_ADDON_CAPABILITIES CActiveAEDSPAddon::GetAddonCapabilities(void) const
{
  AE_DSP_ADDON_CAPABILITIES addonCapabilities(m_addonCapabilities);
  return addonCapabilities;
}

const std::string &CActiveAEDSPAddon::GetAudioDSPName(void) const
{
  return m_strAudioDSPName;
}

const std::string &CActiveAEDSPAddon::GetAudioDSPVersion(void) const
{
  return m_strAudioDSPVersion;
}

const std::string &CActiveAEDSPAddon::GetFriendlyName(void) const
{
  return m_strFriendlyName;
}

bool CActiveAEDSPAddon::HaveMenuHooks(AE_DSP_MENUHOOK_CAT cat) const
{
  if (m_bReadyToUse && !m_menuhooks.empty())
  {
    for (unsigned int i = 0; i < m_menuhooks.size(); ++i)
    {
      if (m_menuhooks[i].category == cat || m_menuhooks[i].category == AE_DSP_MENUHOOK_ALL)
        return true;
    }
  }
  return false;
}

AE_DSP_MENUHOOKS *CActiveAEDSPAddon::GetMenuHooks(void)
{
  return &m_menuhooks;
}

void CActiveAEDSPAddon::CallMenuHook(const AE_DSP_MENUHOOK &hook, AE_DSP_MENUHOOK_DATA &hookData)
{
  if (!m_bReadyToUse || hookData.category == AE_DSP_MENUHOOK_UNKNOWN)
    return;

  m_struct.toAddon.menu_hook(&m_struct, &hook, &hookData);
}

AE_DSP_ERROR CActiveAEDSPAddon::StreamCreate(const AE_DSP_SETTINGS *addonSettings, const AE_DSP_STREAM_PROPERTIES* pProperties, ADDON_HANDLE handle)
{
  AE_DSP_ERROR retVal = m_struct.toAddon.stream_create(&m_struct, addonSettings, pProperties, handle);
  if (retVal == AE_DSP_ERROR_NO_ERROR)
    m_isInUse = true;
  LogError(retVal, __FUNCTION__);

  return retVal;
}

void CActiveAEDSPAddon::StreamDestroy(const ADDON_HANDLE handle)
{
  m_struct.toAddon.stream_destroy(&m_struct, handle);

  m_isInUse = false;
}

bool CActiveAEDSPAddon::StreamIsModeSupported(const ADDON_HANDLE handle, AE_DSP_MODE_TYPE type, unsigned int addon_mode_id, int unique_db_mode_id)
{
  AE_DSP_ERROR retVal = m_struct.toAddon.stream_is_mode_supported(&m_struct, handle, type, addon_mode_id, unique_db_mode_id);
  if (retVal == AE_DSP_ERROR_NO_ERROR)
    return true;
  else if (retVal != AE_DSP_ERROR_IGNORE_ME)
    LogError(retVal, __FUNCTION__);

  return false;
}

AE_DSP_ERROR CActiveAEDSPAddon::StreamInitialize(const ADDON_HANDLE handle, const AE_DSP_SETTINGS *addonSettings)
{
  AE_DSP_ERROR retVal = m_struct.toAddon.stream_initialize(&m_struct, handle, addonSettings);
  LogError(retVal, __FUNCTION__);

  return retVal;
}

bool CActiveAEDSPAddon::InputProcess(const ADDON_HANDLE handle, const float **array_in, unsigned int samples)
{
  return m_struct.toAddon.input_process(&m_struct, handle, array_in, samples);
}

unsigned int CActiveAEDSPAddon::InputResampleProcessNeededSamplesize(const ADDON_HANDLE handle)
{
  return m_struct.toAddon.input_resample_process_needed_samplesize(&m_struct, handle);
}

unsigned int CActiveAEDSPAddon::InputResampleProcess(const ADDON_HANDLE handle, const float **array_in, float **array_out, unsigned int samples)
{
  return m_struct.toAddon.input_resample_process(&m_struct, handle, array_in, array_out, samples);
}

int CActiveAEDSPAddon::InputResampleSampleRate(const ADDON_HANDLE handle)
{
  return m_struct.toAddon.input_resample_samplerate(&m_struct, handle);
}

float CActiveAEDSPAddon::InputResampleGetDelay(const ADDON_HANDLE handle)
{
  return m_struct.toAddon.input_resample_get_delay(&m_struct, handle);
}

unsigned int CActiveAEDSPAddon::PreProcessNeededSamplesize(const ADDON_HANDLE handle, unsigned int mode_id)
{
  return m_struct.toAddon.pre_process_needed_samplesize(&m_struct, handle, mode_id);
}

float CActiveAEDSPAddon::PreProcessGetDelay(const ADDON_HANDLE handle, unsigned int mode_id)
{
  return m_struct.toAddon.pre_process_get_delay(&m_struct, handle, mode_id);
}

unsigned int CActiveAEDSPAddon::PreProcess(const ADDON_HANDLE handle, unsigned int mode_id, const float **array_in, float **array_out, unsigned int samples)
{
  return m_struct.toAddon.pre_process(&m_struct, handle, mode_id, array_in, array_out, samples);
}

AE_DSP_ERROR CActiveAEDSPAddon::MasterProcessSetMode(const ADDON_HANDLE handle, AE_DSP_STREAMTYPE type, unsigned int mode_id, int unique_db_mode_id)
{
  AE_DSP_ERROR retVal = m_struct.toAddon.master_process_set_mode(&m_struct, handle, type, mode_id, unique_db_mode_id);
  LogError(retVal, __FUNCTION__);

  return retVal;
}

unsigned int CActiveAEDSPAddon::MasterProcessNeededSamplesize(const ADDON_HANDLE handle)
{
  return m_struct.toAddon.master_process_needed_samplesize(&m_struct, handle);
}

float CActiveAEDSPAddon::MasterProcessGetDelay(const ADDON_HANDLE handle)
{
  return m_struct.toAddon.master_process_get_delay(&m_struct, handle);
}

int CActiveAEDSPAddon::MasterProcessGetOutChannels(const ADDON_HANDLE handle, unsigned long &out_channel_present_flags)
{
  return m_struct.toAddon.master_process_get_out_channels(&m_struct, handle, &out_channel_present_flags);
}

unsigned int CActiveAEDSPAddon::MasterProcess(const ADDON_HANDLE handle, const float **array_in, float **array_out, unsigned int samples)
{
  return m_struct.toAddon.master_process(&m_struct, handle, array_in, array_out, samples);
}

std::string CActiveAEDSPAddon::MasterProcessGetStreamInfoString(const ADDON_HANDLE handle)
{
  std::string strReturn;

  if (!m_bReadyToUse)
    return strReturn;

  strReturn = m_struct.toAddon.master_process_get_stream_info_string(&m_struct, handle);
  return strReturn;
}

unsigned int CActiveAEDSPAddon::PostProcessNeededSamplesize(const ADDON_HANDLE handle, unsigned int mode_id)
{
  return m_struct.toAddon.post_process_needed_samplesize(&m_struct, handle, mode_id);
}

float CActiveAEDSPAddon::PostProcessGetDelay(const ADDON_HANDLE handle, unsigned int mode_id)
{
  return m_struct.toAddon.post_process_get_delay(&m_struct, handle, mode_id);
}

unsigned int CActiveAEDSPAddon::PostProcess(const ADDON_HANDLE handle, unsigned int mode_id, const float **array_in, float **array_out, unsigned int samples)
{
  return m_struct.toAddon.post_process(&m_struct, handle, mode_id, array_in, array_out, samples);
}

unsigned int CActiveAEDSPAddon::OutputResampleProcessNeededSamplesize(const ADDON_HANDLE handle)
{
  return m_struct.toAddon.output_resample_process_needed_samplesize(&m_struct, handle);
}

unsigned int CActiveAEDSPAddon::OutputResampleProcess(const ADDON_HANDLE handle, const float **array_in, float **array_out, unsigned int samples)
{
  return m_struct.toAddon.output_resample_process(&m_struct, handle, array_in, array_out, samples);
}

int CActiveAEDSPAddon::OutputResampleSampleRate(const ADDON_HANDLE handle)
{
  return m_struct.toAddon.output_resample_samplerate(&m_struct, handle);
}

float CActiveAEDSPAddon::OutputResampleGetDelay(const ADDON_HANDLE handle)
{
  return m_struct.toAddon.output_resample_get_delay(&m_struct, handle);
}

bool CActiveAEDSPAddon::SupportsInputInfoProcess(void) const
{
  return m_addonCapabilities.bSupportsInputProcess;
}

bool CActiveAEDSPAddon::SupportsInputResample(void) const
{
  return m_addonCapabilities.bSupportsInputResample;
}

bool CActiveAEDSPAddon::SupportsPreProcess(void) const
{
  return m_addonCapabilities.bSupportsPreProcess;
}

bool CActiveAEDSPAddon::SupportsMasterProcess(void) const
{
  return m_addonCapabilities.bSupportsMasterProcess;
}

bool CActiveAEDSPAddon::SupportsPostProcess(void) const
{
  return m_addonCapabilities.bSupportsPostProcess;
}

bool CActiveAEDSPAddon::SupportsOutputResample(void) const
{
  return m_addonCapabilities.bSupportsOutputResample;
}

const char *CActiveAEDSPAddon::ToString(const AE_DSP_ERROR error)
{
  switch (error)
  {
  case AE_DSP_ERROR_NO_ERROR:
    return "no error";
  case AE_DSP_ERROR_NOT_IMPLEMENTED:
    return "not implemented";
  case AE_DSP_ERROR_REJECTED:
    return "rejected by the backend";
  case AE_DSP_ERROR_INVALID_PARAMETERS:
    return "invalid parameters for this method";
  case AE_DSP_ERROR_INVALID_SAMPLERATE:
    return "invalid samplerate for this method";
  case AE_DSP_ERROR_INVALID_IN_CHANNELS:
    return "invalid input channel layout for this method";
  case AE_DSP_ERROR_INVALID_OUT_CHANNELS:
    return "invalid output channel layout for this method";
  case AE_DSP_ERROR_FAILED:
    return "the command failed";
  case AE_DSP_ERROR_UNKNOWN:
  default:
    return "unknown error";
  }
}

bool CActiveAEDSPAddon::LogError(const AE_DSP_ERROR error, const char *strMethod) const
{
  if (error != AE_DSP_ERROR_NO_ERROR && error != AE_DSP_ERROR_IGNORE_ME)
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - %s - addon '%s' returned an error: %s",
        strMethod, GetFriendlyName().c_str(), ToString(error));
    return false;
  }
  return true;
}

void CActiveAEDSPAddon::cb_add_menu_hook(void *kodiInstance, AE_DSP_MENUHOOK *hook)
{
  CActiveAEDSPAddon *client = static_cast<CActiveAEDSPAddon*>(kodiInstance);
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

void CActiveAEDSPAddon::cb_remove_menu_hook(void *kodiInstance, AE_DSP_MENUHOOK *hook)
{
  CActiveAEDSPAddon *client = static_cast<CActiveAEDSPAddon*>(kodiInstance);
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

void CActiveAEDSPAddon::cb_register_mode(void* kodiInstance, AE_DSP_MODES::AE_DSP_MODE* mode)
{
  CActiveAEDSPAddon *addon = static_cast<CActiveAEDSPAddon*>(kodiInstance);
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
    CLog::Log(LOGDEBUG, "Audio DSP - %s - successfully registered mode %s of %s adsp-addon", __FUNCTION__, mode->strModeName, addon->Name().c_str());
  }
  else
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - failed to register mode %s of %s adsp-addon", __FUNCTION__, mode->strModeName, addon->Name().c_str());
  }
}

void CActiveAEDSPAddon::cb_unregister_mode(void* kodiInstance, AE_DSP_MODES::AE_DSP_MODE* mode)
{
  CActiveAEDSPAddon *addon = static_cast<CActiveAEDSPAddon*>(kodiInstance);
  if (!mode || !addon)
  {
    CLog::Log(LOGERROR, "Audio DSP - %s - invalid mode data", __FUNCTION__);
    return;
  }

  CActiveAEDSPMode transferMode(*mode, addon->GetID());
  transferMode.Delete();
}
