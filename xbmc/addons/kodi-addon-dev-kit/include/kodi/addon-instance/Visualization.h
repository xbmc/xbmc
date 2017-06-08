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

struct AddonInstance_Visualization;

typedef enum VIS_ACTION : unsigned int /* internal */
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
  kodi::addon::CInstanceVisualization* addonInstance;
  bool (__cdecl* start)(AddonInstance_Visualization* instance, int channels, int samplesPerSec, int bitsPerSample, const char* songName);
  void (__cdecl* stop)(AddonInstance_Visualization* instance);
  void (__cdecl* audio_data)(AddonInstance_Visualization* instance, const float* audioData, int audioDataLength, float *freqData, int freqDataLength);
  bool (__cdecl* is_dirty)(AddonInstance_Visualization* instance);
  void (__cdecl* render)(AddonInstance_Visualization* instance);
  void (__cdecl* get_info)(AddonInstance_Visualization* instance, VIS_INFO *info);
  bool (__cdecl* on_action)(AddonInstance_Visualization* instance, VIS_ACTION action, const void *param);
  unsigned int (__cdecl *get_presets)(AddonInstance_Visualization* instance);
  int (__cdecl *get_active_preset)(AddonInstance_Visualization* instance);
  bool (__cdecl* is_locked)(AddonInstance_Visualization* instance);
} KodiToAddonFuncTable_Visualization;

typedef struct AddonInstance_Visualization /* internal */
{
  AddonProps_Visualization props;
  AddonToKodiFuncTable_Visualization toKodi;
  KodiToAddonFuncTable_Visualization toAddon;
} AddonInstance_Visualization;

struct VisTrack
{
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

} /* extern "C" */

namespace kodi
{
namespace addon
{

  class CInstanceVisualization : public IAddonInstance
  {
  public:
    CInstanceVisualization()
      : IAddonInstance(ADDON_INSTANCE_VISUALIZATION),
        m_presetLockedByUser(false)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstanceVisualization: Cannot create multiple instances of add-on.");

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

    virtual ~CInstanceVisualization() = default;

    virtual bool Start(int channels, int samplesPerSec, int bitsPerSample, std::string songName) { return true; }
    virtual void Stop() {}
    virtual void AudioData(const float* audioData, int audioDataLength, float* freqData, int freqDataLength) {}
    virtual bool IsDirty() { return true; }
    virtual void Render() {}
    virtual void GetInfo(bool& wantsFreq, int& syncDelay) { wantsFreq = false; syncDelay = 0; }
    virtual bool GetPresets(std::vector<std::string>& presets) { return false; }
    virtual int GetActivePreset() { return -1; }
    virtual bool IsLocked() { return false; }
    virtual bool PrevPreset() { return false; }
    virtual bool NextPreset() { return false; }
    virtual bool LoadPreset(int select) { return false; }
    virtual bool RandomPreset() { return false; }
    virtual bool LockPreset(bool lockUnlock) { return false; }
    virtual bool RatePreset(bool plusMinus) { return false; }
    virtual bool UpdateAlbumart(std::string albumart) { return false; }
    virtual bool UpdateTrack(const VisTrack &track) { return false; }

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
        throw std::logic_error("kodi::addon::CInstanceVisualization: Null pointer instance passed.");

      m_instanceData = static_cast<AddonInstance_Visualization*>(instance);
      m_instanceData->toAddon.addonInstance = this;
      m_instanceData->toAddon.start = ADDON_Start;
      m_instanceData->toAddon.stop = ADDON_Stop;
      m_instanceData->toAddon.audio_data = ADDON_AudioData;
      m_instanceData->toAddon.render = ADDON_Render;
      m_instanceData->toAddon.get_info = ADDON_GetInfo;
      m_instanceData->toAddon.on_action = ADDON_OnAction;
      m_instanceData->toAddon.get_presets = ADDON_GetPresets;
      m_instanceData->toAddon.get_active_preset = ADDON_GetActivePreset;
      m_instanceData->toAddon.is_locked = ADDON_IsLocked;
    }

