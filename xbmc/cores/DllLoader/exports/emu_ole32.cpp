
#include "..\..\..\stdafx.h"
#include "emu_dummy.h"
#include "emu_ole32.h"

extern "C" HRESULT dllCoInitialize()
{
  return S_OK;
}

extern "C" void dllCoUninitialize()
{}
extern "C" HRESULT dllCoCreateInstance( REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid)
{
  not_implement("ol32.dll fake function CoCreateInstance called\n"); //warning
  return REGDB_E_CLASSNOTREG;
}

extern "C" void dllCoFreeUnusedLibraries()
{}
extern "C" int dllStringFromGUID2(REFGUID rguid, LPOLESTR lpsz, int cchMax)
{
  not_implement("ol32.dll fake function StringFromGUID2 called\n"); //warning
  return 0;
}

extern "C" void WINAPI dllCoTaskMemFree(void * cb)
{
  if ( cb )
    free(cb);
  cb = 0;
}

extern "C" void * WINAPI dllCoTaskMemAlloc(unsigned long cb)
{
  return malloc(cb);
}
