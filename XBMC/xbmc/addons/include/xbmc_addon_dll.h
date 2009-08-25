#ifndef __XBMC_ADDON_H__
#define __XBMC_ADDON_H__

#ifdef HAS_XBOX_HARDWARE
#include <xtl.h>
#else
#ifndef _LINUX
#include <windows.h>
#else
#undef __cdecl
#define __cdecl
#undef __declspec
#define __declspec(x)
#include <time.h>
#endif
#endif

#include "libaddon.h"
#include "xbmc_addon_types.h"

extern "C"
{
  ADDON_STATUS __declspec(dllexport) Create(void *callbacks, void* props);
  void __declspec(dllexport) Destroy();
  ADDON_STATUS __declspec(dllexport) GetStatus();
  bool __declspec(dllexport) HasSettings();
  addon_settings_t __declspec(dllexport) GetSettings();
  ADDON_STATUS __declspec(dllexport) SetSetting(const char *settingName, const void *settingValue);
  void __declspec(dllexport) Remove();
};

#endif