    inline static bool ADDON_Start(AddonInstance_Visualization* addon, int channels, int samplesPerSec, int bitsPerSample, const char* songName)
    {
      return addon->toAddon.addonInstance->Start(channels, samplesPerSec, bitsPerSample, songName);
    }

    inline static void ADDON_Stop(AddonInstance_Visualization* addon)
    {
      addon->toAddon.addonInstance->Stop();
    }

    inline static void ADDON_AudioData(AddonInstance_Visualization* addon, const float* audioData, int audioDataLength, float *freqData, int freqDataLength)
    {
      addon->toAddon.addonInstance->AudioData(audioData, audioDataLength, freqData, freqDataLength);
    }
    
    inline static bool ADDON_IsDirty(AddonInstance_Visualization* addon)
    {
      return addon->toAddon.addonInstance->IsDirty();
    }

    inline static void ADDON_Render(AddonInstance_Visualization* addon)
    {
      addon->toAddon.addonInstance->Render();
    }

    inline static void ADDON_GetInfo(AddonInstance_Visualization* addon, VIS_INFO *info)
    {
      addon->toAddon.addonInstance->GetInfo(info->bWantsFreq, info->iSyncDelay);
    }

    inline static bool ADDON_OnAction(AddonInstance_Visualization* addon, VIS_ACTION action, const void *param)
    {
      switch (action)
      {
        case VIS_ACTION_NEXT_PRESET:
          return addon->toAddon.addonInstance->NextPreset();
        case VIS_ACTION_PREV_PRESET:
          return addon->toAddon.addonInstance->PrevPreset();
        case VIS_ACTION_LOAD_PRESET:
          return addon->toAddon.addonInstance->LoadPreset(*static_cast<const int*>(param));
        case VIS_ACTION_RANDOM_PRESET:
          return addon->toAddon.addonInstance->RandomPreset();
        case VIS_ACTION_LOCK_PRESET:
          addon->toAddon.addonInstance->m_presetLockedByUser = !addon->toAddon.addonInstance->m_presetLockedByUser;
          return addon->toAddon.addonInstance->LockPreset(addon->toAddon.addonInstance->m_presetLockedByUser);
        case VIS_ACTION_RATE_PRESET_PLUS:
          return addon->toAddon.addonInstance->RatePreset(true);
        case VIS_ACTION_RATE_PRESET_MINUS:
          return addon->toAddon.addonInstance->RatePreset(false);
        case VIS_ACTION_UPDATE_ALBUMART:
          return addon->toAddon.addonInstance->UpdateAlbumart(static_cast<const char*>(param));
        case VIS_ACTION_UPDATE_TRACK:
          return addon->toAddon.addonInstance->UpdateTrack(*static_cast<const VisTrack*>(param));
        case VIS_ACTION_NONE:
        default:
          break;
      }
      return false;
    }

    inline static unsigned int ADDON_GetPresets(AddonInstance_Visualization* addon)
    {
      std::vector<std::string> presets;
      if (addon->toAddon.addonInstance->GetPresets(presets))
      {
        for (auto it : presets)
          addon->toAddon.addonInstance->m_instanceData->toKodi.transfer_preset(addon->toKodi.kodiInstance, it.c_str());
      }

      return presets.size();
    }

    inline static int ADDON_GetActivePreset(AddonInstance_Visualization* addon)
    {
      return addon->toAddon.addonInstance->GetActivePreset();
    }

    inline static bool ADDON_IsLocked(AddonInstance_Visualization* addon)
    {
      return addon->toAddon.addonInstance->IsLocked();
    }

    bool m_presetLockedByUser;
    AddonInstance_Visualization* m_instanceData;
  };

} /* namespace addon */
} /* namespace kodi */
