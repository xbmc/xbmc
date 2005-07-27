
#ifndef _EMU_OLE32_H
#define _EMU_OLE32_H

extern "C" HRESULT dllCoInitialize();
extern "C" void dllCoUninitialize();
extern "C" HRESULT dllCoCreateInstance( REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid);
extern "C" void dllCoFreeUnusedLibraries();
extern "C" int dllStringFromGUID2(REFGUID rguid, LPOLESTR lpsz, int cchMax);
extern "C" void WINAPI dllCoTaskMemFree(void * cb);
extern "C" void * WINAPI dllCoTaskMemAlloc(unsigned long cb);

#endif // _EMU_OLE32_H