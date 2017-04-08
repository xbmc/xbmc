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
#include "addons/kodi-addon-dev-kit/include/kodi/libKODI_guilib.h"
#include "filesystem/SpecialProtocol.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

using namespace ADDON;
using namespace ActiveAE;

#define DEFAULT_INFO_STRING_VALUE "unknown"

CActiveAEDSPAddon::CActiveAEDSPAddon(AddonProps props) :
    CAddonDll(std::move(props)),
    m_apiVersion("0.0.0")
{
  ResetProperties();
}

CActiveAEDSPAddon::~CActiveAEDSPAddon(void)
{
  Destroy();
}

void CActiveAEDSPAddon::OnDisabled()
{
  //! @todo reactive this with AudioDSP V2.0
  //CServiceBroker::GetADSP().UpdateAddons();
}

void CActiveAEDSPAddon::OnEnabled()
{
  //! @todo reactive this with AudioDSP V2.0
  //CServiceBroker::GetADSP().UpdateAddons();
}

AddonPtr CActiveAEDSPAddon::GetRunningInstance() const
{
  return AddonPtr();
}

void CActiveAEDSPAddon::OnPreInstall()
{
  //! @todo reactive this with AudioDSP V2.0
  //CServiceBroker::GetADSP().UpdateAddons();
}

void CActiveAEDSPAddon::OnPostInstall(bool restart, bool update)
{
  //! @todo reactive this with AudioDSP V2.0
  //CServiceBroker::GetADSP().UpdateAddons();
}

void CActiveAEDSPAddon::OnPreUnInstall()
{
  //! @todo implement unloading adsp addons
  //CServiceBroker::GetADSP().Deactivate();
}

void CActiveAEDSPAddon::OnPostUnInstall()
{
  //! @todo reactive this with AudioDSP V2.0
  //CServiceBroker::GetADSP().UpdateAddons();
}

void CActiveAEDSPAddon::ResetProperties(int iClientId /* = AE_DSP_INVALID_ADDON_ID */)
{
  /* initialise members */
  m_strUserPath           = CSpecialProtocol::TranslatePath(Profile());
  m_info.strUserPath     = m_strUserPath.c_str();
  m_strAddonPath          = CSpecialProtocol::TranslatePath(Path());
  m_info.strAddonPath    = m_strAddonPath.c_str();
  m_menuhooks.clear();
  m_bReadyToUse           = false;
  m_isInUse               = false;
  m_iClientId             = iClientId;
  m_strAudioDSPVersion    = DEFAULT_INFO_STRING_VALUE;
  m_strFriendlyName       = DEFAULT_INFO_STRING_VALUE;
  m_strAudioDSPName       = DEFAULT_INFO_STRING_VALUE;
  memset(&m_addonCapabilities, 0, sizeof(m_addonCapabilities));
  m_apiVersion = AddonVersion("0.0.0");
}

ADDON_STATUS CActiveAEDSPAddon::Create(int iClientId)
{
  ADDON_STATUS status(ADDON_STATUS_UNKNOWN);
  if (iClientId <= AE_DSP_INVALID_ADDON_ID)
    return status;

  /* ensure that a previous instance is destroyed */
  Destroy();

  /* reset all properties to defaults */
  ResetProperties(iClientId);

  /* initialise the add-on */
  bool bReadyToUse(false);
  CLog::Log(LOGDEBUG, "ActiveAE DSP - %s - creating audio dsp add-on instance '%s'", __FUNCTION__, Name().c_str());
  if ((status = CAddonDll::Create(&m_struct, &m_info)) == ADDON_STATUS_OK)
    bReadyToUse = GetAddonProperties();

  m_bReadyToUse = bReadyToUse;

  return status;
}

bool CActiveAEDSPAddon::DllLoaded(void) const
{
  return CAddonDll::DllLoaded();
}

void CActiveAEDSPAddon::Destroy(void)
{
  /* reset 'ready to use' to false */
  if (!m_bReadyToUse)
    return;
  m_bReadyToUse = false;

  CLog::Log(LOGDEBUG, "ActiveAE DSP - %s - destroying audio dsp add-on '%s'", __FUNCTION__, GetFriendlyName().c_str());

  /* destroy the add-on */
  CAddonDll::Destroy();

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
};

bool CActiveAEDSPAddon::IsCompatibleAPIVersion(const ADDON::AddonVersion &minVersion, const ADDON::AddonVersion &version)
{
  AddonVersion myMinVersion = AddonVersion(KODI_AE_DSP_MIN_API_VERSION);
  AddonVersion myVersion = AddonVersion(KODI_AE_DSP_API_VERSION);
  return (version >= myMinVersion && minVersion <= myVersion);
}

