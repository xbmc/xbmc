/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      Copyright (C) 2015 Team KODI
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

#include "AddonCallbacks.h"

#include "addons/Addon.h"
#include "addons/binary/ExceptionHandling.h"
#include "addons/binary/callbacks/AddonCallbacksAddonBase.h"
/*#include "addons/binary/callbacks/AudioDSP/AddonCallbacksAudioDSPBase.h"
#include "addons/binary/callbacks/AudioEngine/AddonCallbacksAudioEngineBase.h"
#include "addons/binary/callbacks/Codec/AddonCallbacksCodecBase.h"
#include "addons/binary/callbacks/GUI/AddonCallbacksGUIBase.h"
#include "addons/binary/callbacks/Player/AddonCallbacksPlayerBase.h"
#include "addons/binary/callbacks/PVR/AddonCallbacksPVRBase.h"*/

//#include "addons/binary/callbacks/api1/Addon/AddonCallbacksAddon.h"
#include "addons/binary/callbacks/api1/AudioDSP/AddonCallbacksAudioDSP.h"
#include "addons/binary/callbacks/api1/AudioEngine/AddonCallbacksAudioEngine.h"
#include "addons/binary/callbacks/api1/Codec/AddonCallbacksCodec.h"
#include "addons/binary/callbacks/api1/GUI/AddonCallbacksGUI.h"
#include "addons/binary/callbacks/api1/PVR/AddonCallbacksPVR.h"
#include "filesystem/SpecialProtocol.h"
#include "utils/log.h"

