#ifndef __XBMC_SCR_H__
#define __XBMC_SCR_H__

#ifndef _LINUX
#include <xtl.h>
#else
#define __cdecl
#define __declspec(x)
#endif

#include "ScreensaverTypes.h"

extern "C"
{
  // Functions that your visualisation must implement
#ifndef _LINUX
  ADDON_STATUS Create(ScreensaverCallbacks* cb, LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreensaver, float fPixelRatio);
#else
  ADDON_STATUS Create(ScreensaverCallbacks* cb, void* pd3dDevice, int iWidth, int iHeight, const char* szScreensaver, float fPixelRatio);
#endif
  void Start();
  void Render();
  void Stop();
  void GetInfo(SCR_INFO* pInfo);

  // function to export the above structure to XBMC
  void __declspec(dllexport) get_addon(struct ScreenSaver* pScr)
  {
    pScr->Create = Create;
    pScr->Start = Start;
    pScr->Render = Render;
    pScr->Stop = Stop;
    pScr->GetInfo = GetInfo;
  };
};

#endif
