#ifndef __XBMC_SCR_H__
#define __XBMC_SCR_H__

#include <ctype.h>
#ifdef HAS_XBOX_HARDWARE
#include <xtl.h>
#else
#ifdef _LINUX
//#include "../xbmc/linux/PlatformInclude.h"
#ifndef __APPLE__
#include <sys/sysinfo.h>
#endif
#else
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif
#include "xbmc_addon_dll.h"  
#include "xbmc_scr_types.h"
#include <sys/stat.h>
#include <errno.h>
#endif

extern "C"
{

  // Functions that your visualisation must implement
  void Start();
  void Render();
  void Stop();
  void GetInfo(SCR_INFO* pInfo);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct ScreenSaver* pScr)
  {
    pScr->Start = Start;
    pScr->Render = Render;
    pScr->Stop = Stop;
    pScr->GetInfo = GetInfo;
  };
};

#endif