namespace ADDON
{

CAddonCallbacks::CAddonCallbacks(CAddon* addon)
  : m_callbacks(new AddonCB),
    m_addon(addon),
    m_helperAddOn(nullptr),
    m_helperAudioEngine(nullptr),
    m_helperGUI(nullptr),
    m_helperPlayer(nullptr),
    m_helperPVR(nullptr),
    m_helperADSP(nullptr),
    m_helperCODEC(nullptr)
{
  m_callbacks->libBasePath                  = strdup(CSpecialProtocol::TranslatePath("special://xbmcbin/addons").c_str());
  m_callbacks->addonData                    = this;

  m_callbacks->AddOnLib_RegisterMe          = CAddonCallbacks::AddOnLib_RegisterMe;
  m_callbacks->AddOnLib_RegisterLevel       = CAddonCallbacks::AddOnLib_RegisterLevel;
  m_callbacks->AddOnLib_UnRegisterMe        = CAddonCallbacks::AddOnLib_UnRegisterMe;

  m_callbacks->AudioEngineLib_RegisterMe    = CAddonCallbacks::AudioEngineLib_RegisterMe;
  m_callbacks->AudioEngineLib_UnRegisterMe  = CAddonCallbacks::AudioEngineLib_UnRegisterMe;

  m_callbacks->GUILib_RegisterMe            = CAddonCallbacks::GUILib_RegisterMe;
  m_callbacks->GUILib_UnRegisterMe          = CAddonCallbacks::GUILib_UnRegisterMe;

  m_callbacks->PVRLib_RegisterMe            = CAddonCallbacks::PVRLib_RegisterMe;
  m_callbacks->PVRLib_UnRegisterMe          = CAddonCallbacks::PVRLib_UnRegisterMe;

  m_callbacks->ADSPLib_RegisterMe           = CAddonCallbacks::ADSPLib_RegisterMe;
  m_callbacks->ADSPLib_UnRegisterMe         = CAddonCallbacks::ADSPLib_UnRegisterMe;

  m_callbacks->CodecLib_RegisterMe          = CAddonCallbacks::CodecLib_RegisterMe;
  m_callbacks->CodecLib_UnRegisterMe        = CAddonCallbacks::CodecLib_UnRegisterMe;
}

CAddonCallbacks::~CAddonCallbacks()
{
  CAddonCallbacksAddonBase::DestroyHelper(this);

  delete static_cast<V1::KodiAPI::AudioEngine::CAddonCallbacksAudioEngine*>(m_helperAudioEngine);
  delete static_cast<V1::KodiAPI::PVR::CAddonCallbacksPVR*>(m_helperPVR);
  delete static_cast<V1::KodiAPI::GUI::CAddonCallbacksGUI*>(m_helperGUI);
  delete static_cast<V1::KodiAPI::AudioDSP::CAddonCallbacksADSP*>(m_helperADSP);
  delete static_cast<V1::KodiAPI::Codec::CAddonCallbacksCodec*>(m_helperCODEC);

  free((char*)m_callbacks->libBasePath);
  delete m_callbacks;
}

/*\_____________________________________________________________________________
\*/

void* CAddonCallbacks::AddOnLib_RegisterMe(void *addonData)
{
  return AddOnLib_RegisterLevel(addonData, 1);
}

void* CAddonCallbacks::AddOnLib_RegisterLevel(void *addonData, int level)
{
  try
  {
    fprintf(stderr, "1324325g------------1------fdfedt\n");
    CAddonCallbacks* addon = static_cast<CAddonCallbacks *>(addonData);
    fprintf(stderr, "1324325g------------2------fdfedt\n");
    if (addon == nullptr)
    {
      CLog::Log(LOGERROR, "CAddonCallbacks - %s - called with a null pointer", __FUNCTION__);
      return nullptr;
    }

    void* cb = CAddonCallbacksAddonBase::CreateHelper(addon, level);
    fprintf(stderr, "1324325g----------32--------fdfedt\n");
    if (!cb)
      CLog::Log(LOGERROR, "%s: %s/%s - called with not supported API level '%i'",
                            __FUNCTION__,
                            TranslateType(addon->m_addon->Type()).c_str(),
                            addon->m_addon->Name().c_str(), level);
    fprintf(stderr, "1324325g----------32---kjkkk-----fdfedt\n");
    return cb;
  }
  HANDLE_ADDON_EXCEPTION
    fprintf(stderr, "1324325g----------32----ööö----fdfedt\n");
  return nullptr;
}

void CAddonCallbacks::AddOnLib_UnRegisterMe(void *addonData, void *cbTable)
{
  try
  {
    CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(addonData);
    if (addon == nullptr)
    {
      CLog::Log(LOGERROR, "CAddonCallbacks - %s - called with a null pointer", __FUNCTION__);
      return;
    }

    CAddonCallbacksAddonBase::DestroyHelper(addon);
  }
  HANDLE_ADDON_EXCEPTION
}

int CAddonCallbacks::AddOnLib_APILevel()
{
  return CAddonCallbacksAddonBase::APILevel();
}

int CAddonCallbacks::AddOnLib_MinAPILevel()
{
  return CAddonCallbacksAddonBase::MinAPILevel();
}

std::string CAddonCallbacks::AddOnLib_Version()
{
  return CAddonCallbacksAddonBase::Version();
}

std::string CAddonCallbacks::AddOnLib_MinVersion()
{
  return CAddonCallbacksAddonBase::MinVersion();
}

/*\_____________________________________________________________________________
\*/
void* CAddonCallbacks::AudioEngineLib_RegisterMe(void *addonData)
{
  try
  {
    CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(addonData);
    if (addon == nullptr)
    {
      CLog::Log(LOGERROR, "CAddonCallbacks - %s - called with a null pointer", __FUNCTION__);
      return nullptr;
    }

    addon->m_helperAudioEngine = new V1::KodiAPI::AudioEngine::CAddonCallbacksAudioEngine(addon->m_addon);
    return static_cast<V1::KodiAPI::AudioEngine::CAddonCallbacksAudioEngine*>(addon->m_helperAudioEngine)->GetCallbacks();
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddonCallbacks::AudioEngineLib_UnRegisterMe(void *addonData, void *cbTable)
{
  try
  {
    CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(addonData);
    if (addon == nullptr)
    {
      CLog::Log(LOGERROR, "CAddonCallbacks - %s - called with a null pointer", __FUNCTION__);
      return;
    }

    delete static_cast<V1::KodiAPI::AudioEngine::CAddonCallbacksAudioEngine*>(addon->m_helperAudioEngine);
    addon->m_helperAudioEngine = nullptr;
  }
  HANDLE_ADDON_EXCEPTION
}
/*\_____________________________________________________________________________
\*/
void* CAddonCallbacks::GUILib_RegisterMe(void *addonData)
{
  try
  {
    CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(addonData);
    if (addon == nullptr)
    {
      CLog::Log(LOGERROR, "CAddonCallbacks - %s - called with a null pointer", __FUNCTION__);
      return nullptr;
    }

    addon->m_helperGUI = new V1::KodiAPI::GUI::CAddonCallbacksGUI(addon->m_addon);
    return static_cast<V1::KodiAPI::GUI::CAddonCallbacksGUI*>(addon->m_helperGUI)->GetCallbacks();
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddonCallbacks::GUILib_UnRegisterMe(void *addonData, void *cbTable)
{
  try
  {
    CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(addonData);
    if (addon == nullptr)
    {
      CLog::Log(LOGERROR, "CAddonCallbacks - %s - called with a null pointer", __FUNCTION__);
      return;
    }

    delete static_cast<V1::KodiAPI::GUI::CAddonCallbacksGUI*>(addon->m_helperGUI);
    addon->m_helperGUI = nullptr;
  }
  HANDLE_ADDON_EXCEPTION
}
/*\_____________________________________________________________________________
\*/
void* CAddonCallbacks::PVRLib_RegisterMe(void *addonData)
{
  try
  {
    CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(addonData);
    if (addon == nullptr)
    {
      CLog::Log(LOGERROR, "CAddonCallbacks - %s - called with a null pointer", __FUNCTION__);
      return NULL;
    }

    addon->m_helperPVR = new V1::KodiAPI::PVR::CAddonCallbacksPVR(addon->m_addon);
    return static_cast<V1::KodiAPI::PVR::CAddonCallbacksPVR*>(addon->m_helperPVR)->GetCallbacks();
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddonCallbacks::PVRLib_UnRegisterMe(void *addonData, void *cbTable)
{
  try
  {
    CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(addonData);
    if (addon == nullptr)
    {
      CLog::Log(LOGERROR, "CAddonCallbacks - %s - called with a null pointer", __FUNCTION__);
      return;
    }

    delete static_cast<V1::KodiAPI::PVR::CAddonCallbacksPVR*>(addon->m_helperPVR);
    addon->m_helperPVR = nullptr;
  }
  HANDLE_ADDON_EXCEPTION
}
/*\_____________________________________________________________________________
\*/
void* CAddonCallbacks::ADSPLib_RegisterMe(void *addonData)
{
  try
  {
    CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(addonData);
    if (addon == nullptr)
    {
      CLog::Log(LOGERROR, "CAddonCallbacks - %s - called with a null pointer", __FUNCTION__);
      return nullptr;
    }

    addon->m_helperADSP = new V1::KodiAPI::AudioDSP::CAddonCallbacksADSP(addon->m_addon);
    return static_cast<V1::KodiAPI::AudioDSP::CAddonCallbacksADSP*>(addon->m_helperADSP)->GetCallbacks();
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddonCallbacks::ADSPLib_UnRegisterMe(void *addonData, void *cbTable)
{
  try
  {
    CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(addonData);
    if (addon == nullptr)
    {
      CLog::Log(LOGERROR, "CAddonCallbacks - %s - called with a null pointer", __FUNCTION__);
      return;
    }

    delete static_cast<V1::KodiAPI::AudioDSP::CAddonCallbacksADSP*>(addon->m_helperADSP);
    addon->m_helperADSP = nullptr;
  }
  HANDLE_ADDON_EXCEPTION
}
/*\_____________________________________________________________________________
\*/
void* CAddonCallbacks::CodecLib_RegisterMe(void *addonData)
{
  try
  {
    CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(addonData);
    if (addon == nullptr)
    {
      CLog::Log(LOGERROR, "CAddonCallbacks - %s - called with a null pointer", __FUNCTION__);
      return NULL;
    }

    addon->m_helperCODEC = new V1::KodiAPI::Codec::CAddonCallbacksCodec(addon->m_addon);
    return static_cast<V1::KodiAPI::Codec::CAddonCallbacksCodec*>(addon->m_helperCODEC)->GetCallbacks();
  }
  HANDLE_ADDON_EXCEPTION

  return nullptr;
}

void CAddonCallbacks::CodecLib_UnRegisterMe(void *addonData, void *cbTable)
{
  try
  {
    CAddonCallbacks* addon = static_cast<CAddonCallbacks*>(addonData);
    if (addon == nullptr)
    {
      CLog::Log(LOGERROR, "CAddonCallbacks - %s - called with a null pointer", __FUNCTION__);
      return;
    }

    delete static_cast<V1::KodiAPI::Codec::CAddonCallbacksCodec*>(addon->m_helperCODEC);
    addon->m_helperCODEC = nullptr;
  }
  HANDLE_ADDON_EXCEPTION
}













A_DLLEXPORT void* AddOnLib_Register(void *hdl, int level)
{
  void *cb = nullptr;

  try
  {
    if (!hdl)
      throw ADDON::WrongValueException("CAddonCallbacksAddon - %s - invalid handler data", __FUNCTION__);

    cb = CAddonCallbacks::AddOnLib_RegisterLevel(((AddonCB*)hdl)->addonData, level);
    if (!cb)
      throw ADDON::WrongValueException("CAddonCallbacksAddon - %s - can't get callback table from Kodi !!!", __FUNCTION__);
  }
  HANDLE_ADDON_EXCEPTION

  return cb;
}

A_DLLEXPORT void AddOnLib_UnRegister(void *hdl, void *cb)
{
  try
  {
    if (!hdl || !cb)
      throw ADDON::WrongValueException("CAddonCallbacksAddon - %s - invalid handler data", __FUNCTION__);

    CAddonCallbacks::AddOnLib_UnRegisterMe(((AddonCB*)hdl)->addonData, cb);
  }
  HANDLE_ADDON_EXCEPTION
}

}; /* namespace ADDON */
