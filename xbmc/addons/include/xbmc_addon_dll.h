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

extern "C"
{ 
  enum ADDON_STATUS
  {
    STATUS_OK,
    STATUS_LOST_CONNECTION,
    STATUS_NEED_RESTART,
    STATUS_NEED_SETTINGS,
    STATUS_UNKNOWN
  };

  typedef struct
  {
    int           type;
    char*         id;
    char*         label;
    int           current;
    char**        entry;
    unsigned int  entry_elements;
  } StructSetting;

  ADDON_STATUS __declspec(dllexport) Create(void *callbacks, void* props);
  void __declspec(dllexport) Destroy();
  ADDON_STATUS __declspec(dllexport) GetStatus();
  bool __declspec(dllexport) HasSettings();
  unsigned int __declspec(dllexport) GetSettings(StructSetting ***sSet);
  ADDON_STATUS __declspec(dllexport) SetSetting(const char *settingName, const void *settingValue);
  void __declspec(dllexport) Remove();
};

#endif
