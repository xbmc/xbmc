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

#include "../AddonBase.h"

namespace kodi { namespace addon { class CInstanceScreensaver; }}

extern "C"
{

typedef struct AddonProps_Screensaver
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
} AddonProps_Screensaver;

typedef struct AddonToKodiFuncTable_Screensaver
{
  KODI_HANDLE kodiInstance;
} AddonToKodiFuncTable_Screensaver;

typedef struct KodiToAddonFuncTable_Screensaver
{
  bool (__cdecl* Start) (kodi::addon::CInstanceScreensaver* addonInstance);
  void (__cdecl* Stop) (kodi::addon::CInstanceScreensaver* addonInstance);
  void (__cdecl* Render) (kodi::addon::CInstanceScreensaver* addonInstance);
} KodiToAddonFuncTable_Screensaver;

typedef struct AddonInstance_Screensaver
{
  AddonProps_Screensaver props;
  AddonToKodiFuncTable_Screensaver toKodi;
  KodiToAddonFuncTable_Screensaver toAddon;
} AddonInstance_Screensaver;

} /* extern "C" */

namespace kodi
{
namespace addon
{
  class CInstanceScreensaver : public IAddonInstance
  {
  public:
    CInstanceScreensaver()
      : IAddonInstance(ADDON_INSTANCE_SCREENSAVER)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstanceScreensaver: Creation of more as one in single instance way is not allowed!");

      SetAddonStruct(CAddonBase::m_interface->firstKodiInstance);
      CAddonBase::m_interface->globalSingleInstance = this;
    }

    CInstanceScreensaver(KODI_HANDLE instance)
      : IAddonInstance(ADDON_INSTANCE_SCREENSAVER)
    {
      if (CAddonBase::m_interface->globalSingleInstance != nullptr)
        throw std::logic_error("kodi::addon::CInstanceScreensaver: Creation of multiple together with single instance way is not allowed!");

      SetAddonStruct(instance);
    }

    virtual ~CInstanceScreensaver() { }

    virtual bool Start() { return true; }
    virtual void Stop() {}
    virtual void Render() {}

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
        throw std::logic_error("kodi::addon::CInstanceScreensaver: Creation with empty addon structure not allowed, table must be given from Kodi!");

      m_instanceData = static_cast<AddonInstance_Screensaver*>(instance);

      m_instanceData->toAddon.Start = ADDON_Start;
      m_instanceData->toAddon.Stop = ADDON_Stop;
      m_instanceData->toAddon.Render = ADDON_Render;
    }

    inline static bool ADDON_Start(CInstanceScreensaver* addonInstance)
    {
      return addonInstance->Start();
    }

    inline static void ADDON_Stop(CInstanceScreensaver* addonInstance)
    {
      addonInstance->Stop();
    }

    inline static void ADDON_Render(CInstanceScreensaver* addonInstance)
    {
      addonInstance->Render();
    }

    AddonInstance_Screensaver* m_instanceData;
  };

} /* namespace addon */
} /* namespace kodi */