bool CActiveAEDSPAddon::IsCompatibleGUIAPIVersion(const ADDON::AddonVersion &minVersion, const ADDON::AddonVersion &version)
{
  AddonVersion myMinVersion = AddonVersion(KODI_GUILIB_MIN_API_VERSION);
  AddonVersion myVersion = AddonVersion(KODI_GUILIB_API_VERSION);
  return (version >= myMinVersion && minVersion <= myVersion);
}

bool CActiveAEDSPAddon::CheckAPIVersion(void)
{
  /* check the API version */
  AddonVersion minVersion = AddonVersion(KODI_AE_DSP_MIN_API_VERSION);
  m_apiVersion = AddonVersion(m_struct.GetAudioDSPAPIVersion());

  if (!IsCompatibleAPIVersion(minVersion, m_apiVersion))
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - Add-on '%s' is using an incompatible API version. KODI minimum API version = '%s', add-on API version '%s'", Name().c_str(), minVersion.asString().c_str(), m_apiVersion.asString().c_str());
    return false;
  }

  /* check the GUI API version */
  AddonVersion guiVersion = AddonVersion(m_struct.GetGUIAPIVersion());
  minVersion = AddonVersion(KODI_GUILIB_MIN_API_VERSION);

  if (!IsCompatibleGUIAPIVersion(minVersion, guiVersion))
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - Add-on '%s' is using an incompatible GUI API version. KODI minimum GUI API version = '%s', add-on GUI API version '%s'", Name().c_str(), minVersion.asString().c_str(), guiVersion.asString().c_str());
    return false;
  }

  return true;
}

bool CActiveAEDSPAddon::GetAddonProperties(void)
{
  AE_DSP_ADDON_CAPABILITIES addonCapabilities;

  /* get the capabilities */
  memset(&addonCapabilities, 0, sizeof(addonCapabilities));
  AE_DSP_ERROR retVal = m_struct.GetAddonCapabilities(&addonCapabilities);
  if (retVal != AE_DSP_ERROR_NO_ERROR)
  {
    CLog::Log(LOGERROR, "ActiveAE DSP - couldn't get the capabilities for add-on '%s'. Please contact the developer of this add-on: %s", GetFriendlyName().c_str(), Author().c_str());
    return false;
  }

  /* get the name of the dsp addon */
  std::string strDSPName = m_struct.GetDSPName();

  /* display name = backend name string */
  std::string strFriendlyName = StringUtils::Format("%s", strDSPName.c_str());

  /* backend version number */
  std::string strAudioDSPVersion = m_struct.GetDSPVersion();

  /* update the members */
  m_strAudioDSPName     = strDSPName;
  m_strFriendlyName     = strFriendlyName;
  m_strAudioDSPVersion  = strAudioDSPVersion;
  m_addonCapabilities   = addonCapabilities;

  return true;
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

  m_struct.MenuHook(hook, hookData);
}

AE_DSP_ERROR CActiveAEDSPAddon::StreamCreate(const AE_DSP_SETTINGS *addonSettings, const AE_DSP_STREAM_PROPERTIES* pProperties, ADDON_HANDLE handle)
{
  AE_DSP_ERROR retVal = m_struct.StreamCreate(addonSettings, pProperties, handle);
  if (retVal == AE_DSP_ERROR_NO_ERROR)
    m_isInUse = true;
  LogError(retVal, __FUNCTION__);

  return retVal;
}

void CActiveAEDSPAddon::StreamDestroy(const ADDON_HANDLE handle)
{
  m_struct.StreamDestroy(handle);

  m_isInUse = false;
}

bool CActiveAEDSPAddon::StreamIsModeSupported(const ADDON_HANDLE handle, AE_DSP_MODE_TYPE type, unsigned int addon_mode_id, int unique_db_mode_id)
{
  AE_DSP_ERROR retVal = m_struct.StreamIsModeSupported(handle, type, addon_mode_id, unique_db_mode_id);
  if (retVal == AE_DSP_ERROR_NO_ERROR)
    return true;
  else if (retVal != AE_DSP_ERROR_IGNORE_ME)
    LogError(retVal, __FUNCTION__);

  return false;
}

AE_DSP_ERROR CActiveAEDSPAddon::StreamInitialize(const ADDON_HANDLE handle, const AE_DSP_SETTINGS *addonSettings)
{
  AE_DSP_ERROR retVal = m_struct.StreamInitialize(handle, addonSettings);
  LogError(retVal, __FUNCTION__);

  return retVal;
}

bool CActiveAEDSPAddon::InputProcess(const ADDON_HANDLE handle, const float **array_in, unsigned int samples)
{
  return m_struct.InputProcess(handle, array_in, samples);
}

