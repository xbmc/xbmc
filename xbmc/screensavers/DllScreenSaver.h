#pragma once
#include "../DynamicDll.h"

struct SCR_INFO
{
  int dummy;
};

struct ScreenSaver
{
public:
#ifndef HAS_SDL
    void (__cdecl* Create)(LPDIRECT3DDEVICE8 pd3dDevice, int iWidth, int iHeight, const char* szScreensaver, float pixelRatio);
#else
    void (__cdecl* Create)(void* pd3dDevice, int iWidth, int iHeight, const char* szScreensaver, float pixelRatio);
#endif
    void (__cdecl* Start) ();
    void (__cdecl* Render) ();
    void (__cdecl* Stop) ();
    void (__cdecl* GetInfo)(SCR_INFO *info);
};

class DllScreensaverInterface
{
public:
  void GetModule(struct ScreenSaver* pScr);
};

class DllScreensaver : public DllDynamic, DllScreensaverInterface
{
  DECLARE_DLL_WRAPPER_TEMPLATE(DllScreensaver)
  DEFINE_METHOD1(void, GetModule, (struct ScreenSaver* p1))
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD_RENAME(get_module,GetModule)
  END_METHOD_RESOLVE()
};
