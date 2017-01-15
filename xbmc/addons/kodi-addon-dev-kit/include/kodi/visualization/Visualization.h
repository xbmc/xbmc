#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
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

/*
 * Parts with a comment named "internal" are only used inside header and not
 * used or accessed direct during add-on development!
 */

#include "../AddonBase.h"

namespace kodi { namespace addon { class CInstanceVisualization; }}

extern "C"
{

typedef enum VIS_ACTION /* internal */
{ 
  VIS_ACTION_NONE = 0,
  VIS_ACTION_NEXT_PRESET,
  VIS_ACTION_PREV_PRESET,
  VIS_ACTION_LOAD_PRESET,
  VIS_ACTION_RANDOM_PRESET,
  VIS_ACTION_LOCK_PRESET,
  VIS_ACTION_RATE_PRESET_PLUS,
  VIS_ACTION_RATE_PRESET_MINUS,
  VIS_ACTION_UPDATE_ALBUMART,
  VIS_ACTION_UPDATE_TRACK
} VIS_ACTION;

struct VIS_INFO /* internal */
{
  bool bWantsFreq;
  int iSyncDelay;
};

typedef struct AddonProps_Visualization /* internal */
{
  void *device;
  int x;
  int y;
  int width;
  int height;
  float pixelRatio;
  const char *name;
  const char *presets;
  const char *profile;
} AddonProps_Visualization;

typedef struct AddonToKodiFuncTable_Visualization /* internal */
{
  KODI_HANDLE kodiInstance;
  void (__cdecl* transfer_preset) (void* kodiInstance, const char* preset);
} AddonToKodiFuncTable_Visualization;

typedef struct KodiToAddonFuncTable_Visualization /* internal */
{
  bool (__cdecl* Start)(kodi::addon::CInstanceVisualization* addonInstance, int channels, int samplesPerSec, int bitsPerSample, const char* songName);
  void (__cdecl* Stop)(kodi::addon::CInstanceVisualization* addonInstance);
  void (__cdecl* AudioData)(kodi::addon::CInstanceVisualization* addonInstance, const float* audioData, int audioDataLength, float *freqData, int freqDataLength);
  bool (__cdecl* IsDirty) (kodi::addon::CInstanceVisualization* addonInstance);
  void (__cdecl* Render) (kodi::addon::CInstanceVisualization* addonInstance);
  void (__cdecl* GetInfo)(kodi::addon::CInstanceVisualization* addonInstance, VIS_INFO *info);
  bool (__cdecl* OnAction)(kodi::addon::CInstanceVisualization* addonInstance, VIS_ACTION action, const void *param);
  bool (__cdecl* HasPresets)(kodi::addon::CInstanceVisualization* addonInstance);
  unsigned int (__cdecl *GetPresets)(kodi::addon::CInstanceVisualization* addonInstance);
  unsigned int (__cdecl *GetPreset)(kodi::addon::CInstanceVisualization* addonInstance);
  unsigned int (__cdecl *GetSubModules)(kodi::addon::CInstanceVisualization* addonInstance, char ***modules);
  bool (__cdecl* IsLocked)(kodi::addon::CInstanceVisualization* addonInstance);
} KodiToAddonFuncTable_Visualization;

typedef struct AddonInstance_Visualization /* internal */
{
  AddonProps_Visualization props;
  AddonToKodiFuncTable_Visualization toKodi;
  KodiToAddonFuncTable_Visualization toAddon;
} AddonInstance_Visualization;

} /* extern "C" */

namespace kodi
{
namespace addon
{

  class VisTrack
  {
  public:
    VisTrack()
    {
      title = artist = album = albumArtist = nullptr;
      genre = comment = lyrics = reserved1 = reserved2 = nullptr;
      trackNumber = discNumber = duration = year = 0;
      rating = 0;
      reserved3 = reserved4 = 0;
    }

    const char *title;
    const char *artist;
    const char *album;
    const char *albumArtist;
    const char *genre;
    const char *comment;
    const char *lyrics;
    const char *reserved1;
    const char *reserved2;

    int trackNumber;
    int discNumber;
    int duration;
    int year;
    char rating;
    int reserved3;
    int reserved4;
  };

  class CInstanceVisualization : public IAddonInstance
  {
  public:
    CInstanceVisualization()
      : IAddonInstance(ADDON_INSTANCE_VISUALIZATION),
        m_presetLockedByUser(false)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstanceVisualization: Creation of more as one in single instance way is not allowed!");

      SetAddonStruct(CAddonBase::m_interface->firstKodiInstance);
      CAddonBase::m_interface->globalSingleInstance = this;
    }

    CInstanceVisualization(KODI_HANDLE instance)
      : IAddonInstance(ADDON_INSTANCE_VISUALIZATION),
        m_presetLockedByUser(false)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstanceVisualization: Creation of multiple together with single instance way is not allowed!");

      SetAddonStruct(instance);
    }

    virtual ~CInstanceVisualization() { }