unsigned int CActiveAEDSPAddon::InputResampleProcessNeededSamplesize(const ADDON_HANDLE handle)
{
  return m_struct.InputResampleProcessNeededSamplesize(handle);
}

unsigned int CActiveAEDSPAddon::InputResampleProcess(const ADDON_HANDLE handle, float **array_in, float **array_out, unsigned int samples)
{
  return m_struct.InputResampleProcess(handle, array_in, array_out, samples);
}

int CActiveAEDSPAddon::InputResampleSampleRate(const ADDON_HANDLE handle)
{
  return m_struct.InputResampleSampleRate(handle);
}

float CActiveAEDSPAddon::InputResampleGetDelay(const ADDON_HANDLE handle)
{
  return m_struct.InputResampleGetDelay(handle);
}

unsigned int CActiveAEDSPAddon::PreProcessNeededSamplesize(const ADDON_HANDLE handle, unsigned int mode_id)
{
  return m_struct.PreProcessNeededSamplesize(handle, mode_id);
}

float CActiveAEDSPAddon::PreProcessGetDelay(const ADDON_HANDLE handle, unsigned int mode_id)
{
  return m_struct.PreProcessGetDelay(handle, mode_id);
}

unsigned int CActiveAEDSPAddon::PreProcess(const ADDON_HANDLE handle, unsigned int mode_id, float **array_in, float **array_out, unsigned int samples)
{
  return m_struct.PostProcess(handle, mode_id, array_in, array_out, samples);
}

AE_DSP_ERROR CActiveAEDSPAddon::MasterProcessSetMode(const ADDON_HANDLE handle, AE_DSP_STREAMTYPE type, unsigned int mode_id, int unique_db_mode_id)
{
  AE_DSP_ERROR retVal = m_struct.MasterProcessSetMode(handle, type, mode_id, unique_db_mode_id);
  LogError(retVal, __FUNCTION__);

  return retVal;
}

unsigned int CActiveAEDSPAddon::MasterProcessNeededSamplesize(const ADDON_HANDLE handle)
{
  return m_struct.MasterProcessNeededSamplesize(handle);
}

float CActiveAEDSPAddon::MasterProcessGetDelay(const ADDON_HANDLE handle)
{
  return m_struct.MasterProcessGetDelay(handle);
}

int CActiveAEDSPAddon::MasterProcessGetOutChannels(const ADDON_HANDLE handle, unsigned long &out_channel_present_flags)
{
  return m_struct.MasterProcessGetOutChannels(handle, out_channel_present_flags);
}

unsigned int CActiveAEDSPAddon::MasterProcess(const ADDON_HANDLE handle, float **array_in, float **array_out, unsigned int samples)
{
  return m_struct.MasterProcess(handle, array_in, array_out, samples);
}

std::string CActiveAEDSPAddon::MasterProcessGetStreamInfoString(const ADDON_HANDLE handle)
{
  std::string strReturn;

  if (!m_bReadyToUse)
    return strReturn;

  strReturn = m_struct.MasterProcessGetStreamInfoString(handle);
  return strReturn;
}

unsigned int CActiveAEDSPAddon::PostProcessNeededSamplesize(const ADDON_HANDLE handle, unsigned int mode_id)
{
  return m_struct.PostProcessNeededSamplesize(handle, mode_id);
}

float CActiveAEDSPAddon::PostProcessGetDelay(const ADDON_HANDLE handle, unsigned int mode_id)
{
  return m_struct.PostProcessGetDelay(handle, mode_id);
}

unsigned int CActiveAEDSPAddon::PostProcess(const ADDON_HANDLE handle, unsigned int mode_id, float **array_in, float **array_out, unsigned int samples)
{
  return m_struct.PostProcess(handle, mode_id, array_in, array_out, samples);
}

unsigned int CActiveAEDSPAddon::OutputResampleProcessNeededSamplesize(const ADDON_HANDLE handle)
{
  return m_struct.OutputResampleProcessNeededSamplesize(handle);
}

unsigned int CActiveAEDSPAddon::OutputResampleProcess(const ADDON_HANDLE handle, float **array_in, float **array_out, unsigned int samples)
{
  return m_struct.OutputResampleProcess(handle, array_in, array_out, samples);
}

int CActiveAEDSPAddon::OutputResampleSampleRate(const ADDON_HANDLE handle)
{
  return m_struct.OutputResampleSampleRate(handle);
}

float CActiveAEDSPAddon::OutputResampleGetDelay(const ADDON_HANDLE handle)
{
  return m_struct.OutputResampleGetDelay(handle);
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
