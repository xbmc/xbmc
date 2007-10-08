#ifndef __XBMC_SCR_H__
#define __XBMC_SCR_H__

#ifndef _LINUX
#include <xtl.h>
#else
#define __cdecl
#define __declspec(x) 
#endif

extern "C"
{

struct SCR_INFO
{
  int dummy;
};

  // Functions that your visualisation must implement
#ifndef _LINUX
  void Create(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreensaver, float fPixelRatio);
#else
  void Create(void* pd3dDevice, int iWidth, int iHeight, const char* szScreensaver, float fPixelRatio);
#endif
  void Start();
  void Render();
  void Stop();
  void GetInfo(SCR_INFO* pInfo)
  {
  }


  // Structure to transfer the above functions to XBMC
  struct ScreenSaver
  {
#ifndef _LINUX
    void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreensaver, float pixelRatio);
#else
    void (__cdecl* Create)(void* pd3dDevice, int iWidth, int iHeight, const char* szScreensaver, float pixelRatio);
#endif
    void (__cdecl* Start) ();
    void (__cdecl* Render) ();
    void (__cdecl* Stop) ();
    void (__cdecl* GetInfo)(SCR_INFO *info);
  };

  // function to export the above structure to XBMC
    void __declspec(dllexport) get_module(struct ScreenSaver* pScr)
    {
      pScr->Create = Create;
      pScr->Start = Start;
      pScr->Render = Render;
      pScr->Stop = Stop;
      pScr->GetInfo = GetInfo;
    };
};

#endif