    virtual bool Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName) { return true; }
    virtual void Stop() {}
    virtual void AudioData(const float* audioData, int audioDataLength, float* freqData, int freqDataLength) {}
    virtual bool IsDirty() { return true; }
    virtual void Render() {}
    virtual void GetInfo(bool& wantsFreq, int& syncDelay) { wantsFreq = false; syncDelay = 0; }
    virtual bool HasPresets() { return false; }
    virtual bool GetPresets(std::vector<std::string>& presets) { return false; }
    virtual unsigned int GetPreset() { return 0; }
    virtual bool IsLocked() { return false; }
    virtual bool PrevPreset() { return false; }
    virtual bool NextPreset() { return false; }
    virtual bool LoadPreset(int select) { return false; }
    virtual bool RandomPreset() { return false; }
    virtual bool LockPreset(bool lockUnlock) { return false; }
    virtual bool RatePreset(bool plusMinus) { return false; }
    virtual bool UpdateAlbumart(std::string albumart) { return false; }
    virtual bool UpdateTrack(const kodi::addon::VisTrack &track) { return false; }

    inline void* Device() { return m_instanceData->props.device; }
    inline int X() { return m_instanceData->props.x; }
    inline int Y() { return m_instanceData->props.y; }
    inline int Width() { return m_instanceData->props.width; }
    inline int Height() { return m_instanceData->props.height; }
    inline float PixelRatio() { return m_instanceData->props.pixelRatio; }
    inline std::string Name() { return m_instanceData->props.name; }
    inline std::string Presets() { return m_instanceData->props.presets; }
    inline std::string Profile() { return m_instanceData->props.profile; }

  private:
    void SetAddonStruct(KODI_HANDLE instance)
    {
      if (instance == nullptr)
        throw std::logic_error("kodi::addon::CInstanceVisualization: Creation with empty addon structure not allowed, table must be given from Kodi!");

      m_instanceData = static_cast<AddonInstance_Visualization*>(instance);

      m_instanceData->toAddon.Start = ADDON_Start;
      m_instanceData->toAddon.Stop = ADDON_Stop;
      m_instanceData->toAddon.AudioData = ADDON_AudioData;
      m_instanceData->toAddon.Render = ADDON_Render;
      m_instanceData->toAddon.GetInfo = ADDON_GetInfo;
      m_instanceData->toAddon.OnAction = ADDON_OnAction;
      m_instanceData->toAddon.HasPresets = ADDON_HasPresets;
      m_instanceData->toAddon.GetPresets = ADDON_GetPresets;
      m_instanceData->toAddon.GetPreset = ADDON_GetPreset;
      m_instanceData->toAddon.IsLocked = ADDON_IsLocked;
    }

    inline static bool ADDON_Start(CInstanceVisualization* addonInstance, int channels, int samplesPerSec, int bitsPerSample, const char* songName)
    {
      return addonInstance->Start(channels, samplesPerSec, bitsPerSample, songName);
    }

    inline static void ADDON_Stop(CInstanceVisualization* addonInstance)
    {
      addonInstance->Stop();
    }

    inline static void ADDON_AudioData(CInstanceVisualization* addonInstance, const float* audioData, int audioDataLength, float *freqData, int freqDataLength)
    {
      addonInstance->AudioData(audioData, audioDataLength, freqData, freqDataLength);
    }
    
    inline static bool ADDON_IsDirty(CInstanceVisualization* addonInstance)
    {
      return addonInstance->IsDirty();
    }

    inline static void ADDON_Render(CInstanceVisualization* addonInstance)
    {
      addonInstance->Render();
    }

    inline static void ADDON_GetInfo(CInstanceVisualization* addonInstance, VIS_INFO *info)
    {
      addonInstance->GetInfo(info->bWantsFreq, info->iSyncDelay);
    }

    inline static bool ADDON_OnAction(CInstanceVisualization* addonInstance, VIS_ACTION action, const void *param)
    {
      switch (action)
      {
        case VIS_ACTION_NEXT_PRESET:
          return addonInstance->NextPreset();
        case VIS_ACTION_PREV_PRESET:
          return addonInstance->PrevPreset();
        case VIS_ACTION_LOAD_PRESET:
          return addonInstance->LoadPreset(*(int*)param);
        case VIS_ACTION_RANDOM_PRESET:
          return addonInstance->RandomPreset();
        case VIS_ACTION_LOCK_PRESET:
          addonInstance->m_presetLockedByUser = !addonInstance->m_presetLockedByUser;
          return addonInstance->LockPreset(addonInstance->m_presetLockedByUser);
        case VIS_ACTION_RATE_PRESET_PLUS:
          return addonInstance->RatePreset(true);
        case VIS_ACTION_RATE_PRESET_MINUS:
          return addonInstance->RatePreset(false);
        case VIS_ACTION_UPDATE_ALBUMART:
          return addonInstance->UpdateAlbumart(static_cast<const char*>(param));
        case VIS_ACTION_UPDATE_TRACK:
          return addonInstance->UpdateTrack(*static_cast<const VisTrack*>(param));
        case VIS_ACTION_NONE:
        default:
          break;
      }
      return false;
    }

    inline static bool ADDON_HasPresets(CInstanceVisualization* addonInstance)
    {
      return addonInstance->HasPresets();
    }

    inline static unsigned int ADDON_GetPresets(CInstanceVisualization* addonInstance)
    {
      CInstanceVisualization* addon = addonInstance;
      std::vector<std::string> presets;
      if (addon->GetPresets(presets))
      {
        for (auto it : presets)
          addon->m_instanceData->toKodi.transfer_preset(addon->m_instanceData->toKodi.kodiInstance, it.c_str());
      }

      return presets.size();
    }

    inline static unsigned int ADDON_GetPreset(CInstanceVisualization* addonInstance)
    {
      return addonInstance->GetPreset();
    }

    inline static bool ADDON_IsLocked(CInstanceVisualization* addonInstance)
    {
      return addonInstance->IsLocked();
    }

    bool m_presetLockedByUser;
    AddonInstance_Visualization* m_instanceData;
  };

} /* namespace addon */
} /* namespace kodi */
