
#ifndef _EMU_OLE32_H
#define _EMU_OLE32_H

extern "C" HRESULT  WINAPI dllCoInitialize();
extern "C" void     WINAPI dllCoUninitialize();
extern "C" HRESULT  WINAPI dllCoCreateInstance( REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID * ppv);
extern "C" void     WINAPI dllCoFreeUnusedLibraries();
extern "C" int      WINAPI dllStringFromGUID2(REFGUID rguid, LPOLESTR lpsz, int cchMax);
extern "C" void     WINAPI dllCoTaskMemFree(void * cb);
extern "C" void *   WINAPI dllCoTaskMemAlloc(unsigned long cb);

#endif // _EMU_OLE32_H